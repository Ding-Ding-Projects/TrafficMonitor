// Read-only summary of real observer JSONL. Never emits a v2/v3 receipt.
import fs from 'node:fs';
import { pathToFileURL } from 'node:url';
import { inspectInterfaceInventory } from './native-interface-inventory.mjs';

function need(ok, reason) { if (!ok) throw new Error(reason); }
function number(value) { need(typeof value === 'string' && /^\d+$/u.test(value), 'invalid-numeric-identity'); return BigInt(value); }
function hwnd(value) { need(typeof value === 'string' && /^0x[0-9a-f]{1,16}$/iu.test(value), 'invalid-hwnd'); return BigInt(value).toString(); }
function one(rows, phase) { const matches = rows.filter(row => row.phase === phase); need(matches.length === 1, 'missing-or-duplicate-' + phase); return matches[0]; }

export function inspectNativeRenderJoin(rows, options = {}) {
  need(Array.isArray(rows) && rows.length <= 10000, 'observation-size');
  need(!rows.some(row => row.phase === 'rejected'), 'native-observer-rejected');
  const inventory = inspectInterfaceInventory(rows, options);
  const start = one(rows, 'renderer.interval.begin'), end = one(rows, 'renderer.interval.end');
  const sequence = number(start.sequence);
  need(sequence > 0n && sequence % 2n === 0n && sequence === number(end.sequence), 'changed-render-sequence');
  need(number(end.qpc) >= number(start.qpc), 'invalid-render-interval');
  const minimum = number(start.minimumSequence), age = number(start.maximumAgeTicks);
  need(minimum > 0n && sequence >= minimum && age > 0n, 'invalid-render-freshness');
  need(Number.isSafeInteger(start.logicalBytes) && start.logicalBytes > 0 && Number.isSafeInteger(start.pageBytes) && start.pageBytes > 0 &&
    start.sectionBytes === Math.ceil(start.logicalBytes / start.pageBytes) * start.pageBytes, 'invalid-section-extent');
  const controls = new Map(rows.filter(row => row.phase === 'control').map(row => [hwnd(row.hwnd), row]));
  const seen = new Set(), observed = new Set(), unavailable = new Set(inventory.limitations);
  let paints = 0, measurements = 0, fonts = 0, rails = 0;
  for (const record of rows.filter(row => row.phase === 'renderer.record')) {
    const id = hwnd(record.hwnd), control = controls.get(id); need(control && control.visible, 'foreign-renderer-control');
    const key = id + ':' + record.kind; need(!seen.has(key), 'duplicate-renderer-record'); seen.add(key); observed.add(id);
    need(record.dpi === control.dpi, 'renderer-dpi');
    const stamp = number(record.qpc), recordSequence = number(record.sequence);
    need(recordSequence >= minimum && recordSequence <= sequence && stamp <= number(start.publishedQpc) &&
      stamp <= number(start.qpc) && number(end.qpc) - stamp <= age, 'stale-renderer-record');
    need(record.measuredOverflow !== true, 'measured-text-overflow');
    if (record.measuredOverflow === null) unavailable.add('rectangle-origin-overflow-assessment');
    if (record.kind === 1 && record.outcome === 1 && [1, 2].includes(record.route)) paints++;
    else if ([2, 3].includes(record.kind)) measurements++;
    else if (record.kind === 5) rails++;
    if (record.fontSource === 1 && (record.validity & 2)) fonts++;
    else unavailable.add(record.fontSource === 3 ? 'nontext-font-not-applicable-but-strict-schema-requires-font' : 'resolved-font-or-fallback-unavailable');
    if (record.validity & 128) unavailable.add('font-fallback-unavailable');
    if (record.validity & 256) unavailable.add('native-paint-unobserved');
    if (record.validity & 2048) unavailable.add('measurement-only-not-paint');
  }
  for (const [id, control] of controls) if (control.visible) need(observed.has(id), 'missing-visible-renderer-record');
  need(observed.size === end.observedControls, 'renderer-control-count');
  need(one(rows, 'complete').receiptComplete === false, 'unexpected-completion-claim');
  return { kind: 'incomplete-native-render-join-summary', inventoryState: inventory.state,
    visibleControls: observed.size, successfulDirectWritePaintRecords: paints, nativeMeasurementRecords: measurements,
    selectedGdiFontRecords: fonts, customRailRecords: rails, unavailable: [...unavailable],
    source: 'actual-observer-records-required', receiptComplete: false, validV2Receipt: false, validV3Receipt: false };
}
if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  need(process.argv.length === 4 && ['collapsed', 'expanded'].includes(process.argv[3]), 'usage: node inspect-native-render-join.mjs INPUT.jsonl collapsed|expanded');
  const stat = fs.statSync(process.argv[2]); need(stat.isFile() && stat.size <= 4 * 1024 * 1024, 'input-bound');
  const rows = fs.readFileSync(process.argv[2], 'utf8').split(/\r?\n/u).filter(Boolean).map(line => JSON.parse(line));
  console.log(JSON.stringify(inspectNativeRenderJoin(rows, { expanded: process.argv[3] === 'expanded' })));
}
