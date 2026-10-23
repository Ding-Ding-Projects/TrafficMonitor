export function parseWording(text) {
 if(new TextEncoder().encode(text).length>262144)throw Error('The file exceeds 256 KiB.');
 // Parse tokens before JSON.parse so duplicate keys cannot silently overwrite data.
 let i=0;
 const ws=()=>{while(/\s/.test(text[i]??'')&&i<text.length)i++;};
 function string(){const start=i++;while(i<text.length){if(text[i]==='\\'){i+=2;continue;}if(text[i++]==='"')return JSON.parse(text.slice(start,i));}throw Error('Invalid JSON string.');}
 function value(depth=0){if(depth>8)throw Error('JSON is too deeply nested.');ws();const c=text[i];if(c==='"')return string();if(c==='{'){i++;const out=Object.create(null),keys=new Set();ws();if(text[i]==='}'){i++;return out;}while(i<text.length){ws();if(text[i]!=='"')throw Error('Expected a JSON key.');const key=string();if(keys.has(key)||['__proto__','prototype','constructor'].includes(key))throw Error('Duplicate or unsafe JSON key.');keys.add(key);ws();if(text[i++]!==':')throw Error('Expected a colon.');out[key]=value(depth+1);ws();const end=text[i++];if(end==='}')return out;if(end!==',')throw Error('Expected a comma.');}throw Error('Unclosed object.');}if(c==='[')throw Error('Arrays are not supported.');const m=text.slice(i).match(/^(?:true|false|null|-?(?:0|[1-9]\d*)(?:\.\d+)?(?:[eE][+-]?\d+)?)/);if(!m)throw Error('Invalid JSON value.');i+=m[0].length;return JSON.parse(m[0]);}
 const data=value();ws();if(i!==text.length)throw Error('Unexpected trailing data.');
 if(!data||Object.keys(data).sort().join(',')!=='entries,schemaVersion'||data.schemaVersion!==1||!data.entries||typeof data.entries!=='object')throw Error('Expected schemaVersion 1 and entries object.');
 if(Object.keys(data.entries).length>10000)throw Error('Too many entries.');
 for(const [key,val] of Object.entries(data.entries)){if(!key.trim()||[...key].length>160||typeof val!=='string'||!val.trim()||[...val].length>1000||/[\u0000-\u0008\u000b\u000c\u000e-\u001f]/.test(key+val))throw Error('Invalid wording entry.');}
 return data;
}
export const storage={read(key,fallback){try{return JSON.parse(localStorage.getItem('trafficmonitor.docs.'+key))??fallback;}catch{return fallback;}},write(key,value){try{localStorage.setItem('trafficmonitor.docs.'+key,JSON.stringify(value));return true;}catch{return false;}},remove(key){try{localStorage.removeItem('trafficmonitor.docs.'+key);return true;}catch{return false;}}};
