//! Buffered 16550 UART for a single-threaded Worker. No DOM or host I/O.
use crate::{bus::UART_BASE, cpu::BYTE, exception::Exception};
use std::collections::VecDeque;

pub const UART_IRQ: u64 = 10;

pub struct Uart {
    registers: [u8; 8],
    input: VecDeque<u8>,
    pub output: Vec<u8>,
    tx_pending: bool,
}

impl Uart {
    pub fn new() -> Self {
        Self { registers: [0; 8], input: VecDeque::new(), output: Vec::new(), tx_pending: false }
    }

    pub fn input(&mut self, byte: u8) {
        // Bound pasted input. The terminal deliberately doesn't enable bracketed paste.
        if self.input.len() < 16_384 { self.input.push_back(byte); }
    }

    pub fn is_interrupting(&mut self) -> bool {
        let pending = (!self.input.is_empty() && self.registers[1] & 1 != 0)
            || (self.tx_pending && self.registers[1] & 2 != 0);
        self.tx_pending = false;
        pending
    }

    pub fn read(&mut self, address: u64, size: u8) -> Result<u64, Exception> {
        if size != BYTE { return Err(Exception::LoadAccessFault); }
        let offset = (address - UART_BASE) as usize;
        Ok(match offset {
            0 if self.registers[3] & 0x80 == 0 => self.input.pop_front().unwrap_or(0) as u64,
            2 => { self.tx_pending = false; if self.input.is_empty() { 2 } else { 4 } },
            5 => 0x60 | u64::from(!self.input.is_empty()),
            _ => *self.registers.get(offset).ok_or(Exception::LoadAccessFault)? as u64,
        })
    }

    pub fn write(&mut self, address: u64, value: u8, size: u8) -> Result<(), Exception> {
        if size != BYTE { return Err(Exception::StoreAMOAccessFault); }
        let offset = (address - UART_BASE) as usize;
        if offset == 0 && self.registers[3] & 0x80 == 0 {
            self.output.push(value);
            self.tx_pending = true;
        } else {
            *self.registers.get_mut(offset).ok_or(Exception::StoreAMOAccessFault)? = value;
        }
        Ok(())
    }
}
