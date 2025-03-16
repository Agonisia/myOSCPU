#include "monitor/tracer.h"

void exception_trace() {
  printf("\necall detected, have fun\n");
  printf("csr[mstatus] = 0x%x\n", core.csr[mstatus]);
  printf("csr[mtvec] = 0x%x\n", core.csr[mtvec]);
  printf("csr[mepc] = 0x%x\n", core.csr[mepc]);
  printf("csr[mcause] = 0x%x\n", core.csr[mcause]);
}
