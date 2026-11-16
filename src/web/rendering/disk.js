(() => {
  const host=document.querySelector('#canvas'),status=document.querySelector('#status'),tenthsButton=document.querySelector('#toggle-tenths');
  let state=null,scale=300,panX=0,panY=0,dragging=false,lastX=0,lastY=0,tilingVisible=true,tenthsVisible=false;
  const colors={A:'#c7242f',B:'#1769c2'};
  function config(){return state?.request||state?.preview||{tiling:{id:'elliptical-square',version:'1',parameters:{centerX:'0',centerY:'0'}},trajectories:[{id:'A',startU:'0',startV:'0',angleDegrees:'20'}]}}
  function tilingSource(){return state?.result||config()}
  function screen(p,u,v){return[p.width/2+panX+u*scale,p.height/2+panY-v*scale]}
  function fit(p){scale=Math.min(p.width,p.height)*.455;panX=panY=0}
  function zoom(p,factor,x=p.width/2,y=p.height/2){const u=(x-p.width/2-panX)/scale,v=(p.height/2+panY-y)/scale;scale=Math.max(80,scale*factor);panX=x-p.width/2-u*scale;panY=y-p.height/2+v*scale}
  function vertex(p,u,v){const s=screen(p,u,v);p.vertex(s[0],s[1])}
  function drawGrid(p){const {renderer,config}=HTBTilings.get(tilingSource());renderer.drawDiskGrid(p,config,{screen,vertex,scale:()=>scale})}
  function drawTenths(p){const {renderer,config}=HTBTilings.get(tilingSource());renderer.drawDiskTenths?.(p,config,{screen,vertex,scale:()=>scale})}
  function updateTenthsAvailability(){const supported=Boolean(HTBTilings.get(tilingSource()).renderer.drawDiskTenths);tenthsButton.disabled=!supported;tenthsButton.title=supported?'Show the five square-edge rays and their five angular bisectors':'Tenths are available for the order-5 square tiling'}
  function drawArrow(p,t){
    const u=HTB.rational(t.startU),v=HTB.rational(t.startV),degrees=HTB.rational(t.angleDegrees),legacy=t.angleDegrees===undefined,du=legacy?HTB.rational(t.directionU):Math.cos(degrees*Math.PI/180),dv=legacy?HTB.rational(t.directionV):Math.sin(degrees*Math.PI/180),n=Math.hypot(du,dv);if(!Number.isFinite(n)||n===0)return;
    const length=.18,[x1,y1]=screen(p,u,v),[x2,y2]=screen(p,u+length*du/n,v+length*dv/n);p.stroke('#14813d');p.strokeWeight(2);p.line(x1,y1,x2,y2);const arrowAngle=Math.atan2(y2-y1,x2-x1);p.line(x2,y2,x2-9*Math.cos(arrowAngle-.5),y2-9*Math.sin(arrowAngle-.5));p.line(x2,y2,x2-9*Math.cos(arrowAngle+.5),y2-9*Math.sin(arrowAngle+.5));p.noStroke();p.fill('#14813d');p.textSize(12);p.text(`${t.id} launch direction`,x1+6,y1-7);
  }
  function lineBetween(p,a,b){const x=screen(p,a[0],a[1]),y=screen(p,b[0],b[1]);p.line(x[0],x[1],y[0],y[1])}
  function drawTrajectory(p,t,selected){
    const events=t.events||[];if(!events.length)return;const count=Math.min(events.length,selected+1), launch=[HTB.approx(t.launch.u),HTB.approx(t.launch.v)],first=HTB.point(events[0]);p.stroke(colors[t.id]);p.strokeWeight(1.35);p.drawingContext.setLineDash([6,5]);lineBetween(p,launch,first);p.drawingContext.setLineDash([]);p.strokeWeight(2);
    for(let i=1;i<count;i++)lineBetween(p,HTB.point(events[i-1]),HTB.point(events[i]));p.noStroke();p.fill(colors[t.id]);for(let i=0;i<count;i++){const q=screen(p,...HTB.point(events[i]));p.circle(q[0],q[1],i===0?7:4)}
    const head=screen(p,...HTB.point(events[count-1]));p.noFill();p.stroke(colors[t.id]);p.strokeWeight(2);p.circle(head[0],head[1],13);if(t.status!=='Completed'&&selected>=events.length-1){p.line(head[0]-7,head[1]-7,head[0]+7,head[1]+7);p.line(head[0]-7,head[1]+7,head[0]+7,head[1]-7)}
  }
  function drawWalk(p,walk,selected){const steps=walk?.steps||[];if(!steps.length)return;const count=Math.min(steps.length,selected+1);p.noFill();p.stroke('#c7242f');p.strokeWeight(2);for(let i=1;i<count;i++)lineBetween(p,[steps[i-1].center.u,steps[i-1].center.v],[steps[i].center.u,steps[i].center.v]);p.noStroke();p.fill('#c7242f');for(let i=0;i<count;i++){const q=screen(p,steps[i].center.u,steps[i].center.v);p.circle(q[0],q[1],i===0?7:4)}const last=steps[count-1],head=screen(p,last.center.u,last.center.v);p.noFill();p.stroke('#c7242f');p.strokeWeight(2);p.circle(head[0],head[1],14);p.noStroke();p.fill('#c7242f');p.textSize(11);p.text(`step ${last.index}`,head[0]+9,head[1]-8)}
  function drawWalkStart(p,walk){const u=HTB.rational(walk?.startU),v=HTB.rational(walk?.startV);if(!Number.isFinite(u)||!Number.isFinite(v))return;const q=screen(p,u,v);p.noFill();p.stroke('#14813d');p.strokeWeight(2);p.circle(q[0],q[1],10);p.line(q[0]-7,q[1],q[0]+7,q[1]);p.line(q[0],q[1]-7,q[0],q[1]+7);p.noStroke();p.fill('#14813d');p.textSize(12);p.text('starting-tile selector',q[0]+8,q[1]-8)}
  new p5(p=>{
    p.setup=()=>{const c=p.createCanvas(host.clientWidth,host.clientHeight);c.parent(host);fit(p)};
    p.draw=()=>{p.background(255);p.push();if(tilingVisible)drawGrid(p);if(tenthsVisible)drawTenths(p);const selected=state?.selectedBounce||0,current=config(),walk=(state?.result?.simulationType||current.simulationType)==='randomWalk';if(walk){if(!state?.locked&&!state?.result)drawWalkStart(p,current.walk);if(state?.result)drawWalk(p,state.result.walk,selected)}else{if(!state?.locked&&!state?.result)(current.trajectories||[]).forEach(t=>drawArrow(p,t));(state?.result?.trajectories||[]).forEach(t=>drawTrajectory(p,t,selected))}p.pop()};
    p.windowResized=()=>p.resizeCanvas(host.clientWidth,host.clientHeight);
    p.mousePressed=()=>{if(p.mouseY>0){dragging=true;lastX=p.mouseX;lastY=p.mouseY}};p.mouseReleased=()=>dragging=false;p.mouseDragged=()=>{if(dragging){panX+=p.mouseX-lastX;panY+=p.mouseY-lastY;lastX=p.mouseX;lastY=p.mouseY}};
    p.mouseWheel=e=>{zoom(p,e.delta>0?.9:1.1,p.mouseX,p.mouseY);return false};
    document.querySelector('#zoom-in').onclick=()=>zoom(p,1.1);document.querySelector('#zoom-out').onclick=()=>zoom(p,.9);document.querySelector('#reset-view').onclick=()=>fit(p);const toggleTiling=document.querySelector('#toggle-tiling');toggleTiling.onclick=()=>{tilingVisible=!tilingVisible;toggleTiling.textContent=tilingVisible?'Hide tiling':'Show tiling';toggleTiling.setAttribute('aria-pressed',String(!tilingVisible))};tenthsButton.onclick=()=>{tenthsVisible=!tenthsVisible;tenthsButton.textContent=tenthsVisible?'Hide tenths':'Show tenths';tenthsButton.setAttribute('aria-pressed',String(tenthsVisible))};updateTenthsAvailability();document.querySelector('#focus-head').onclick=()=>{const selected=state?.selectedBounce||0;if(state?.result?.simulationType==='randomWalk'){const steps=state.result.walk?.steps||[],s=steps[Math.min(selected,steps.length-1)];if(s){panX=-s.center.u*scale;panY=s.center.v*scale}return}const ts=state?.result?.trajectories||[];if(!ts.length)return;const points=ts.map(t=>HTB.point(t.events[Math.min(selected,t.events.length-1)])).filter(q=>q.every(Number.isFinite));if(points.length){const u=points.reduce((s,q)=>s+q[0],0)/points.length,v=points.reduce((s,q)=>s+q[1],0)/points.length;panX=-u*scale;panY=v*scale;}};
  },host);
  HTB.subscribe(next=>{state=next;status.textContent=next.status;updateTenthsAvailability()});
})();
