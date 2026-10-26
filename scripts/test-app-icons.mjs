// Inspects the icon resources embedded in built Windows executables.
// Usage: node test-app-icons.mjs --ico <master.ico> [--reject-default <electron.exe>] --exe <label>=<path> [--exe ...]
// Fails when an executable's primary icon group is missing a required size, does not byte-match
// the project master icon at every required size, or matches the framework default icon.
import fs from 'node:fs';
import {createHash} from 'node:crypto';

const REQUIRED = [16, 24, 32, 48, 64, 256];
const sha = bytes => createHash('sha256').update(bytes).digest('hex');

function parseIco(buffer) {
  if (buffer.readUInt16LE(0) !== 0 || buffer.readUInt16LE(2) !== 1) throw new Error('Not an ICO file');
  const count = buffer.readUInt16LE(4), images = new Map();
  for (let i = 0; i < count; i++) {
    const o = 6 + i * 16, size = buffer[o] || 256, length = buffer.readUInt32LE(o + 8), offset = buffer.readUInt32LE(o + 12);
    images.set(size, buffer.subarray(offset, offset + length));
  }
  return images;
}

function readPeIcons(file) {
  const b = fs.readFileSync(file);
  if (b.toString('latin1', 0, 2) !== 'MZ') throw new Error(`${file}: not a PE file`);
  const pe = b.readUInt32LE(0x3c);
  if (b.toString('latin1', pe, pe + 4) !== 'PE\0\0') throw new Error(`${file}: bad PE signature`);
  const sectionCount = b.readUInt16LE(pe + 6), optionalSize = b.readUInt16LE(pe + 20), optional = pe + 24;
  const magic = b.readUInt16LE(optional), dirBase = optional + (magic === 0x20b ? 112 : 96);
  const resourceRva = b.readUInt32LE(dirBase + 2 * 8);
  if (!resourceRva) throw new Error(`${file}: no resource directory`);
  const sections = [];
  for (let i = 0; i < sectionCount; i++) {
    const s = optional + optionalSize + i * 40;
    sections.push({rva: b.readUInt32LE(s + 12), size: Math.max(b.readUInt32LE(s + 8), b.readUInt32LE(s + 16)), raw: b.readUInt32LE(s + 20)});
  }
  const rvaToOffset = rva => {
    const s = sections.find(x => rva >= x.rva && rva < x.rva + x.size);
    if (!s) throw new Error(`${file}: RVA 0x${rva.toString(16)} is outside every section`);
    return s.raw + (rva - s.rva);
  };
  const base = rvaToOffset(resourceRva);
  const entries = at => {
    const n = b.readUInt16LE(at + 12) + b.readUInt16LE(at + 14), out = [];
    for (let i = 0; i < n; i++) {
      const id = b.readUInt32LE(at + 16 + i * 8), target = b.readUInt32LE(at + 20 + i * 8);
      out.push({named: (id & 0x80000000) !== 0, id: id & 0x7fffffff, dir: (target & 0x80000000) !== 0, target: target & 0x7fffffff});
    }
    return out;
  };
  const leaf = entry => {
    let current = entry;
    while (current.dir) current = entries(base + current.target)[0];
    const data = base + current.target, rva = b.readUInt32LE(data), size = b.readUInt32LE(data + 4), off = rvaToOffset(rva);
    return b.subarray(off, off + size);
  };
  const types = entries(base);
  const groups = types.find(e => !e.named && e.id === 14), icons = types.find(e => !e.named && e.id === 3);
  if (!groups || !icons) throw new Error(`${file}: executable has no icon resources`);
  const iconById = new Map(entries(base + icons.target).map(e => [e.id, leaf(e)]));
  return entries(base + groups.target).map(group => {
    const data = leaf(group), count = data.readUInt16LE(4), images = new Map();
    for (let i = 0; i < count; i++) {
      const o = 6 + i * 14, size = data[o] || 256, id = data.readUInt16LE(o + 12);
      images.set(size, iconById.get(id));
    }
    return {name: group.named ? 'named' : group.id, images};
  });
}

const args = process.argv.slice(2), exes = [];
let masterPath, defaultPath;
for (let i = 0; i < args.length; i++) {
  if (args[i] === '--ico') masterPath = args[++i];
  else if (args[i] === '--reject-default') defaultPath = args[++i];
  else if (args[i] === '--exe') { const [label, ...rest] = args[++i].split('='); exes.push({label, path: rest.join('=')}); }
  else throw new Error(`Unknown argument ${args[i]}`);
}
if (!masterPath || !exes.length) { console.error('Usage: --ico <master.ico> [--reject-default <electron.exe>] --exe <label>=<path>'); process.exit(2); }

const master = parseIco(fs.readFileSync(masterPath));
const defaultIcon = defaultPath ? readPeIcons(defaultPath)[0].images : null;
const failures = [];
console.log(`Master ${masterPath}: sizes ${[...master.keys()].sort((a, b) => a - b).join(',')}`);
for (const size of REQUIRED) if (!master.get(size)) failures.push(`master icon lacks ${size}px`);
if (defaultIcon) console.log(`Framework default icon (${defaultPath}) 256px sha256 ${defaultIcon.get(256) ? sha(defaultIcon.get(256)) : 'n/a'}`);
for (const exe of exes) {
  let groups;
  try { groups = readPeIcons(exe.path); } catch (error) { failures.push(`${exe.label}: ${error.message}`); continue; }
  const primary = groups[0], sizes = [...primary.images.keys()].sort((a, b) => a - b);
  console.log(`${exe.label}: ${exe.path}`);
  console.log(`  primary icon group ${primary.name} of ${groups.length}; sizes ${sizes.join(',')}`);
  for (const size of sizes) console.log(`  ${size}px sha256 ${sha(primary.images.get(size))}`);
  for (const size of REQUIRED) {
    const image = primary.images.get(size);
    if (!image) { failures.push(`${exe.label}: primary icon lacks ${size}px`); continue; }
    if (master.get(size) && sha(image) !== sha(master.get(size))) failures.push(`${exe.label}: ${size}px image differs from the project master icon`);
    if (defaultIcon?.get(size) && sha(image) === sha(defaultIcon.get(size))) failures.push(`${exe.label}: ${size}px image is the framework default icon`);
  }
}
if (failures.length) { for (const f of failures) console.error(`FAIL ${f}`); process.exit(1); }
console.log('PASS packaged icons match the project master icon at every required size and are not the framework default.');
