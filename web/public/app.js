const terminal = new Terminal({
  cursorBlink: true,
  convertEol: true,
  fontSize: matchMedia('(max-width: 620px)').matches ? 12 : 14,
  fontFamily: 'ui-monospace, SFMono-Regular, Menlo, Consolas, monospace',
  scrollback: 3000,
  screenReaderMode: true,
  theme: { background: '#090d0b', foreground: '#dce5d7', cursor: '#c3e88a', selectionBackground: '#38523b' },
});
const fit = new FitAddon.FitAddon();
terminal.loadAddon(fit);
terminal.open(document.getElementById('terminal'));
new ResizeObserver(() => fit.fit()).observe(document.getElementById('terminal'));
const container = document.getElementById('terminal');
let worker, ready = false, bootOutput = '', bootTimeout;

function failure(message) {
  clearTimeout(bootTimeout);
  worker?.terminate();
  ready = false;
  container.setAttribute('aria-busy', 'false');
  terminal.writeln(`\r\n${message}\r\nReload the page to restart.`);
}

function boot() {
  terminal.writeln('Loading xv6…');
  try {
    worker = new Worker(new URL('./worker.js', location.href), { type: 'module' });
    worker.onerror = (event) => failure(event.message || 'Browser worker failed.');
    worker.onmessage = ({ data }) => {
      if (data.type === 'output') {
        terminal.write(data.text);
        if (!ready) {
          bootOutput = (bootOutput + data.text).slice(-4096);
          if (bootOutput.includes('$ ')) {
            clearTimeout(bootTimeout);
            ready = true;
            container.setAttribute('aria-busy', 'false');
            terminal.focus();
          }
        }
      } else if (data.type === 'error') failure(data.message);
    };
    worker.postMessage({ type: 'boot' });
    bootTimeout = setTimeout(() => failure('Boot took too long.'), 120_000);
  } catch (error) { failure(error.message); }
}

terminal.onData(text => {
  if (ready) worker.postMessage({ type: 'input', text });
});
boot();
