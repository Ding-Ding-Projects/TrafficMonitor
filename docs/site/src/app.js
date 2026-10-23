import '@material/web/button/filled-button.js';
import '@material/web/button/outlined-button.js';
import '@material/web/button/text-button.js';
import '@material/web/textfield/outlined-text-field.js';
import '@material/web/checkbox/checkbox.js';
import '@material/web/switch/switch.js';
import '@material/web/select/outlined-select.js';
import '@material/web/select/select-option.js';
import {createLocalizer} from './locale.js';
import {parseWording,storage} from './wording.js';
import {setupGuide} from './guide.js';
import {setupHistory} from './history.js';
const $=id=>document.getElementById(id);
const base=document.body.dataset.base;
export function notify(message){$('notice').textContent=message;}
let prefs=storage.read('preferences',{}),wording=null,localizer;
if(!prefs||typeof prefs!=='object')prefs={};
try{const saved=storage.read('wording',null);if(saved)wording=parseWording(JSON.stringify(saved));}catch{storage.remove('wording');}
function apply(){
 const lang=['en','yue','both'].includes(prefs.language)?prefs.language:'en';
 document.documentElement.lang=lang==='yue'?'yue-Hant':'en';
 document.documentElement.dataset.theme=['light','dark'].includes(prefs.theme)?prefs.theme:'system';
 document.documentElement.style.fontSize=([100,115,130,150].includes(Number(prefs.size))?prefs.size:100)+'%';
 document.documentElement.dataset.motion=prefs.motion?'reduced':'system';
 if($('wording-state'))$('wording-state').textContent=wording?'Personal wording is active on matching interface labels.':'Original wording active.';
 localizer?.apply();
}
apply();
localizer=createLocalizer(document,()=>({mode:prefs.language??'en',entries:wording?.entries??{}}));
for(const a of document.querySelectorAll('.primary a'))if(new URL(a.href).pathname===location.pathname)a.setAttribute('aria-current','page');
for(const [id,key] of [['language','language'],['theme','theme'],['text-size','size'],['motion','motion']]){
 const control=$(id);if(!control)continue;if(id==='motion')control.selected=Boolean(prefs.motion);else control.value=prefs[key]??(key==='size'?'100':key==='language'?'en':'system');
 control.addEventListener('change',()=>{prefs[key]=id==='motion'?control.selected:control.value;apply();if(!storage.write('preferences',prefs))notify('Preferences apply for this page only because browser storage is unavailable.');});
}
$('reset-preferences')?.addEventListener('click',()=>{storage.remove('preferences');location.reload();});
$('wording-load')?.addEventListener('click',()=> $('wording-file').click());
$('wording-file')?.addEventListener('change',async event=>{const file=event.target.files[0];if(!file)return;try{if(file.size>262144)throw Error('Too large');const next=parseWording(await file.text());if(!storage.write('wording',next))throw Error('Storage unavailable');wording=next;apply();notify('Personal wording saved locally.');}catch{notify('The wording file could not be applied. Use a valid version 1 JSON file within 256 KiB and available browser storage. Existing wording is unchanged.');}finally{event.target.value='';}});
$('wording-clear')?.addEventListener('click',()=>{if(storage.remove('wording')){wording=null;apply();notify('Original wording restored.');}else notify('Browser storage could not be cleared. Existing wording remains.');});
const explanations={taskbar:'Taskbar mode keeps the meter beside your open applications. The maintained installer starts with the main window hidden.',floating:'The floating main window can be positioned on your desktop. Open it from the notification-area menu when you need a larger display.',both:'Both displays can be enabled in the application. This demonstration only explains their roles and does not change application settings.'};
for(const button of document.querySelectorAll('[data-display]'))button.addEventListener('click',()=>{for(const b of document.querySelectorAll('[data-display]'))b.setAttribute('aria-pressed',String(b===button));$('display-explanation').textContent=explanations[button.dataset.display];document.querySelector('.diagram').dataset.view=button.dataset.display;});
const filter=()=>{const query=($('article-search')?.value??'').toLocaleLowerCase(),category=$('article-category')?.value??'';let count=0;for(const row of document.querySelectorAll('[data-search]')){row.hidden=!(row.dataset.search.toLocaleLowerCase().includes(query)&&(!category||row.dataset.search.includes(category)));if(!row.hidden)count++;}if($('article-count'))$('article-count').textContent=count?`${count} matching articles`:'No matching articles. Try a shorter phrase or another category.';};
$('article-search')?.addEventListener('input',filter);$('article-category')?.addEventListener('change',filter);
setupGuide(base,notify);setupHistory(base,notify);
