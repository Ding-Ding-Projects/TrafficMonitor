import test from 'node:test';
import assert from 'node:assert/strict';
import {JSDOM} from 'jsdom';
import {readerSettings} from '../lib/reader-settings.mjs';
import {readerDefaults,validateReaderPreferences,setupReaderPreferences} from '../src/reader-preferences.js';
import {parseReadingOptions} from '../src/reading-options-parser.js';

test('reading preferences reject unknown controls and invalid values',()=>{
 assert.deepEqual(validateReaderPreferences({}),readerDefaults);
 for(const input of [null,[],new Date(),Object.create({font:'serif'}),{font:'remote-font'},{collapseFilters:'false'},{unknown:true}])assert.throws(()=>validateReaderPreferences(input));
});
const envelope=preferences=>`{"schemaVersion":1,"encoding":"UTF-8","lineEndings":"LF","preferences":${preferences}}`;
const duplicateInputs=[
 '{"schemaVersion":2,"schemaVersion":1,"encoding":"UTF-8","lineEndings":"LF","preferences":{"font":"serif"}}',
 envelope('{"font":"sans","font":"serif"}'),
 envelope('{"font":"sans","f\\u006fnt":"serif"}')
];
test('reading options reject duplicate keys and malformed bounded input',()=>{
 assert.equal(parseReadingOptions(envelope('{"font":"serif"}')).font,'serif');
 for(const input of [...duplicateInputs,'\u00a0'+envelope('{}'),envelope('[]'),envelope('{"__proto__":{}}'),envelope('{}')+'x',' '.repeat(8193)])assert.throws(()=>parseReadingOptions(input));
});
test('duplicate imports preserve stored and visible reading options',async()=>{
 const dom=new JSDOM(readerSettings(),{url:'https://example.org/TrafficMonitor/settings/'});
 for(const key of ['document','localStorage','location'])globalThis[key]=dom.window[key];
 const saved=JSON.stringify({...readerDefaults,font:'sans'});localStorage.setItem('trafficmonitor.docs.reader-preferences',saved);
 let finish;setupReaderPreferences(message=>finish(message));
 const upload=document.getElementById('reader-file');
 for(const text of duplicateInputs){
  const outcome=new Promise(resolve=>{finish=resolve;});
  Object.defineProperty(upload,'files',{configurable:true,value:[{size:new TextEncoder().encode(text).length,text:async()=>text}]});
  upload.dispatchEvent(new dom.window.Event('change'));
  assert.equal(await outcome,'Reading options were not imported. Existing settings are unchanged.');
  assert.equal(localStorage.getItem('trafficmonitor.docs.reader-preferences'),saved);
  assert.equal(document.getElementById('reader-font').value,'sans');
  assert.equal(document.documentElement.style.getPropertyValue('--tm-font-family'),'Arial, "Microsoft JhengHei UI", sans-serif');
 }
 dom.window.close();for(const key of ['document','localStorage','location'])delete globalThis[key];
});
test('reading controls persist actual consumers and reset only owned data',()=>{
 const dom=new JSDOM(readerSettings()+'<div class="filters"><md-outlined-text-field id="query"></md-outlined-text-field><md-outlined-select id="category"></md-outlined-select></div>',{url:'https://example.org/TrafficMonitor/features/'});
 for(const key of ['document','localStorage','location'])globalThis[key]=dom.window[key];
 const messages=[];setupReaderPreferences(message=>messages.push(message));
 const font=document.getElementById('reader-font');font.value='serif';font.dispatchEvent(new dom.window.Event('change'));
 assert.match(document.documentElement.style.getPropertyValue('--tm-font-family'),/Georgia/);
 assert.equal(JSON.parse(localStorage.getItem('trafficmonitor.docs.reader-preferences')).font,'serif');
 assert.equal(document.getElementById('query').closest('details'),null);
 assert.ok(document.getElementById('category').closest('details'));
 localStorage.setItem('trafficmonitor.docs.wording','retained');
 localStorage.setItem('trafficmonitor.docs.filters-other','false');
 document.getElementById('reader-filter-reset').click();
 assert.equal(localStorage.getItem('trafficmonitor.docs.filters-other'),null);
 assert.equal(localStorage.getItem('trafficmonitor.docs.wording'),'retained');
 document.getElementById('reader-reset').click();
 assert.equal(localStorage.getItem('trafficmonitor.docs.reader-preferences'),null);
 assert.match(document.documentElement.style.getPropertyValue('--tm-font-family'),/Segoe UI/);
 dom.window.close();for(const key of ['document','localStorage','location'])delete globalThis[key];
});
