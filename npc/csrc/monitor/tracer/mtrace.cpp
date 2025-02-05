#include "monitor/tracer.h"

void mem_read_trace(paddr_t addr, int len) {
  printf("Read from physical memory: addr = " FMT_PADDR ", len = %d\n", addr, len);
}

void mem_write_trace(paddr_t addr, int len, word_t data) {
  printf("Write to physical memory: addr = " FMT_PADDR ", len = %d, data = " FMT_WORD "\n", addr, len, data);
}