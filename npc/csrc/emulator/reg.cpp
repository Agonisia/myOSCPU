#include "emulator/reg.h"

/*
$0: just zero
ra: return address, x1
sp: stack pointer, x2
gp: general pointer, x3
tp: thread pointer, x4
t0: temp reg / alter link reg
t1-t2: temp reg 
s0: save reg / alter frame pointer 
s1: save reg
a0-a1: func arg / func return val
a2-a5: func arg
*/
const char *regs[] = {
  "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
  "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
};

const char *csrs[] = {
  "mstatus", "mtvec", "mepc", "mcause"
};

/*
* Output: regName hexNum decNum 
*/
void reg_display() {
  // core.gpr[16]
  printf("gpr:\n");
  for (int i = 0; i < ARRLEN(core.gpr); i++) {
    printf("  %-3s: 0x_%04x_%04x  (%u)\n", 
      regs[i], 
      (core.gpr[i] >> 16) & 0xFFFF, core.gpr[i] & 0xFFFF,
      core.gpr[i]);
  }

  // core.csr[4]
  printf("csr:\n");
  for (int i = 0; i < csr_num; i++) {
    printf("  %-7s: 0x_%04x_%04x  (%u)\n", 
      csrs[i],
      (core.csr[i] >> 16) & 0xFFFF, core.csr[i] & 0xFFFF,
      core.csr[i]);
  }
}

word_t reg_str2val(const char *s, bool *success) {
  *success = false;

  if (s[0] == '$') {
    s++;
  }

  for (int i = 0; i < ARRLEN(core.gpr); i++) {
    if (strcmp(s, regs[i]) == 0) {
      *success = true;
      return core.gpr[i];
    }
  }

  if (strcmp(s, "pc") == 0) {
    *success = true;
    return core.pc;
  }

  return 0;
}