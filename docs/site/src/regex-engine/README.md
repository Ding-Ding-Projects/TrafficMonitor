# Bounded local regular-expression execution

Import `createRegexEngine` from `client.mjs`. Supply a same-origin HTTP(S) `workerUrl` pointing to `worker.mjs`. Publish `worker.mjs`, `evaluate.mjs` and `request.mjs` in the same directory. Create one engine per independent owning search or workbench. User expressions are compiled and executed only in a fresh module Worker. The caller must retain its previous working view when a request fails.

```js
const engine = createRegexEngine({ workerUrl: new URL('./regex-engine/worker.mjs', import.meta.url) });
const response = await engine.execute({ operation: 'match', pattern: '(?<word>\\w+)', flags: 'dg', subject: 'one two' });
engine.cancel();
engine.dispose();
```

`execute` accepts one plain object with `operation`, `pattern`, `flags`, `subject`, `replacement` and optional `maxMatches`. Operations are `capabilities`, `match`, `search`, `replace` and `split`. All text fields default to empty strings. Unknown fields, types, flags, duplicated flags and incompatible `u`/`v` flags are rejected. Known flags unavailable in the actual engine produce `UNSUPPORTED_FLAG`. Syntax errors produce `SYNTAX_ERROR` without echoing user input.

Success is `{ok:true, operation, engine, elapsedMs, result}`. Match results contain `matches` and `count`. Each match has `text`, `index`, `end`, `captures`, `namedCaptures`, `indices` and `namedIndices`. Captures exclude the full match; indices include it. Unmatched captures and indices are `null`. Named records are objects or `null`. Native indices are returned only when `d` was requested. Positions use UTF-16 code units and end-exclusive ranges. Global zero-width iteration advances whole code points under `u` or `v`.

Search returns `{index,match}`, using `-1` and `null` for no match. Replace returns `{text,replacements}`, with native ECMAScript substitution semantics and bounded incremental construction. Split returns `{parts,separators}`, preserving unmatched captured separators as `null`. Limits reject the entire result rather than silently truncating it.

Failures are `{ok:false,code,message,details?}` with no partial result. Codes include `INVALID_REQUEST`, `INVALID_OPERATION`, `INPUT_LIMIT`, `INVALID_FLAGS`, `UNSUPPORTED_FLAG`, `SYNTAX_ERROR`, `MATCH_LIMIT`, `CAPTURE_LIMIT`, `OUTPUT_LIMIT`, `TIMEOUT`, `CANCELLED`, `SUPERSEDED`, `DISPOSED`, `WORKER_UNAVAILABLE`, `WORKER_ERROR`, `INVALID_WORKER_RESULT` and `EVALUATION_ERROR`. A new request terminates its predecessor, including when the new request is invalid. Cancellation and timeout terminate the Worker; a subsequent request starts a fresh one. Disposal settles pending work and prevents further execution.

Configuration defaults and hard ceilings are 250 ms (5000 ms ceiling), 16384 pattern bytes, 1048576 subject bytes, 65536 replacement bytes, 1000 matches, 2097152 serialized output bytes and 256 captures per match. Options are `timeoutMs`, `maxPatternBytes`, `maxSubjectBytes`, `maxReplacementBytes`, `maxMatches`, `maxOutputBytes` and `maxCapturesPerMatch`. Values must be positive integers within the hard ceilings. Input and output bounds count UTF-8 bytes; positions remain UTF-16. Very small output bounds may reject even a capability response. Timeout includes startup and depends on the browser scheduling its callback. Worker termination is not an operating-system memory quota.

The capabilities operation reports actual current-worker flag and feature probes, browser build identification when exposed, and explicit unknown engine/Unicode versions. No other engine, debugger trace, conversion, encoding support or complete RegexBuddy parity is claimed. The module performs no payload persistence, logging, evaluation of generated code or network requests. Its local module assets are loaded through the browser's ordinary same-origin module mechanism.

Run the focused local checks with `node --test docs/site/test/regex-engine.test.mjs`. The explicit Node Worker adapter proves operation semantics, bounds, actual catastrophic-expression termination, cancellation and recovery. It does not prove browser execution, a built interface or deployed interactions. Those checks remain pending. `workerFactory` and file URLs are supported only for this explicit local test adapter outside a browser.

The official online manual topic inventory and remaining functionality are recorded in `docs/features/search/regex-capabilities.json`. Every parity topic remains unfinished. Guided controls, state synchronization, explanation, persistence, accessibility, localization, deployment and browser evidence belong to the owning interface.
