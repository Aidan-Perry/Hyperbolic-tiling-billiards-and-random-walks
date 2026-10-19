(() => {
  const listeners = new Set();
  const channel = new BroadcastChannel('htb-workspace-v1');
  let snapshot = null;
  let lastRevision = -1;
  async function refresh() {
    try {
      const response = await fetch('/api/state', {cache:'no-store'});
      if (!response.ok) return;
      const next = await response.json();
      if (next.revision !== lastRevision) { snapshot=next; lastRevision=next.revision; listeners.forEach(fn=>fn(next)); }
    } catch { /* the next poll will recover */ }
  }
  channel.onmessage = event => {
    if(event.data?.type==='refresh') refresh();
    if(event.data?.type==='selection'&&snapshot){ snapshot={...snapshot,selectedBounce:event.data.bounce}; listeners.forEach(fn=>fn(snapshot)); }
    if(event.data?.type==='preview'&&snapshot&&!snapshot.locked){snapshot={...snapshot,preview:event.data.preview};listeners.forEach(fn=>fn(snapshot));}
  };
  function subscribe(fn){listeners.add(fn);if(snapshot)fn(snapshot);return()=>listeners.delete(fn)}
  async function post(url,body={}){const response=await fetch(url,{method:'POST',headers:{'content-type':'application/json'},body:JSON.stringify(body)});const value=await response.json();if(!response.ok)throw new Error(value.error||`request failed: ${response.status}`);channel.postMessage({type:'refresh'});await refresh();return value}
  function selectBounce(bounce){bounce=Math.max(0,Math.trunc(bounce));channel.postMessage({type:'selection',bounce});post('/api/selection',{bounce}).catch(()=>{});}
  function publishPreview(preview){
    channel.postMessage({type:'preview',preview});
    if(snapshot&&!snapshot.locked){snapshot={...snapshot,preview};listeners.forEach(fn=>fn(snapshot));}
  }
  function persistPreview(preview){return post('/api/preview',preview)}
  function sendPreview(preview){publishPreview(preview);persistPreview(preview).catch(()=>{});}
  function rational(text){const s=String(text??'0');if(s.includes('/')){const [a,b]=s.split('/');return Number(a)/Number(b)}return Number(s)}
  function approx(scalar){if(scalar===undefined||scalar===null)return NaN;return scalar?.approx ?? Number(scalar?.display)}
  function point(event,space='disk'){const p=space==='disk'?event?.disk:event?.reference;return space==='disk'?[approx(p?.u),approx(p?.v)]:[approx(p?.x),approx(p?.y)]}
  window.HTB={subscribe,refresh,post,selectBounce,sendPreview,publishPreview,persistPreview,rational,approx,point,get snapshot(){return snapshot}};
  refresh();setInterval(refresh,500);
})();
