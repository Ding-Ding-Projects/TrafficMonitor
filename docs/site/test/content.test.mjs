import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs/promises';
import os from 'node:os';
import path from 'node:path';
import {createHash} from 'node:crypto';
import {execFileSync} from 'node:child_process';
import {createContent} from '../lib/content.mjs';
test('own wiki links stay local while foreign projects retain their destinations',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'trafficmonitor-content-'));
 try{
  const text='[Install](Installation) [Own home](https://github.com/Ding-Ding-Projects/TrafficMonitor/wiki/Home) [Other](https://github.com/zhongyang219/TrafficMonitorSkinEditor/blob/master/README.md)';
  await fs.mkdir(path.join(root,'docs/wiki/pending'),{recursive:true});await fs.writeFile(path.join(root,'docs/wiki/pending/Home.md'),text);
  const git=args=>execFileSync('git',args,{cwd:root,encoding:'utf8',windowsHide:true,stdio:['pipe','pipe','pipe']}).trim();
  git(['init','--quiet']);
  git(['config','user.name','Claude Fable 5.1']);
  git(['config','user.email','noreply@anthropic.com']);
  git(['add','docs/wiki/pending/Home.md']);
  git(['-c','core.hooksPath=','commit','--quiet','-m','Record immutable article fixture','-m','Give the article a real address before checking its directions.\n\n先畀文章一個真實地址，再檢查佢有冇指錯路。\n\nCo-Authored-By: Claude Fable 5.1 <noreply@anthropic.com>']);
  const sourceCommit=git(['rev-parse','HEAD']);
  const home={id:'own-home',sourcePath:'docs/wiki/pending/Home.md',sourceUrl:'https://github.com/Ding-Ding-Projects/TrafficMonitor/blob/main/docs/wiki/pending/Home.md',wikiSourceUrl:'https://github.com/Ding-Ding-Projects/TrafficMonitor/wiki/Home',route:'/articles/own-home/',provenanceKind:'fork-wiki',sourceSha256:createHash('sha256').update(text).digest('hex')};
  const install={id:'own-install',sourcePath:'docs/wiki/pending/Installation.md',route:'/articles/own-install/'},old={id:'old-home',sourcePath:'docs/wiki/upstream/Home.md',route:'/wiki/home/'};
  const content=await createContent(root,path.join(root,'dist'),{sourceCommit,articles:[home,install,old],wikiAliases:[{alias:'Home',articleId:'old-home'}]},'/TrafficMonitor/');
  const {html}=await content.render(home);assert.match(html,/href="\/TrafficMonitor\/articles\/own-install\/"/);assert.match(html,/href="\/TrafficMonitor\/articles\/own-home\/"/);assert.match(html,/href="https:\/\/github.com\/zhongyang219\/TrafficMonitorSkinEditor\/blob\/master\/README.md"/);
 }finally{await fs.rm(root,{recursive:true,force:true});}
});
