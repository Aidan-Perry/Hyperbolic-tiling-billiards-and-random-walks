(() => {
  const definitions=[
    ['graph-displacement','displacement'],['graph-tiles','tiles'],['graph-length','length'],['graph-clearance','clearance'],['graph-separation','separation']
  ];
  const charts=new Map();let chartState=null,space='disk';
  function seriesFor(kind){
    const result=chartState?.result;if(!result)return[];const ts=result.trajectories||[];
    if(kind==='separation'){
      if(ts.length<2)return[];const n=Math.min(ts[0].events.length,ts[1].events.length),data=[];for(let i=0;i<n;i++){const a=HTB.point(ts[0].events[i],space),b=HTB.point(ts[1].events[i],space);data.push([i,Math.hypot(a[0]-b[0],a[1]-b[1])])}return[{name:'A ↔ B (presentation enclosure centers)',color:'#444',data}];
    }
    return ts.map(t=>({name:t.id,color:t.id==='A'?'#c7242f':'#1769c2',data:(t.events||[]).map((e,i)=>{
      if(kind==='displacement')return[i,HTB.approx(space==='disk'?e.diskDisplacement:e.referenceDisplacement)];
      if(kind==='tiles')return[i,e.uniqueTiles??i+2];if(kind==='length')return[i,HTB.approx(e.pathLength)];return[i,HTB.approx(space==='disk'?e.diskClearance:e.referenceClearance)];
    })}));
  }
  function sketch(id,kind){const host=document.querySelector(`#${id}`);return new p5(p=>{
    p.setup=()=>{const c=p.createCanvas(host.clientWidth,205);c.parent(host)};
    p.draw=()=>{const width=host.clientWidth;if(width>0&&width!==p.width)p.resizeCanvas(width,205);p.background(255);const series=seriesFor(kind),all=series.flatMap(s=>s.data).filter(q=>Number.isFinite(q[1]));p.textFont('sans-serif');p.textSize(10);p.stroke(210);p.line(48,12,48,p.height-30);p.line(48,p.height-30,p.width-12,p.height-30);if(!all.length){p.noStroke();p.fill(110);p.text('No computed data',60,70);return}const maxX=Math.max(1,...all.map(q=>q[0])),maxY=Math.max(1e-30,...all.map(q=>q[1]));const X=x=>48+x/maxX*(p.width-64),Y=y=>p.height-30-y/maxY*(p.height-48);p.noStroke();p.fill(90);p.text('0',35,p.height-26);p.text(String(maxX),p.width-25,p.height-16);p.text(maxY.toPrecision(4),3,17);p.text('bounce',p.width/2,p.height-5);
      series.forEach((s,index)=>{p.noFill();p.stroke(s.color);p.strokeWeight(1.7);p.beginShape();s.data.forEach(q=>{if(Number.isFinite(q[1]))p.vertex(X(q[0]),Y(q[1]))});p.endShape();p.noStroke();p.fill(s.color);p.text(s.name,58+index*145,16)});
      const selected=Math.min(chartState?.selectedBounce||0,maxX);p.stroke(70,110);p.line(X(selected),12,X(selected),p.height-30);p.noStroke();p.fill(50);p.text(String(selected),X(selected)+3,p.height-34);
    };p.windowResized=()=>p.resizeCanvas(host.clientWidth,205);
  },host)}
  definitions.forEach(([id,kind])=>charts.set(id,sketch(id,kind)));
  window.HTBGraphs={update(next,nextSpace){chartState=next;space=nextSpace||space}};
})();
