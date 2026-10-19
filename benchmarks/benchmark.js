const { spawnSync } = require('node:child_process');
const path = require('node:path');
const root=path.resolve(__dirname,'..'),engine=path.join(root,'build','htb_engine');
for(const mode of ['scientific','exact']){const r=spawnSync(engine,['--benchmark',mode],{cwd:root,encoding:'utf8',stdio:['ignore','pipe','inherit']});if(r.error)throw r.error;if(r.status!==0)process.exit(r.status);process.stdout.write(r.stdout)}
