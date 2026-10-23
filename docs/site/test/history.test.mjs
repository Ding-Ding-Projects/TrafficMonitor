import test from 'node:test';
import assert from 'node:assert/strict';
import {validateHistoryEntries} from '../lib/history.mjs';
const upstream='a'.repeat(40),fork='b'.repeat(40),upstreamSet=new Set([upstream]),forkSet=new Set([fork]);
const u={sha:upstream,lineage:'upstream',summary:'Improve the network display.'};
const f={sha:fork,lineage:'fork',originalMessage:'Exact subject\n\n完整內文\n',summaryMethod:'verbatim-bilingual'};
test('history validates exact required fields and unique identities',()=>{
 assert.equal(validateHistoryEntries([u,f],upstreamSet,forkSet,true).upstreamCount,1);
 for(const entries of [[u,u],[{...f,originalMessage:undefined}],[{...f,summaryMethod:'summary'}],[{...u,summary:''}],[{...u,summary:'只有中文'}]])assert.throws(()=>validateHistoryEntries(entries,upstreamSet,forkSet));
});
test('partial history stays valid while a complete declaration fails on omissions',()=>{
 assert.equal(validateHistoryEntries([f],upstreamSet,forkSet).upstreamCount,0);
 assert.throws(()=>validateHistoryEntries([f],upstreamSet,forkSet,true));
});
