import {bindRegexSearch} from './regex-search.js';
import {storage} from './wording.js';
export function setupReadingSettings(notify){
 const input=document.getElementById('settings-search');if(!input)return;
 const sections=[...document.querySelectorAll('main>section')].filter(s=>!s.contains(input));
 const saved=storage.read('search-settings',{});input.value=typeof saved?.query==='string'?saved.query:'';
 let regexMatches=null;
 const filter=()=>{const q=input.value.trim().toLocaleLowerCase();let count=0;for(const section of sections){section.hidden=!(regexMatches?regexMatches.has(section):section.textContent.toLocaleLowerCase().includes(q));if(!section.hidden)count++;}document.getElementById('settings-search-state').textContent=`${count} matching settings groups`;storage.write('search-settings',{query:input.value});};
 bindRegexSearch(input,sections,s=>s.textContent,matches=>{regexMatches=matches;filter();},document.getElementById('settings-search-state'));
 input.addEventListener('input',filter);document.getElementById('settings-search-clear').addEventListener('click',()=>{regexMatches=null;input.value='';input.dispatchEvent(new Event('regex-plain'));filter();input.focus();});filter();
 document.getElementById('reading-progress-reset').addEventListener('click',()=>{const results=['learning-progress-v1','guide'].map(key=>storage.remove(key));notify(results.every(Boolean)?'Reading progress cleared. Application settings are unchanged.':'Some reading progress could not be cleared. Browser storage is unavailable.');});
 document.getElementById('search-state-reset').addEventListener('click',()=>{const results=['documentation','capability','settings','history'].flatMap(id=>[storage.remove('search-'+id),storage.remove('regex-workbench-'+id+'-mode')]);regexMatches=null;input.value='';filter();notify(results.every(Boolean)?'Saved search queries cleared. Experiments are unchanged.':'Some search queries could not be cleared. Browser storage is unavailable.');});
}
