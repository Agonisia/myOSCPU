#include "memory/paddr.h"
#include "emulator/simulate.h"

static uint8_t pmem[CONFIG_MSIZE] PG_ALIGN = {};

static const uint32_t img_default [] = {
  0x00000013,  // 00, NOP
  0x01600093,  // 04, addi x1, x0, 22
  0x00b08093,  // 08, addi x1, x1, 11
  0x00100073,  // 0c, ebreak
  0x06308093,  // 10, addi x1, x1, 99
  // result in gpr[1] should be 33 (0x21)
};

uint8_t* guest_to_host(paddr_t paddr) { 
  return pmem + paddr - CONFIG_MBASE; 
}

paddr_t host_to_guest(uint8_t *haddr) { 
  return haddr - pmem + CONFIG_MBASE; 
}

void mem_init() {
  memset(pmem, rand(), CONFIG_MSIZE);
  Log("npc physical memory area [" FMT_PADDR ", " FMT_PADDR "]", PMEM_LEFT, PMEM_RIGHT);
  memcpy(guest_to_host(RESET_VECTOR), img_default, sizeof(img_default));
  core.pc = RESET_VECTOR;
}