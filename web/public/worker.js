let vm;
let running = false;
let timer;
const decoder = new TextDecoder();

function fail(error) {
  running = false;
  clearTimeout(timer);
  self.postMessage({ type: 'error', message: error.message || String(error) });
}

function drain() {
  const length = vm.output_len();
  if (length) {
    const text = decoder.decode(new Uint8Array(vm.memory.buffer, vm.output_ptr(), length), { stream: true });
    vm.clear_output();
    self.postMessage({ type: 'output', text });
  }
}

function tick() {
  if (!running) return;
  try {
    const started = performance.now();
    do {
      if (vm.run_steps(25_000)) throw new Error(`CPU stopped at 0x${vm.program_counter().toString(16)}`);
    } while (performance.now() - started < 12);
    drain();
    timer = setTimeout(tick, 0);
  } catch (error) { fail(error); }
}

async function asset(name) {
  const response = await fetch(new URL(`assets/${name}`, self.location.href));
  if (!response.ok) throw new Error(`Could not load ${name} (HTTP ${response.status})`);
  return response.arrayBuffer();
}

self.onmessage = async ({ data }) => {
  try {
    if (data.type === 'boot') {
      const [wasm, kernel, disk] = await Promise.all([asset('xv6.wasm'), asset('kernel.bin'), asset('fs.img')]);
      const { instance } = await WebAssembly.instantiate(wasm);
      vm = instance.exports;
      for (const [buffer, load] of [[kernel, 'load_kernel'], [disk, 'load_disk']]) {
        const pointer = vm.upload_buffer(buffer.byteLength);
        new Uint8Array(vm.memory.buffer, pointer, buffer.byteLength).set(new Uint8Array(buffer));
        vm[load]();
      }
      self.postMessage({ type: 'loaded' });
      running = true;
      tick();
    } else if (data.type === 'input' && vm) {
      for (const byte of new TextEncoder().encode(data.text)) vm.input_byte(byte);
    } else if (data.type === 'pause') {
      running = false;
      clearTimeout(timer);
    } else if (data.type === 'resume' && vm && !running) {
      running = true;
      tick();
    }
  } catch (error) { fail(error); }
};
