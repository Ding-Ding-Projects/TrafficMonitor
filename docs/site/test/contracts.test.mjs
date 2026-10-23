import test from 'node:test';
import assert from 'node:assert/strict';
import {parseWording} from '../src/wording.js';
import {safeRelative} from '../lib/content.mjs';
test('wording accepts only a bounded versioned string map',()=>{assert.equal(parseWording('{"schemaVersion":1,"entries":{"Overview":"My overview"}}').entries.Overview,'My overview');for(const text of ['{"schemaVersion":1,"entries":{"x":"one","x":"two"}}','{"schemaVersion":1,"entries":{"__proto__":"value"}}','{"schemaVersion":2,"entries":{}}','{"schemaVersion":1,"entries":{"x":4}}','{"schemaVersion":1,"entries":{},"extra":true}'])assert.throws(()=>parseWording(text));});
test('wording rejects oversized data before application',()=>assert.throws(()=>parseWording(' '.repeat(262145))));
test('output paths stay within the generated root',()=>{assert.equal(safeRelative('wiki/topic/index.html'),'wiki/topic/index.html');for(const value of ['../outside','/absolute','C:/absolute'])assert.throws(()=>safeRelative(value));});
