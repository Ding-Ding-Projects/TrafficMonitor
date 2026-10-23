import {execFileSync,spawnSync} from 'node:child_process';
import {mkdirSync,readFileSync,writeFileSync,existsSync,realpathSync,lstatSync} from 'node:fs';
import {homedir} from 'node:os';
import path from 'node:path';
const root=realpathSync(process.cwd());
const git=args=>execFileSync('git',['-c','core.hooksPath=',...args],{cwd:root,windowsHide:true,maxBuffer:16777216,stdio:['pipe','pipe','pipe']});
try {
 const pin=readFileSync(path.join(root,'.publication-scanner-pin'),'utf8').trim();
 if(!/^[a-f0-9]{40}$/.test(pin))throw new Error();
 if(git(['remote','get-url','origin']).toString().trim().replace(/\.git$/,'')!=='https://github.com/Ding-Ding-Projects/TrafficMonitor')throw new Error();
 try{git(['cat-file','-e',pin+'^{commit}']);}catch{git(['fetch','--no-tags','origin',pin]);}
 const cache=path.join(process.env.LOCALAPPDATA||path.join(homedir(),'.cache'),'TrafficMonitorPublicationScanner',pin);
 if(cache===root||cache.startsWith(root+path.sep))throw new Error();
 mkdirSync(cache,{recursive:true});
 if(lstatSync(cache).isSymbolicLink()||realpathSync(cache)!==cache)throw new Error();
 const files=['public-language-scan.mjs','publication-binary-content.mjs','publication-exceptions.mjs','publication-font-decoder.mjs','publication-font-decoder-child.mjs','run-publication-language-ci.mjs','legacy-text-inventory.mjs','publication-history.mjs','run-local.mjs'];
 for(const file of files){const bytes=git(['show',pin+':scripts/publication/'+file]);const target=path.join(cache,file);if(existsSync(target)&&lstatSync(target).isSymbolicLink())throw new Error();if(!existsSync(target)||!readFileSync(target).equals(bytes))writeFileSync(target,bytes,{mode:0o600});}
 const result=spawnSync(process.execPath,[path.join(cache,'run-local.mjs'),root,...process.argv.slice(2)],{stdio:'inherit',windowsHide:true});
 process.exit(result.status??2);
}catch{process.stderr.write('Trusted publication snapshot could not be verified; operation blocked.\n');process.exit(2);}
