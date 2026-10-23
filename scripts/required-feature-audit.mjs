import fs from 'node:fs';
import path from 'node:path';
import {createHash} from 'node:crypto';
import {execFileSync} from 'node:child_process';
import {inflateSync} from 'node:zlib';
import {fileURLToPath} from 'node:url';

// Independent reviewed inventory. Do not derive this list from discovered articles.
export const REQUIRED_IDS = Object.freeze((
'job-experience-document-label language funny-emoji personal-vocabulary vocabulary-drop-apply school-mode narration scheduled-settings display-name adhd-modes dim-sum dim-sum-photo-source material-design workflow-navigation motion appearance-editor color-picker logo-customization overlay-panels collapse-filters accessibility-sizing functional-ui onboarding-wizard tabs regex-search command-palette context-menus menu-shortcuts rich-controls guided-forms info-icon-guidance settings-explanations blank-editors super-confirmation element-locks support-tickets unlock-ladder authenticator qr-pairing honest-monetization local-history exports bulk-actions changelog offline-docs rendered-provider-text notifications file-converter minecraft-server-manager ollama-suite external-editor download-handoff forge-publishing progress-recovery completeness-parity front-provenance product-evidence screen-recording design-reference-parity design-folder layout-verification-matrix public-screenshot-gallery landing-site responsive-documentation installer-download-button feature-articles vendored-fonts shared-link-embed homepage-link vocabulary-unlock-boundary documentation-companion status-hub discord-status-bridge panic-webhooks tidbyt-displays build-entrypoints fresh-build-run-command dependency-fetcher bundled-dependencies squirrel-installer self-signing auto-updates app-icon release-workflow release-timing release-line-counts release-dim-sum-photo dim-sum-code-names ci-bootstrap runner-selection encrypted-public-builder tabbed-readme human-time-estimate sanitized-instruction-copy agents-md-vocabulary-block vocabulary-hash-lock roadmap-checklist feature-docs postman-collections wiki-documentation-sync handoff-record closeout-prompt discussion-records project-board operational-skill readme-prompt-banner instruction-corpus-editions project-profile roblox-model-catalogue roblox-visual-realism'
).split(' '));
export const SURFACES = Object.freeze(['native-application','hosted-documentation','documentation-companion','build-companion','repository','browser-extension','game','instruction-source']);
export const DIMENSIONS = Object.freeze(['implementation','documentation','localization','persistence','focusedTests','negativeRegression','builtInteraction','screenshots']);
const statuses = new Set(['UNFINISHED','PARTIAL','COMPLETE','NOT_APPLICABLE']);
const object = value => value && typeof value === 'object' && !Array.isArray(value);
const text = value => typeof value === 'string' && value.trim().length > 0;
const hash = bytes => createHash('sha256').update(bytes).digest('hex');
const exactKeys = (value, expected) => object(value) && Object.keys(value).length === expected.length && expected.every(key => Object.hasOwn(value,key));
const sha = value => typeof value === 'string' && /^[a-f0-9]{40}$/.test(value);
const sha256 = value => typeof value === 'string' && /^[a-f0-9]{64}$/.test(value);

// Decode bounded PNG scanlines, including filters and Adam7 passes. This validates
// the file format and dimensions, not capture provenance or human inspection.
export function decodePng(bytes) {
  const fail=()=>{throw Error('Invalid or unsupported bounded PNG');};
  const limit=64*1024*1024;
  if(!Buffer.isBuffer(bytes)||bytes.length<45||bytes.length>32*1024*1024||!bytes.subarray(0,8).equals(Buffer.from('89504e470d0a1a0a','hex')))fail();
  const crc=buffer=>{let n=0xffffffff;for(const byte of buffer){n^=byte;for(let i=0;i<8;i++)n=(n>>>1)^((n&1)?0xedb88320:0);}return (n^0xffffffff)>>>0;};
  let offset=8,header,ended=false,palette=null,transparency=false,idatStarted=false,idatEnded=false,chunks=0;
  const data=[];
  while(offset<bytes.length){
    if(++chunks>8192)fail();
    if(offset+12>bytes.length)fail();
    const length=bytes.readUInt32BE(offset);
    if(length>bytes.length-offset-12)fail();
    const kind=bytes.toString('ascii',offset+4,offset+8),body=bytes.subarray(offset+8,offset+8+length);
    if(!bytes.subarray(offset+4,offset+8).every(byte=>(byte>=65&&byte<=90)||(byte>=97&&byte<=122))||(bytes[offset+6]&32)!==0||crc(bytes.subarray(offset+4,offset+8+length))!==bytes.readUInt32BE(offset+8+length))fail();
    if(!header&&kind!=='IHDR')fail();
    if(kind==='IHDR'){
      if(header||length!==13||offset!==8)fail();
      const width=body.readUInt32BE(0),height=body.readUInt32BE(4),depth=body[8],color=body[9],interlace=body[12];
      const depths={0:[1,2,4,8,16],2:[8,16],3:[1,2,4,8],4:[8,16],6:[8,16]};
      if(!width||!height||width>8192||height>8192||!depths[color]?.includes(depth)||body[10]!==0||body[11]!==0||interlace>1)fail();
      header={width,height,depth,color,interlace,channels:({0:1,2:3,3:1,4:2,6:4})[color]};
    }else if(kind==='PLTE'){
      if(palette||transparency||idatStarted||header.color===0||header.color===4||!length||length%3||length>768||(header.color===3&&length/3>2**header.depth))fail();
      palette=body;
    }else if(kind==='tRNS'){
      if(transparency||idatStarted||header.color===4||header.color===6||(header.color===0&&length!==2)||(header.color===2&&length!==6)||(header.color===3&&(!palette||!length||length>palette.length/3)))fail();
      transparency=true;
    }else if(kind==='IDAT'){
      if(idatEnded||(header.color===3&&!palette))fail();
      idatStarted=true;data.push(body);
    }else if(kind==='IEND'){
      if(length||!idatStarted||offset+12!==bytes.length)fail();
      ended=true;offset+=12;break;
    }else{
      if((bytes[offset+4]&32)===0)fail();
      if(idatStarted)idatEnded=true;
    }
    offset+=length+12;
  }
  if(!ended||offset!==bytes.length)fail();
  const bits=header.channels*header.depth,bpp=Math.max(1,Math.ceil(bits/8));
  const pattern=header.interlace?[[0,0,8,8],[4,0,8,8],[0,4,4,8],[2,0,4,4],[0,2,2,4],[1,0,2,2],[0,1,1,2]]:[[0,0,1,1]];
  const passes=pattern.map(([x,y,dx,dy])=>({width:header.width<=x?0:Math.ceil((header.width-x)/dx),height:header.height<=y?0:Math.ceil((header.height-y)/dy)}));
  let expected=0;
  for(const p of passes)if(p.width&&p.height)expected+=(Math.ceil(p.width*bits/8)+1)*p.height;
  if(!expected||expected>limit)fail();
  const compressed=Buffer.concat(data);
  let decoded;
  try{
    const result=inflateSync(compressed,{maxOutputLength:expected+1,info:true});
    if(result.engine.bytesWritten!==compressed.length)fail();
    decoded=result.buffer;
  }catch{fail();}
  if(decoded.length!==expected)fail();
  let position=0;
  const paeth=(a,b,c)=>{const n=a+b-c,da=Math.abs(n-a),db=Math.abs(n-b),dc=Math.abs(n-c);return da<=db&&da<=dc?a:db<=dc?b:c;};
  for(const p of passes){
    if(!p.width||!p.height)continue;
    const stride=Math.ceil(p.width*bits/8);
    let previous=Buffer.alloc(stride),row=Buffer.alloc(stride);
    for(let y=0;y<p.height;y++){
      const filter=decoded[position++];if(filter>4)fail();
      for(let x=0;x<stride;x++){
        const a=x>=bpp?row[x-bpp]:0,b=previous[x],c=x>=bpp?previous[x-bpp]:0;
        const correction=filter===0?0:filter===1?a:filter===2?b:filter===3?Math.floor((a+b)/2):paeth(a,b,c);
        row[x]=(decoded[position++]+correction)&255;
      }
      if(header.color===3){
        const mask=(1<<header.depth)-1;
        for(let x=0;x<p.width;x++){const bit=x*header.depth,index=(row[Math.floor(bit/8)]>>(8-header.depth-(bit%8)))&mask;if(index>=palette.length/3)fail();}
      }
      [row,previous]=[previous,row];
    }
  }
  return {width:header.width,height:header.height};
}

export function audit({root, inventory, expected, sourceCommit, evidenceCommit=sourceCommit}) {
  const errors=[], gaps=[];
  const error = (location, message) => errors.push({location,message});
  const gap = (location, message) => gaps.push({location,message});
  const git = args => execFileSync('git',args,{cwd:root,windowsHide:true,maxBuffer:32*1024*1024,stdio:['ignore','pipe','pipe']});
  let sourceFiles=new Set();
  if(!sha(sourceCommit)) error('sourceCommit','An exact source commit is required');
  else {try {if(git(['rev-parse',sourceCommit+'^{commit}']).toString().trim()!==sourceCommit) error('sourceCommit','Source must resolve exactly');sourceFiles=new Set(git(['ls-tree','-rz','--name-only',sourceCommit]).toString().split('\0'));}catch{error('sourceCommit','Source commit is unavailable');}}
  if(!sha(evidenceCommit))error('evidenceCommit','An exact evidence commit is required');
  else try{if(git(['rev-parse',evidenceCommit+'^{commit}']).toString().trim()!==evidenceCommit)error('evidenceCommit','Evidence source must resolve exactly');}catch{error('evidenceCommit','Evidence commit is unavailable');}
  if(!Array.isArray(expected)||expected.length!==110||new Set(expected).size!==110||REQUIRED_IDS.some(id=>!expected.includes(id)))error('expected-ids','The independent 110 public identities must remain present exactly once');
  if(!object(inventory)||inventory.schemaVersion!==1||inventory.inventoryCount!==110||!Array.isArray(inventory.items))return {sourceCommit,errors:[...errors,{location:'inventory',message:'Invalid schema or declared item count'}],gaps,complete:false};
  if(!Array.isArray(inventory.surfaces)||inventory.surfaces.length!==8||new Set(inventory.surfaces).size!==8||SURFACES.some(s=>!inventory.surfaces.includes(s)))error('surfaces','Exactly eight reviewed surfaces are required');
  if(inventory.items.length!==110)error('items','Exactly 110 item records are required');
  const ids=new Set();
  function evidence(reference,location,dimension) {
    const parts=location.split('.'),featureId=parts[0],surface=SURFACES.includes(parts[1])?parts[1]:'shared';
    if(!object(reference)||reference.featureId!==featureId||reference.surface!==surface){error(location,'Evidence must identify its owning feature and surface');return;}
    if(!object(reference)||!text(reference.path)||!sha256(reference.sha256)||reference.sourceCommit!==sourceCommit){error(location,'Evidence requires a relative path, SHA-256 and exact audited sourceCommit');return;}
    const name=reference.path;
    if(name.includes('\\')||name.includes(':')||name.startsWith('/')||name.split('/').some(p=>!p||p==='.'||p==='..')){error(location,'Evidence path must be a safe repository-relative file');return;}
    let bytes;
    try {bytes=git(['show',evidenceCommit+':'+name]);if(hash(bytes)!==reference.sha256)error(location,'Evidence bytes do not match source-bound SHA-256');}catch{error(location,'Evidence file is absent from the evidence commit');return;}
    if(['builtInteraction','screenshots','negativeRegression'].includes(dimension)){
      let receipt;try{receipt=JSON.parse(bytes.toString('utf8').replace(/^\uFEFF/,''));}catch{error(location,'Behavioral evidence must reference a JSON receipt');return;}
      if(!object(receipt)||receipt.sourceCommit!==sourceCommit||receipt.verified!==true)error(location,'Receipt must explicitly verify the audited source');
      if(receipt.featureId!==featureId||receipt.surface!==surface)error(location,'Receipt belongs to another feature or surface');
      if(dimension==='negativeRegression'&&(receipt.redObserved!==true||receipt.restoredGreen!==true))error(location,'Negative receipt must record actual red and restored green');
      if(['builtInteraction','screenshots'].includes(dimension)){
        if(!sha256(receipt.buildSHA256)||!text(receipt.state)||!text(receipt.route)||!text(receipt.language)||!text(receipt.theme)||!Number.isFinite(receipt.scale)||receipt.scale<=0||!object(receipt.viewport)||!Number.isInteger(receipt.viewport.width)||receipt.viewport.width<1||!Number.isInteger(receipt.viewport.height)||receipt.viewport.height<1)error(location,'Runtime receipt requires build hash, state, route, language, theme, scale and viewport');
        if(receipt.privacyReviewed!==true)error(location,'Runtime receipt requires a privacy verdict');
        if(dimension==='screenshots'&&(receipt.pixelsInspected!==true||!text(receipt.imagePath)||!sha256(receipt.imageSHA256)))error(location,'Screenshot receipt requires inspected pixels and image identity');
        if(dimension==='screenshots'&&text(receipt.imagePath)){
          const image=receipt.imagePath;
          if(image.includes('\\')||image.includes(':')||image.startsWith('/')||image.split('/').some(p=>!p||p==='.'||p==='..'))error(location,'Image path must be repository-relative');
          else {
            let imageBytes;
            try{imageBytes=git(['show',evidenceCommit+':'+image]);}catch{error(location,'Receipt image is absent from evidence commit');}
            if(imageBytes){
              if(hash(imageBytes)!==receipt.imageSHA256)error(location,'Image bytes do not match receipt');
              try{const size=decodePng(imageBytes);if(size.width!==receipt.viewport?.width||size.height!==receipt.viewport?.height)error(location,'Decoded image dimensions do not match receipt viewport');}
              catch{error(location,'Receipt image must be a complete bounded decodable PNG');}
            }
          }
        }
      }
    }
  }
  function proofs(proof,location,applicable,claimComplete) {
    if(!exactKeys(proof,DIMENSIONS)){error(location,'Exactly eight proof dimensions are required');return;}
    for(const dimension of DIMENSIONS){
      const record=proof[dimension], where=location+'.'+dimension;
      if(!object(record)||!statuses.has(record.status)||!text(record.expected)||!Array.isArray(record.evidence)){error(where,'Proof requires a supported status, expectation and evidence array');continue;}
      if(record.status==='NOT_APPLICABLE'){
        const surface=location.split('.')[1];
        const runtime=['native-application','hosted-documentation','documentation-companion','build-companion'].includes(surface);
        if(applicable&&(['implementation','documentation','focusedTests','negativeRegression'].includes(dimension)||(runtime&&dimension!=='persistence')))error(where,'Applicable surface cannot waive required proof dimensions');
        if(applicable&&!text(record.reason))error(where,'Dimension nonapplicability requires its own specific reason');
        if(record.evidence.length)error(where,'Nonapplicable proof cannot claim evidence');
        continue;
      }
      if(!applicable)error(where,'Nonapplicable item cannot claim applicable proof');
      for(const [index,reference] of record.evidence.entries()){
        const entry=where+'.evidence['+index+']';
        if(typeof reference==='string'&&record.status!=='COMPLETE'){
          // Earlier partial records use article-relative links, not acceptance receipts.
          const resolved=path.posix.normalize(path.posix.join('docs/features/required-features',reference));
          if(reference.includes('\\')||reference.includes(':')||resolved.startsWith('../')||path.posix.isAbsolute(resolved)||!sourceFiles.has(resolved))error(entry,'Historical evidence link is invalid or absent from source');
          else gap(entry,'Historical link has no source-bound acceptance receipt');
        }else evidence(reference,entry,dimension);
      }
      if(record.status==='COMPLETE'&&!record.evidence.length)error(where,'Complete proof requires source-bound evidence');
      if(record.status!=='COMPLETE'||!record.evidence.length)gap(where,'Proof is unfinished');
        if(claimComplete&&record.status!=='COMPLETE')error(where,'Complete parent cannot contain unfinished proof');
    }
  }
  for(const item of inventory.items){
    if(!object(item)||!text(item.id)){error('items','Item must have an identity');continue;}
    const loc=item.id;
    if(ids.has(item.id))error(loc,'Duplicate identity');ids.add(item.id);
    if(!REQUIRED_IDS.includes(item.id))error(loc,'Unknown public identity');
    if(!statuses.has(item.status)||!text(item.requirement)||!text(item.baseline)||!text(item.title))error(loc,'Invalid item status or required description');
    if(!exactKeys(item.surfaces,SURFACES)){error(loc+'.surfaces','Exactly eight surface rows are required');continue;}
    let applicableCount=0, incomplete=false;
    for(const surface of SURFACES){
      const row=item.surfaces[surface],where=loc+'.'+surface;
      if(!object(row)||!['APPLICABLE','NOT_APPLICABLE'].includes(row.applicability)||!statuses.has(row.status)||!text(row.reason)){error(where,'Surface requires applicability, supported status and a specific reason');continue;}
      const applicable=row.applicability==='APPLICABLE';
      if(!applicable){if(row.status!=='NOT_APPLICABLE')error(where,'Nonapplicable row must use NOT_APPLICABLE status');if(row.proof)proofs(row.proof,where+'.proof',false,false);continue;}
      applicableCount++;
      if(row.status==='NOT_APPLICABLE')error(where,'Applicable row cannot use NOT_APPLICABLE status');
      if(row.status!=='COMPLETE'){gap(where,'Applicable surface remains unfinished');incomplete=true;}
      // Shared historical proof is never enough to accept an individual surface.
      if(row.proof)proofs(row.proof,where+'.proof',true,row.status==='COMPLETE');
      else {gap(where+'.proof','Independent eight-dimension surface proof is missing');if(row.status==='COMPLETE')error(where,'Complete surface requires independent proof');}
    }
    if(item.status==='NOT_APPLICABLE'&&applicableCount)error(loc,'Nonapplicable item has applicable surfaces');
    if(item.status!=='NOT_APPLICABLE'&&!applicableCount)error(loc,'Applicable item has no applicable surfaces');
    if(item.status==='COMPLETE'&&incomplete)error(loc,'Complete item has unfinished surfaces');
    if(applicableCount&&item.status!=='COMPLETE')gap(loc,'Item remains unfinished');
    proofs(item.proof,loc+'.proof',applicableCount>0,item.status==='COMPLETE');
    const article='docs/features/required-features/'+item.id+'.md';
    if(!sourceFiles.has(article))error(loc,'Required feature article is absent from audited source');
  }
  for(const id of REQUIRED_IDS)if(!ids.has(id))error(id,'Required identity is missing');
  return {schemaVersion:1,sourceCommit,evidenceCommit,identityCount:ids.size,surfaceCount:8,proofDimensionCount:8,errors,gaps,complete:!errors.length&&!gaps.length,limitation:'Validates declared evidence and immutable bytes; does not itself execute product workflows or establish truthful human inspection.'};
}
function main(){
  const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
  const args=process.argv.slice(2);
  const allowed=new Set(['--inventory','--source','--evidence-source','--require-complete']);
  for(let i=0;i<args.length;i++){if(!allowed.has(args[i]))throw Error('Unknown argument');if(args[i]!=='--require-complete'&&(!args[i+1]||args[++i].startsWith('--')))throw Error('Missing argument value');}
  const value=name=>{const i=args.indexOf(name);return i<0?null:args[i+1];};
  const head=execFileSync('git',['rev-parse','HEAD'],{cwd:root,encoding:'utf8'}).trim();
  const sourceCommit=value('--source')||head,evidenceCommit=value('--evidence-source')||head;
  const parse=name=>JSON.parse(fs.readFileSync(name,'utf8').replace(/^\uFEFF/,''));
  const report=audit({root,sourceCommit,evidenceCommit,inventory:parse(value('--inventory')||path.join(root,'docs/features/required-features/inventory.json')),expected:parse(path.join(root,'docs/features/required-features/expected-ids.json'))});
  console.log(JSON.stringify(report,null,2));
  process.exitCode=report.errors.length||args.includes('--require-complete')&&!report.complete?1:0;
}
if(process.argv[1]&&path.resolve(process.argv[1])===fileURLToPath(import.meta.url)){try{main();}catch{console.error('Required-feature audit could not read valid inputs');process.exitCode=1;}}
