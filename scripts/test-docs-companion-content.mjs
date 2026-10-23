import fs from 'node:fs/promises';
import path from 'node:path';
import {execFileSync,spawnSync} from 'node:child_process';
import {createHash} from 'node:crypto';
import assert from 'node:assert/strict';
import {createContent} from '../docs/site/lib/content.mjs';
const root=process.cwd(),manifest=JSON.parse(await fs.readFile('artifacts/documentation-content-manifest.json','utf8'));
const source=execFileSync('git',['rev-parse','HEAD'],{encoding:'utf8'}).trim();
assert.equal(manifest.sourceCommit,source);
const receipt=JSON.parse(await fs.readFile('docs/site/dist/build-receipt.json','utf8'));assert.equal(receipt.sourceCommit,source);
const expected=['README.md','docs/features/installation/preparation-reliability.md','docs/features/build/offline-documentation-companion.md','docs/wiki/pending/Home.md'];
for(const name of expected){const article=manifest.articles.find(a=>a.sourcePath===name);assert.ok(article,`Missing required article ${name}`);const bytes=execFileSync('git',['show',`${source}:${name}`]);assert.equal(article.sourceSha256,createHash('sha256').update(bytes).digest('hex'));await fs.access(path.join('docs/site/dist',decodeURIComponent(article.route.slice(1)),'index.html'));}
const negative=structuredClone(manifest);negative.articles=negative.articles.filter(a=>a.sourcePath!==expected[1]);const badPath='artifacts/documentation-omission-negative.json';await fs.writeFile(badPath,JSON.stringify(negative));
const rejected=spawnSync(process.execPath,['docs/site/build.mjs','--manifest',badPath],{encoding:'utf8'});assert.notEqual(rejected.status,0);assert.match(rejected.stderr,/omits or duplicates/);
const content=await createContent(root,path.join(root,'artifacts/content-negative'),manifest,'/TrafficMonitor/');const changed={...manifest.articles.find(a=>a.sourcePath==='README.md'),sourceSha256:'0'.repeat(64)};await assert.rejects(()=>content.render(changed),/Content hash mismatch/);
const after=JSON.parse(await fs.readFile('docs/site/dist/build-receipt.json','utf8'));assert.deepEqual(after,receipt);
console.log(JSON.stringify({sourceCommit:source,articleCount:manifest.articles.length,expectedArticles:expected,currentArticlePositive:true,modifiedArticleHashPositive:true,omissionNegativeExit:rejected.status,modifiedHashNegativeObserved:true,outputReceiptUnchanged:true}));

