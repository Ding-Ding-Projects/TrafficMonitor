import test from 'node:test';
import assert from 'node:assert/strict';
import {JSDOM} from 'jsdom';
import {readerSettings} from '../lib/reader-settings.mjs';
import {readerDefaults,validateReaderPreferences,setupReaderPreferences} from '../src/reader-preferences.js';

test('reading preferences reject unknown controls and invalid values',()=>{
 assert.deepEqual(validateReaderPreferences({}),readerDefaults);
 for(const input of [null,[],{font:'remote-font'},{collapseFilters:'false'},{unknown:true}])assert.throws(()=>validateReaderPreferences(input));
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
