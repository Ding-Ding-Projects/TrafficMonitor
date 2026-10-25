// Validation of reader-normalized numeric records, not a transport or receipt schema.
// The native reader must normalize the actual shared protocol explicitly.
import assert from 'node:assert/strict';
import { pathToFileURL } from 'node:url';

function need(condition, reason) { if (!condition) throw new Error(reason); }
function keys(value, allowed) { need(value && typeof value === 'object' && Object.keys(value).every(key => allowed.includes(key)), 'unexpected-renderer-field'); }
function integer(value, name) { need(typeof value === 'string' && /^\d+$/u.test(value), name); return BigInt(value); }
function handle(value) { need(typeof value === 'string' && /^0x[0-9a-f]{1,16}$/iu.test(value) && BigInt(value) > 0n, 'renderer-hwnd'); return BigInt(value).toString(); }
function rect(value) {
  need(value && Object.keys(value).sort().join(',') === 'bottom,left,right,top', 'renderer-rectangle');
  need(Object.values(value).every(x => Number.isFinite(x) && Math.abs(x) <= 1_000_000) && value.right > value.left && value.bottom > value.top, 'renderer-rectangle');
  return value;
}
function contains(outer, inner) { return inner.left >= outer.left && inner.top >= outer.top && inner.right <= outer.right && inner.bottom <= outer.bottom; }

export function checkRenderJoin(snapshot, expected, controls) {
  need(snapshot && expected && Array.isArray(controls) && controls.length <= 2048, 'renderer-input');
  keys(snapshot, ['pid', 'creationFiletime', 'sequenceBefore', 'sequenceAfter', 'overflow', 'records']);
  need(snapshot.pid === expected.pid && integer(snapshot.creationFiletime, 'renderer-creation') === integer(expected.creationFiletime, 'expected-creation'), 'renderer-identity');
  const before = integer(snapshot.sequenceBefore, 'renderer-sequence');
  const after = integer(snapshot.sequenceAfter, 'renderer-sequence');
  need(before > 0n && before === after && (before & 1n) === 0n && snapshot.overflow === false, 'renderer-torn-or-overflow');
  const start = integer(expected.startedTick, 'interval-start');
  const end = integer(expected.completedTick, 'interval-end');
  need(end >= start && integer(expected.minimumRecordSequence, 'minimum-sequence') > 0n, 'renderer-interval');
  need(Array.isArray(snapshot.records) && snapshot.records.length <= 4096, 'renderer-capacity');
  const records = new Map();
  for (const record of snapshot.records) {
    keys(record, ['hwnd', 'pid', 'creationFiletime', 'kind', 'dpi', 'sequence', 'observedTick', 'available', 'bounds', 'textStatus', 'textReason', 'fontStatus', 'face', 'height', 'ascent', 'descent']);
    need(record && ['paint', 'native-text', 'layout-measurement', 'rail'].includes(record.kind), 'renderer-kind');
    const key = `${handle(record.hwnd)}:${record.kind}`; need(!records.has(key), 'renderer-duplicate'); records.set(key, record);
  }
  const unavailable = [], findings = [];
  const controlHandles = new Set();
  for (const control of controls) {
    const id = handle(control.hwnd); need(!controlHandles.has(id), 'structural-duplicate'); controlHandles.add(id);
    need(control.ownerPid === expected.pid && Number.isInteger(control.dpi) && control.dpi > 0, 'structural-identity');
    if (!control.visible) continue;
    need(Array.isArray(control.requiredKinds) && control.requiredKinds.length > 0, 'missing-observation-inventory');
    for (const kind of control.requiredKinds) {
      const record = records.get(`${id}:${kind}`); need(record, 'renderer-missing-observation');
      need(record.pid === expected.pid && record.creationFiletime === expected.creationFiletime, 'renderer-record-identity');
      need(record.dpi === control.dpi, 'renderer-dpi');
      const stamp = integer(record.observedTick, 'record-time');
      const sequence = integer(record.sequence, 'record-sequence');
      need(stamp >= start && stamp <= end && sequence >= BigInt(expected.minimumRecordSequence) && sequence <= after, 'renderer-stale');
      if (kind === 'layout-measurement') { unavailable.push({ hwnd: control.hwnd, reason: 'layout-measurement-is-not-paint-proof' }); continue; }
      if (kind === 'rail') { unavailable.push({ hwnd: control.hwnd, reason: 'raw-rail-record-needs-protocol-specific-axis-validation' }); continue; }
      const available = rect(record.available);
      if (!contains(rect(control.rect), available)) findings.push({ hwnd: control.hwnd, kind: 'text-area-overflow' });
      if (record.textStatus === 'observed') {
        if (!contains(available, rect(record.bounds))) findings.push({ hwnd: control.hwnd, kind: 'text-overflow' });
      } else {
        need(record.textStatus === 'not-applicable' && record.bounds === null && record.textReason === 'empty-text', 'renderer-text-unavailable');
      }
      if (record.fontStatus !== 'observed') unavailable.push({ hwnd: control.hwnd, reason: 'strict-sidecar-requires-observed-font-even-for-nontext' });
      else need(typeof record.face === 'string' && record.face.length > 0 && Number.isInteger(record.height) && record.height > 0 && Number.isInteger(record.ascent) && record.ascent >= 0 && Number.isInteger(record.descent) && record.descent >= 0 && record.ascent + record.descent === record.height, 'renderer-font');
    }
  }
  return { kind: 'incomplete-render-join', findings, unavailable, measurementJoinAccepted: findings.length === 0,
    receiptComplete: false, validV2Receipt: false, validV3Receipt: false };
}

export function selfTest() {
  const expected = { pid: 41, creationFiletime: '123', startedTick: '100', completedTick: '200', minimumRecordSequence: '2' };
  const r = { left: 0, top: 0, right: 100, bottom: 40 };
  const controls = [{ hwnd: '0x10', ownerPid: 41, dpi: 96, visible: true, rect: r, requiredKinds: ['paint'] }];
  const snapshot = { pid: 41, creationFiletime: '123', sequenceBefore: '4', sequenceAfter: '4', overflow: false,
    records: [{ hwnd: '0x10', pid: 41, creationFiletime: '123', kind: 'paint', dpi: 96, sequence: '4', observedTick: '150', available: r, bounds: { left: 2, top: 2, right: 98, bottom: 30 }, textStatus: 'observed', fontStatus: 'observed', face: 'Synthetic', height: 16, ascent: 12, descent: 4 }] };
  assert.equal(checkRenderJoin(snapshot, expected, controls).receiptComplete, false);
  for (const [edit, reason] of [
    [s => s.pid++, /renderer-identity/],
    [s => s.sequenceAfter = '6', /renderer-torn-or-overflow/],
    [s => s.sequenceBefore = s.sequenceAfter = '3', /renderer-torn-or-overflow/],
    [s => s.overflow = true, /renderer-torn-or-overflow/],
    [s => s.records = [], /renderer-missing-observation/],
    [s => s.records[0].dpi = 120, /renderer-dpi/],
    [s => s.records[0].observedTick = '99', /renderer-stale/],
    [s => s.records[0].sequence = '1', /renderer-stale/],
    [s => s.records[0].textHash = 'forbidden', /unexpected-renderer-field/],
  ]) { const changed = structuredClone(snapshot); edit(changed); assert.throws(() => checkRenderJoin(changed, expected, controls), reason); }
  const overflow = structuredClone(snapshot); overflow.records[0].bounds.right = 101;
  assert.equal(checkRenderJoin(overflow, expected, controls).measurementJoinAccepted, false);
  const noFont = structuredClone(snapshot); noFont.records[0].fontStatus = 'unavailable';
  assert.match(checkRenderJoin(noFont, expected, controls).unavailable[0].reason, /nontext/);
  console.log('Render join checks: 12 passed; synthetic numeric records only, no renderer execution proof.');
}
if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  if (process.argv.length === 3 && process.argv[2] === '--self-test') selfTest();
  else throw new Error('Use the exported normalized-record checker or --self-test. No receipt is emitted.');
}
