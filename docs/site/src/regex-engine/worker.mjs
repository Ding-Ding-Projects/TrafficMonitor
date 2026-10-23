import { evaluate } from './evaluate.mjs';

// A module Worker owns all pattern compilation and execution. Termination is
// controlled by the client even when the native engine cannot yield cooperatively.
self.addEventListener('message', event => {
  const message = event.data;
  if (!message || !Number.isSafeInteger(message.id)) return;
  const response = evaluate(message.request, message.limits);
  self.postMessage({ id: message.id, response });
});
