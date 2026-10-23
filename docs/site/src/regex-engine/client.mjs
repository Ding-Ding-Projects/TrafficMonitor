import { byteLength, failure, resolveLimits, validateRequest } from './request.mjs';

function localWorkerUrl(value, testFactory) {
  if (typeof value !== 'string' && !(value instanceof URL)) throw new TypeError('workerUrl must be a local URL.');
  if (typeof location === 'object') {
    const url = new URL(value, location.href);
    if (!['http:', 'https:'].includes(url.protocol) || url.origin !== location.origin || url.username || url.password || url.hash)
      throw new TypeError('workerUrl must be an uncredentialed same-origin HTTP(S) asset.');
    return url;
  }
  // Node is supported only through the explicit test adapter, never as a silent
  // browser-engine substitute or a route to an external script.
  const url = new URL(value);
  if (!testFactory || url.protocol !== 'file:' || url.host || url.hash || url.search)
    throw new TypeError('No browser Worker environment; a local file test adapter is required.');
  return url;
}
function responseIsBounded(response, request, limits) {
  if (!response || typeof response !== 'object' || Array.isArray(response) || typeof response.ok !== 'boolean') return false;
  if (byteLength(JSON.stringify(response)) > limits.maxOutputBytes) return false;
  if (!response.ok) return typeof response.code === 'string' && response.code.length <= 64
    && typeof response.message === 'string' && response.message.length <= 1024;
  if (response.operation !== request.operation || !Number.isFinite(response.elapsedMs) || response.elapsedMs < 0
      || !response.engine || response.engine.name !== 'ECMAScript RegExp' || !response.result) return false;
  const point = value => value === null || Array.isArray(value) && value.length === 2
    && value.every(n => Number.isSafeInteger(n) && n >= 0 && n <= request.subject.length) && value[0] <= value[1];
  const capture = value => value === null || typeof value === 'string';
  const record = value => value && typeof value.text === 'string'
    && Number.isSafeInteger(value.index) && Number.isSafeInteger(value.end)
    && value.index >= 0 && value.end >= value.index && value.end <= request.subject.length
    && value.text === request.subject.slice(value.index, value.end)
    && Array.isArray(value.captures) && value.captures.length <= limits.maxCapturesPerMatch && value.captures.every(capture)
    && (value.namedCaptures === null || typeof value.namedCaptures === 'object' && !Array.isArray(value.namedCaptures)
      && Object.values(value.namedCaptures).every(capture))
    && (value.indices === null || Array.isArray(value.indices) && value.indices.length === value.captures.length + 1 && value.indices.every(point))
    && (value.namedIndices === null || typeof value.namedIndices === 'object' && !Array.isArray(value.namedIndices)
      && Object.values(value.namedIndices).every(point));
  const result = response.result;
  if (request.operation === 'match') return Array.isArray(result.matches) && result.matches.length <= request.maxMatches
    && result.count === result.matches.length && result.matches.every(record);
  if (request.operation === 'search') return result.index === -1 && result.match === null
    || record(result.match) && result.index === result.match.index;
  if (request.operation === 'replace') return typeof result.text === 'string' && Number.isSafeInteger(result.replacements)
    && result.replacements >= 0 && result.replacements <= request.maxMatches;
  if (request.operation === 'split') return Array.isArray(result.parts)
    && result.parts.length <= request.maxMatches * (limits.maxCapturesPerMatch + 1) + 1 && result.parts.every(capture)
    && Number.isSafeInteger(result.separators) && result.separators >= 0 && result.separators <= request.maxMatches;
  return request.operation === 'capabilities' && typeof result.limits === 'object';
}

// One engine belongs to one search/workbench instance. No patterns, subjects,
// replacement text or results are persisted, logged or transmitted by this module.
export function createRegexEngine(options = {}) {
  if (!options || typeof options !== 'object' || Array.isArray(options)) throw new TypeError('Expected engine options.');
  const limits = resolveLimits(options);
  if (options.workerFactory !== undefined && typeof options.workerFactory !== 'function') throw new TypeError('Invalid workerFactory.');
  const workerUrl = localWorkerUrl(options.workerUrl, options.workerFactory);
  const factory = options.workerFactory ?? (url => new Worker(url, { type: 'module', name: 'bounded-regex' }));
  let active = null, disposed = false, sequence = 0;
  function settle(value) {
    const current = active;
    if (!current) return;
    active = null;
    clearTimeout(current.timer);
    try {
      current.worker.removeEventListener('message', current.message);
      current.worker.removeEventListener('error', current.error);
      current.worker.removeEventListener('messageerror', current.error);
    } catch { /* Cleanup cannot prevent the terminal result from settling. */ }
    try { current.worker.terminate(); } catch { /* State still settles without retaining the worker reference. */ }
    current.resolve(value);
  }
  function execute(input) {
    if (disposed) return Promise.resolve(failure('DISPOSED', 'The regex engine has been disposed.'));
    // New requests supersede even invalid input, so old results cannot arrive
    // after the user has replaced their query with a new invalid expression.
    if (active) settle(failure('SUPERSEDED', 'A newer request replaced this operation.'));
    let checked;
    try { checked = validateRequest(input, limits); }
    catch { return Promise.resolve(failure('INVALID_REQUEST', 'The request could not be read safely.')); }
    if (!checked.ok) return Promise.resolve(checked);
    const request = checked.request, id = ++sequence;
    return new Promise(resolve => {
      let worker;
      try {
        worker = factory(workerUrl);
        if (!worker || ['addEventListener', 'removeEventListener', 'postMessage', 'terminate']
          .some(method => typeof worker[method] !== 'function')) throw new TypeError('Invalid Worker interface.');
      }
      catch { resolve(failure('WORKER_UNAVAILABLE', 'The local module Worker could not be created.')); return; }
      const error = () => { if (active?.id === id) settle(failure('WORKER_ERROR', 'The isolated regex worker failed.')); };
      const message = event => {
        if (active?.id !== id || event.data?.id !== id) return;
        try {
          settle(responseIsBounded(event.data.response, request, limits) ? event.data.response
            : failure('INVALID_WORKER_RESULT', 'The worker returned an invalid or oversized result.'));
        } catch { settle(failure('INVALID_WORKER_RESULT', 'The worker returned an invalid result.')); }
      };
      active = { id, worker, resolve, message, error, timer: null };
      try {
        worker.addEventListener('message', message);
        worker.addEventListener('error', error);
        worker.addEventListener('messageerror', error);
        active.timer = setTimeout(() => {
          if (active?.id === id) settle(failure('TIMEOUT', 'The elapsed deadline terminated the worker; no partial result was returned.'));
        }, limits.timeoutMs);
        worker.postMessage({ id, request, limits });
      } catch { settle(failure('WORKER_UNAVAILABLE', 'The isolated worker could not receive this request.')); }
    });
  }
  return Object.freeze({ execute,
    cancel() { if (active) settle(failure('CANCELLED', 'The operation was cancelled without a partial result.')); },
    dispose() { disposed = true; if (active) settle(failure('DISPOSED', 'The regex engine has been disposed.')); },
    limits });
}
