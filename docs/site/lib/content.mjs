import path from 'node:path';
import {createHash} from 'node:crypto';
import fs from 'node:fs/promises';
import {execFileSync} from 'node:child_process';
import {marked} from 'marked';
import {JSDOM} from 'jsdom';
import createDOMPurify from 'dompurify';
export const escape = s => String(s ?? '').replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
export function safeRelative(value) {
 const p=path.posix.normalize(value.replaceAll('\\','/'));
 if(p.startsWith('../')||p==='..'||path.posix.isAbsolute(p)||p.includes(':')) throw Error('Path is outside the content root');
 return p;
}
export function isOwnedGithubUrl(url, trustedRefs=['main','master']) {
 if(url.hostname!=='github.com')return false;
 const parts=url.pathname.split('/');
 if(!['Ding-Ding-Projects','zhongyang219'].includes(parts[1])||parts[2]!=='TrafficMonitor')return false;
 return parts[3]==='wiki'||(parts[3]==='blob'&&trustedRefs.includes(parts[4]));
}
export async function createContent(root,dist,manifest,base) {
 const window=new JSDOM('').window, purifier=createDOMPurify(window);
 const byPath=new Map(manifest.articles.map(a=>[a.sourcePath.toLowerCase(),a]));
 const byId=new Map(manifest.articles.map(a=>[a.id,a]));
 const aliases=new Map((manifest.wikiAliases??[]).map(a=>[a.alias.toLowerCase(),byId.get(a.articleId)]));
 const copied=new Set(), unresolved=[];
 async function resolve(raw,article,image=false) {
  if(!raw||raw.startsWith('#')) return raw;
  let decoded; try{decoded=decodeURI(raw);}catch{return raw;}
  const [withoutHash,hash='']=decoded.split('#');
  const suffix=hash?'#'+encodeURI(hash):'';
  let match=aliases.get(withoutHash.toLowerCase());
  const known=manifest.articles.find(a=>a.sourceUrl===withoutHash);
  if(known) match=known;
  if(match&&!image) return base+match.route.slice(1)+suffix;
  if(/^https?:/i.test(withoutHash)) {
   const u=new URL(withoutHash);
   if(isOwnedGithubUrl(u,['main','master',manifest.sourceCommit])) {
    const segments=u.pathname.split('/');
    const wikiIndex=segments.indexOf('wiki');
    if(wikiIndex>=0) match=aliases.get(decodeURIComponent(segments.slice(wikiIndex+1).join('/')).toLowerCase());
    else if(segments[3]==='blob') match=byPath.get(decodeURIComponent(segments.slice(5).join('/')).toLowerCase());
    if(match&&!image)return base+match.route.slice(1)+suffix;
   }
   return image?null:raw;
  }
  if(/^[a-z][a-z\d+.-]*:/i.test(withoutHash)) return image?null:raw;
  let local;
  try{local=safeRelative(path.posix.join(path.posix.dirname(article.sourcePath),withoutHash));}catch{return null;}
  match=byPath.get(local.toLowerCase());
  if(!match&&article.provenanceKind.includes('wiki'))match=aliases.get(withoutHash.toLowerCase());
  if(match&&!image)return base+match.route.slice(1)+suffix;
  if(/\.(png|jpe?g|gif|webp|svg|ico|pdf)$/i.test(local)) {
   try {
    await fs.access(path.join(root,local));
    if(!copied.has(local)){await fs.mkdir(path.dirname(path.join(dist,'media',local)),{recursive:true});await fs.copyFile(path.join(root,local),path.join(dist,'media',local));copied.add(local);}
    return base+'media/'+local.split('/').map(encodeURIComponent).join('/')+suffix;
   }catch{unresolved.push({article:article.id,target:raw});return null;}
  }
  if(!image)return new URL(raw,article.sourceUrl.replace('/blob/','/raw/')).href;
  return null;
 }
 return {unresolved,async render(article){
  const markdown=article.provenanceKind==='repository-markdown' ? execFileSync('git',['show',`${manifest.sourceCommit}:${article.sourcePath}`],{cwd:root,encoding:'utf8',maxBuffer:8*1024*1024}) : await fs.readFile(path.join(root,article.sourcePath),'utf8');
  if(createHash('sha256').update(markdown).digest('hex')!==article.sourceSha256)throw Error('Content hash mismatch: '+article.id);
  const document=new JSDOM(purifier.sanitize(marked.parse(markdown),{FORBID_TAGS:['style','iframe','form','input','button'],FORBID_ATTR:['style']})).window.document;
  const used=new Map();
  for(const h of document.querySelectorAll('h1,h2,h3,h4,h5,h6')){
   const slug=h.textContent.toLowerCase().replace(/[^\p{L}\p{N}_\s-]/gu,'').replace(/\s/g,'-');const n=used.get(slug)??0;used.set(slug,n+1);h.id=slug+(n?'-'+n:'');
  }
  for(const a of document.querySelectorAll('a[href]')){const resolved=await resolve(a.getAttribute('href'),article);if(resolved)a.setAttribute('href',resolved);else a.removeAttribute('href');a.setAttribute('rel','noopener noreferrer');}
  for(const img of document.querySelectorAll('img')){
   const raw=img.getAttribute('src');const resolved=await resolve(raw,article,true);
   if(resolved){img.src=resolved;img.loading='lazy';img.removeAttribute('srcset');img.alt=img.alt||'Historical documentation illustration';}
   else{const a=document.createElement('a');a.textContent='External historical image (open original)';if(/^https?:/i.test(raw??''))a.href=raw;img.replaceWith(a);}
  }
  const toc=[...document.querySelectorAll('h2,h3')].map(h=>({id:h.id,title:h.textContent}));
  return {html:document.body.innerHTML,toc};
 }};
}



