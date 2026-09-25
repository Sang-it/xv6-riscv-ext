# Vendored rvemu core

Upstream: https://github.com/d0iasm/rvemu
Commit: `f55eb5b376f22a73c0cf2630848c03f8d5c93922`
Author: Asami Doi. MIT license preserved in LICENSE.

This is a source copy, not a new emulator authored from scratch. It is scoped to
the bundled xv6 image on one RV64 hart, not a fully compliant RISC-V machine.

Local changes for current xv6:

- Replace DOM/stdin UART implementations with bounded input and buffered output,
  including transmit interrupts needed by xv6's blocking UART writes.
- Replace the legacy VirtIO block device with the modern MMIO split-queue
  interface used by this kernel. Support disk reads/writes and ring wraparound.
- Add Sstc supervisor timer comparison through `stimecmp` and `menvcfg.STCE`.
- Wake WFI for a locally enabled pending timer even with global interrupts
  disabled, matching current xv6's idle scheduler.
- Fetch the halves of a page-straddling 32-bit instruction through separate
  address translations. The compiled c4 program exercises this case. Program
  counter increments use wrapping arithmetic in debug and release builds.
- Use 128 MiB RAM to match xv6; supply an empty ROM because xv6 starts directly
  at `0x80000000` and does not use a device tree.
- Remove browser-binding dependencies from Cargo.toml. The parent crate exposes
  a small Worker-friendly ABI without nightly generators or a DOM dependency.

CPU instruction execution, address translation, and the remaining device logic
come from rvemu. Retain this attribution when publishing the demo.
