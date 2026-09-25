//! Small, synchronous ABI. JS calls this only from a dedicated Worker.
use rvemu::{bus::DRAM_BASE, emulator::Emulator, exception::Trap};
use std::cell::RefCell;

#[derive(Default)]
struct Machine {
    emulator: Option<Emulator>,
    upload: Vec<u8>,
}

thread_local! {
    static MACHINE: RefCell<Machine> = RefCell::new(Machine::default());
}

#[no_mangle]
pub extern "C" fn upload_buffer(length: usize) -> *mut u8 {
    MACHINE.with(|m| {
        let mut m = m.borrow_mut();
        assert!(length <= 16 * 1024 * 1024);
        m.upload.resize(length, 0);
        m.upload.as_mut_ptr()
    })
}

#[no_mangle]
pub extern "C" fn load_kernel() {
    MACHINE.with(|m| {
        let mut m = m.borrow_mut();
        let mut emulator = Emulator::new();
        emulator.initialize_dram(std::mem::take(&mut m.upload));
        emulator.initialize_pc(DRAM_BASE);
        m.emulator = Some(emulator);
    });
}

#[no_mangle]
pub extern "C" fn load_disk() {
    MACHINE.with(|m| {
        let mut m = m.borrow_mut();
        let disk = std::mem::take(&mut m.upload);
        m.emulator.as_mut().unwrap().initialize_disk(disk);
    });
}

#[no_mangle]
pub extern "C" fn run_steps(count: u32) -> u32 {
    MACHINE.with(|m| {
        let mut m = m.borrow_mut();
        let cpu = &mut m.emulator.as_mut().unwrap().cpu;
        for _ in 0..count.min(500_000) {
            cpu.devices_increment();
            if let Some(interrupt) = cpu.check_pending_interrupt() {
                interrupt.take_trap(cpu);
            }
            if let Err(exception) = cpu.execute() {
                if matches!(exception.take_trap(cpu), Trap::Fatal) {
                    return 1;
                }
            }
        }
        0
    })
}

#[no_mangle]
pub extern "C" fn input_byte(byte: u8) {
    MACHINE.with(|m| {
        m.borrow_mut().emulator.as_mut().unwrap().cpu.bus.uart.input(byte);
    });
}

#[no_mangle]
pub extern "C" fn output_ptr() -> *const u8 {
    MACHINE.with(|m| m.borrow().emulator.as_ref().unwrap().cpu.bus.uart.output.as_ptr())
}

#[no_mangle]
pub extern "C" fn output_len() -> usize {
    MACHINE.with(|m| m.borrow().emulator.as_ref().unwrap().cpu.bus.uart.output.len())
}

#[no_mangle]
pub extern "C" fn clear_output() {
    MACHINE.with(|m| m.borrow_mut().emulator.as_mut().unwrap().cpu.bus.uart.output.clear());
}

#[no_mangle]
pub extern "C" fn program_counter() -> u64 {
    MACHINE.with(|m| m.borrow().emulator.as_ref().unwrap().cpu.pc)
}
