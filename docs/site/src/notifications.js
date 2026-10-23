import {storage} from './wording.js';
export const notificationDefaults={corner:'right',duration:6000};
export function validateNotificationPreferences(value){
 if(!value||typeof value!=='object'||Array.isArray(value)||Object.keys(value).some(key=>!['corner','duration'].includes(key))||!['left','right'].includes(value.corner)||![0,6000,10000].includes(value.duration))throw Error('Invalid notification preferences');
 return {corner:value.corner,duration:value.duration};
}
export function createNotificationFeedback(document,{schedule=setTimeout,unschedule=clearTimeout}={}){
 let prefs;try{prefs=validateNotificationPreferences(storage.read('notifications',notificationDefaults));}catch{prefs={...notificationDefaults};}
 const host=document.getElementById('notice');host.removeAttribute('role');host.removeAttribute('aria-live');host.setAttribute('aria-label','Notifications');
 const active=new Set();
 const apply=()=>{host.dataset.corner=prefs.corner;for(const key of ['corner','duration']){const field=document.getElementById('notification-'+key);if(field)field.value=String(prefs[key]);}for(const item of active)item.restart();};
 const notify=(message,{severity='info'}={})=>{
  const card=document.createElement('section'),body=document.createElement('p'),dismiss=document.createElement('md-text-button');
  card.className='feedback-card';body.setAttribute('role','status');body.setAttribute('aria-live','polite');body.setAttribute('aria-atomic','true');body.textContent=message;dismiss.textContent='Dismiss notification';dismiss.setAttribute('aria-label','Dismiss notification');card.append(body,dismiss);host.append(card);
  let timer=null,hover=false;const persistent=['warning','error'].includes(severity);card.dataset.severity=severity;
  const stop=()=>{if(timer!==null)unschedule(timer);timer=null;};
  const remove=()=>{stop();const focused=card.contains(document.activeElement);const next=card.nextElementSibling?.querySelector('md-text-button')??card.previousElementSibling?.querySelector('md-text-button');card.remove();active.delete(item);if(focused)(next??document.getElementById('main'))?.focus();};
  const item={restart(){stop();if(!persistent&&prefs.duration&&!hover&&!card.contains(document.activeElement))timer=schedule(remove,prefs.duration);}};
  active.add(item);dismiss.addEventListener('click',remove);card.addEventListener('pointerenter',()=>{hover=true;stop();});card.addEventListener('pointerleave',()=>{hover=false;item.restart();});card.addEventListener('focusin',stop);card.addEventListener('focusout',()=>queueMicrotask(()=>item.restart()));item.restart();return card;
 };
 apply();
 for(const key of ['corner','duration'])document.getElementById('notification-'+key)?.addEventListener('change',event=>{prefs=validateNotificationPreferences({...prefs,[key]:key==='duration'?Number(event.target.value):event.target.value});apply();if(!storage.write('notifications',prefs))notify('Notification preferences apply for this visit only. Browser storage is unavailable.',{severity:'warning'});});
 document.getElementById('notification-reset')?.addEventListener('click',()=>{if(!storage.remove('notifications')){notify('Notification preferences could not be reset. Existing preferences remain.',{severity:'error'});return;}prefs={...notificationDefaults};apply();notify('Notification preferences reset.');});
 document.getElementById('notification-preview')?.addEventListener('click',()=>notify('Example informational notification. No application action was performed.'));
 document.getElementById('notification-warning')?.addEventListener('click',()=>notify('Example warning. This remains until you dismiss it.',{severity:'warning'}));
 return notify;
}
