// Local semantic regression checks. Never run this file in the release workflow.
import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import {execFileSync,spawnSync} from 'node:child_process';
import {fileURLToPath} from 'node:url';
import {audit,REQUIRED_IDS,SURFACES,DIMENSIONS} from './required-feature-audit.mjs';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const original=JSON.parse(fs.readFileSync(path.join(root,'docs/features/required-features/inventory.json'),'utf8'));
const expected=JSON.parse(fs.readFileSync(path.join(root,'docs/features/required-features/expected-ids.json'),'utf8'));
const sourceCommit=execFileSync('git',['rev-parse','HEAD'],{cwd:root,encoding:'utf8'}).trim();
const run=inventory=>audit({root,inventory,expected,sourceCommit});
let cases=0;
const baseline=run(original);
assert.equal(baseline.errors.length,0);assert.ok(baseline.gaps.length>0);assert.equal(baseline.complete,false);
assert.equal(REQUIRED_IDS.length,110);assert.equal(new Set(REQUIRED_IDS).size,110);
function negative(name,change,find){
  const changed=structuredClone(original);change(changed);
  const result=run(changed);
  assert.ok(result.errors.some(find),name+': expected rejection was absent');
  assert.equal(result.complete,false,name);
  assert.equal(run(original).errors.length,0,name+': restoration did not recover');
  console.log('PASS '+name);cases++;
}
for(const id of REQUIRED_IDS)negative('missing identity '+id,i=>{i.items=i.items.filter(item=>item.id!==id);},e=>e.location===id&&e.message==='Required identity is missing');
for(const surface of SURFACES)negative('missing surface '+surface,i=>{delete i.items.find(item=>item.id==='language').surfaces[surface];},e=>e.location==='language.surfaces');
for(const dimension of DIMENSIONS)negative('missing proof '+dimension,i=>{delete i.items.find(item=>item.id==='language').proof[dimension];},e=>e.location==='language.proof');
negative('duplicate identity',i=>{i.items[1]=structuredClone(i.items[0]);},e=>e.message==='Duplicate identity');
negative('invalid applicability',i=>{i.items[1].surfaces['native-application'].applicability='OPTIONAL';},e=>e.message.startsWith('Surface requires'));
negative('unexplained nonapplicability',i=>{i.items[0].surfaces['native-application'].reason='';},e=>e.message.startsWith('Surface requires'));
negative('missing evidence array',i=>{delete i.items[1].proof.implementation.evidence;},e=>e.location==='language.proof.implementation');
negative('complete without evidence',i=>{const p=i.items[1].proof.implementation;p.status='COMPLETE';p.evidence=[];},e=>e.message==='Complete proof requires source-bound evidence');
negative('complete surface without independent proof',i=>{i.items[1].surfaces['native-application'].status='COMPLETE';},e=>e.message==='Complete surface requires independent proof');
negative('missing claimed evidence file',i=>{const p=i.items[1].proof.implementation;p.status='COMPLETE';p.evidence=[{featureId:'language',surface:'shared',path:'absent-evidence.json',sourceCommit,sha256:'0'.repeat(64)}];},e=>e.message==='Evidence file is absent from the evidence commit');
negative('stale evidence source',i=>{i.items[1].proof.implementation.evidence=[{featureId:'language',surface:'shared',path:'README.md',sourceCommit:'0'.repeat(40),sha256:'0'.repeat(64)}];},e=>e.message.startsWith('Evidence requires'));
negative('wrong evidence hash',i=>{i.items[1].proof.implementation.evidence=[{featureId:'language',surface:'shared',path:'README.md',sourceCommit,sha256:'0'.repeat(64)}];},e=>e.message==='Evidence bytes do not match source-bound SHA-256');
negative('unrelated evidence owner',i=>{i.items[1].proof.implementation.evidence=[{featureId:'tabs',surface:'shared',path:'README.md',sourceCommit,sha256:'0'.repeat(64)}];},e=>e.message==='Evidence must identify its owning feature and surface');
negative('complete parent with unfinished surface',i=>{i.items[1].status='COMPLETE';},e=>e.message==='Complete item has unfinished surfaces');
negative('complete cannot waive every proof',i=>{const row=i.items[1].surfaces['native-application'];row.status='COMPLETE';row.proof=structuredClone(i.items[1].proof);for(const p of Object.values(row.proof)){p.status='NOT_APPLICABLE';p.reason='Synthetic waiver must not accept an applicable feature';p.evidence=[];}},e=>e.message==='Applicable surface cannot waive required proof dimensions');
const directory=fs.mkdtempSync(path.join(os.tmpdir(),'trafficmonitor-audit-local-'));
try{
  const script=path.join(root,'scripts/required-feature-audit.mjs');
  const normal=spawnSync(process.execPath,[script],{cwd:root,encoding:'utf8'});
  assert.equal(normal.status,0);assert.equal(JSON.parse(normal.stdout).complete,false);
  const strict=spawnSync(process.execPath,[script,'--require-complete'],{cwd:root,encoding:'utf8'});
  assert.equal(strict.status,1);assert.ok(JSON.parse(strict.stdout).gaps.length>0);
  const file=path.join(directory,'invalid.json');fs.writeFileSync(file,'{');
  assert.equal(spawnSync(process.execPath,[script,'--inventory',file],{cwd:root,encoding:'utf8'}).status,1);
  console.log('PASS diagnostic incomplete / strict rejection / malformed input');cases+=3;
}finally{fs.rmSync(directory,{recursive:true,force:true});}
console.log('Required-feature audit regressions: '+cases+' passed; baseline remains incomplete with '+baseline.gaps.length+' gaps. Product acceptance was not executed.');
