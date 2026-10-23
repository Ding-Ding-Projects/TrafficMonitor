import {storage} from './wording.js';
export function setupCapabilities(){
 const query=document.getElementById('capability-search');if(!query)return;
 const surface=document.getElementById('capability-surface'),status=document.getElementById('capability-status'),rows=[...document.querySelectorAll('[data-capability]')],key='search-capability',saved=storage.read(key,{});
 if(typeof saved?.query==='string')query.value=saved.query;if(typeof saved?.surface==='string'&&rows.some(r=>Object.hasOwn(JSON.parse(r.dataset.states),saved.surface)))surface.value=saved.surface;if(['','UNFINISHED','PARTIAL','NOT_APPLICABLE'].includes(saved?.status))status.value=saved.status;
 const labels={UNFINISHED:'Not yet delivered',PARTIAL:'Partially verified',NOT_APPLICABLE:'Not applicable'};
 const render=()=>{let count=0;for(const row of rows){const s=JSON.parse(row.dataset.states)[surface.value];row.hidden=!s||!row.dataset.match.toLocaleLowerCase().includes(query.value.trim().toLocaleLowerCase())||Boolean(status.value&&s.status!==status.value);row.querySelector('.capability-state').textContent=labels[s?.status]??'Not recorded';row.querySelector('.capability-reason').textContent=s?.reason??'Not recorded';if(!row.hidden)count++;}document.getElementById('capability-count').textContent=`${count} of ${rows.length} capabilities`;storage.write(key,{query:query.value,surface:surface.value,status:status.value});};
 query.addEventListener('input',render);surface.addEventListener('change',render);status.addEventListener('change',render);document.getElementById('capability-clear').addEventListener('click',()=>{query.value='';surface.value='native-application';status.value='';render();query.focus();});render();
}
