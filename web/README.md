# xv6 in the browser

The repository's real RISC-V kernel and filesystem run in a WebAssembly build
of [rvemu](https://github.com/d0iasm/rvemu), with an xterm.js console. A dedicated
Web Worker keeps emulation off the UI thread. The finished site is entirely
static: no VM server, accounts, external CDN, or special isolation headers.

## Build and run

Prerequisites: Node.js 22+, Rust, a RISC-V GCC/binutils toolchain, Make, Perl,
and a host C compiler for `mkfs`.

macOS:

```sh
brew install riscv64-elf-gcc
rustup target add wasm32-unknown-unknown
cd web
npm ci
npm run build
npm start
```

Open http://127.0.0.1:4173. On Debian/Ubuntu, install `gcc-riscv64-unknown-elf`
and `binutils-riscv64-unknown-elf` instead. If needed, set
`TOOLPREFIX=riscv64-unknown-elf-` when running the build. QEMU is not required.
The first Rust installation is available from https://rustup.rs/.

The build uses the ordinary `kernel/kernel` and `fs.img` targets, including
all existing utilities and `hello.c`. It converts the ELF kernel into a flat
binary for rvemu, builds the local Rust emulator, and copies the terminal
dependencies and licenses into `web/dist`. Kernel and user code are unchanged.
`EXTRA_CFLAGS` allows suppressing GCC 16's unused test-counter warning without
disabling other compiler errors.

## Try it

- `ps`: custom `getprocs` syscall and process inspection.
- `mkdir demo`, `cd demo`, `pwd`: custom `getcwd` syscall.
- `tree`: filesystem traversal.
- `c4 hello.c`: compile and interpret the included C example; factorial is 120.
- `echo hello > note`, `cat note`: real filesystem writes and reads.

This is xv6, not Linux. It has one emulated CPU and 128 MiB of RAM. Each tab
gets its own filesystem in memory. Reload discards edits. Uptime follows emulated execution, not wall-clock time.
Boot time varies with device performance. The demo is a teaching/portfolio
environment, not a production VM or a full RISC-V conformance implementation.

## Tests

```sh
npm test
npm run test:emulator
npx playwright install chromium
npm run test:browser
```

The Wasm smoke test boots the current image and checks processes, nested paths,
disk writes/reads, c4 execution, and the timer. Browser tests exercise the Worker,
full-window console, keyboard input, reload, mobile layout, and missing
asset recovery. Screenshots are written to `web/test-results`.

## Hosting

Upload **the contents of `web/dist`** to any static web host, or mount them at
a subdirectory such as `/xv6/` on your website. All asset URLs are relative.
Serve over HTTP(S), not `file://`. Serve `.wasm` as `application/wasm` when possible.

The GitHub Actions workflow builds and tests on push/PR. Successful pushes to
`riscv` deploy automatically to GitHub Pages; pull requests only build and test.
The **Browser demo** workflow can also be run manually on `riscv` to redeploy.
Pages must use **Settings → Pages → Source → GitHub Actions**. The local build
script only produces static files and does not publish anything.

## Implementation and attribution

MIT xv6 and Sangit Manandhar's kernel extensions remain the guest OS. c4 is a
port of Robert Swierczek's compiler/interpreter. rvemu supplies CPU execution
and memory translation; xterm.js supplies terminal rendering.

The vendored emulator is pinned to a source commit and includes its MIT license.
Local changes implement the modern VirtIO disk interface, Sstc timer, buffered
UART, and idle wake-up behavior required by this xv6 revision. See
[`emulator/vendor/rvemu/README.md`](emulator/vendor/rvemu/README.md) for details.
`dist/NOTICE.txt` accompanies the distributable site.
