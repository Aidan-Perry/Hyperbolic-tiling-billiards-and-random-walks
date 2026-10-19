(() => {
  function center(config) {
    const p=config?.parameters||{};
    return [HTB.rational(p.centerX ?? '0'), HTB.rational(p.centerY ?? '0')];
  }

  function drawDiskGrid(p, config, view) {
    const [cx,cy]=center(config), extent=18, samples=100;
    p.noFill(); p.stroke(0); p.strokeWeight(Math.max(.55,1/Math.max(1,view.scale()/300)));
    for(let k=Math.floor(cx)-extent;k<=Math.ceil(cx)+extent;k++){
      const a=k-cx; p.beginShape();
      if(Math.abs(a)<1e-14){view.vertex(p,0,-1);view.vertex(p,0,1)}
      else for(let s=0;s<=samples;s++){const th=-Math.PI/2+Math.PI*s/samples;view.vertex(p,a/Math.sqrt(1+a*a)*Math.cos(th),Math.sin(th))}
      p.endShape();
    }
    for(let l=Math.floor(cy)-extent;l<=Math.ceil(cy)+extent;l++){
      const b=l-cy; p.beginShape();
      if(Math.abs(b)<1e-14){view.vertex(p,-1,0);view.vertex(p,1,0)}
      else for(let s=0;s<=samples;s++){const th=-Math.PI/2+Math.PI*s/samples;view.vertex(p,Math.sin(th),b/Math.sqrt(1+b*b)*Math.cos(th))}
      p.endShape();
    }
    const o=view.screen(p,0,0);p.strokeWeight(1.6);p.circle(o[0],o[1],2*view.scale());
  }

  function inverse(config,u,v) {
    const [cx,cy]=center(config), d=1-u*u-v*v;
    if(!(d>0))return null;
    const s=Math.sqrt(d),x=cx+u/s,y=cy+v/s;
    return Number.isFinite(x)&&Number.isFinite(y)&&Math.abs(x)<=1e14&&Math.abs(y)<=1e14?[x,y]:null;
  }

  function drawReferenceGrid(p, config, view) {
    const v=view.visible(p),xmin=Math.floor(v.left),xmax=Math.ceil(v.right),ymin=Math.floor(v.bottom),ymax=Math.ceil(v.top);
    p.stroke(0);p.strokeWeight(1);
    if(xmax-xmin<240)for(let x=xmin;x<=xmax;x++){const a=view.screen(p,x,v.bottom),b=view.screen(p,x,v.top);p.line(a[0],a[1],b[0],b[1])}
    if(ymax-ymin<240)for(let y=ymin;y<=ymax;y++){const a=view.screen(p,v.left,y),b=view.screen(p,v.right,y);p.line(a[0],a[1],b[0],b[1])}
    p.noStroke();p.fill(80);p.textSize(10);
    if(view.scale()>22){
      for(let x=xmin;x<=xmax;x++){const a=view.screen(p,x,0);p.text(x,a[0]+2,Math.min(p.height-4,Math.max(12,a[1]-3)))}
      for(let y=ymin;y<=ymax;y++){const a=view.screen(p,0,y);p.text(y,Math.min(p.width-20,Math.max(3,a[0]+3)),a[1]-2)}
    }
  }

  HTBTilings.register({id:'elliptical-square',referenceName:'Euclidean Square Grid',drawDiskGrid,drawReferenceGrid,inverse});
})();
