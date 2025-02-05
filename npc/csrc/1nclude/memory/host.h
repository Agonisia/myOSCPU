#ifndef __MEMORY_HOST_H__
#define __MEMORY_HOST_H__

#include "constant.h"

static inline word_t host_read(void *addr, int len) {
  switch (len) {
    case 1: return *(uint8_t  *)addr;
    case 2: return *(uint16_t *)addr;
    case 4: return *(uint32_t *)addr;
    IFDEF(CONFIG_ISA64, case 8: return *(uint64_t *)addr);
    default: MUXDEF(CONFIG_RT_CHECK, assert(0), return 0);
  }
}

static inline void host_write(void *addr, int len, word_t data) {
  switch (len) {
    case 1: *(uint8_t  *)addr = data; return;
    case 2: *(uint16_t *)addr = data; return;
    case 4: *(uint32_t *)addr = data; return;
    IFDEF(CONFIG_ISA64, case 8: *(uint64_t *)addr = data; return);
    IFDEF(CONFIG_RT_CHECK, default: assert(0));
  }
}

void mem_init();
/* convert the guest physical address in the guest program to host virtual address in emulator */
uint8_t* guest_to_host(paddr_t paddr);
/* convert the host virtual address in emulator to guest physical address in the guest program */
paddr_t host_to_guest(uint8_t *haddr);

#endif //__MEMORY_HOST_H__
