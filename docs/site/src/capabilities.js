import {bindRegexSearch} from './regex-search.js';
import {storage} from './wording.js';
export function setupCapabilities(){
 const query=document.getElementById('capability-search');if(!query)return;
 const benefit=document.getElementById('capability-benefit');
 const surface=document.getElementById('capability-surface'),status=document.getElementById('capability-status'),rows=[...document.querySelectorAll('[data-capability]')],key='search-capability',saved=storage.read(key,{});
 if(typeof saved?.query==='string')query.value=saved.query;if(typeof saved?.surface==='string'&&rows.some(r=>Object.hasOwn(JSON.parse(r.dataset.states),saved.surface)))surface.value=saved.surface;if(['','UNFINISHED','PARTIAL','NOT_APPLICABLE'].includes(saved?.status))status.value=saved.status;
 if(typeof saved?.benefit==='string'&&rows.some(r=>r.dataset.benefit===saved.benefit))benefit.value=saved.benefit;
 let regexMatches=null;
 const labels={UNFINISHED:'Not yet delivered',PARTIAL:'Partially verified',NOT_APPLICABLE:'Not applicable'};
 const render=()=>{let count=0;for(const row of rows){const s=JSON.parse(row.dataset.states)[surface.value];row.hidden=Boolean(benefit.value&&row.dataset.benefit!==benefit.value)||!s||!(regexMatches?regexMatches.has(row):row.dataset.match.toLocaleLowerCase().includes(query.value.trim().toLocaleLowerCase()))||Boolean(status.value&&s.status!==status.value);row.querySelector('.capability-state').textContent=labels[s?.status]??'Not recorded';row.querySelector('.capability-reason').textContent=s?.reason??'Not recorded';if(!row.hidden)count++;}document.getElementById('capability-count').textContent=`${count} of ${rows.length} capabilities`;storage.write(key,{query:query.value,surface:surface.value,status:status.value,benefit:benefit.value});};
 bindRegexSearch(query,rows,r=>r.dataset.match,matches=>{regexMatches=matches;render();},document.getElementById('capability-count'));
 benefit.addEventListener('change',render);query.addEventListener('input',render);surface.addEventListener('change',render);status.addEventListener('change',render);document.getElementById('capability-clear').addEventListener('click',()=>{regexMatches=null;query.value='';surface.value='native-application';status.value='';benefit.value='';query.dispatchEvent(new Event('regex-plain'));render();query.focus();});render();
}
