#include "memory/paddr.h"
#include "emulator/simulate.h"
#include "device/mmio.h"

void mem_read_trace(paddr_t addr, int len);
void mem_write_trace(paddr_t addr, int len, word_t data);

static word_t pmem_read(paddr_t addr, int len) {
  word_t ret = host_read(guest_to_host(addr), len);
  return ret;
}

static void pmem_write(paddr_t addr, int len, word_t data) {
  host_write(guest_to_host(addr), len, data);
}

static void out_of_bound(paddr_t addr) {
  panic("npc address = " FMT_PADDR " is out of bound of pmem [" FMT_PADDR ", " FMT_PADDR "] at pc = " FMT_WORD,
      addr, PMEM_LEFT, PMEM_RIGHT, core.pc);
}

word_t paddr_read(paddr_t addr, int len) {
  IFONE(CONFIG_MTRACE, mem_read_trace(addr, len));
  if (likely(in_pmem(addr))) {
    return pmem_read(addr, len);
  }
  IFONE(CONFIG_DEVICE, return mmio_read(addr, len));
  out_of_bound(addr);
  return 0;
}

void paddr_write(paddr_t addr, int len, word_t data) {
  IFONE(CONFIG_MTRACE, mem_write_trace(addr, len, data));
  if (likely(in_pmem(addr))) {
    pmem_write(addr, len, data); return;
  }
  IFONE(CONFIG_DEVICE, mmio_write(addr, len, data); return);
  out_of_bound(addr);
}