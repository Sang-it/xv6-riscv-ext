import { spawnSync } from 'node:child_process';
import { cpSync, mkdirSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import path from 'node:path';

const web = fileURLToPath(new URL('..', import.meta.url));
const root = path.resolve(web, '..');
function run(command, args, cwd = root) {
  const result = spawnSync(command, args, { cwd, stdio: 'inherit' });
  if (result.error || result.status !== 0) throw new Error(`${command} failed: ${result.error?.message || result.status}`);
}
const prefix = process.env.TOOLPREFIX || ['riscv64-elf-', 'riscv64-unknown-elf-', 'riscv64-linux-gnu-', 'riscv64-unknown-linux-gnu-']
  .find(prefix => spawnSync(`${prefix}gcc`, ['--version'], { stdio: 'ignore' }).status === 0);
if (!prefix) throw new Error('Install a RISC-V GCC toolchain; see web/README.md.');

run('make', ['-j8', `TOOLPREFIX=${prefix}`, 'EXTRA_CFLAGS=-Wno-unused-but-set-variable', 'kernel/kernel', 'fs.img']);
run('cargo', ['build', '--manifest-path', 'web/emulator/Cargo.toml', '--locked', '--release', '--target', 'wasm32-unknown-unknown']);
const dist = path.join(web, 'dist');
rmSync(dist, { recursive: true, force: true });
mkdirSync(path.join(dist, 'assets'), { recursive: true });
mkdirSync(path.join(dist, 'vendor'), { recursive: true });
cpSync(path.join(web, 'public'), dist, { recursive: true, filter: source => !source.includes(`${path.sep}assets`) });
run(`${prefix}objcopy`, ['-O', 'binary', 'kernel/kernel', path.join(dist, 'assets/kernel.bin')]);
cpSync(path.join(root, 'fs.img'), path.join(dist, 'assets/fs.img'));
cpSync(path.join(web, 'emulator/target/wasm32-unknown-unknown/release/xv6_web.wasm'), path.join(dist, 'assets/xv6.wasm'));
for (const [source, destination] of [
  ['@xterm/xterm/lib/xterm.js', 'xterm.js'],
  ['@xterm/xterm/css/xterm.css', 'xterm.css'],
  ['@xterm/addon-fit/lib/addon-fit.js', 'addon-fit.js'],
]) cpSync(path.join(web, 'node_modules', source), path.join(dist, 'vendor', destination));
const notices = [
  ['xv6 — MIT', path.join(root, 'LICENSE')],
  ['rvemu — Asami Doi (local modifications documented in web/emulator/vendor/rvemu/README.md)', path.join(web, 'emulator/vendor/rvemu/LICENSE')],
  ['xterm.js', path.join(web, 'node_modules/@xterm/xterm/LICENSE')],
  ['xterm fit addon', path.join(web, 'node_modules/@xterm/addon-fit/LICENSE')],
].map(([label, file]) => `${label}\n\n${readFileSync(file, 'utf8')}`).join('\n\n---\n\n');
writeFileSync(path.join(dist, 'NOTICE.txt'), `Includes a port of Robert Swierczek's c4 compiler (see user/c4.c for attribution).\n\n${notices}`);
writeFileSync(path.join(dist, '.nojekyll'), '');
console.log(`\nBrowser demo built: ${dist}\nRun npm start, or serve this directory on any static host.`);
