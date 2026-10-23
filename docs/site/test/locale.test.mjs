import test from 'node:test';
import assert from 'node:assert/strict';
import {JSDOM} from 'jsdom';
import {translate,createLocalizer} from '../src/locale.js';
import {materialControls,settings} from '../lib/pages.mjs';
test('language switching restores originals and protects source records',()=>{
 const dom=new JSDOM('<body><p>Back</p><article>Back</article><pre>Next</pre><md-outlined-text-field label="Search history"></md-outlined-text-field></body>');let mode='yue';const localizer=createLocalizer(dom.window.document,()=>({mode,entries:{}}));
 assert.equal(dom.window.document.querySelector('p').textContent,'上一步');assert.equal(dom.window.document.querySelector('article').textContent,'Back');assert.equal(dom.window.document.querySelector('pre').textContent,'Next');assert.equal(dom.window.document.querySelector('md-outlined-text-field').getAttribute('label'),'搜尋歷史記錄');
 mode='both';localizer.apply();assert.equal(dom.window.document.querySelector('p').textContent,'Back · 上一步');mode='en';localizer.apply();assert.equal(dom.window.document.querySelector('p').textContent,'Back');localizer.disconnect();
});
test('dynamic counts and wording retain exact values',()=>{
 assert.equal(translate('STEP 2 OF 5','yue'),'第 2 步，共 5 步');assert.equal(translate('3 matching articles','both'),'3 matching articles · 3 篇符合條件嘅文章');assert.equal(translate('Back','yue',{Back:'Custom'}),'Custom');
});
test('settings use registered Material selects with stable option values',()=>{
 const document=new JSDOM(materialControls(settings())).window.document;
 assert.equal(document.querySelectorAll('select').length,0);assert.equal(document.querySelectorAll('md-outlined-select').length,3);assert.ok(document.querySelector('md-select-option[value="both"]'));assert.ok(document.querySelector('#wording-load'));assert.ok(document.querySelector('#wording-file[hidden]'));
});
