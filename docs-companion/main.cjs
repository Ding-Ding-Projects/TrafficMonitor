const {app, BrowserWindow, protocol, session, Menu} = require('electron');
const fs = require('node:fs/promises');
const path = require('node:path');
const {origin, localPath, isLocal} = require('./policy.cjs');
protocol.registerSchemesAsPrivileged([{scheme: 'traffic-docs', privileges: {standard: true, secure: true, supportFetchAPI: true, corsEnabled: true, stream: true}}]);
app.enableSandbox();
app.setName('TrafficMonitor Documentation');
const types = {'.html':'text/html', '.js':'text/javascript', '.mjs':'text/javascript', '.json':'application/json', '.css':'text/css', '.svg':'image/svg+xml', '.png':'image/png', '.jpg':'image/jpeg', '.jpeg':'image/jpeg', '.gif':'image/gif', '.webp':'image/webp', '.woff2':'font/woff2'};
app.whenReady().then(async () => {
  const reader = session.fromPartition('persist:trafficmonitor-documentation');
  reader.setPermissionRequestHandler((_contents, _permission, callback) => callback(false));
  reader.setPermissionCheckHandler(() => false);
  reader.webRequest.onBeforeRequest((details, callback) => callback({cancel: !isLocal(details.url) && !details.url.startsWith('blob:traffic-docs://reader/')}));
  reader.on('will-download', event => event.preventDefault());
  reader.protocol.handle('traffic-docs', async request => {
    try {
      if (request.method !== 'GET') return new Response('Method not allowed', {status:405});
      const file = localPath(request.url, path.join(__dirname, 'site'));
      return new Response(await fs.readFile(file), {headers: {'Content-Type': types[path.extname(file)] || 'application/octet-stream', 'X-Content-Type-Options':'nosniff', 'Content-Security-Policy':"default-src 'self'; script-src 'self'; worker-src 'self' blob:; style-src 'self' 'unsafe-inline'; img-src 'self' data:; font-src 'self'; connect-src 'self'; object-src 'none'; frame-src 'none'; base-uri 'self'; form-action 'none'"}});
    } catch { return new Response('Documentation page unavailable', {status:404}); }
  });
  Menu.setApplicationMenu(null);
  const window = new BrowserWindow({width:1180, height:820, minWidth:640, minHeight:480, show:false, title:'TrafficMonitor Documentation', webPreferences:{session:reader, sandbox:true, contextIsolation:true, nodeIntegration:false, nodeIntegrationInWorker:false, webSecurity:true, webviewTag:false, devTools:false}});
  window.webContents.setWindowOpenHandler(() => ({action:'deny'}));
  window.webContents.on('will-navigate', (event, url) => { if (!isLocal(url)) event.preventDefault(); });
  window.webContents.on('will-redirect', (event, url) => { if (!isLocal(url)) event.preventDefault(); });
  window.webContents.on('will-attach-webview', event => event.preventDefault());
  window.once('ready-to-show', () => window.show());
  await window.loadURL(`${origin}/TrafficMonitor/`);
}).catch(() => { app.exit(1); });
app.on('window-all-closed', () => app.quit());
