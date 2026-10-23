import fs from 'node:fs/promises';
import {createHash} from 'node:crypto';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import {execFileSync} from 'node:child_process';
import {build} from 'esbuild';
import {createContent,escape as e,safeRelative} from './lib/content.mjs';
import {collectHistory} from './lib/history.mjs';
import {JSDOM} from 'jsdom';
import {paths,search,docsHome,taskCards,taskPage} from './lib/learning.mjs';
import {capabilityPage} from './lib/capabilities.mjs';
import {readerSettings} from './lib/reader-settings.mjs';
import {settingsContract} from './lib/settings-contract.mjs';
import {shell,home,guide,settings,releasePage,changelogPage} from './lib/pages.mjs';
const here=path.dirname(fileURLToPath(import.meta.url)),root=path.resolve(here,'../..'),dist=path.join(here,'dist'),base='/TrafficMonitor/';
const manifest=JSON.parse(await fs.readFile(path.join(root,'docs/site-content/manifest.json'),'utf8'));
const releaseBytes=await fs.readFile(path.join(root,'docs/release-metadata.json'));
const release=JSON.parse(releaseBytes);
if(manifest.schemaVersion!==1||!manifest.articles.length)throw Error('Content manifest is missing articles');
if(release.isDraft!==false||release.isPrerelease!==false)throw Error('Download metadata must describe a normal release');
const source=execFileSync('git',['rev-parse','HEAD'],{cwd:root,encoding:'utf8'}).trim();
if(path.dirname(dist)!==here||path.basename(dist)!=='dist')throw Error('Unexpected output directory');
await fs.rm(dist,{recursive:true,force:true});
await fs.mkdir(dist,{recursive:true});
for(const asset of manifest.assets??[]){
 const relative=safeRelative(asset.sourcePath),bytes=await fs.readFile(path.join(root,relative));
 if(createHash('sha256').update(bytes).digest('hex')!==asset.sha256)throw Error('Asset hash mismatch');
 const target=path.join(dist,'media',relative);await fs.mkdir(path.dirname(target),{recursive:true});await fs.writeFile(target,bytes);
}
const context={base,release,source,manifest};
async function page(route,title,body){const destination=path.join(dist,safeRelative(route?decodeURIComponent(route.replace(/^\//,'')):'index.html'));await fs.mkdir(path.dirname(destination),{recursive:true});await fs.writeFile(destination,shell(title,body,context));}
await build({entryPoints:[path.join(here,'src/app.js')],bundle:true,format:'esm',outfile:path.join(dist,'app.js'),minify:true,legalComments:'linked'});
await fs.copyFile(path.join(here,'src/style.css'),path.join(dist,'style.css'));
await fs.mkdir(path.join(dist,'regex-engine'),{recursive:true});
for(const name of ['worker.mjs','evaluate.mjs','request.mjs'])await fs.copyFile(path.join(here,'src/regex-engine',name),path.join(dist,'regex-engine',name));
await fs.writeFile(path.join(dist,'.nojekyll'),'');
await page('index.html','TrafficMonitor',home(context));
await page('guide/index.html','Guided setup',guide(context));
await page('settings/index.html','Reading preferences',`<section class="doc-search"><div class="search-row"><md-outlined-text-field id="settings-search" label="Search settings" type="search"></md-outlined-text-field><md-text-button id="settings-search-clear">Clear search</md-text-button></div><p id="settings-search-state" role="status"></p></section>`+settings(context)+readerSettings()+`<section class="panel"><h2>Reading and search recovery</h2><p>Clear visitor-owned progress or saved search queries without changing application settings or personal wording.</p><div class="actions"><md-outlined-button id="reading-progress-reset">Clear reading progress</md-outlined-button><md-outlined-button id="search-state-reset">Clear saved search queries</md-outlined-button></div><a href="${base}settings-contract.json">Complete control and consumer mapping</a></section>`);
await page('downloads/index.html','Downloads and release status',releasePage(context));
const {entries,manifest:history}=await collectHistory(root,source);
await fs.writeFile(path.join(dist,'history.json'),JSON.stringify({entries,manifest:history}));
await page('changelog/index.html','Changelog',changelogPage(entries.length));
const content=await createContent(root,dist,manifest,base);
const renderedArticles=new Map(),searchIndex=[];
for(const article of manifest.articles){const rendered=await content.render(article);renderedArticles.set(article.id,rendered);const document=new JSDOM(rendered.html).window.document;searchIndex.push({id:article.id,title:article.title,route:article.route,text:document.body.textContent.replace(/\s+/g,' ').trim()});}
await fs.writeFile(path.join(dist,'search-index.json'),JSON.stringify(searchIndex));
const inventoryBytes=execFileSync('git',['show',`${manifest.sourceCommit}:docs/features/required-features/inventory.json`],{cwd:root,maxBuffer:8*1024*1024});
const inventory=JSON.parse(inventoryBytes);if(inventory.schemaVersion!==1||inventory.inventoryCount!==110)throw Error('Unexpected capability inventory');
await fs.writeFile(path.join(dist,'settings-contract.json'),JSON.stringify(settingsContract(inventory,manifest.sourceCommit),null,2));
await fs.writeFile(path.join(dist,'capability-provenance.json'),JSON.stringify({sourceCommit:manifest.sourceCommit,sha256:createHash('sha256').update(inventoryBytes).digest('hex'),identities:inventory.items.map(i=>i.id)}));
const capabilities=capabilityPage(inventory.items,base,manifest.articles);
await page('features/index.html','Explore capabilities',capabilities);
const categories=[...new Set(manifest.articles.map(a=>a.category))];
const list=items=>`<ul class="article-list">${items.map(a=>`<li data-search="${e(a.title+' '+a.category+' '+a.language)}"><a href="${base+a.route.slice(1)}">${e(a.title)}</a><span>${e(a.category)} · ${e(a.language)}</span></li>`).join('')}</ul>`;
const categoryNames={overview:'Getting started',help:'Understand and troubleshoot',releases:'Release notes',build:'Build and contribute',installation:'Install and update','required-features':'Explore capabilities',wiki:'Complete manual','contributor-records':'Project records',evidence:'Verification evidence'};
await page('docs/index.html','Documentation',docsHome(base)+`<details class="manual-library"><summary>Browse the complete documentation library (${manifest.articles.length} articles)</summary><nav class="category-links">${categories.map(c=>`<a href="${base}categories/${c}/">${e(categoryNames[c]??c)}</a>`).join('')}</nav>${list(manifest.articles)}</details>`);
for(const p of paths)await page(`docs/tasks/${p.id}/index.html`,p.title,taskPage(p,base,list(manifest.articles.filter(a=>a.category===p.category).slice(0,5))));
for(const category of categories)await page(`categories/${category}/index.html`,categoryNames[category]??category,category==='required-features'?capabilities:`<nav class="breadcrumbs"><a href="${base}docs/">Documentation</a></nav><h1>${e(categoryNames[category]??category)}</h1>${search()}${taskCards(base,paths.filter(p=>p.category===category))}<h2>Complete articles</h2>${list(manifest.articles.filter(a=>a.category===category))}`);
for(const article of manifest.articles){const rendered=renderedArticles.get(article.id);await page(article.route.slice(1)+'index.html',article.title,`<nav class="breadcrumbs"><a href="${base}docs/">Documentation</a><a href="${base}categories/${article.category}/">${e(categoryNames[article.category]??article.category)}</a></nav>${search()}<div class="article-layout"><aside aria-label="On this page"><details open><summary>On this page</summary>${rendered.toc.map(h=>`<a href="#${e(h.id)}">${e(h.title)}</a>`).join('')}</details></aside><div><details class="source-provenance"><summary>Source and version</summary><p>Source language: ${e(article.language)}. Preserved documentation snapshot, ${e(manifest.sourceCommit.slice(0,7))}. ${article.historicalPresentation?'Images and interface descriptions are historical upstream material. They do not show the rebuilt interface.':'Images retain their documented version and provenance; they do not verify a newer build.'}</p><a href="${e(article.sourceUrl)}">View original source</a></details><article>${rendered.html}</article><section class="related"><h2>What to do next</h2>${taskCards(base,paths.filter(p=>p.category===article.category).slice(0,2))}<a href="${base}docs/">Explore related documentation</a></section></div></div>`);}
await page('404.html','Page not found',`<h1>Page not found</h1><p>This address is not in this documentation snapshot.</p><a href="${base}docs/">Find an article</a>`);
await fs.writeFile(path.join(dist,'build-receipt.json'),JSON.stringify({schemaVersion:1,sourceCommit:source,contentSource:manifest.sourceCommit,wikiSource:manifest.wikiSource,releaseMetadataSHA256:createHash('sha256').update(releaseBytes).digest('hex'),releaseTag:release.tagName,releaseSource:release.targetCommitish,articleCount:manifest.articles.length,historyCount:entries.length,base,unresolvedAssets:content.unresolved},null,2));
console.log(`Built ${manifest.articles.length} complete articles; ${entries.length} history records; ${content.unresolved.length} unavailable local images.`);



