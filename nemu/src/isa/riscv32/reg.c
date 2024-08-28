/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include "local-include/reg.h"

/*
$0: just zero
ra: return address, just like x1
sp: stack pointer, like x2
gp: general pointer, x3
tp: thread pointer, x4
t0: temp reg / alter link reg
t1 - t2: temp reg 
s0: save reg / alter frame pointer 
s1: save reg
a0 - a1: func arg / func return val
a2 - a7: func arg
s2 - s11: save reg
t3 - t6: temp reg
*/

const char *regs[] = {
  "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
  "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
  "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
  "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

/*
* Output: regName  hexNum  decNum 
*/
void isa_reg_display() {
  // cpu.gpr[32]
  for (int i = 0; i < 32; i++) {
    printf("%-3s: 0x %04x %04x  (%u)\n", 
      regs[i], 
      (cpu.gpr[i] >> 16) & 0xFFFF, cpu.gpr[i] & 0xFFFF,
      cpu.gpr[i]
    );
  }
}

word_t isa_reg_str2val(const char *s, bool *success) {
  *success = false;

  if (s[0] == '$') {
    s++;
  }

  for (int i = 0; i < 32; i++) {
    if (strcmp(s, regs[i]) == 0) {
      *success = true;
      return cpu.gpr[i];
    }
  }

  if (strcmp(s, "pc") == 0) {
    *success = true;
    return cpu.pc;
  }

  return 0;
}
