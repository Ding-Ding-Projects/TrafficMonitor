import test from 'node:test';
import assert from 'node:assert/strict';
import {JSDOM} from 'jsdom';
import {createNotificationFeedback,validateNotificationPreferences} from '../src/notifications.js';
import {notificationSettings} from '../lib/notification-settings.mjs';
test('notification preferences reject unsupported values',()=>{
 for(const value of [null,[],{corner:'top',duration:6000},{corner:'right',duration:1},{corner:'right',duration:6000,muted:true}])assert.throws(()=>validateNotificationPreferences(value));
});
test('feedback stacks, persists warnings, pauses timers and resets only preferences',()=>{
 const dom=new JSDOM('<main id="main" tabindex="-1">'+notificationSettings()+'</main><div id="notice"></div>',{url:'https://example.org/'});
 globalThis.localStorage=dom.window.localStorage;const timers=new Map();let id=0;
 const notify=createNotificationFeedback(dom.window.document,{schedule(fn,ms){timers.set(++id,{fn,ms});return id;},unschedule(key){timers.delete(key);}});
 const info=notify('Information'),warning=notify('Warning',{severity:'warning'});
 assert.equal(dom.window.document.querySelectorAll('.feedback-card').length,2);assert.equal(timers.size,1);
 info.dispatchEvent(new dom.window.Event('pointerenter'));assert.equal(timers.size,0);
 info.dispatchEvent(new dom.window.Event('pointerleave'));assert.equal(timers.size,1);
 [...timers.values()][0].fn();assert.equal(info.isConnected,false);assert.equal(warning.isConnected,true);
 const corner=dom.window.document.getElementById('notification-corner');corner.value='left';corner.dispatchEvent(new dom.window.Event('change'));
 assert.equal(dom.window.document.getElementById('notice').dataset.corner,'left');assert.equal(JSON.parse(localStorage.getItem('trafficmonitor.docs.notifications')).corner,'left');
 localStorage.setItem('trafficmonitor.docs.wording','retained');dom.window.document.getElementById('notification-reset').click();
 assert.equal(localStorage.getItem('trafficmonitor.docs.notifications'),null);assert.equal(localStorage.getItem('trafficmonitor.docs.wording'),'retained');
 assert.equal(Object.values(localStorage).includes('Warning'),false);
 dom.window.close();delete globalThis.localStorage;
});
