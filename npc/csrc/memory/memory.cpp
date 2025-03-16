#include "memory/paddr.h"
#include "emulator/simulate.h"

static uint8_t pmem[CONFIG_MSIZE] PG_ALIGN = {};

/* test set for srai
  0xaeb10637,  // 00: lui a2, 0xaeb10
  0x41f65693,  // 0C: srai a3, a2, 0x1f   
  0x00100073,  // 10: ebreak
*/

/* test set for csrrw and csrrs 
  0x00000013,  // 00, NOP
  0x01600093,  // 04, addi x1, x0, 22
  0x30009173,  // 08, csrrw x2, mstatus, x1
  0x3000a1f3,  // 0c, csrrs x3, mstatus, x1
  0x00100073,  // 0c, ebreak
  0x06308093,  // 10, addi x1, x1, 99
*/

/* test set for load 
  0x00000097,  // 00: auipc x1, 0
  0x01C08103,  // 04, lb   x2, 28(x1) 
  0x01C0C183,  // 08, lbu  x3, 28(x1)
  0x01C09203,  // 0C, lh   x4, 28(x1)
  0x01C0D283,  // 10, lhu  x5, 28(x1)
  0x01C0A303,  // 14, lw   x6, 28(x1)
  0x00100073,  // 18, ebreak
  0x123480FF,  // 1C, data for load
*/

/* test set for store 
  0x00000097,  // 00: auipc x1, 0
  // test for sb
  0x0FF00113,  // 04: addi x2, x0, 0xFF  --> x2 = 0x0000_00FF
  0x02208C23,  // 08: sb   x2, 56(x1)    --> store 0xFF to 0x38
  0x0380C183,  // 0C: lbu  x3, 56(x1)    --> x3 = 0x000000FF
  0x03808203,  // 10: lb   x4, 56(x1)    --> x4 = 0xFFFF_FFFF
  // test for sh
  0x000082b7,  // 14: lui  x5, 0x8       --> x5 = 0x8000_0000
  0x02509c23,  // 18: sh   x5, 56(x1)    --> store 0x8000 to 0x38
  0x0380d303,  // 1C: lhu  x6, 56(x1)    --> x6 = 0x0000_8000
  0x03809383,  // 20: lh   x7, 56(x1)    --> x7 = 0xFFFF_8000
  // test for sw
  0x12345437,  // 24: lui  x8, 0x12345   --> x8 = 0x1234_5000
  0x67840413,  // 28: addi x8, x8, 0x678 --> x8 = 0x1234_5678
  0x0280ac23,  // 2C: sw   x8, 56(x1)    --> store 0x1234_5678 to 0x38
  0x0380a483,  // 30: lw   x9, 56(x1)    --> x9 = 0x1234_5678
  // ebreak
  0x00100073,  // 34: ebreak
  // data for load
  0x00000000,  // 38: store target (dont need this actually)
*/

/* test set for branch 
  0x00100093,  // 00: addi x1, x0, 1   --> x1 = 1
  0x00200113,  // 04: addi x2, x0, 2   --> x2 = 2
  0xFFF00193,  // 08: addi x3, x0, -1  --> x3 = -1 (0xFFFF_FFFF)
  // beq
  0x00208463,  // 10: beq  x1, x2, 0x08  fail to jump
  0x00108463,  // 14: beq  x1, x1, 0x08  jump to 1C
  0x00000013,  // 18: nop
  // bne
  0x00109463,  // 1C: bne  x1, x1, 0x08  fail to jump
  0x00209463,  // 20: bne  x1, x2, 0x08  jump to 28
  0x00000013,  // 24: nop
  // blt
  0x00114463,  // 28: blt  x2, x1, 0x08  fail to jump
  0x0020C463,  // 2C: blt  x1, x2, 0x08  jump to 34
  0x00000013,  // 30: nop
  // bge
  0x0020D463,  // 34: bge  x1, x2, 0x08  fail to jump
  0x00115463,  // 38: bge  x2, x1, 0x08  jump to 40
  0x00000013,  // 3C: nop
  // bltu
  0x0021E463,  // 40: bltu x3, x2, 0x08  fail to jump
  0x00316463,  // 44: bltu x2, x3, 0x08  jump to 4C
  0x00000013,  // 48: nop
  // bgeu 
  0x00317463,  // 50: bgeu x2, x3, 0x08  fail to jump
  0x0021F463,  // 4C: bgeu x3, x2, 0x08  jump to 58
  0x00000013,  // 54: nop
  // ebreak 
  0x00100073,  // 58: ebreak
*/

/* test set for slti/sltiu 
  0x00100093,  // 00: addi x1, x0, 1    --> x1 = 1
  0xFFF00113,  // 04: addi x2, x0, -1   --> x2 = -1 (0xFFFFFFFF)
  0x00000193,  // 08: addi x3, x0, 0    --> x3 = 0
  0x00500213,  // 0C: addi x4, x0, 5    --> x4 = 5
  0xFFE00293,  // 10: addi x5, x0, -2   --> x5 = -2 (0xFFFFFFFE)
  // sltiu 
  0x0020B313,  // 14: sltiu x6, x1, 2       --> x6 = 1 (1 < 2)
  0x00013393,  // 18: sltiu x7, x2, 0       --> x7 = 0 (0xFFFFFFFF < 0)
  0xFFF13413,  // 1C: sltiu x8, x2, 0xFFF   --> x8 = 0 (0xFFFFFFFF < 0xFFF)
  // slti
  0xFFF02493,  // 20: slti  x9, x0, -1      --> x9 = 0 (0 > -1)
  0x00112593,  // 24: slti  x11, x2, 1      --> x11 = 1 (0xFFFFFFFF < 1)
  0xFFE0A613,  // 28: slti  x12, x1, -2     --> x12 = 0 (1 > -2)
  0xFFF2A693,  // 2C: slti  x13, x5, -1     --> x13 = 1 (-2 < -1)
  0x00522713,  // 2C: slti  x14, x4, 5      --> x14 = 0 (5 < 5)
  // ebreak 
  0x00100073,  // 34: ebreak
*/

/* tips1: ref in default might have diffrent memory action behaviour due to unaligned */
/* tips2: leave x10 untouched, nemu exits correctly by determining its value is 0.*/
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