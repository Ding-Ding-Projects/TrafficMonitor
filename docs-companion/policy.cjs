const path = require('node:path');
const origin = 'traffic-docs://reader';
function localPath(address, root) {
  const url = new URL(address);
  if (url.protocol !== 'traffic-docs:' || url.hostname !== 'reader' || url.port || url.username || url.password) throw Error('Invalid documentation origin');
  const name = decodeURIComponent(url.pathname);
  if (!name.startsWith('/TrafficMonitor/') || name.includes('\\') || name.includes('\0') || name.split('/').some(p => p === '..' || p === '.')) throw Error('Invalid documentation path');
  let relative = name.slice('/TrafficMonitor/'.length);
  if (!relative || relative.endsWith('/')) relative += 'index.html';
  const resolved = path.resolve(root, relative);
  if (!resolved.startsWith(path.resolve(root) + path.sep)) throw Error('Path escapes documentation bundle');
  return resolved;
}
function isLocal(address) { try { localPath(address, process.cwd()); return true; } catch { return false; } }
module.exports = { origin, localPath, isLocal };
