// Applies the project icon to the unpacked application executable, which is the process whose
// taskbar and Alt-Tab icon people see. electron-builder's own resource editing is disabled
// (it would download a signing toolchain), so the rcedit binary already installed with the
// locked dependencies is used locally. No signing, certificate or network access is involved.
const {execFileSync} = require('node:child_process');
const fs = require('node:fs');
const path = require('node:path');
exports.default = async function applyProjectIcon(context) {
  const rcedit = path.join(__dirname, 'node_modules', 'electron-winstaller', 'vendor', 'rcedit.exe');
  const icon = path.join(__dirname, 'project-mark.ico');
  const exe = path.join(context.appOutDir, `${context.packager.appInfo.productFilename}.exe`);
  for (const file of [rcedit, icon, exe]) if (!fs.existsSync(file)) throw new Error(`Icon step input is missing: ${file}`);
  execFileSync(rcedit, [exe, '--set-icon', icon], {stdio: 'inherit'});
};