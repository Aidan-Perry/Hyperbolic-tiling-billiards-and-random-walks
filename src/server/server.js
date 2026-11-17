const http = require('node:http');
const fs = require('node:fs');
const path = require('node:path');
const { spawn } = require('node:child_process');
const crypto = require('node:crypto');

const ROOT = path.resolve(__dirname, '../..');
const WEB = path.join(ROOT, 'src/web');
const ENGINE = process.env.HTB_ENGINE || path.join(ROOT, 'build', 'htb_engine');
const PORT = Number(process.env.PORT || 8765);
const MAX_BODY = 64 * 1024;
function loadTilingManifests() {
  const directory=path.join(ROOT,'src/tilings'), manifests=[], ids=new Set();
  for(const entry of fs.readdirSync(directory,{withFileTypes:true})){
    if(!entry.isDirectory())continue;
    const file=path.join(directory,entry.name,'manifest.json');if(!fs.existsSync(file))continue;
    const manifest=JSON.parse(fs.readFileSync(file,'utf8'));
    if(!manifest.id||!manifest.version||!Array.isArray(manifest.parameters))throw new Error(`invalid tiling manifest: ${file}`);
    if(ids.has(manifest.id))throw new Error(`duplicate tiling manifest: ${manifest.id}`);ids.add(manifest.id);manifests.push(manifest);
  }
  return Object.freeze(manifests.sort((a,b)=>a.name.localeCompare(b.name)).map(Object.freeze));
}
const TILING_MANIFESTS = loadTilingManifests();
let child = null;
let killTimer = null;
let state = freshState();

function freshState(preview = null) {
  return { revision: Date.now(), status: 'Ready', locked: false, runId: null, request: null,
    preview, progress: [], result: null, error: null, selectedBounce: 0, updatedAt: new Date().toISOString() };
}
function touch() { state.revision += 1; state.updatedAt = new Date().toISOString(); }
function json(res, status, value) {
  const body = JSON.stringify(value); res.writeHead(status, {'content-type':'application/json; charset=utf-8','content-length':Buffer.byteLength(body),'cache-control':'no-store'}); res.end(body);
}
function readBody(req) {
  return new Promise((resolve, reject) => { let size=0, chunks=[]; req.on('data', c => { size += c.length; if(size > MAX_BODY){ reject(new Error('request body exceeds 64 KiB')); req.destroy(); } else chunks.push(c); }); req.on('end',()=>{try{resolve(JSON.parse(Buffer.concat(chunks).toString('utf8')))}catch(e){reject(new Error('invalid JSON'))}}); req.on('error',reject); });
}
const scalarPattern = /^[+-]?(?:\d+|\d+\.\d+|\d+\/\d+)$/;
function validateRequest(body) {
  if (!body || typeof body !== 'object') throw new Error('request must be an object');
  const simulationType=body.simulationType||'billiards';if(!['billiards','randomWalk'].includes(simulationType))throw new Error('invalid simulation type');
  if (simulationType==='billiards'&&!['scientific','exact'].includes(body.mode)) throw new Error('invalid computation mode');
  let scalars=[];
  if(body.tiling!==undefined){
    if(!body.tiling||typeof body.tiling!=='object')throw new Error('tiling must be an object');
    const definition=TILING_MANIFESTS.find(item=>item.id===body.tiling.id);if(!definition)throw new Error(`unknown tiling definition: ${String(body.tiling.id)}`);
    if(String(body.tiling.version||definition.version)!==definition.version)throw new Error(`unsupported version for tiling ${definition.id}`);
    const parameters=body.tiling.parameters||{};
    const knownParameters=new Set(definition.parameters.map(spec=>spec.id));
    for(const id of Object.keys(parameters))if(!knownParameters.has(id))throw new Error(`unknown parameter ${id} for tiling ${definition.id}`);
    for(const spec of definition.parameters){const value=parameters[spec.id]??spec.default;if(spec.type==='scalar')scalars.push(value)}
  }else scalars=[body.center?.x,body.center?.y];
  if(simulationType==='randomWalk'){const w=body.walk;if(!w||typeof w!=='object')throw new Error('walk configuration is required');if(w.distribution!=='euclidean-area-neighborhood')throw new Error('unsupported random-walk distribution');if(!Number.isInteger(w.steps)||w.steps<0||w.steps>2000)throw new Error('step count must be between 0 and 2000');if(typeof w.seed!=='string'||!/^(?:0|[1-9]\d*)$/.test(w.seed)||w.seed.length>20)throw new Error('seed must be an unsigned 64-bit integer string');if(BigInt(w.seed)>18446744073709551615n)throw new Error('seed must fit in an unsigned 64-bit integer');scalars.push(w.startU,w.startV)}else{
    if (!Array.isArray(body.trajectories) || body.trajectories.length < 1 || body.trajectories.length > 2) throw new Error('one or two trajectories are required');
    body.trajectories.forEach((t,i) => { if(t.id !== (i ? 'B':'A')) throw new Error('trajectory identifiers must be A and then B'); scalars.push(t.startU,t.startV);if(typeof t.angleDegrees==='string')scalars.push(t.angleDegrees);else scalars.push(t.directionU,t.directionV);if(!Number.isSafeInteger(t.bounces)||t.bounces<0)throw new Error('bounce count must be a non-negative integer'); });
  }
  scalars.forEach(value => { if(typeof value !== 'string'||value.length>150||!scalarPattern.test(value))throw new Error(`invalid exact rational input: ${String(value)}`); });
  const r=body.resources||{}; if(simulationType==='billiards'&&r.initialPrecision!==undefined&&(!Number.isInteger(r.initialPrecision)||r.initialPrecision<32||r.initialPrecision>262144))throw new Error('invalid initial precision');
  if(simulationType==='billiards'&&r.maxPrecision!==undefined&&(!Number.isInteger(r.maxPrecision)||r.maxPrecision<32||r.maxPrecision>262144))throw new Error('invalid maximum precision');
}
function appendProgress(event, runId) {
  if(state.runId !== runId) return;
  if(event.type === 'result') {
    state.result=event;
    state.status=event.status;
    state.error=null;
    state.selectedBounce=event.simulationType==='randomWalk'?Math.max(0,(event.walk?.steps||[]).length-1):Math.max(0,...(event.trajectories||[]).map(trajectory=>Math.max(0,(trajectory.events||[]).length-1)));
  }
  else { state.progress.push({...event, receivedAt:new Date().toISOString()}); if(state.progress.length>1000)state.progress.shift(); }
  touch();
}
function stopChild(signal='SIGTERM') {
  if(!child)return; const processToKill=child; processToKill.kill(signal); clearTimeout(killTimer); killTimer=setTimeout(()=>{if(child===processToKill)processToKill.kill('SIGKILL')},1000); killTimer.unref();
}
function assignRandomWalkSeed(body) {
  if(body?.simulationType!=='randomWalk'||!body.walk||body.walk.seed!==undefined)return body;
  return {...body,walk:{...body.walk,seed:crypto.randomBytes(8).readBigUInt64BE(0).toString()}};
}
function startRun(body) {
  body=assignRandomWalkSeed(body);validateRequest(body); if(child)throw new Error('a native run is already active'); if(!fs.existsSync(ENGINE))throw new Error(`native executable unavailable at ${ENGINE}; run the CMake build first`);
  const runId=crypto.randomUUID(); body={...body,runId}; state={...freshState(body),status:'Computing',locked:true,runId,request:body};
  const proc=spawn(ENGINE,[],{cwd:ROOT,stdio:['pipe','pipe','pipe'],shell:false}); child=proc; let stdoutBuffer='',stderr='';
  proc.stdout.setEncoding('utf8'); proc.stdout.on('data',chunk=>{stdoutBuffer+=chunk;let lines=stdoutBuffer.split('\n');stdoutBuffer=lines.pop();for(const line of lines){if(!line.trim())continue;try{appendProgress(JSON.parse(line),runId)}catch{stderr+=`Malformed native output: ${line.slice(0,300)}\n`;}}});
  proc.stderr.setEncoding('utf8');proc.stderr.on('data',chunk=>{stderr=(stderr+chunk).slice(-16000)});
  proc.on('error',error=>{if(state.runId===runId){state.status='Internal error';state.error=error.message;touch();}child=null;});
  proc.on('close',(code,signal)=>{clearTimeout(killTimer);if(child===proc)child=null;if(state.runId!==runId)return;if(stdoutBuffer.trim()){try{appendProgress(JSON.parse(stdoutBuffer),runId)}catch{stderr+=stdoutBuffer;}}
    if(signal){state.status='Cancelled';state.error=null;}else if(code!==0&&!state.result){state.status='Internal error';state.error=stderr||`native process exited with code ${code}`;}else if(!state.result){state.status='Internal error';state.error='native process exited without a final result';}touch();});
  proc.stdin.end(JSON.stringify(body));
  return runId;
}
function scalarApprox(s){return s?.approx ?? ''}
function csvEscape(value){const s=String(value??'');return /[",\n]/.test(s)?`"${s.replaceAll('"','""')}"`:s}
function runMeta(result){return[result?.runId,result?.simulationType||'billiards',result?.mode,result?.tiling?.id,result?.tiling?.version,JSON.stringify(result?.tiling?.parameters||{}),result?.status,result?.applicationVersion]}
function diagnosticsCsv(result){if(result?.simulationType==='randomWalk'){let rows=[['run_id','simulation_type','backend','tiling_id','tiling_version','tiling_parameters','run_status','application_version','seed','step','tile_key','tile_label','center_u','center_v','tile_area_approx','tile_area_estimated_error','selected_probability','random_draw','stayed','disk_displacement','cumulative_center_length','unique_tiles','repeated_visits']];for(const s of result.walk?.steps||[])rows.push([...runMeta(result),result.walk?.seed,s.index,s.tile?.key,s.tile?.label,s.center?.u,s.center?.v,s.tileArea?.approx,s.tileArea?.estimatedError,s.selectedProbability,s.randomDraw,s.stayed,s.diskDisplacement,s.cumulativeLength,s.uniqueTiles,s.repeatedVisits]);return rows.map(r=>r.map(csvEscape).join(',')).join('\n')+'\n'}let rows=[['run_id','simulation_type','backend','tiling_id','tiling_version','tiling_parameters','run_status','application_version','trajectory','bounce','disk_displacement_approx','reference_displacement_approx','path_length_approx','disk_clearance_approx','reference_clearance_approx','tile_key','tile_label','edge','boundary_key','outgoing_side','incoming_side','symbol','precision','certification']];for(const t of result?.trajectories||[])for(const e of t.events||[])rows.push([...runMeta(result),t.id,e.index,scalarApprox(e.diskDisplacement),scalarApprox(e.referenceDisplacement),scalarApprox(e.pathLength),scalarApprox(e.diskClearance),scalarApprox(e.referenceClearance),e.tileAfter?.key,e.tileAfter?.label,e.edge,e.edgeIdentity?.boundaryKey,e.outgoingSide,e.incomingSide,e.symbol,e.precision,e.certification]);return rows.map(r=>r.map(csvEscape).join(',')).join('\n')+'\n'}
function crossingsCsv(result){if(result?.simulationType==='randomWalk')return diagnosticsCsv(result);let rows=[['run_id','simulation_type','backend','tiling_id','tiling_version','tiling_parameters','run_status','application_version','trajectory','bounce','disk_u_approx','disk_v_approx','reference_x_approx','reference_y_approx','before_tile_key','after_tile_key','edge','boundary_key','outgoing_side','incoming_side','symbol','incoming_du_approx','incoming_dv_approx','outgoing_du_approx','outgoing_dv_approx','time_enclosure','certification']];for(const t of result?.trajectories||[])for(const e of t.events||[])rows.push([...runMeta(result),t.id,e.index,scalarApprox(e.disk?.u),scalarApprox(e.disk?.v),scalarApprox(e.reference?.x),scalarApprox(e.reference?.y),e.tileBefore?.key,e.tileAfter?.key,e.edge,e.edgeIdentity?.boundaryKey,e.outgoingSide,e.incomingSide,e.symbol,scalarApprox(e.incoming?.du),scalarApprox(e.incoming?.dv),scalarApprox(e.outgoing?.du),scalarApprox(e.outgoing?.dv),e.collisionTime?.enclosure,e.certification]);return rows.map(r=>r.map(csvEscape).join(',')).join('\n')+'\n'}
function serveFile(res,file,mime){fs.readFile(file,(err,data)=>{if(err){res.writeHead(404);res.end('Not found');return}res.writeHead(200,{'content-type':mime,'cache-control':'no-cache'});res.end(data)});}
const mime={'.html':'text/html; charset=utf-8','.js':'text/javascript; charset=utf-8','.css':'text/css; charset=utf-8','.svg':'image/svg+xml'};

const server=http.createServer(async(req,res)=>{
  const url=new URL(req.url,`http://${req.headers.host||'localhost'}`);
  try{
    if(req.method==='GET'&&url.pathname==='/api/state')return json(res,200,state);
    if(req.method==='GET'&&url.pathname==='/api/health')return json(res,200,{ok:true,engine:ENGINE,engineAvailable:fs.existsSync(ENGINE),active:Boolean(child)});
    if(req.method==='GET'&&url.pathname==='/api/tilings')return json(res,200,{schemaVersion:'1.0',tilings:TILING_MANIFESTS});
    if(req.method==='POST'&&url.pathname==='/api/run'){const body=await readBody(req);const runId=startRun(body);return json(res,202,{runId});}
    if(req.method==='POST'&&url.pathname==='/api/cancel'){if(child)stopChild();state.status='Cancelled';state.locked=true;touch();return json(res,200,state);}
    if(req.method==='POST'&&url.pathname==='/api/reset'){if(child)stopChild();state=freshState(state.preview||state.request);return json(res,200,state);}
    if(req.method==='POST'&&url.pathname==='/api/preview'){const body=await readBody(req);if(!state.locked){state.preview=body;touch();}return json(res,200,{ok:true});}
    if(req.method==='POST'&&url.pathname==='/api/selection'){const body=await readBody(req);if(Number.isInteger(body.bounce)&&body.bounce>=0){state.selectedBounce=body.bounce;touch();}return json(res,200,{selectedBounce:state.selectedBounce});}
    if(req.method==='GET'&&url.pathname==='/api/export/run.json'){if(!state.result)return json(res,404,{error:'no run result'});res.writeHead(200,{'content-type':'application/json','content-disposition':`attachment; filename="${state.runId}.json"`});return res.end(JSON.stringify({request:state.request,result:state.result},null,2));}
    if(req.method==='GET'&&url.pathname==='/api/export/diagnostics.csv'){if(!state.result)return json(res,404,{error:'no run result'});res.writeHead(200,{'content-type':'text/csv','content-disposition':`attachment; filename="${state.runId}-diagnostics.csv"`});return res.end(diagnosticsCsv(state.result));}
    if(req.method==='GET'&&url.pathname==='/api/export/crossings.csv'){if(!state.result)return json(res,404,{error:'no run result'});res.writeHead(200,{'content-type':'text/csv','content-disposition':`attachment; filename="${state.runId}-crossings.csv"`});return res.end(crossingsCsv(state.result));}
    if(req.method==='GET'&&url.pathname==='/vendor/p5.min.js')return serveFile(res,path.join(ROOT,'node_modules/p5/lib/p5.min.js'),'text/javascript; charset=utf-8');
    const pages={'/':'launcher.html','/disk':'disk.html','/plane':'plane.html','/diagnostics':'diagnostics.html'};if(req.method==='GET'&&pages[url.pathname])return serveFile(res,path.join(WEB,'pages',pages[url.pathname]),mime['.html']);
    if(req.method==='GET'&&url.pathname.startsWith('/assets/')){const relative=url.pathname.slice('/assets/'.length);const file=path.resolve(WEB,relative);if(!file.startsWith(WEB+path.sep))return json(res,403,{error:'forbidden'});return serveFile(res,file,mime[path.extname(file)]||'application/octet-stream');}
    res.writeHead(404);res.end('Not found');
  }catch(error){json(res,400,{error:error.message});}
});
server.listen(PORT,'127.0.0.1',()=>console.log(`Hyperbolic Tiling Billiards: http://127.0.0.1:${PORT}`));
process.on('SIGINT',()=>{if(child)stopChild();server.close(()=>process.exit(0))});

module.exports={server,validateRequest};
