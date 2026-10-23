/** Executes only from the verified immutable external scanner snapshot. */
import {execFileSync} from 'node:child_process';
import {existsSync,readFileSync,realpathSync,mkdtempSync,writeFileSync,rmSync} from 'node:fs';
import {tmpdir} from 'node:os';
import path from 'node:path';
import {pathToFileURL,fileURLToPath} from 'node:url';
import {readPolicy,scanRepository} from './public-language-scan.mjs';
import {readExceptionCarrier,selectRevisionExceptions} from './publication-exceptions.mjs';
import {outgoingRevisions} from './publication-history.mjs';
try{
 const root=realpathSync(process.argv[2]),mode=process.argv[3];
 if(!['index','outgoing'].includes(mode)||fileURLToPath(import.meta.url).startsWith(root+path.sep))throw new Error();
 const git=(args,input)=>execFileSync('git',['-c','core.hooksPath=',...args],{cwd:root,input,windowsHide:true,encoding:'utf8',maxBuffer:16777216,stdio:['pipe','pipe','pipe']}).trim();
 const producer=git(['config','--local','--get','publication.producerModule']);
 if(!path.isAbsolute(producer)||!existsSync(producer)||realpathSync(producer).startsWith(root+path.sep))throw new Error();
 const {publicationLanguageTransport}=await import(pathToFileURL(realpathSync(producer)).href);
 const policy=readPolicy(Buffer.from(publicationLanguageTransport()));
 const policyFile=path.join(process.env.LOCALAPPDATA||tmpdir(),'TrafficMonitorPublicationPolicy','exceptions.json');
 if(!existsSync(policyFile))throw new Error();
 const carrier=readExceptionCarrier(policyFile,root);
 let revisions;
 if(mode==='index'){
  const tree=git(['write-tree']);
  revisions=[git(['-c','user.name=Claude Fable 5.1','-c','user.email=noreply@anthropic.com','commit-tree',tree,'-p','HEAD'],'Publication index snapshot\n\nCo-Authored-By: Claude Fable 5.1 <noreply@anthropic.com>\n')];
 }else{
  revisions=outgoingRevisions({root,remoteName:process.argv[4],remoteLocation:process.argv[5],expectedRepository:'https://github.com/Ding-Ding-Projects/TrafficMonitor.git',input:readFileSync(0)});
 }
 const scratch=mkdtempSync(path.join(tmpdir(),'trafficmonitor-policy-'));
 try{
  const selected=path.join(scratch,'exceptions.json');
  let code=0;
  for(const revision of new Set(revisions)){
   writeFileSync(selected,JSON.stringify(selectRevisionExceptions(carrier,revision)),{mode:0o600});
   const report=scanRepository({root,revision,policy,exceptionsFile:selected});
   process.stdout.write(JSON.stringify(report)+'\n');
   if(report.findings.length)code=1;
  }
  process.exitCode=code;
 }finally{rmSync(scratch,{recursive:true,force:true});}
}catch{process.stderr.write('Local publication check incomplete; operation blocked.\n');process.exitCode=2;}
