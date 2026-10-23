import fs from 'node:fs/promises';
import path from 'node:path';
import {execFileSync} from 'node:child_process';
import {safeRelative} from './content.mjs';
export const upstreamBaseline='930f17533d6098989ebad62f210aa97d75ef174b';
export function validateHistoryEntries(entries,upstreamSet,forkSet,complete=false){
 const seen=new Set();let upstreamCount=0;
 for(const entry of entries){
  if(!/^[a-f0-9]{40}$/.test(entry.sha)||seen.has(entry.sha))throw Error('Invalid or duplicate history SHA');seen.add(entry.sha);
  if(entry.lineage==='fork'){
   if(!forkSet.has(entry.sha)||typeof entry.originalMessage!=='string'||!entry.originalMessage.trim()||entry.summaryMethod!=='verbatim-bilingual')throw Error('Fork history requires an exact source message and verbatim method');
  }else if(entry.lineage==='upstream'){
   if(!upstreamSet.has(entry.sha)||typeof entry.summary!=='string'||!entry.summary.trim()||!/[A-Za-z]/.test(entry.summary))throw Error('Upstream history requires a nonempty English summary');upstreamCount++;
  }else throw Error('Unknown history lineage');
 }
 if(complete&&upstreamCount!==upstreamSet.size)throw Error('Complete history is missing upstream translations');
 return {upstreamCount,seen};
}
export async function collectHistory(root,source){
 const git=args=>execFileSync('git',args,{cwd:root,encoding:'utf8',maxBuffer:32*1024*1024});
 const revisions=args=>new Set(git(['rev-list',...args]).trim().split('\n').filter(Boolean));
 const upstreamSet=revisions([upstreamBaseline]),forkSet=revisions([source,'--not',upstreamBaseline]);
 if(upstreamSet.size!==1387)throw Error('Unexpected upstream history baseline');
 let inputManifest=null,entries=[];
 try{inputManifest=JSON.parse(await fs.readFile(path.join(root,'docs/site-changelog/manifest.json'),'utf8'));}catch(error){if(error.code!=='ENOENT')throw error;}
 if(inputManifest){
  if(inputManifest.schemaVersion!==1)throw Error('Unsupported history schema');
  for(const file of inputManifest.files){const chunk=JSON.parse(await fs.readFile(path.join(root,'docs/site-changelog',safeRelative(file.path)),'utf8'));if(!Array.isArray(chunk.entries)||chunk.entries.length!==file.count)throw Error('History file count mismatch');entries.push(...chunk.entries);}
  if(entries.length!==inputManifest.entryCount)throw Error('History manifest count mismatch');
 }
 const {upstreamCount,seen}=validateHistoryEntries(entries,upstreamSet,forkSet,inputManifest?.coverage?.complete===true);
 const loaded=new Map(entries.map(entry=>[entry.sha,entry]));
 for(const sha of forkSet){
  const raw=git(['show','-s','--format=format:%H%x00%aI%x00%B',sha]),first=raw.indexOf('\0'),second=raw.indexOf('\0',first+1);
  const actualSha=raw.slice(0,first),date=raw.slice(first+1,second),originalMessage=raw.slice(second+1);
  if(actualSha!==sha||first<0||second<0||!originalMessage.trim())throw Error('Unable to read exact fork history');
  if(seen.has(sha)){
   const existing=loaded.get(sha);if(existing.originalMessage!==originalMessage)throw Error('Fork history differs from its immutable commit message');existing.date=date;
  }else entries.push({sha,date,originalMessage,originalSubject:originalMessage.split('\n')[0],summaryMethod:'verbatim-bilingual',category:'source-record',lineage:'fork',verificationStatus:'not-asserted'});
 }
 for(const entry of entries)entry.commitUrl=`https://github.com/Ding-Ding-Projects/TrafficMonitor/commit/${entry.sha}`;
 entries.sort((a,b)=>Date.parse(b.date)-Date.parse(a.date)||a.sha.localeCompare(b.sha));
 return {entries,manifest:{schemaVersion:1,buildSourceCommit:source,translationSourceCommit:inputManifest?.sourceCommit??null,upstreamBaseline,entryCount:entries.length,coverage:{complete:upstreamCount===upstreamSet.size&&inputManifest?.coverage?.complete===true,upstreamExpected:upstreamSet.size,upstreamRepresented:upstreamCount,upstreamRemaining:upstreamSet.size-upstreamCount,forkExpected:forkSet.size,forkRepresented:forkSet.size},inputManifest}};
}
