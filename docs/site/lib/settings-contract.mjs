import {benefitFor} from './capabilities.mjs';

// Existing controls have actual consumers. Planned records never imply that a
// similarly named website preference implements the full requirement.
const consumers={
 notifications:{controls:['notification-corner','notification-duration','notification-preview','notification-warning','notification-reset'],consumer:'src/notifications.js and all shared notify callers',keys:['notifications'],reset:'notification-reset',recovery:'Invalid saved preferences use defaults; unavailable storage retains session choices and shows a persistent warning; message content is never persisted'},
 'collapse-filters':{controls:['reader-collapseFilters','reader-filter-reset'],consumer:'src/reader-preferences.js collection options on capability and history pages',keys:['reader-preferences.collapseFilters','filters-<route>-<index>'],reset:'reader-filter-reset',recovery:'Search remains visible; active filter count remains in the disclosure; missing storage uses the selected default'},
 exports:{controls:['reader-export','reader-import'],consumer:'src/reader-preferences.js bounded six-option JSON exchange',keys:['reader-preferences'],reset:'reader-reset',recovery:'Invalid or oversized imports and unavailable storage retain previous options; other formats and full settings exchange remain unfinished'},
 language:{controls:['language'],consumer:'src/app.js apply and src/locale.js',keys:['preferences.language'],reset:'reset-preferences',recovery:'English fallback when the stored mode is invalid'},
 'personal-vocabulary':{controls:['wording-load','wording-clear'],consumer:'src/wording.js and src/locale.js',keys:['wording'],reset:'wording-clear',recovery:'Invalid input retains the previous valid wording'},
 motion:{controls:['motion'],consumer:'src/app.js and src/style.css',keys:['preferences.motion'],reset:'reset-preferences',recovery:'System preference is used when no override exists'},
 'accessibility-sizing':{controls:['text-size'],consumer:'src/app.js apply',keys:['preferences.size'],reset:'reset-preferences',recovery:'Unsupported stored values fall back to 100 percent'},
 'appearance-editor':{controls:['theme','reader-density','reader-font','reader-measure','reader-lineHeight','reader-accent','reader-reset'],consumer:'src/app.js, src/reader-preferences.js and src/style.css',keys:['preferences.theme','reader-preferences'],reset:'reset-preferences and reader-reset',recovery:'System theme fallback; complete per-element editing remains unfinished'},
 'progress-recovery':{controls:['reading-progress-reset'],consumer:'src/learning.js and src/guide.js',keys:['learning-progress-v1','guide'],reset:'reading-progress-reset',recovery:'Unavailable storage retains current-session reading behavior'},
 'regex-search':{controls:['settings-search','search-state-reset'],consumer:'src/learning.js, src/capabilities.js and src/regex-workbench.js',keys:['search-documentation','search-capability','search-settings','search-history'],reset:'search-state-reset',recovery:'Invalid patterns and deadlines retain the last useful results; full workbench parity remains unfinished'}
};
export function settingsContract(inventory,sourceCommit){
 const items=inventory.items.map(item=>({id:item.id,group:benefitFor(item.id),applicability:item.surfaces['hosted-documentation'].applicability,sourceStatus:item.surfaces['hosted-documentation'].status,reason:item.surfaces['hosted-documentation'].reason,
  requiredBehavior:item.requirement,
  implementation:item.surfaces['hosted-documentation'].applicability==='NOT_APPLICABLE'?'NOT_APPLICABLE':consumers[item.id]?'PARTIAL':'UNFINISHED',
  missingConsumer:item.surfaces['hosted-documentation'].applicability==='NOT_APPLICABLE'?null:{requirement:item.requirement,reason:item.surfaces['hosted-documentation'].reason,acceptance:'The complete requirement still needs its own functional controls, persistence, reset, recovery and built interaction evidence beyond any partial consumer listed here'},
  ...(consumers[item.id]??{controls:[],consumer:null,keys:[],reset:null,recovery:item.surfaces['hosted-documentation'].applicability==='NOT_APPLICABLE'?'No documentation setting: '+item.surfaces['hosted-documentation'].reason:'No runtime consumer exists for this requirement. No placeholder preference is stored.'})}));
 if(items.length!==110||new Set(items.map(i=>i.id)).size!==110||items.some(i=>!i.group))throw Error('Settings identity mapping is incomplete');
 return {schemaVersion:1,sourceCommit,purpose:'Control and consumer mapping, not feature acceptance',items};
}
