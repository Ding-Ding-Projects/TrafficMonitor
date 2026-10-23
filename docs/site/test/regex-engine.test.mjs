import test from 'node:test';
import assert from 'node:assert/strict';
import { Worker, isMainThread, parentPort, workerData } from 'node:worker_threads';
import { createRegexEngine } from '../src/regex-engine/client.mjs';

if (!isMainThread && workerData?.role === 'regex-engine-test-adapter') {
  const { evaluate } = await import('../src/regex-engine/evaluate.mjs');
  parentPort.on('message', message => {
    parentPort.postMessage({ type: 'test-entered' });
    parentPort.postMessage({ id: message.id, response: evaluate(message.request, message.limits) });
  });
} else {
  function setup(options = {}) {
    const live = new Set(); let enteredResolve;
    const entered = new Promise(resolve => { enteredResolve = resolve; });
    const factory = () => {
      const worker = new Worker(new URL(import.meta.url), { workerData: { role: 'regex-engine-test-adapter' } });
      live.add(worker);
      const bindings = new Map();
      return {
        addEventListener(type, listener) {
          const wrapped = value => {
            if (type === 'message' && value?.type === 'test-entered') { enteredResolve(); return; }
            listener(type === 'message' ? { data: value } : value);
          };
          bindings.set(listener, wrapped); worker.on(type, wrapped);
        },
        removeEventListener(type, listener) { const wrapped = bindings.get(listener); if (wrapped) worker.off(type, wrapped); },
        postMessage(value) { worker.postMessage(value); },
        terminate() { live.delete(worker); return worker.terminate(); }
      };
    };
    const engine = createRegexEngine({ workerUrl: new URL('../src/regex-engine/worker.mjs', import.meta.url),
      timeoutMs: 1500, workerFactory: factory, ...options });
    return { engine, entered, live };
  }
  async function run(request, options) {
    const { engine } = setup(options);
    try { return await engine.execute(request); } finally { engine.dispose(); }
  }
  test('match preserves positional, optional, named captures and native indices', async () => {
    const result = await run({ operation: 'match', pattern: '(?<letter>a)(b)?', flags: 'dg', subject: 'a ab' });
    assert.equal(result.ok, true);
    assert.deepEqual(result.result.matches.map(m => [m.index, m.end, m.captures]), [[0, 1, ['a', null]], [2, 4, ['a', 'b']]]);
    assert.deepEqual(result.result.matches[0].namedCaptures, { letter: 'a' });
    assert.deepEqual(result.result.matches[0].indices, [[0, 1], [0, 1], null]);
    assert.deepEqual(result.result.matches[0].namedIndices, { letter: [0, 1] });
    assert.equal(result.engine.runtime, 'node-worker-test-adapter');
  });
  test('zero-width Unicode iteration advances complete code points', async () => {
    const unicode = await run({ operation: 'match', pattern: '(?:)', flags: 'gu', subject: '😀x' });
    assert.deepEqual(unicode.result.matches.map(m => m.index), [0, 2, 3]);
    const units = await run({ operation: 'match', pattern: '(?:)', flags: 'g', subject: '😀x' });
    assert.deepEqual(units.result.matches.map(m => m.index), [0, 1, 2, 3]);
  });
  test('search honors sticky, multiline and no-match semantics', async () => {
    assert.equal((await run({ operation: 'search', pattern: 'b', flags: 'y', subject: 'ab' })).result.index, -1);
    assert.equal((await run({ operation: 'search', pattern: '^b', flags: 'm', subject: 'a\nb' })).result.index, 2);
  });
  test('replacement semantics match the native string substitution contract', async () => {
    const cases = [
      ['(?<first>a)(b)?', 'g', 'a ab', "[$$][$&][$`][$'][$1][$2][$01][$12][$<first>][$<missing>]"],
      ['(a)', '', 'aba', '$0-$00-$01-$10-$99-$$-$<none>'],
      ['(?:)', 'gu', '😀x', '•'],
      ['(a)(b)', 'g', 'abab', '$2$1'],
      ['x', 'g', 'abc', '$&']
    ];
    for (const [pattern, flags, subject, replacement] of cases) {
      const actual = await run({ operation: 'replace', pattern, flags, subject, replacement });
      assert.equal(actual.ok, true);
      assert.equal(actual.result.text, subject.replace(new RegExp(pattern, flags), replacement));
    }
  });
  test('split preserves captures, unmatched entries and Unicode zero-width rules', async () => {
    for (const [pattern, flags, subject] of [['(,)|(x)', '', 'a,b'], ['(?:)', 'u', '😀x'], ['', '', ''], ['x', '', ''], ['(?=b)', '', 'abc'], ['a*', '', 'baaac'], [',', 'y', 'a,b,c']]) {
      const actual = await run({ operation: 'split', pattern, flags, subject });
      assert.equal(actual.ok, true);
      assert.deepEqual(actual.result.parts, subject.split(new RegExp(pattern, flags)).map(x => x ?? null));
    }
  });
  test('syntax and flag errors are typed without partial results', async () => {
    for (const [request, code] of [[{ pattern: '(' }, 'SYNTAX_ERROR'], [{ flags: 'gg' }, 'INVALID_FLAGS'], [{ flags: 'z' }, 'INVALID_FLAGS'], [{ flags: 'uv' }, 'INVALID_FLAGS']]) {
      const result = await run({ operation: 'match', subject: 'a', ...request });
      assert.equal(result.ok, false); assert.equal(result.code, code); assert.equal('result' in result, false);
    }
  });
  test('unknown, mistyped and excessive inputs are rejected before worker creation', async () => {
    const { engine, live } = setup({ maxPatternBytes: 4, maxSubjectBytes: 8, maxReplacementBytes: 4 });
    for (const request of [null, [], { operation: 'match', subject: 1 }, { operation: 'match', extra: true },
      { operation: 'match', pattern: 'abcde' }, { operation: 'match', subject: '😀😀😀' },
      { operation: 'replace', replacement: 'abcde' }, { operation: 'match', maxMatches: 0 }]) {
      assert.equal((await engine.execute(request)).ok, false); assert.equal(live.size, 0);
    }
    engine.dispose();
  });
  test('match, capture and serialized output limits return no partial result', async () => {
    const requests = [
      [{ operation: 'match', pattern: '.', flags: 'g', subject: 'abcdef', maxMatches: 2 }, {}, 'MATCH_LIMIT'],
      [{ operation: 'match', pattern: '(a)(b)', subject: 'ab' }, { maxCapturesPerMatch: 1 }, 'CAPTURE_LIMIT'],
      [{ operation: 'replace', pattern: 'a', flags: 'g', subject: 'a'.repeat(100), replacement: "$`$'" }, { maxOutputBytes: 1500 }, 'OUTPUT_LIMIT'],
      [{ operation: 'match', pattern: '(.*)', subject: 'x'.repeat(2000) }, { maxOutputBytes: 1500 }, 'OUTPUT_LIMIT'],
      [{ operation: 'split', pattern: ',', subject: 'a,b,c,d', maxMatches: 1 }, {}, 'MATCH_LIMIT']
    ];
    for (const [request, options, code] of requests) {
      const result = await run(request, options);
      assert.equal(result.ok, false); assert.equal(result.code, code); assert.equal('result' in result, false);
    }
  });
  test('a catastrophic native expression is actually entered, terminated, then healthy work succeeds', async () => {
    const { engine, entered, live } = setup({ timeoutMs: 500 });
    const hanging = engine.execute({ operation: 'match', pattern: '(a+)+$', subject: 'a'.repeat(20000) + '!' });
    await entered;
    const result = await hanging;
    assert.equal(result.code, 'TIMEOUT'); assert.equal(live.size, 0);
    const healthy = await engine.execute({ operation: 'search', pattern: 'safe', subject: 'safe result' });
    assert.equal(healthy.ok, true); assert.equal(healthy.result.index, 0); engine.dispose();
  });
  test('cancel terminates actual work without poisoning the next request', async () => {
    const { engine, entered, live } = setup();
    const pending = engine.execute({ operation: 'match', pattern: '(a+)+$', subject: 'a'.repeat(20000) + '!' });
    await entered; engine.cancel();
    assert.equal((await pending).code, 'CANCELLED'); assert.equal(live.size, 0);
    assert.equal((await engine.execute({ operation: 'match', pattern: 'a', subject: 'a' })).ok, true);
    engine.dispose();
  });
  test('a newer request supersedes and terminates the former worker', async () => {
    const { engine, entered } = setup();
    const former = engine.execute({ operation: 'match', pattern: '(a+)+$', subject: 'a'.repeat(20000) + '!' });
    await entered;
    const newer = engine.execute({ operation: 'search', pattern: 'b', subject: 'abc' });
    assert.equal((await former).code, 'SUPERSEDED'); assert.equal((await newer).result.index, 1); engine.dispose();
  });
  test('disposal settles pending work and rejects future requests', async () => {
    const { engine, entered, live } = setup();
    const pending = engine.execute({ operation: 'match', pattern: '(a+)+$', subject: 'a'.repeat(20000) + '!' });
    await entered; engine.dispose();
    assert.equal((await pending).code, 'DISPOSED'); assert.equal(live.size, 0);
    assert.equal((await engine.execute({ operation: 'capabilities' })).code, 'DISPOSED');
  });
  test('capabilities expose factual engine identity and unsupported feature boundaries', async () => {
    const result = await run({ operation: 'capabilities' });
    assert.equal(result.ok, true); assert.equal(result.engine.name, 'ECMAScript RegExp');
    assert.equal(result.engine.engineVersion, null); assert.equal(result.engine.features.debuggerTrace, false);
    assert.equal(result.engine.features.otherEngineExecution, false);
    assert.equal(typeof result.engine.flags.d, 'boolean');
  });
  test('browser worker URL boundary rejects external, data, blob and credentialed targets', () => {
    const old = Object.getOwnPropertyDescriptor(globalThis, 'location');
    Object.defineProperty(globalThis, 'location', { value: { href: 'https://example.test/docs/', origin: 'https://example.test' }, configurable: true });
    try {
      for (const workerUrl of ['https://other.test/worker.mjs', 'data:text/javascript,0', 'blob:https://example.test/123', 'https://user:pass@example.test/a', 'https://example.test/a#x'])
        assert.throws(() => createRegexEngine({ workerUrl }));
      const engine = createRegexEngine({ workerUrl: '/docs/local-worker.mjs' }); engine.dispose();
    } finally { if (old) Object.defineProperty(globalThis, 'location', old); else delete globalThis.location; }
  });
  test('malformed worker adapters settle with an explicit unavailable result', async () => {
    const engine = createRegexEngine({ workerUrl: new URL('../src/regex-engine/worker.mjs', import.meta.url), workerFactory: () => ({}) });
    assert.equal((await engine.execute({ operation: 'search' })).code, 'WORKER_UNAVAILABLE');
    engine.dispose();
  });

}
