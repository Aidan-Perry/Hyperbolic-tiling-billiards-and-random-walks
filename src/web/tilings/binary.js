(() => {
  function toDisk(x,y){const d=x*x+(y+1)*(y+1);return[(x*x+y*y-1)/d,-2*x/d]}
  function inverse(_config,u,v){const d=(1-u)*(1-u)+v*v;if(!(d>0))return null;const x=-2*v/d,y=(1-u*u-v*v)/d;return Number.isFinite(x)&&Number.isFinite(y)&&y>0&&Math.abs(x)<1e14&&y<1e14?[x,y]:null}
  function drawDiskGrid(p,_config,view){
    const minLevel=-10,maxLevel=10,columns=28,samples=12;p.noFill();p.stroke(0);p.strokeWeight(Math.max(.55,1/Math.max(1,view.scale()/300)));
    for(let n=minLevel;n<=maxLevel+1;n++){const h=2**n,c=h/(h+1),r=1/(h+1),center=view.screen(p,c,0);p.circle(center[0],center[1],2*r*view.scale())}
    for(let n=minLevel;n<=maxLevel;n++){const h=2**n;for(let k=-columns;k<=columns;k++){p.beginShape();for(let i=0;i<=samples;i++){const q=toDisk(k*h,h*(1+i/samples));view.vertex(p,q[0],q[1])}p.endShape()}}
    const origin=view.screen(p,0,0);p.strokeWeight(1.6);p.circle(origin[0],origin[1],2*view.scale());
  }
  function drawReferenceGrid(p,_config,view){
    const bounds=view.visible(p),bottom=Math.max(Number.MIN_VALUE,bounds.bottom,.5/view.scale()),top=bounds.top;if(!(top>0))return;let min=Math.floor(Math.log2(bottom))-1,max=Math.ceil(Math.log2(top))+1;min=Math.max(-40,min);max=Math.min(40,max);p.stroke(0);p.strokeWeight(1);
    for(let n=min;n<=max;n++){const h=2**n;if(h>=bottom&&h<=top){const a=view.screen(p,bounds.left,h),b=view.screen(p,bounds.right,h);p.line(a[0],a[1],b[0],b[1])}if(2*h<bottom||h>top)continue;let first=Math.floor(bounds.left/h),last=Math.ceil(bounds.right/h),stride=Math.max(1,Math.ceil((last-first)/220));for(let k=first;k<=last;k+=stride){const a=view.screen(p,k*h,Math.max(h,bottom)),b=view.screen(p,k*h,Math.min(2*h,top));p.line(a[0],a[1],b[0],b[1])}}
    p.noStroke();p.fill(80);p.textSize(10);p.text('y > 0',8,14);
  }
  HTBTilings.register({id:'binary',referenceName:'Poincaré Upper Half-Plane',drawDiskGrid,drawReferenceGrid,inverse});
})();
