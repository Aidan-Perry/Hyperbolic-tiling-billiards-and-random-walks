(() => {
  const J=[-1,1,1];
  const geometryCache=new Map();
  const ident=()=>[[1,0,0],[0,1,0],[0,0,1]];
  const mul=(a,b)=>a.map((row,i)=>row.map((_,j)=>row.reduce((sum,_v,k)=>sum+a[i][k]*b[k][j],0)));
  const mv=(a,v)=>a.map(row=>row.reduce((sum,x,k)=>sum+x*v[k],0));
  const dot=(a,b)=>-a[0]*b[0]+a[1]*b[1]+a[2]*b[2];
  const reflection=n=>ident().map((row,i)=>row.map((x,j)=>x-2*n[i]*J[j]*n[j]));
  const normalize=v=>{const s=Math.sqrt(Math.max(0,-dot(v,v))),w=v.map(x=>x/s);return w[0]<0?w.map(x=>-x):w};
  const crossPlanes=(n,m)=>{
    const a=[-n[0],n[1],n[2]],b=[-m[0],m[1],m[2]];
    return [a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]];
  };
  const disk=v=>{const w=normalize(v);return [w[1]/(w[0]+1),w[2]/(w[0]+1)]};
  const hyper=(u,v)=>{const d=1-u*u-v*v;return[(1+u*u+v*v)/d,2*u/d,2*v/d]};
  const boost=(p,inverse=false)=>{
    const out=ident(),den=p[0]+1;
    out[0][0]=p[0];
    for(let i=1;i<3;i++){out[0][i]=inverse?-p[i]:p[i];out[i][0]=inverse?-p[i]:p[i]}
    for(let i=1;i<3;i++)for(let j=1;j<3;j++)out[i][j]=(i===j?1:0)+p[i]*p[j]/den;
    return out;
  };

  function construction(config){
    const phi=(1+Math.sqrt(5))/2,b=Math.sqrt(phi);
    const mirrors=[[0,1,0],[0,0,1],[1/(2*b),-phi/2,-Math.SQRT2/2]];
    const refs=mirrors.map(reflection),q=mul(refs[1],refs[2]);
    const normals=[],sideRefs=[];let power=ident();
    for(let k=0;k<4;k++){
      normals.push(mv(power,mirrors[0]));
      let inverse=ident();for(let z=0;z<(4-k)%4;z++)inverse=mul(inverse,q);
      sideRefs.push(mul(mul(power,refs[0]),inverse));power=mul(power,q);
    }
    const vertices=normals.map((n,k)=>crossPlanes(n,normals[(k+1)%4]));
    const baseCenter=[b*b*b,-1,0],p=normalize(vertices[0]);
    const parameters=config?.parameters||{},u=HTB.rational(parameters.centerU??'0'),v=HTB.rational(parameters.centerV??'0');
    const target=hyper(u,v),placement=mul(boost(target),boost(p,true));
    return {vertices,sideRefs,baseCenter,placement};
  }

  function geodesicPoints(a,b){
    const ar=a[0],ai=a[1],br=b[0],bi=b[1];
    const nr=br-ar,ni=bi-ai,dr=1-ar*br-ai*bi,di=ai*br-ar*bi,dd=dr*dr+di*di;
    const wr=(nr*dr+ni*di)/dd,wi=(ni*dr-nr*di)/dd,points=[];
    for(let i=0;i<=16;i++){
      const t=i/16,x=t*wr,y=t*wi,er=1+ar*x+ai*y,ei=ar*y-ai*x,ed=er*er+ei*ei;
      points.push([((x+ar)*er+(y+ai)*ei)/ed,((y+ai)*er-(x+ar)*ei)/ed]);
    }
    return points;
  }

  function geometry(config){
    const parameters=config?.parameters||{},cacheKey=`${parameters.centerU??'0'}|${parameters.centerV??'0'}`;
    if(geometryCache.has(cacheKey))return geometryCache.get(cacheKey);
    const c=construction(config),queue=[{g:ident(),depth:0}],tiles=new Set(),edgeKeys=new Set(),paths=[];let head=0;
    while(head<queue.length&&tiles.size<900){
      const item=queue[head++],center=disk(mv(c.placement,mv(item.g,c.baseCenter)));
      if(center[0]*center[0]+center[1]*center[1]>.9998)continue;
      const key=`${Math.round(center[0]*1e8)},${Math.round(center[1]*1e8)}`;if(tiles.has(key))continue;tiles.add(key);
      const vertices=c.vertices.map(v=>disk(mv(c.placement,mv(item.g,v))));
      for(let k=0;k<4;k++){
        const a=vertices[(k+3)%4],b=vertices[k],ak=`${Math.round(a[0]*1e7)},${Math.round(a[1]*1e7)}`,bk=`${Math.round(b[0]*1e7)},${Math.round(b[1]*1e7)}`,edge=ak<bk?`${ak}|${bk}`:`${bk}|${ak}`;
        if(!edgeKeys.has(edge)){edgeKeys.add(edge);paths.push(geodesicPoints(a,b))}
        if(item.depth<10)queue.push({g:mul(item.g,c.sideRefs[k]),depth:item.depth+1});
      }
    }
    if(geometryCache.size>2)geometryCache.clear();geometryCache.set(cacheKey,paths);return paths;
  }

  function drawDiskGrid(p,config,view){
    p.noFill();p.stroke(0);p.strokeWeight(Math.max(.55,1/Math.max(1,view.scale()/300)));
    for(const path of geometry(config)){p.beginShape();for(const point of path)view.vertex(p,point[0],point[1]);p.endShape()}
    const origin=view.screen(p,0,0);p.strokeWeight(1.6);p.circle(origin[0],origin[1],2*view.scale());
  }

  HTBTilings.register({id:'order5-square',drawDiskGrid});
})();
