// Local semantic regression checks. Never run this file in the release workflow.
import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import {execFileSync,spawnSync} from 'node:child_process';
import {fileURLToPath,pathToFileURL} from 'node:url';
import {createHash} from 'node:crypto';
import {deflateSync} from 'node:zlib';
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

async function imageRegressionChecks() {
  const temp=fs.mkdtempSync(path.join(os.tmpdir(),'trafficmonitor-png-audit-'));
  try {
    let check=audit;
    if(process.argv.includes('--baseline-png')||process.argv.includes('--baseline-png-order')){
      const legacy=path.join(temp,'baseline.mjs');
      const revision=process.argv.includes('--baseline-png-order')?'917d00be84976312932211ae37c7009db7583c77':'070a6058d7fc6ac270e4b499510e0025ee87fc15';
      fs.writeFileSync(legacy,execFileSync('git',['show',revision+':scripts/required-feature-audit.mjs'],{cwd:root}));
      check=(await import(pathToFileURL(legacy))).audit;
    }
    const fixture=path.join(temp,'repository');fs.mkdirSync(fixture);
    const git=args=>execFileSync('git',args,{cwd:fixture,windowsHide:true,encoding:'utf8',stdio:['ignore','pipe','pipe']}).trim();
    git(['init','--quiet']);git(['config','user.name','Claude Fable 5.1']);git(['config','user.email','noreply@anthropic.com']);
    git(['config','core.hooksPath',path.join(temp,'no-hooks')]);
    fs.mkdirSync(path.join(fixture,'docs/features/required-features'),{recursive:true});
    for(const id of REQUIRED_IDS)fs.writeFileSync(path.join(fixture,'docs/features/required-features',id+'.md'),'# Isolated structural fixture\n');
    git(['add','--','docs']);git(['commit','--quiet','-m','Create isolated audit fixture']);
    const product=git(['rev-parse','HEAD']);
    const genuine=execFileSync('git',['show','HEAD:TrafficMonitor/skins/默认皮肤2/background.png'],{cwd:root});
    const width=genuine.readUInt32BE(16),height=genuine.readUInt32BE(20);
    const crc=bytes=>{let n=0xffffffff;for(const byte of bytes){n^=byte;for(let i=0;i<8;i++)n=(n>>>1)^((n&1)?0xedb88320:0);}return (n^0xffffffff)>>>0;};
    const chunk=(kind,body)=>{const head=Buffer.alloc(8),tail=Buffer.alloc(4);head.writeUInt32BE(body.length);head.write(kind,4,'ascii');tail.writeUInt32BE(crc(Buffer.concat([head.subarray(4),body])));return Buffer.concat([head,body,tail]);};
    const ihdr=Buffer.alloc(13);ihdr.writeUInt32BE(1,0);ihdr.writeUInt32BE(1,4);ihdr[8]=8;ihdr[9]=2;
    const plte=chunk('PLTE',Buffer.from([12,34,56])),trns=chunk('tRNS',Buffer.from([0,12,0,34,0,56]));
    const colorPng=middle=>Buffer.concat([Buffer.from('89504e470d0a1a0a','hex'),chunk('IHDR',ihdr),...middle,chunk('IDAT',deflateSync(Buffer.from([0,12,34,56]))),chunk('IEND',Buffer.alloc(0))]);
    const corruptDeflate=Buffer.from(genuine);
    for(let offset=8;offset+12<=corruptDeflate.length;){
      const length=corruptDeflate.readUInt32BE(offset),kind=corruptDeflate.toString('ascii',offset+4,offset+8);
      if(kind==='IDAT'){corruptDeflate[offset+8]=0;corruptDeflate.writeUInt32BE(crc(corruptDeflate.subarray(offset+4,offset+8+length)),offset+8+length);break;}
      offset+=length+12;
    }
    const badCRC=Buffer.from(genuine);badCRC[badCRC.length-1]^=1;
    const inputs=[
      {name:'genuine project PNG',bytes:genuine,accept:true},
      {name:'truecolor palette before transparency',bytes:colorPng([plte,trns]),accept:true,viewport:{width:1,height:1}},
      {name:'truecolor transparency without palette',bytes:colorPng([trns]),accept:true,viewport:{width:1,height:1}},
      {name:'truecolor palette after transparency',bytes:colorPng([trns,plte]),viewport:{width:1,height:1}},
      {name:'UTF-8 text named PNG',bytes:Buffer.from('This is text, not image pixels.')},
      {name:'zero byte PNG',bytes:Buffer.alloc(0)},
      {name:'signature only PNG',bytes:Buffer.from('89504e470d0a1a0a','hex')},
      {name:'truncated PNG',bytes:genuine.subarray(0,genuine.length-7)},
      {name:'corrupt chunk CRC',bytes:badCRC},
      {name:'corrupt deflate with valid chunk CRC',bytes:corruptDeflate},
      {name:'wrong viewport dimensions',bytes:genuine,wrongDimensions:true},
      {name:'wrong image hash control',bytes:genuine,wrongHash:true}
    ];
    fs.mkdirSync(path.join(fixture,'evidence'));
    const hash=bytes=>createHash('sha256').update(bytes).digest('hex');
    for(const [i,input] of inputs.entries()){
      const imagePath='evidence/image-'+i+'.png',receiptPath='evidence/receipt-'+i+'.json';
      fs.writeFileSync(path.join(fixture,imagePath),input.bytes);
      const receipt={sourceCommit:product,verified:true,featureId:'language',surface:'native-application',buildSHA256:'1'.repeat(64),state:'structural fixture only',route:'synthetic-local-test',language:'English',theme:'light',scale:1,viewport:input.viewport||{width:width+(input.wrongDimensions?1:0),height},privacyReviewed:true,pixelsInspected:true,imagePath,imageSHA256:input.wrongHash?'0'.repeat(64):hash(input.bytes)};
      fs.writeFileSync(path.join(fixture,receiptPath),JSON.stringify(receipt));
      input.receiptPath=receiptPath;input.receiptHash=hash(fs.readFileSync(path.join(fixture,receiptPath)));
    }
    git(['add','--','evidence']);git(['commit','--quiet','-m','Preserve isolated image audit inputs']);
    const evidenceCommit=git(['rev-parse','HEAD']);
    let failed=0;
    for(const input of inputs){
      const inventory=structuredClone(original),item=inventory.items.find(item=>item.id==='language'),row=item.surfaces['native-application'];
      row.proof=structuredClone(item.proof);
      row.proof.screenshots={status:'COMPLETE',expected:'Validate image bytes only, never product execution.',evidence:[{featureId:'language',surface:'native-application',path:input.receiptPath,sourceCommit:product,sha256:input.receiptHash}]};
      const result=check({root:fixture,inventory,expected,sourceCommit:product,evidenceCommit});
      const imageErrors=result.errors.filter(e=>e.location.startsWith('language.native-application.proof.screenshots'));
      const accepted=imageErrors.length===0;
      const pass=accepted===Boolean(input.accept);
      console.log((pass?'PASS ':'FAIL ')+input.name+' accepted='+accepted+' imageErrors='+imageErrors.length);
      if(!pass)failed++;cases++;
      assert.equal(result.complete,false,'Image fixtures never imply full product completion');
    }
    assert.equal(failed,0,'PNG structural acceptance regressions');
  } finally {fs.rmSync(temp,{recursive:true,force:true});}
}
await imageRegressionChecks();
if(process.argv.includes('--png-only')){console.log('PNG audit checks: '+cases+' passed; structural fixture evidence is not runtime acceptance.');process.exit(0);}

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
