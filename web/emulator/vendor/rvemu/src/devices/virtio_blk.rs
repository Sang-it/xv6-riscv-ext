//! Modern VirtIO-MMIO block device for xv6 (one split queue, read/write requests).
//! Local replacement for rvemu's legacy-only device; see vendor/rvemu/README.md.
use crate::{bus::VIRTIO_BASE, cpu::{Cpu, BYTE, HALFWORD, WORD, DOUBLEWORD}, exception::Exception};

pub const VIRTIO_IRQ: u64 = 1;
const QUEUE_SIZE: u32 = 8;

pub struct Virtio {
    disk: Vec<u8>,
    status: u32,
    features_sel: u32,
    driver_features_sel: u32,
    driver_features: [u32; 2],
    queue_sel: u32,
    queue_num: u32,
    ready: u32,
    desc: u64,
    avail: u64,
    used: u64,
    consumed: u16,
    interrupt_status: u32,
    notified: bool,
}

impl Virtio {
    pub fn new() -> Self {
        Self { disk: Vec::new(), status: 0, features_sel: 0, driver_features_sel: 0,
            driver_features: [0; 2], queue_sel: 0, queue_num: 0, ready: 0,
            desc: 0, avail: 0, used: 0, consumed: 0, interrupt_status: 0, notified: false }
    }

    pub fn initialize(&mut self, binary: Vec<u8>) { self.disk = binary; }

    pub fn is_interrupting(&mut self) -> bool {
        std::mem::take(&mut self.notified) && self.ready == 1 && self.status & 4 != 0
    }

    pub fn read(&self, address: u64, size: u8) -> Result<u64, Exception> {
        if size != WORD { return Err(Exception::LoadAccessFault); }
        Ok(match address - VIRTIO_BASE {
            0x00 => 0x74726976,
            0x04 => 2, // Modern MMIO, matching kernel/virtio_disk.c.
            0x08 => 2, // Block device.
            0x0c => 0x554d4551,
            0x10 => if self.features_sel == 1 { 1 } else { 0 }, // VIRTIO_F_VERSION_1
            0x34 => if self.queue_sel == 0 { QUEUE_SIZE as u64 } else { 0 },
            0x44 => self.ready as u64,
            0x60 => self.interrupt_status as u64,
            0x70 => self.status as u64,
            0xfc => 0, // Config generation.
            0x100 => (self.disk.len() as u64 / 512) & 0xffff_ffff,
            0x104 => (self.disk.len() as u64 / 512) >> 32,
            _ => 0,
        })
    }

    pub fn write(&mut self, address: u64, value: u32, size: u8) -> Result<(), Exception> {
        if size != WORD { return Err(Exception::StoreAMOAccessFault); }
        match address - VIRTIO_BASE {
            0x14 => self.features_sel = value,
            0x20 => if let Some(f) = self.driver_features.get_mut(self.driver_features_sel as usize) { *f = value; },
            0x24 => self.driver_features_sel = value,
            0x30 => self.queue_sel = value,
            0x38 => self.queue_num = value,
            0x44 => {
                if value == 1 && (self.queue_sel != 0 || self.queue_num == 0 || self.queue_num > QUEUE_SIZE) {
                    return Err(Exception::StoreAMOAccessFault);
                }
                self.ready = value & 1;
            }
            0x50 => self.notified = value == 0,
            0x64 => self.interrupt_status &= !value,
            0x70 => {
                self.status = value;
                if value == 0 {
                    let disk = std::mem::take(&mut self.disk);
                    *self = Self::new();
                    self.disk = disk;
                }
            }
            0x80 => self.desc = (self.desc & 0xffff_ffff_0000_0000) | value as u64,
            0x84 => self.desc = (self.desc & 0xffff_ffff) | ((value as u64) << 32),
            0x90 => self.avail = (self.avail & 0xffff_ffff_0000_0000) | value as u64,
            0x94 => self.avail = (self.avail & 0xffff_ffff) | ((value as u64) << 32),
            0xa0 => self.used = (self.used & 0xffff_ffff_0000_0000) | value as u64,
            0xa4 => self.used = (self.used & 0xffff_ffff) | ((value as u64) << 32),
            _ => {}
        }
        Ok(())
    }

    pub fn disk_access(cpu: &mut Cpu) -> Result<(), Exception> {
        let (desc, avail, used, size) = {
            let v = &cpu.bus.virtio;
            (v.desc, v.avail, v.used, v.queue_num as u64)
        };
        let available = cpu.bus.read(avail + 2, HALFWORD)? as u16;
        if available.wrapping_sub(cpu.bus.virtio.consumed) as u64 > size {
            return Err(Exception::LoadAccessFault);
        }
        while cpu.bus.virtio.consumed != available {
            let index = cpu.bus.virtio.consumed as u64 % size;
            let head = cpu.bus.read(avail + 4 + 2 * index, HALFWORD)?;
            let header = Descriptor::read(cpu, desc, head, size)?;
            let data = Descriptor::read(cpu, desc, header.next, size)?;
            let status = Descriptor::read(cpu, desc, data.next, size)?;
            if header.flags & 1 == 0 || data.flags & 1 == 0 || status.flags & 2 == 0
                || status.flags & 1 != 0 || header.len < 16 || status.len < 1 {
                return Err(Exception::LoadAccessFault);
            }
            let request = cpu.bus.read(header.addr, WORD)?;
            let sector = cpu.bus.read(header.addr + 8, DOUBLEWORD)?;
            let offset = sector.checked_mul(512).ok_or(Exception::LoadAccessFault)?;
            let end = offset.checked_add(data.len).ok_or(Exception::LoadAccessFault)?;
            let valid = end <= cpu.bus.virtio.disk.len() as u64
                && ((request == 0 && data.flags & 2 != 0) || (request == 1 && data.flags & 2 == 0));
            let mut written = 1;
            if valid {
                for i in 0..data.len {
                    if request == 0 {
                        let byte = cpu.bus.virtio.disk[(offset + i) as usize];
                        cpu.bus.write(data.addr + i, byte as u64, BYTE)?;
                    } else {
                        let byte = cpu.bus.read(data.addr + i, BYTE)?;
                        cpu.bus.virtio.disk[(offset + i) as usize] = byte as u8;
                    }
                }
                if request == 0 { written += data.len; }
            }
            cpu.bus.write(status.addr, if valid { 0 } else { 1 }, BYTE)?;
            cpu.bus.write(used + 4 + 8 * index, head, WORD)?;
            cpu.bus.write(used + 8 + 8 * index, written, WORD)?;
            cpu.bus.virtio.consumed = cpu.bus.virtio.consumed.wrapping_add(1);
            cpu.bus.write(used + 2, cpu.bus.virtio.consumed as u64, HALFWORD)?;
        }
        cpu.bus.virtio.interrupt_status |= 1;
        Ok(())
    }
}

struct Descriptor { addr: u64, len: u64, flags: u64, next: u64 }
impl Descriptor {
    fn read(cpu: &mut Cpu, table: u64, index: u64, size: u64) -> Result<Self, Exception> {
        if index >= size { return Err(Exception::LoadAccessFault); }
        let addr = table + index * 16;
        Ok(Self {
            addr: cpu.bus.read(addr, DOUBLEWORD)?,
            len: cpu.bus.read(addr + 8, WORD)?,
            flags: cpu.bus.read(addr + 12, HALFWORD)?,
            next: cpu.bus.read(addr + 14, HALFWORD)?,
        })
    }
}
