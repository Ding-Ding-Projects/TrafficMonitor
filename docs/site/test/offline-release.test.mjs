import test from 'node:test';
import assert from 'node:assert/strict';
import {JSDOM} from 'jsdom';
import {shell,home,releasePage} from '../lib/pages.mjs';
import {createLocalizer} from '../src/locale.js';
import {applyOfflineReleaseLabels,isOfflineReader} from '../src/offline-release.js';
const tag='v1.0.15-<script>&"'+ 'long'.repeat(40);
const context={base:'/TrafficMonitor/',source:'a'.repeat(40),release:{tagName:tag,publishedAt:'2026-10-23T01:02:03Z',url:'https://example.test/release',assets:[{name:'Setup.exe',url:'https://example.test/Setup.exe',size:123,digest:'sha256:abc'}]}};
function page(body){return new JSDOM(shell('Documentation',body,context)).window.document;}
test('offline release mode requires the exact parsed trusted origin',()=>{
 assert.equal(isOfflineReader('traffic-docs://reader/TrafficMonitor/'),true);
 for(const url of ['traffic-docs://other/TrafficMonitor/','traffic-docs://reader:80/TrafficMonitor/','https://reader/TrafficMonitor/','https://example.test/?offline=true','traffic-docs://user@reader/','broken'])assert.equal(isOfflineReader(url),false,url);
});
test('hosted and rejected origins leave release markup unchanged',()=>{
 for(const url of ['https://example.test/TrafficMonitor/','traffic-docs://wrong/','traffic-docs://reader:123/']){
  const doc=page(home(context)+releasePage(context)),before=doc.body.innerHTML;
  assert.equal(applyOfflineReleaseLabels(doc,url),false);assert.equal(doc.body.innerHTML,before);
 }
});
test('offline snapshot labels preserve escaped literal data and do not invent identity or dates',()=>{
 const doc=page(home(context)+releasePage(context));assert.equal(doc.querySelectorAll('script:not([src])').length,0);
 assert.equal(applyOfflineReleaseLabels(doc,'traffic-docs://reader/TrafficMonitor/'),true);
 assert.equal(doc.querySelectorAll('[data-offline-release-warning]').length,2);
 for(const node of doc.querySelectorAll('[data-release-tag]'))assert.equal(node.textContent,'Bundled release snapshot: '+tag);
 assert.match(doc.querySelector('[data-release-provenance]').textContent,/Check time unavailable/);
 assert.match(doc.querySelector('[data-release-facts]').textContent,/2026-10-23T01:02:03Z/);
 assert.doesNotMatch(doc.body.textContent,/Companion version|Build date|Latest release|Installed version/);
 const button=doc.querySelector('[data-snapshot-installer]');assert.equal(button.getAttribute('href'),'https://example.test/Setup.exe');assert.equal(button.textContent,'Installer link from bundled snapshot');assert.equal(button.hasAttribute('aria-label'),false);
 assert.equal(applyOfflineReleaseLabels(doc,'traffic-docs://reader/'),false);assert.equal(doc.querySelectorAll('[data-offline-release-warning]').length,2);
});
test('English Cantonese bilingual English cycle keeps one prefix and unchanged literal values',()=>{
 const doc=page(home(context)+releasePage(context));applyOfflineReleaseLabels(doc,'traffic-docs://reader/');let mode='en';
 const localizer=createLocalizer(doc,()=>({mode,entries:{[tag]:'Must not replace literal tag'}}));
 for(const next of ['en','yue','both','en']){
  mode=next;localizer.apply();
  const prefix=next==='en'?'Bundled release snapshot':next==='yue'?'隨附版本資料快照':'Bundled release snapshot · 隨附版本資料快照';
  for(const node of doc.querySelectorAll('[data-release-tag]'))assert.equal(node.textContent,prefix+': '+tag);
  const button=doc.querySelector('[data-snapshot-installer]');
  const name=next==='en'?'Installer link from bundled snapshot':next==='yue'?'隨附快照內嘅安裝程式連結':'Installer link from bundled snapshot · 隨附快照內嘅安裝程式連結';
  assert.equal(button.textContent,name);assert.equal(button.hasAttribute('aria-label'),false);
 }
 localizer.disconnect();
});
test('supplied check time stays separate from translated labels',()=>{
 const ctx={...context,release:{...context.release,verifiedAtUTC:'2026-10-22T03:04:05Z'}};
 const doc=new JSDOM(shell('Downloads',releasePage(ctx),ctx)).window.document;applyOfflineReleaseLabels(doc,'traffic-docs://reader/');
 assert.ok(doc.querySelector('[data-release-provenance]').textContent.endsWith('2026-10-22T03:04:05Z'));
 assert.doesNotMatch(doc.querySelector('[data-release-provenance]').textContent,/unavailable/);
});

test('hosted home retains its existing combined release-summary localization',()=>{
 const doc=page(home(context));let mode='yue';
 applyOfflineReleaseLabels(doc,'https://example.test/TrafficMonitor/');
 const localizer=createLocalizer(doc,()=>({mode,entries:{}}));
 assert.match(doc.querySelector('[data-release-home]').textContent,/未簽署安裝程式/);
 assert.doesNotMatch(doc.querySelector('[data-release-home]').textContent,/隨附版本資料快照/);
 localizer.disconnect();
});
