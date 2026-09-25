import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';

const assets = new URL('../dist/assets/', import.meta.url);
const { instance } = await WebAssembly.instantiate(await readFile(new URL('xv6.wasm', assets)));
const vm = instance.exports;
for (const [name, load] of [['kernel.bin', 'load_kernel'], ['fs.img', 'load_disk']]) {
  const data = await readFile(new URL(name, assets));
  const pointer = vm.upload_buffer(data.length);
  new Uint8Array(vm.memory.buffer, pointer, data.length).set(data);
  vm[load]();
}
let output = '';
function until(predicate, timeout = 90_000) {
  const deadline = Date.now() + timeout;
  while (Date.now() < deadline) {
    assert.equal(vm.run_steps(500_000), 0, 'Fatal CPU trap');
    const length = vm.output_len();
    if (length) {
      output += new TextDecoder().decode(new Uint8Array(vm.memory.buffer, vm.output_ptr(), length));
      vm.clear_output();
    }
    assert(!output.includes('panic:'), output);
    if (predicate(output)) return;
  }
  throw new Error(`Timed out at PC 0x${vm.program_counter().toString(16)}\n${output}`);
}
function command(text, expected) {
  output = '';
  for (const byte of new TextEncoder().encode(text + '\n')) vm.input_byte(byte);
  until(text => expected.test(text) && text.endsWith('$ '), 30_000);
  console.log(output.trimEnd());
}
const started = Date.now();
until(text => text.includes('init: starting sh') && text.endsWith('$ '));
console.log(`Booted real xv6 kernel in ${Date.now() - started} ms`);
command('ps', /PID\s+PPID\s+STATE[\s\S]*init[\s\S]*sh[\s\S]*ps/);
command('mkdir demo', /\$ /);
command('cd demo', /\$ /);
command('pwd', /\/demo/);
command('echo browser-ok > proof', /\$ /);
command('cat proof', /browser-ok/);
command('cd /', /\$ /);
command('c4 hello.c', /5! = 120[\s\S]*string: AAAA/);
command('uptime', /\d/);
console.log('PASS: boot, getprocs, getcwd, disk read/write, c4 recursion and allocation, timer.');
