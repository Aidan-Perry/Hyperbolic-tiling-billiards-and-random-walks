((root,factory)=>{const api=factory();if(typeof module==='object'&&module.exports)module.exports=api;else root.HTBLanguage=api})(typeof globalThis==='object'?globalThis:this,()=>{
  // Every non-root suffix-automaton state represents one distinct factor at
  // each length in (len(link(state)), len(state)]. Range accumulation counts
  // all of those intervals in O(number of tokens).
  function factorComplexity(tokens){
    const states=[{len:0,link:-1,next:new Map()}];let last=0;
    for(const token of tokens){let p=last,cur=states.length;states.push({len:states[p].len+1,link:0,next:new Map()});while(p>=0&&!states[p].next.has(token)){states[p].next.set(token,cur);p=states[p].link}if(p>=0){const q=states[p].next.get(token);if(states[p].len+1===states[q].len)states[cur].link=q;else{const clone=states.length;states.push({len:states[p].len+1,link:states[q].link,next:new Map(states[q].next)});while(p>=0&&states[p].next.get(token)===q){states[p].next.set(token,clone);p=states[p].link}states[q].link=states[cur].link=clone}}last=cur}
    const difference=new Array(tokens.length+2).fill(0);for(let v=1;v<states.length;v++){const lo=states[states[v].link].len+1,hi=states[v].len;difference[lo]++;difference[hi+1]--}const values=[];let active=0;for(let n=1;n<=tokens.length;n++){active+=difference[n];values.push(active)}return values;
  }
  return{factorComplexity};
});
