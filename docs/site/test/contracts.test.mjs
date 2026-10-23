import test from 'node:test';
import assert from 'node:assert/strict';
import {parseWording} from '../src/wording.js';
import {safeRelative,isOwnedGithubUrl} from '../lib/content.mjs';
test('wording accepts only a bounded versioned string map',()=>{assert.equal(parseWording('{"schemaVersion":1,"entries":{"Overview":"My overview"}}').entries.Overview,'My overview');for(const text of ['{"schemaVersion":1,"entries":{"x":"one","x":"two"}}','{"schemaVersion":1,"entries":{"__proto__":"value"}}','{"schemaVersion":2,"entries":{}}','{"schemaVersion":1,"entries":{"x":4}}','{"schemaVersion":1,"entries":{},"extra":true}'])assert.throws(()=>parseWording(text));});
test('wording rejects oversized data before application',()=>assert.throws(()=>parseWording(' '.repeat(262145))));
test('output paths stay within the generated root',()=>{assert.equal(safeRelative('wiki/topic/index.html'),'wiki/topic/index.html');for(const value of ['../outside','/absolute','C:/absolute'])assert.throws(()=>safeRelative(value));});

test('GitHub rewrites never adopt another repository README',()=>{
 assert.equal(isOwnedGithubUrl(new URL('https://github.com/zhongyang219/TrafficMonitorSkinEditor/blob/master/README.md')),false);
 assert.equal(isOwnedGithubUrl(new URL('https://github.com/other/TrafficMonitor/blob/main/README.md')),false);
 assert.equal(isOwnedGithubUrl(new URL('https://github.com/zhongyang219/TrafficMonitor/blob/master/README.md')),true);
 assert.equal(isOwnedGithubUrl(new URL('https://github.com/zhongyang219/TrafficMonitor/wiki/Home')),true);
});

test('wording allows byte-bounded large maps and rejects non-JSON whitespace',()=>{
 const entries=Object.fromEntries(Array.from({length:10001},(_,i)=>['k'+i,'v']));
 assert.equal(Object.keys(parseWording(JSON.stringify({schemaVersion:1,entries})).entries).length,10001);
 assert.throws(()=>parseWording('\u00a0{"schemaVersion":1,"entries":{}}'));
});
