import {benefitFor} from './capabilities.mjs';

// Existing controls have actual consumers. Planned records never imply that a
// similarly named website preference implements the full requirement.
const consumers={
 language:{controls:['language'],consumer:'src/app.js apply and src/locale.js',keys:['preferences.language'],reset:'reset-preferences',recovery:'English fallback when the stored mode is invalid'},
 'personal-vocabulary':{controls:['wording-load','wording-clear'],consumer:'src/wording.js and src/locale.js',keys:['wording'],reset:'wording-clear',recovery:'Invalid input retains the previous valid wording'},
 motion:{controls:['motion'],consumer:'src/app.js and src/style.css',keys:['preferences.motion'],reset:'reset-preferences',recovery:'System preference is used when no override exists'},
 'accessibility-sizing':{controls:['text-size'],consumer:'src/app.js apply',keys:['preferences.size'],reset:'reset-preferences',recovery:'Unsupported stored values fall back to 100 percent'},
 'appearance-editor':{controls:['theme'],consumer:'src/app.js and src/style.css',keys:['preferences.theme'],reset:'reset-preferences',recovery:'System theme fallback; complete per-element editing remains unfinished'},
 'progress-recovery':{controls:['reading-progress-reset'],consumer:'src/learning.js and src/guide.js',keys:['learning-progress-v1','guide'],reset:'reading-progress-reset',recovery:'Unavailable storage retains current-session reading behavior'},
 'regex-search':{controls:['settings-search','search-state-reset'],consumer:'src/learning.js, src/capabilities.js and src/regex-workbench.js',keys:['search-documentation','search-capability','search-settings','search-history'],reset:'search-state-reset',recovery:'Invalid patterns and deadlines retain the last useful results; full workbench parity remains unfinished'}
};
export function settingsContract(inventory,sourceCommit){
 const items=inventory.items.map(item=>({id:item.id,group:benefitFor(item.id),applicability:item.surfaces['hosted-documentation'].applicability,sourceStatus:item.surfaces['hosted-documentation'].status,reason:item.surfaces['hosted-documentation'].reason,
  implementation:consumers[item.id]?'PARTIAL':'UNFINISHED',...(consumers[item.id]??{controls:[],plannedControl:`settings-${item.id}`,consumer:null,keys:[],reset:null,recovery:'No consumer is implemented; do not apply or store this requirement through a placeholder control'})}));
 if(items.length!==110||new Set(items.map(i=>i.id)).size!==110||items.some(i=>!i.group))throw Error('Settings identity mapping is incomplete');
 return {schemaVersion:1,sourceCommit,purpose:'Control and consumer mapping, not feature acceptance',items};
}
