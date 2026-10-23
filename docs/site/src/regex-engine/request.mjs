// Request validation is deliberately syntax-free: regular expressions run only
// inside the disposable worker, never on the caller's event loop.
export const DEFAULT_LIMITS = Object.freeze({ timeoutMs: 250, maxPatternBytes: 16384,
  maxSubjectBytes: 1048576, maxReplacementBytes: 65536, maxMatches: 1000,
  maxOutputBytes: 2097152, maxCapturesPerMatch: 256 });
const HARD_LIMITS = { timeoutMs: 5000, maxPatternBytes: 16384, maxSubjectBytes: 1048576,
  maxReplacementBytes: 65536, maxMatches: 1000, maxOutputBytes: 2097152, maxCapturesPerMatch: 256 };
const encoder = new TextEncoder();
export const byteLength = value => encoder.encode(value).byteLength;
export const failure = (code, message, details) => ({ ok: false, code, message,
  ...(details === undefined ? {} : { details }) });
export function resolveLimits(options = {}) {
  const limits = {};
  for (const [key, maximum] of Object.entries(HARD_LIMITS)) {
    const value = options[key] ?? DEFAULT_LIMITS[key];
    if (!Number.isSafeInteger(value) || value < 1 || value > maximum)
      throw new TypeError('Invalid bounded regex engine configuration: ' + key);
    limits[key] = value;
  }
  return Object.freeze(limits);
}
export function validateRequest(input, limits) {
  if (!input || typeof input !== 'object' || Array.isArray(input)
      || Object.getPrototypeOf(input) !== Object.prototype)
    return failure('INVALID_REQUEST', 'Expected one plain request object.');
  const allowed = ['operation', 'pattern', 'flags', 'subject', 'replacement', 'maxMatches'];
  if (Object.keys(input).some(key => !allowed.includes(key)))
    return failure('INVALID_REQUEST', 'Unknown request field.');
  if (!['match', 'search', 'replace', 'split', 'capabilities'].includes(input.operation))
    return failure('INVALID_OPERATION', 'Unknown regex operation.');
  const request = { operation: input.operation };
  for (const [key, limit] of [['pattern', limits.maxPatternBytes], ['flags', 8],
    ['subject', limits.maxSubjectBytes], ['replacement', limits.maxReplacementBytes]]) {
    const value = input[key] ?? '';
    if (typeof value !== 'string') return failure('INVALID_REQUEST', key + ' must be a string.');
    // UTF-16 length is a cheap preflight before allocating a UTF-8 byte view.
    if (value.length > limit || byteLength(value) > limit)
      return failure('INPUT_LIMIT', key + ' exceeds its UTF-8 byte limit.', { field: key, limit });
    request[key] = value;
  }
  const seen = new Set();
  for (const flag of request.flags) {
    if (!'dgimsuvy'.includes(flag)) return failure('INVALID_FLAGS', 'Unknown ECMAScript flag.', { flag });
    if (seen.has(flag)) return failure('INVALID_FLAGS', 'Duplicate ECMAScript flag.', { flag });
    seen.add(flag);
  }
  if (seen.has('u') && seen.has('v')) return failure('INVALID_FLAGS', 'Flags u and v cannot be combined.');
  request.maxMatches = input.maxMatches ?? limits.maxMatches;
  if (!Number.isSafeInteger(request.maxMatches) || request.maxMatches < 1 || request.maxMatches > limits.maxMatches)
    return failure('INVALID_REQUEST', 'maxMatches must be within the configured bound.');
  return { ok: true, request };
}
