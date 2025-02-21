#include "monitor/tracer.h"
#include "device/map.h"

void device_read_trace(paddr_t addr, int len, IOMap *map) {
  printf("\nRead from [%8s]: addr = "FMT_PADDR", len = %d\n", map->name, addr, len);
}

void device_write_trace(paddr_t addr, int len, word_t data, IOMap *map) {
  printf("\nWrite to [%8s]: addr = "FMT_PADDR", len = %d, data = " FMT_WORD "\n", map->name, addr, len, data);
}