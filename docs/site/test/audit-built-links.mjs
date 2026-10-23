import fs from 'node:fs/promises';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import {JSDOM} from 'jsdom';
const root=new URL('../dist/',import.meta.url),files=[];
async function walk(url){for(const entry of await fs.readdir(url,{withFileTypes:true})){const child=new URL(encodeURIComponent(entry.name)+(entry.isDirectory()?'/':''),url);if(entry.isDirectory())await walk(child);else if(entry.name.endsWith('.html'))files.push(child);}}
await walk(root);let checked=0;const missing=[];
for(const file of files){const document=new JSDOM(await fs.readFile(file,'utf8')).window.document;for(const element of document.querySelectorAll('a[href],img[src],script[src],link[href]')){const value=element.getAttribute('href')??element.getAttribute('src');if(!value.startsWith('/TrafficMonitor/'))continue;const target=value.slice('/TrafficMonitor/'.length).split('#')[0].split('?')[0];const decoded=decodeURIComponent(target);const resolved=path.join(fileURLToPath(root),decoded.endsWith('/')?decoded+'index.html':decoded||'index.html');try{await fs.access(resolved);checked++;}catch{missing.push({page:path.basename(file.pathname),target:value});}}}
console.log(JSON.stringify({pages:files.length,checked,missingCount:missing.length,missing:missing.slice(0,20)},null,2));if(missing.length)process.exitCode=1;


