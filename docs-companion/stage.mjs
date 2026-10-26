import fs from 'node:fs/promises';
import path from 'node:path';
import {createHash} from 'node:crypto';
import {execFileSync} from 'node:child_process';
import {fileURLToPath} from 'node:url';
const here=path.dirname(fileURLToPath(import.meta.url)),root=path.dirname(here);
const source=execFileSync('git',['rev-parse','HEAD'],{cwd:root,encoding:'utf8'}).trim();
const input=path.join(root,'docs/site/dist'),output=path.join(here,'site');
const receipt=JSON.parse(await fs.readFile(path.join(input,'build-receipt.json'),'utf8'));
if(receipt.sourceCommit!==source||receipt.articleCount<1)throw Error('Documentation output is not bound to HEAD');
const content=JSON.parse(await fs.readFile(path.join(root,'artifacts/documentation-content-manifest.json'),'utf8'));
if(content.sourceCommit!==source)throw Error('Offline inventory source does not match HEAD');
for(const article of content.articles)await fs.access(path.join(input,decodeURIComponent(article.route.replace(/^\//,''))+'index.html'));
// The project mark is staged from the native application's master icon; the build never fetches an icon remotely.
const masterIcon=await fs.readFile(path.join(root,'TrafficMonitor/res/TrafficMonitor.ico'));
const iconCount=masterIcon.readUInt16LE(4),iconSizes=new Set();
for(let i=0;i<iconCount;i++)iconSizes.add(masterIcon[6+i*16]||256);
for(const size of [16,24,32,48,64,256])if(!iconSizes.has(size))throw Error('Master icon lacks '+size+'px image');
await fs.writeFile(path.join(here,'project-mark.ico'),masterIcon);
await fs.rm(output,{recursive:true,force:true});
await fs.cp(input,output,{recursive:true,dereference:false});
const files=[];
async function inventory(dir){for(const entry of await fs.readdir(dir,{withFileTypes:true})){const file=path.join(dir,entry.name);if(entry.isSymbolicLink())throw Error('Symbolic links are not permitted');if(entry.isDirectory())await inventory(file);else{const bytes=await fs.readFile(file);files.push({path:path.relative(output,file).replaceAll('\\','/'),bytes:bytes.length,sha256:createHash('sha256').update(bytes).digest('hex')});}}}
await inventory(output);files.sort((a,b)=>a.path.localeCompare(b.path));
await fs.writeFile(path.join(output,'offline-inventory.json'),JSON.stringify({schemaVersion:1,sourceCommit:source,version:process.argv[2]||'1.0.0',runtime:'45.0.0',builder:'26.15.3',articleCount:receipt.articleCount,wikiSource:receipt.wikiSource,unresolvedAssets:receipt.unresolvedAssets,networkPolicy:'All external requests and navigation blocked; source and download links require the hosted documentation.',runtimeVerification:'pending',files},null,2));
console.log(`Staged ${files.length} offline files and ${receipt.articleCount} articles at source ${source}`);
