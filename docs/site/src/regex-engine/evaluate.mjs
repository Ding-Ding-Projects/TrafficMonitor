import { byteLength, failure, resolveLimits, validateRequest } from './request.mjs';

// This module is imported by a Worker only. No evaluation is performed by the
// client module. The native engine has no tracing hook or cross-flavor emulation.
export function engineCapabilities() {
  const supports = (pattern, flags = '') => { try { new RegExp(pattern, flags); return true; } catch { return false; } };
  const flags = Object.fromEntries([... 'dgimsuvy'].map(flag => [flag, supports('', flag)]));
  return { name: 'ECMAScript RegExp', runtime: typeof WorkerGlobalScope !== 'undefined' ? 'browser-worker' : 'node-worker-test-adapter',
    runtimeBuild: typeof WorkerGlobalScope !== 'undefined' ? navigator.userAgent.slice(0, 2048) : null,
    engineVersion: null, engineVersionReason: 'The host does not expose a reliable separate RegExp engine version.',
    unicodeVersion: null, unicodeVersionReason: 'No verified host Unicode-data version is exposed.',
    positions: 'UTF-16 code units; end exclusive', flags,
    features: { namedCaptures: supports('(?<part>a)'), lookbehind: supports('(?<=a)b'),
      unicodeProperties: supports('\\p{Letter}', 'u'), matchIndices: flags.d, unicodeSets: flags.v,
      debuggerTrace: false, otherEngineExecution: false, legacyByteEncodings: false } };
}
class LimitError extends Error { constructor(code) { super(code); this.code = code; } }
function advance(subject, index, unicode) {
  if (!unicode || index + 1 >= subject.length) return index + 1;
  const first = subject.charCodeAt(index), second = subject.charCodeAt(index + 1);
  return first >= 0xD800 && first <= 0xDBFF && second >= 0xDC00 && second <= 0xDFFF ? index + 2 : index + 1;
}
function collector(maximum) {
  let bytes = 0;
  return value => {
    const encoded = JSON.stringify(value);
    bytes += byteLength(encoded);
    if (bytes > maximum) throw new LimitError('OUTPUT_LIMIT');
    return value;
  };
}
function matchRecord(match, limits, charge) {
  if (match.length - 1 > limits.maxCapturesPerMatch) throw new LimitError('CAPTURE_LIMIT');
  const pairs = values => values.map(pair => pair === undefined ? null : [pair[0], pair[1]]);
  return charge({ text: match[0], index: match.index, end: match.index + match[0].length,
    captures: Array.from(match).slice(1).map(value => value ?? null),
    namedCaptures: match.groups ? Object.fromEntries(Object.entries(match.groups).map(([key, value]) => [key, value ?? null])) : null,
    indices: match.indices ? pairs(match.indices) : null,
    namedIndices: match.indices?.groups ? Object.fromEntries(Object.entries(match.indices.groups).map(([key, pair]) =>
      [key, pair === undefined ? null : [pair[0], pair[1]]])) : null });
}
function enumerate(regex, subject, maxMatches, visit) {
  let count = 0;
  const unicode = regex.unicode || regex.unicodeSets;
  for (;;) {
    const match = regex.exec(subject);
    if (!match) return count;
    if (++count > maxMatches) throw new LimitError('MATCH_LIMIT');
    visit(match);
    if (!regex.global) return count;
    if (match[0] === '') regex.lastIndex = advance(subject, regex.lastIndex, unicode);
  }
}
function outputBuilder(maximum) {
  const chunks = []; let bytes = 0;
  return { append(text) {
    if (typeof text !== 'string') throw new TypeError('Invalid replacement value');
    if (text.length > maximum || (bytes += byteLength(text)) > maximum) throw new LimitError('OUTPUT_LIMIT');
    chunks.push(text);
  }, finish: () => chunks.join('') };
}
// ECMAScript string replacement tokens only. No callback, code or templates execute.
function substitute(replacement, match, subject, output) {
  let literal = '';
  for (let i = 0; i < replacement.length; ++i) {
    if (replacement[i] !== '$' || i + 1 === replacement.length) { literal += replacement[i]; continue; }
    const next = replacement[i + 1]; let value;
    if (next === '$') value = '$';
    else if (next === '&') value = match[0];
    else if (next === '`') value = subject.slice(0, match.index);
    else if (next === "'") value = subject.slice(match.index + match[0].length);
    else if (next === '<' && match.groups !== undefined) {
      const end = replacement.indexOf('>', i + 2);
      if (end !== -1) { value = match.groups[replacement.slice(i + 2, end)] ?? ''; i = end - 1; }
    } else if (next >= '0' && next <= '9') {
      let group = Number(next), consumed = 1;
      const second = replacement[i + 2];
      if (second >= '0' && second <= '9' && group * 10 + Number(second) < match.length) {
        group = group * 10 + Number(second); consumed = 2;
      }
      if (group > 0 && group < match.length) { value = match[group] ?? ''; i += consumed - 1; }
    }
    if (value === undefined) literal += '$';
    else { output.append(literal); literal = ''; output.append(value); ++i; }
  }
  output.append(literal);
}
function split(regex, subject, request, limits, charge) {
  const parts = [], unicode = regex.unicode || regex.unicodeSets;
  const splitter = new RegExp(regex.source, regex.flags.includes('y') ? regex.flags : regex.flags + 'y');
  const append = value => { parts.push(charge(value ?? null)); };
  if (subject.length === 0) { if (!splitter.exec(subject)) append(''); return { parts, separators: 0 }; }
  let p = 0, q = 0, count = 0;
  while (q < subject.length) {
    splitter.lastIndex = q;
    const match = splitter.exec(subject);
    if (!match || splitter.lastIndex === p) { q = advance(subject, q, unicode); continue; }
    if (++count > request.maxMatches) throw new LimitError('MATCH_LIMIT');
    if (match.length - 1 > limits.maxCapturesPerMatch) throw new LimitError('CAPTURE_LIMIT');
    append(subject.slice(p, q));
    for (let index = 1; index < match.length; ++index) append(match[index]);
    p = splitter.lastIndex; q = p;
  }
  append(subject.slice(p));
  return { parts, separators: count };
}
export function evaluate(input, configuration) {
  const started = performance.now();
  try {
    const limits = resolveLimits(configuration);
    const checked = validateRequest(input, limits);
    if (!checked.ok) return checked;
    const request = checked.request, engine = engineCapabilities();
    for (const flag of request.flags)
      if (!engine.flags[flag]) return failure('UNSUPPORTED_FLAG', 'Flag is unavailable in this host engine.', { flag });
    if (request.operation === 'capabilities') {
      const response = { ok: true, operation: request.operation, engine, elapsedMs: performance.now() - started, result: { limits } };
      if (byteLength(JSON.stringify(response)) > limits.maxOutputBytes) throw new LimitError('OUTPUT_LIMIT');
      return response;
    }
    let regex;
    try { regex = new RegExp(request.pattern, request.flags); }
    catch { return failure('SYNTAX_ERROR', 'The pattern is invalid for this ECMAScript engine.'); }
    const charge = collector(limits.maxOutputBytes);
    let result;
    if (request.operation === 'match' || request.operation === 'search') {
      const matches = [];
      if (request.operation === 'search') {
        const match = regex.exec(request.subject);
        result = { index: match?.index ?? -1, match: match ? matchRecord(match, limits, charge) : null };
      } else {
        enumerate(regex, request.subject, request.maxMatches, match => matches.push(matchRecord(match, limits, charge)));
        result = { matches, count: matches.length };
      }
    } else if (request.operation === 'replace') {
      const output = outputBuilder(limits.maxOutputBytes); let end = 0;
      const count = enumerate(regex, request.subject, request.maxMatches, match => {
        if (match.length - 1 > limits.maxCapturesPerMatch) throw new LimitError('CAPTURE_LIMIT');
        output.append(request.subject.slice(end, match.index));
        substitute(request.replacement, match, request.subject, output);
        end = match.index + match[0].length;
      });
      output.append(request.subject.slice(end));
      result = { text: output.finish(), replacements: count };
    } else result = split(regex, request.subject, request, limits, charge);
    const response = { ok: true, operation: request.operation, engine, elapsedMs: performance.now() - started, result };
    if (byteLength(JSON.stringify(response)) > limits.maxOutputBytes) throw new LimitError('OUTPUT_LIMIT');
    return response;
  } catch (error) {
    return failure(error instanceof LimitError ? error.code : 'EVALUATION_ERROR',
      error instanceof LimitError ? 'The operation exceeded its configured result bound; no partial result was returned.' : 'The host engine could not complete this operation.');
  }
}
