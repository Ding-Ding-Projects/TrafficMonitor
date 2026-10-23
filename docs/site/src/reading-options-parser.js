// Reading options contain objects and scalar values only. Parse object members
// before JSON.parse can discard duplicate keys, including escaped duplicates.
export function isPlainObject(value){
 if(!value||typeof value!=='object')return false;
 const prototype=Object.getPrototypeOf(value);
 return prototype===Object.prototype||prototype===null;
}
export function parseReadingOptions(text){
 if(typeof text!=='string'||new TextEncoder().encode(text).length>8192)throw Error('Reading options exceed the size limit');
 let cursor=0;
 const whitespace=()=>{while(cursor<text.length&&/[ \t\r\n]/.test(text[cursor]))cursor++;};
 const string=()=>{const start=cursor++;while(cursor<text.length){if(text[cursor]==='\\'){cursor+=2;continue;}if(text[cursor++]==='"')return JSON.parse(text.slice(start,cursor));}throw Error('Unterminated string');};
 const value=(depth=0)=>{
  if(depth>4)throw Error('Reading options are too deeply nested');
  whitespace();
  if(text[cursor]==='"')return string();
  if(text[cursor]==='{'){
   cursor++;const output=Object.create(null),keys=new Set();whitespace();
   if(text[cursor]==='}'){cursor++;return output;}
   while(cursor<text.length){
    whitespace();if(text[cursor]!=='"')throw Error('Expected an object key');
    const key=string();if(keys.has(key)||['__proto__','prototype','constructor'].includes(key))throw Error('Duplicate or unsafe object key');keys.add(key);
    whitespace();if(text[cursor++]!==':')throw Error('Expected a colon');output[key]=value(depth+1);whitespace();
    const delimiter=text[cursor++];if(delimiter==='}')return output;if(delimiter!==',')throw Error('Expected a comma');
   }
   throw Error('Unterminated object');
  }
  const scalar=text.slice(cursor).match(/^(?:true|false|null|-?(?:0|[1-9]\d*)(?:\.\d+)?(?:[eE][+-]?\d+)?)/);
  if(!scalar)throw Error('Invalid JSON value');cursor+=scalar[0].length;return JSON.parse(scalar[0]);
 };
 const result=value();whitespace();if(cursor!==text.length)throw Error('Unexpected trailing content');
 if(!isPlainObject(result)||Object.keys(result).sort().join(',')!=='encoding,lineEndings,preferences,schemaVersion'||result.schemaVersion!==1||result.encoding!=='UTF-8'||result.lineEndings!=='LF'||!isPlainObject(result.preferences))throw Error('Invalid reading options envelope');
 return result.preferences;
}
