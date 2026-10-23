import {storage} from './wording.js';
import {bindRegexSearch} from './regex-search.js';
export function setupLearning(base,notify){
 const key='learning-progress-v1',saved=storage.read(key,{});let progress=saved&&typeof saved==='object'&&!Array.isArray(saved)?saved:{};
 const workspace=document.querySelector('[data-learning-path]');
 const write=()=>{if(!storage.write(key,progress))notify('Reading progress is available for this visit only. Browser storage is unavailable.');};
 for(const el of document.querySelectorAll('[data-path-progress]')){const p=progress[el.dataset.pathProgress];if(p?.read?.length)el.textContent=`${p.read.length} steps marked read`;}
 const resume=document.getElementById('resume-learning');
 const latest=Object.entries(progress).filter(([id,p])=>/^[a-z]+$/.test(id)&&p&&Number.isInteger(p.step)).sort((a,b)=>(b[1].updated??0)-(a[1].updated??0))[0];
 if(resume&&latest){resume.hidden=false;const a=document.createElement('a');a.href=`${base}docs/tasks/${latest[0]}/#step-${latest[1].step}`;a.textContent='Resume your learning path';resume.append(a);}
 if(workspace){
  const id=workspace.dataset.learningPath,steps=[...workspace.querySelectorAll('[data-step]')],old=progress[id];
  let active=Math.max(0,Math.min(steps.length-1,Number(location.hash.match(/^#step-(\d+)$/)?.[1]??old?.step??0)||0));
  let read=new Set(Array.isArray(old?.read)?old.read.filter(i=>Number.isInteger(i)&&i>=0&&i<steps.length):[]);
  const render=()=>{steps.forEach((s,i)=>s.hidden=i!==active);workspace.querySelectorAll('[data-step-link]').forEach((a,i)=>{if(i===active)a.setAttribute('aria-current','step');else a.removeAttribute('aria-current');});workspace.querySelectorAll('[data-step-read]').forEach(c=>c.checked=read.has(Number(c.dataset.stepRead)));workspace.querySelector('[data-path-back]').disabled=active===0;workspace.querySelector('[data-path-next]').disabled=active===steps.length-1;workspace.querySelector('[data-path-status]').textContent=`${read.size} of ${steps.length} steps marked read. This is not installation verification.`;};
  const save=()=>{progress[id]={step:active,read:[...read],updated:Date.now()};write();};
  const go=i=>{active=Math.max(0,Math.min(steps.length-1,i));save();render();history.replaceState(null,'',`#step-${active}`);};
  workspace.querySelectorAll('[data-step-link]').forEach(a=>a.addEventListener('click',event=>{event.preventDefault();go(Number(a.dataset.stepLink));}));
  workspace.querySelectorAll('[data-step-read]').forEach(c=>c.addEventListener('change',()=>{const i=Number(c.dataset.stepRead);c.checked?read.add(i):read.delete(i);save();render();}));
  workspace.querySelector('[data-path-back]').addEventListener('click',()=>go(active-1));workspace.querySelector('[data-path-next]').addEventListener('click',()=>go(active+1));
  workspace.querySelector('[data-path-reset]').addEventListener('click',()=>{read.clear();go(0);});render();
 }
 const directions={receive:'Receive means data arriving at your computer.',send:'Send means data leaving your computer.'};
 for(const b of document.querySelectorAll('[data-direction]'))b.addEventListener('click',()=>{document.getElementById('flow-arrow').textContent=b.dataset.direction==='send'?'←':'→';document.getElementById('flow-explanation').textContent=directions[b.dataset.direction];for(const other of document.querySelectorAll('[data-direction]'))other.setAttribute('aria-pressed',String(other===b));});
 const symptoms={missing:'Look in the notification area and check the taskbar display option before changing other settings.',zero:'Check whether the selected adapter is connected, then compare with an application that is actively transferring data.',connection:'Open the connection menu and compare automatic, all-connections and a specific adapter with the network you are using.'};
 document.getElementById('symptom')?.addEventListener('change',event=>document.getElementById('symptom-explanation').textContent=symptoms[event.target.value]);
}

export async function setupDocumentationSearch(base){
 const regions=[...document.querySelectorAll('[data-search-owner]')];if(!regions.length)return;
 let index;try{const response=await fetch(base+'search-index.json');if(!response.ok)throw Error();index=await response.json();if(!Array.isArray(index))throw Error();}catch{for(const r of regions)r.querySelector('[data-search-state]').textContent='Search is unavailable. Browse the learning paths and complete manual below.';return;}
 for(const region of regions){
  const field=region.querySelector('[data-doc-search]'),results=region.querySelector('[data-search-results]'),state=region.querySelector('[data-search-state]'),more=region.querySelector('[data-search-more]');
  const key='search-'+region.dataset.searchOwner,stored=storage.read(key,{});field.value=typeof stored?.query==='string'?stored.query:'';let limit=12,regexMatches=null;
  const render=()=>{const q=field.value.trim().toLocaleLowerCase();results.replaceChildren();if(!q&&!regexMatches){state.textContent='Choose a learning path or search the complete manual.';more.hidden=true;return;}const matches=index.filter(a=>regexMatches?regexMatches.has(a):(a.title+' '+a.text).toLocaleLowerCase().includes(q));state.textContent=`${matches.length} matching articles`;more.hidden=matches.length<=limit;
   for(const a of matches.slice(0,limit)){const row=document.createElement('div'),link=document.createElement('a'),excerpt=document.createElement('p');row.className='search-result';link.href=base+a.route.slice(1);link.textContent=a.title;const at=a.text.toLocaleLowerCase().indexOf(q);excerpt.textContent=a.text.slice(Math.max(0,at-65),Math.max(0,at-65)+210);link.addEventListener('click',()=>storage.write(key,{query:field.value,focus:a.route}));row.append(link,excerpt);results.append(row);}if(!matches.length)state.textContent='No matching articles. Try a shorter phrase or another category.';storage.write(key,{...storage.read(key,{}),query:field.value});};
  bindRegexSearch(field,index,a=>a.title+' '+a.text,matches=>{regexMatches=matches;render();},state);
  field.addEventListener('input',()=>{limit=12;render();});field.addEventListener('keydown',event=>{if(event.key==='Escape'&&!event.isComposing){field.value='';field.dispatchEvent(new Event('input'));}});region.querySelector('[data-search-clear]').addEventListener('click',()=>{field.value='';field.dispatchEvent(new Event('input'));field.focus();});more.addEventListener('click',()=>{limit+=12;render();});render();
  if(performance.getEntriesByType('navigation')[0]?.type==='back_forward'&&stored?.focus){const found=[...results.querySelectorAll('a')].find(a=>a.getAttribute('href')===base+stored.focus.slice(1));found?.focus();}
 }
}
