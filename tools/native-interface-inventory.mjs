// Hand-written inventory for unfiltered, nonconflicting Settings > Interface.
// This validates structural scope only; it does not certify renderer observations.
import assert from 'node:assert/strict';
import { pathToFileURL } from 'node:url';

export const interfaceInventory = Object.freeze([
  { role: 'navigation', id: 54001, parent: 'root' },
  { role: 'search', id: 54010, parent: 'root' },
  { role: 'clear', id: 54011, parent: 'root' },
  { role: 'legacy-settings', id: 54015, parent: 'root' },
  { role: 'regex', id: 54017, parent: 'root' },
  { role: 'body', id: 53999, parent: 'root' },
  { role: 'language-panel', id: 56220, parent: 'body' },
  { role: 'appearance-panel', id: 56221, parent: 'body' },
  { role: 'preview-panel', id: 56222, parent: 'body' },
  { role: 'language', id: 56200, parent: 'language-panel' },
  { role: 'appearance', id: 56201, parent: 'appearance-panel' },
  { role: 'apply', id: 56202, parent: 'root' },
  { role: 'use-saved', id: 56203, parent: 'root' },
  { role: 'reset', id: 56204, parent: 'root' },
  { role: 'details', id: 56206, parent: 'body' },
  { role: 'preview', id: 56210, parent: 'preview-panel' },
]);

function need(condition, reason) { if (!condition) throw new Error(reason); }
function hwnd(value) {
  need(typeof value === 'string' && /^0x[0-9a-f]{1,16}$/iu.test(value), 'invalid-hwnd');
  const number = BigInt(value); need(number > 0n, 'invalid-hwnd'); return `0x${number.toString(16)}`;
}

export function inspectInterfaceInventory(rows, { expanded = false } = {}) {
  need(Array.isArray(rows) && rows.length <= 4096, 'invalid-structural-input');
  const starts = rows.filter(row => row.phase === 'identity.start.verified');
  const ends = rows.filter(row => row.phase === 'identity.end.verified');
  need(starts.length === 1 && ends.length === 1, 'missing-identity-interval');
  const identity = starts[0];
  for (const key of ['pid', 'creationFiletime', 'rootHwnd', 'executablePath', 'executableSha256', 'desktop'])
    need(identity[key] !== undefined && identity[key] === ends[0][key], 'changed-identity');
  const root = hwnd(identity.rootHwnd);
  const controls = rows.filter(row => row.phase === 'control');
  need(controls.length > 1 && controls.length <= 2049, 'control-inventory-size');
  const map = new Map();
  for (const row of controls) {
    const id = hwnd(row.hwnd); need(!map.has(id), 'duplicate-control');
    need(row.ownerPid === identity.pid, 'foreign-control');
    map.set(id, row);
  }
  need(map.has(root) && map.get(root).visible === true, 'missing-visible-root');
  const roles = new Map([['root', map.get(root)]]);
  const covered = new Set([root]);
  for (const item of interfaceInventory) {
    const parent = roles.get(item.parent); need(parent, 'inventory-order');
    const matches = controls.filter(row => row.visible && row.controlId === item.id && row.parent && hwnd(row.parent) === hwnd(parent.hwnd));
    need(matches.length === 1, `missing-or-ambiguous-${item.role}`);
    roles.set(item.role, matches[0]); covered.add(hwnd(matches[0].hwnd));
  }
  // Anonymous native statics cannot safely be distinguished by list position or text.
  // Their owner/member identities remain a renderer-side requirement.
  const staticGroups = [
    { parent: 'root', count: 3, roles: ['page-title', 'result-count', 'preference-status'] },
    { parent: 'body', count: expanded ? 2 : 1, roles: expanded ? ['section-title', 'scope'] : ['section-title'] },
    { parent: 'language-panel', count: 1, roles: ['language-label'] },
    { parent: 'appearance-panel', count: 1, roles: ['appearance-label'] },
  ];
  const anonymousGroups = [];
  for (const group of staticGroups) {
    const parent = roles.get(group.parent);
    const matches = controls.filter(row => row.visible && row.className.toLowerCase() === 'static' && row.parent && hwnd(row.parent) === hwnd(parent.hwnd));
    need(matches.length === group.count, `anonymous-count-${group.parent}`);
    for (const row of matches) covered.add(hwnd(row.hwnd));
    anonymousGroups.push({ parent: group.parent, possibleRoles: group.roles, hwnds: matches.map(row => hwnd(row.hwnd)), exactMemberMapping: 'unavailable' });
  }
  for (const [parentRole, id] of [['body', 53998], ['navigation', 54901]]) {
    const matches = controls.filter(row => row.visible && row.controlId === id && row.parent && hwnd(row.parent) === hwnd(roles.get(parentRole).hwnd));
    need(matches.length <= 1, 'ambiguous-scroll-rail');
    for (const row of matches) covered.add(hwnd(row.hwnd));
  }
  for (const row of controls) {
    let current = hwnd(row.hwnd); const visited = new Set();
    while (current !== root) {
      need(!visited.has(current) && map.has(current), 'invalid-ancestor-chain'); visited.add(current);
      const parent = map.get(current).parent; need(parent, 'missing-ancestor'); current = hwnd(parent);
    }
    if (row.visible) need(covered.has(hwnd(row.hwnd)), 'unexpected-visible-control');
  }
  return { kind: 'incomplete-interface-inventory', state: expanded ? 'expanded' : 'collapsed', visibleCount: covered.size,
    roles: Object.fromEntries([...roles].map(([role, row]) => [role, hwnd(row.hwnd)])), anonymousGroups,
    receiptComplete: false, limitations: ['anonymous-member-binding', 'renderer-font-for-every-control', 'renderer-text', 'custom-scroll', 'paint-sequence', 'image-binding'] };
}

export function selfTest() {
  const identity = { pid: 41, creationFiletime: '123', rootHwnd: '0x1', executablePath: 'synthetic.exe', executableSha256: 'a'.repeat(64), desktop: 'synthetic' };
  const rows = [{ ...identity, phase: 'identity.start.verified' }, { ...identity, phase: 'identity.end.verified' }];
  let next = 2; const roles = new Map([['root', '0x1']]);
  const add = (id, parent, cls = 'Synthetic') => { const h = `0x${(next++).toString(16)}`; rows.push({ phase: 'control', hwnd: h, parent, controlId: id, className: cls, ownerPid: 41, visible: true }); return h; };
  rows.push({ phase: 'control', hwnd: '0x1', parent: null, controlId: 0, className: 'Synthetic', ownerPid: 41, visible: true });
  for (const item of interfaceInventory) roles.set(item.role, add(item.id, roles.get(item.parent)));
  for (const [role, count] of [['root', 3], ['body', 1], ['language-panel', 1], ['appearance-panel', 1]])
    for (let i = 0; i < count; i++) add(-1, roles.get(role), 'Static');
  assert.equal(inspectInterfaceInventory(rows).receiptComplete, false);
  assert.throws(() => inspectInterfaceInventory(rows.filter(row => row.controlId !== 56206)), /missing-or-ambiguous-details/);
  const foreign = structuredClone(rows); foreign.at(-1).ownerPid = 42;
  assert.throws(() => inspectInterfaceInventory(foreign), /foreign-control/);
  const moved = structuredClone(rows); moved[1].creationFiletime = '124';
  assert.throws(() => inspectInterfaceInventory(moved), /changed-identity/);
  const extra = structuredClone(rows); extra.push({ ...extra.at(-1), hwnd: '0x100', className: 'Unexpected' });
  assert.throws(() => inspectInterfaceInventory(extra), /unexpected-visible-control/);
  console.log('Interface inventory checks: 5 passed; synthetic scope only, no runtime proof.');
}
if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  if (process.argv.length === 3 && process.argv[2] === '--self-test') selfTest();
  else throw new Error('Use the exported read-only inspector or --self-test. No receipt is emitted.');
}
