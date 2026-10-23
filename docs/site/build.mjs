import fs from 'node:fs/promises';
import {createHash} from 'node:crypto';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import {execFileSync} from 'node:child_process';
import {build} from 'esbuild';
import {createContent,escape as e,safeRelative} from './lib/content.mjs';
import {shell,home,guide,settings,releasePage,changelogPage} from './lib/pages.mjs';
const here=path.dirname(fileURLToPath(import.meta.url)),root=path.resolve(here,'../..'),dist=path.join(here,'dist'),base='/TrafficMonitor/';
const manifest=JSON.parse(await fs.readFile(path.join(root,'docs/site-content/manifest.json'),'utf8'));
const release=JSON.parse(await fs.readFile(path.join(root,'docs/release-metadata.json'),'utf8'));
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
await fs.writeFile(path.join(dist,'.nojekyll'),'');
await page('index.html','TrafficMonitor',home(context));
await page('guide/index.html','Guided setup',guide(context));
await page('settings/index.html','Reading preferences',settings(context));
await page('downloads/index.html','Downloads and release status',releasePage(context));
let entries=[],history=null;
try{history=JSON.parse(await fs.readFile(path.join(root,'docs/site-changelog/manifest.json'),'utf8'));for(const file of history.files){const data=JSON.parse(await fs.readFile(path.join(root,'docs/site-changelog',safeRelative(file.path)),'utf8'));entries.push(...data.entries);}if(entries.length!==history.entryCount)throw Error('History entry count mismatch');}catch(err){if(err.code!=='ENOENT')throw err;}
await fs.writeFile(path.join(dist,'history.json'),JSON.stringify({entries,manifest:history}));
await page('changelog/index.html','Changelog',changelogPage(entries.length));
const content=await createContent(root,dist,manifest,base);
const categories=[...new Set(manifest.articles.map(a=>a.category))];
const list=items=>`<ul class="article-list">${items.map(a=>`<li data-search="${e(a.title+' '+a.category+' '+a.language)}"><a href="${base+a.route.slice(1)}">${e(a.title)}</a><span>${e(a.category)} · ${e(a.language)}</span></li>`).join('')}</ul>`;
await page('docs/index.html','Documentation',`<p class="eyebrow">LEARN AND TROUBLESHOOT</p><h1 data-en="Documentation" data-yue="使用文件">Documentation</h1><p>Complete articles and the preserved upstream wiki, organized by the task you want to finish. Original articles keep their source language.</p><div class="filters"><md-outlined-text-field id="article-search" label="Search article titles" type="search"></md-outlined-text-field><label>Category <select id="article-category"><option value="">All categories</option>${categories.map(c=>`<option>${e(c)}</option>`).join('')}</select></label></div><p role="status" id="article-count">${manifest.articles.length} articles</p><nav class="category-links">${categories.map(c=>`<a href="${base}categories/${c}/">${e(c)}</a>`).join('')}</nav>${list(manifest.articles)}`);
for(const category of categories)await page(`categories/${category}/index.html`,category,`<p class="eyebrow">DOCUMENTATION COLLECTION</p><h1>${e(category)}</h1><a href="${base}docs/">All documentation</a>${list(manifest.articles.filter(a=>a.category===category))}`);
for(const article of manifest.articles){const rendered=await content.render(article);await page(article.route.slice(1)+'index.html',article.title,`<div class="article-layout"><aside aria-label="On this page"><a href="${base}categories/${article.category}/">${e(article.category)}</a><h2>On this page</h2>${rendered.toc.map(h=>`<a href="#${e(h.id)}">${e(h.title)}</a>`).join('')}</aside><div><div class="notice">Source language: ${e(article.language)}. Preserved documentation snapshot, ${e(manifest.sourceCommit.slice(0,7))}. ${article.historicalPresentation?'Images and interface descriptions are historical upstream material. They do not show the rebuilt interface.':'Images retain their documented version and provenance; they do not verify a newer build.'} <a href="${e(article.sourceUrl)}">View original source</a></div><article>${rendered.html}</article><a href="${base}docs/">Explore related documentation</a></div></div>`);}
await page('404.html','Page not found',`<h1>Page not found</h1><p>This address is not in this documentation snapshot.</p><a href="${base}docs/">Find an article</a>`);
await fs.writeFile(path.join(dist,'build-receipt.json'),JSON.stringify({schemaVersion:1,sourceCommit:source,contentSource:manifest.sourceCommit,wikiSource:manifest.wikiSource,articleCount:manifest.articles.length,historyCount:entries.length,base,unresolvedAssets:content.unresolved},null,2));
console.log(`Built ${manifest.articles.length} complete articles; ${entries.length} history records; ${content.unresolved.length} unavailable local images.`);


