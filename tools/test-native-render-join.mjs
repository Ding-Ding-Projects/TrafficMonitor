import assert from 'node:assert/strict';
import { interfaceInventory } from './native-interface-inventory.mjs';
import { inspectNativeRenderJoin } from './inspect-native-render-join.mjs';

// Deliberately synthetic protocol-shaped records, never runtime evidence.
const identity = { pid: 41, creationFiletime: '123', rootHwnd: '0x1', executablePath: 'synthetic.exe', executableSha256: 'a'.repeat(64), desktop: 'synthetic' };
const rows = [{ ...identity, phase: 'identity.start.verified' }, { ...identity, phase: 'identity.end.verified' }];
const roles = new Map([['root', '0x1']]);
let next = 2;
function add(id, parent, cls = 'Synthetic') {
  const hwnd = '0x' + (next++).toString(16);
  rows.push({ phase: 'control', hwnd, parent, controlId: id, className: cls, ownerPid: 41, visible: true, dpi: 96 }); return hwnd;
}
rows.push({ phase: 'control', hwnd: '0x1', parent: null, controlId: 0, className: 'Synthetic', ownerPid: 41, visible: true, dpi: 96 });
for (const item of interfaceInventory) roles.set(item.role, add(item.id, roles.get(item.parent)));
for (const [role, count] of [['root', 3], ['body', 1], ['language-panel', 1], ['appearance-panel', 1]])
  for (let i = 0; i < count; i++) add(-1, roles.get(role), 'Static');
const controls = rows.filter(row => row.phase === 'control');
for (const control of controls) rows.push({ phase: 'renderer.record', hwnd: control.hwnd, kind: 4, dpi: 96, qpc: '110', sequence: '4', measuredOverflow: null, fontSource: 3, validity: 512 });
rows.push({ phase: 'renderer.interval.begin', protocolVersion: 2, sequence: '4', qpc: '120', publishedQpc: '115', minimumSequence: '2', maximumAgeTicks: '100', logicalBytes: 92224, pageBytes: 4096, sectionBytes: 94208 });
rows.push({ phase: 'renderer.interval.end', sequence: '4', qpc: '130', observedControls: controls.length });
rows.push({ phase: 'complete', receiptComplete: false });
assert.equal(inspectNativeRenderJoin(rows).validV2Receipt, false);
for (const [mutate, expected] of [
  [r => r.find(x => x.phase === 'renderer.record').dpi = 120, /renderer-dpi/],
  [r => r.find(x => x.phase === 'renderer.record').qpc = '1', /stale-renderer-record/],
  [r => r.find(x => x.phase === 'renderer.interval.end').sequence = '6', /changed-render-sequence/],
  [r => r.splice(r.findIndex(x => x.phase === 'renderer.record'), 1), /missing-visible-renderer-record/],
  [r => r.find(x => x.phase === 'renderer.record').measuredOverflow = true, /measured-text-overflow/],
  [r => r.find(x => x.phase === 'renderer.record').textHash = 'forbidden', /forbidden-private-payload-field/],
  [r => r.find(x => x.phase === 'identity.end.verified').creationFiletime = '124', /changed-identity/],
  [r => r.find(x => x.phase === 'renderer.interval.begin').protocolVersion = 1, /unsupported-renderer-protocol/],
  [r => r.find(x => x.phase === 'renderer.record').kind = 1, /missing-actual-render-dpi/],
]) { const changed = structuredClone(rows); mutate(changed); assert.throws(() => inspectNativeRenderJoin(changed), expected); }
console.log('Native join summary checks: 10 passed; synthetic protocol-shaped input, no product observation.');
