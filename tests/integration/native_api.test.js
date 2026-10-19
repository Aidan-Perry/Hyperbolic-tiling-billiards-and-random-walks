const test = require('node:test');
const assert = require('node:assert/strict');
const { spawnSync } = require('node:child_process');
const path = require('node:path');

const root = path.resolve(__dirname, '../..');
const engine = path.join(root, 'build', 'htb_engine');
function request(mode='scientific', trajectories=null) {
  return {runId:`test-${mode}`,mode,tiling:{id:'elliptical-square',version:'1',parameters:{centerX:'0',centerY:'0'}},trajectories:trajectories||[{id:'A',startU:'1/10',startV:'-1/5',directionU:'1',directionV:'37/100',bounces:12}],resources:{initialPrecision:128,maxPrecision:8192,memoryLimitMb:1024}};
}
function run(body) {
  const proc=spawnSync(engine,[],{cwd:root,input:JSON.stringify(body),encoding:'utf8',timeout:30000});assert.equal(proc.status,0,proc.stderr);const lines=proc.stdout.trim().split('\n').map(JSON.parse);const result=lines.findLast(x=>x.type==='result');assert.ok(result,'missing result JSONL event');return result;
}
test('scientific and exact backends certify the same edge and tile sequence',()=>{
  const scientific=run(request('scientific')),exact=run(request('exact'));assert.equal(scientific.trajectories[0].status,'Completed');assert.equal(exact.trajectories[0].status,'Completed');
  const signature=t=>t.events.map(e=>`${e.edge}:${e.tileAfter.key}`);assert.deepEqual(signature(scientific.trajectories[0]),signature(exact.trajectories[0]));
  for(const e of scientific.trajectories[0].events)assert.equal(e.certification,'certified');
});
test('rational degree input produces rigorous matching directions and crossings in both backends',()=>{
  const input={id:'A',startU:'1/10',startV:'-1/5',angleDegrees:'30',bounces:5};
  const scientific=run(request('scientific',[input])).trajectories[0],exact=run(request('exact',[input])).trajectories[0];
  assert.equal(scientific.status,'Completed');assert.equal(exact.status,'Completed');
  assert.ok(Math.abs(scientific.initialDirection.du.approx-Math.sqrt(3)/2)<1e-14);assert.ok(Math.abs(scientific.initialDirection.dv.approx-0.5)<1e-14);
  assert.deepEqual(scientific.events.map(e=>e.edge),exact.events.map(e=>e.edge));
  assert.equal(exact.input.angleDegrees,'30');
});
test('two identical trajectories remain identical at every matching bounce',()=>{
  const a={id:'A',startU:'1/10',startV:'-1/5',directionU:'1',directionV:'37/100',bounces:8};const b={...a,id:'B'};const result=run(request('exact',[a,b]));assert.equal(result.trajectories.length,2);
  for(let i=0;i<result.trajectories[0].events.length;i++){const x=result.trajectories[0].events[i],y=result.trajectories[1].events[i];assert.deepEqual(x.tileAfter,y.tileAfter);assert.equal(x.disk.u.display,y.disk.u.display);assert.equal(x.disk.v.display,y.disk.v.display)}
});
test('exact vertex is stopped without an invented continuation',()=>{
  const q=request('exact',[{id:'A',startU:'0',startV:'0',directionU:'1',directionV:'1',bounces:4}]);q.tiling.parameters={centerX:'1/2',centerY:'1/2'};const result=run(q),trajectory=result.trajectories[0];assert.equal(trajectory.status,'Vertex collision');assert.equal(trajectory.events.length,0);assert.equal(trajectory.uncertainty.classification,'exact_vertex_collision');assert.equal(trajectory.uncertainty.vertexEqualityProven,true);assert.deepEqual(trajectory.uncertainty.candidateStates,['certified crossing','certified crossing']);
});
test('scientific near-vertex perturbations certify opposite sides',()=>{
  for(const [dv,edge] of [['1000000000001/1000000000000','top'],['999999999999/1000000000000','right']]){const q=request('scientific',[{id:'A',startU:'0',startV:'0',directionU:'1',directionV:dv,bounces:1}]);q.tiling.parameters={centerX:'1/2',centerY:'1/2'};const result=run(q);assert.ok(['Completed','Unresolved crossing'].includes(result.trajectories[0].status));if(result.trajectories[0].events.length)assert.equal(result.trajectories[0].events[0].edge,edge)}
});
test('scientific mode refines a lone unresolved candidate instead of certifying it',()=>{
  const q=request('scientific',[{id:'A',startU:'0.25',startV:'-0.7',directionU:'-1',directionV:'-0.3',bounces:100}]);
  const result=run(q),trajectory=result.trajectories[0],event=trajectory.events[96];
  assert.equal(trajectory.status,'Completed');
  assert.ok(trajectory.arithmetic.precisionRefinements>0);
  assert.ok(trajectory.arithmetic.maximumPrecision>=1024);
  assert.equal(event.edge,'bottom');
  assert.deepEqual(event.tileBefore.coordinates,[-45,-53]);
  assert.ok(Math.abs(event.reference.y.approx+53)<1e-12);
  assert.ok(Math.hypot(event.disk.u.approx,event.disk.v.approx)>0.99);
  assert.ok(!event.collisionTime.enclosure.startsWith('[+/-'));
});
test('unresolved scientific crossings report their mathematical classification and certified prefix',()=>{
  const q=request('scientific',[{id:'A',startU:'0.25',startV:'-0.7',directionU:'-1',directionV:'-0.3',bounces:100}]);q.resources.maxPrecision=512;
  const trajectory=run(q).trajectories[0],report=trajectory.uncertainty;
  assert.equal(trajectory.status,'Unresolved crossing');
  assert.equal(report.classification,'numerical_uncertainty');
  assert.equal(report.vertexEqualityProven,false);
  assert.equal(report.lastCertifiedBounce,95);
  assert.equal(report.bounce,96);
  assert.match(report.reason,/no vertex equality was proved/);
  assert.deepEqual(report.candidateStates,['unresolved candidate']);
});
test('scientific mode ignores unresolved unit-circle-limit roots after a certified earlier crossing',()=>{
  const q=request('scientific',[{id:'A',startU:'0.25',startV:'-0.9',directionU:'-1',directionV:'-0.4',bounces:50}]);
  const trajectory=run(q).trajectories[0],first=trajectory.events[0];
  assert.equal(trajectory.status,'Completed');
  assert.equal(trajectory.completedBounces,50);
  assert.equal(first.edge,'bottom');
  assert.deepEqual(first.tileBefore.coordinates,[0,-3]);
  assert.deepEqual(first.tileAfter.coordinates,[0,-4]);
  assert.ok(first.collisionTime.approx<0.25);
  assert.ok(Math.abs(first.reference.y.approx+3)<1e-12);
});
test('scientific mode recovers the next edge when cancellation leaves a direction component enclosing zero',()=>{
  const q=request('scientific',[{id:'A',startU:'-0.67',startV:'-0.21',angleDegrees:'270',bounces:12}]);
  const trajectory=run(q).trajectories[0];
  assert.equal(trajectory.status,'Completed');
  assert.equal(trajectory.completedBounces,12);
  assert.deepEqual(trajectory.events.slice(0,5).map(e=>e.edge),['left','top','right','bottom','left']);
  assert.match(trajectory.events[2].outgoing.du.enclosure,/\[\+\/-/);
  assert.deepEqual(trajectory.events[3].tileBefore.coordinates,[-1,0]);
  assert.deepEqual(trajectory.events[3].tileAfter.coordinates,[-1,-1]);
});
test('dual representation and cumulative diagnostics remain consistent',()=>{
  const t=run(request('exact')).trajectories[0];let path=-Infinity,unique=0;for(const e of t.events){const x=e.reference.x.approx,y=e.reference.y.approx,onVertical=Math.abs(x-Math.round(x))<1e-12,onHorizontal=Math.abs(y-Math.round(y))<1e-12;assert.ok(onVertical||onHorizontal);assert.ok(e.pathLength.approx>=path-1e-12);assert.ok(e.uniqueTiles>=unique);path=e.pathLength.approx;unique=e.uniqueTiles}assert.equal(t.events[0].diskDisplacement.approx,0);assert.equal(t.events[0].referenceDisplacement.approx,0);
});
test('one trajectory can stop while the other completes',()=>{
  const q=request('exact',[{id:'A',startU:'0',startV:'0',directionU:'1',directionV:'1',bounces:4},{id:'B',startU:'0',startV:'0',directionU:'1',directionV:'0',bounces:4}]);q.tiling.parameters={centerX:'1/2',centerY:'1/2'};const result=run(q);assert.equal(result.trajectories[0].status,'Vertex collision');assert.equal(result.trajectories[1].status,'Completed');assert.equal(result.trajectories[1].completedBounces,4);
});

test('schema 2 output preserves the selected tiling and canonical identities',()=>{
  const result=run(request('scientific'));
  assert.equal(result.schemaVersion,'2.0');
  assert.equal(result.tiling.id,'elliptical-square');
  assert.equal(result.tiling.version,'1');
  assert.match(result.trajectories[0].events[0].tileAfter.key,/^elliptical-square:/);
  assert.match(result.trajectories[0].events[0].edgeIdentity.boundaryKey,/^elliptical-square:/);
});
