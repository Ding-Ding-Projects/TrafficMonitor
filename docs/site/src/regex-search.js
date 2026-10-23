// UTF-16 offsets match the browser RegExp API. No expressions execute here.
export function bindRegexSearch(input,records,text,apply,status){
 let sequence=0;
 input.addEventListener('input',()=>{sequence++;apply(null);});
 input.addEventListener('regex-plain',()=>{sequence++;apply(null);});
 input.addEventListener('regex-search',async event=>{
  const current=++sequence,{pattern,flags,execute}=event.detail;
  const spans=[];let subject='';for(const record of records){const start=subject.length;subject+=text(record);spans.push({record,start,end:subject.length});subject+='\n';}
  status.textContent='Running locally…';
  const result=await execute({operation:'match',pattern,flags:flags.includes('g')?flags:flags+'g',subject,maxMatches:1000});
  if(sequence!==current)return;
  if(!result.ok){status.textContent=`${result.code}: ${result.message}`;return;}
  const selected=new Set();for(const match of result.result.matches)for(const span of spans)if((match.index===match.end?match.index>=span.start&&match.index<span.end:match.index<span.end&&match.end>span.start))selected.add(span.record);
  apply(selected);status.textContent='Regex search applied. Clear the mode to return to literal search.';
 });
}
