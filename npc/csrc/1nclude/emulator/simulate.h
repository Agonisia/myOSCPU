#ifndef __EMULATOR_SIMULATE_H__
#define __EMULATOR_SIMULATE_H__

#include <verilated.h>
#include <verilated_fst_c.h>
#include <VCore.h>

#include "constant.h"
#include "memory/vaddr.h"

#define SIM_MAX_LIMIT 10000

typedef enum {
  mstatus,
  mtvec,
  mepc,
  mcause, 
  csr_num
} csr_index;

typedef struct {
  word_t gpr[16]; // must be alligned with CPU_state in nemu, array must be same size
  vaddr_t pc;
  word_t csr[csr_num];
  word_t inst;
  IFDEF(CONFIG_ITRACE, char logbuf[128]); // generate a buffer when itrace is on
} CORE_state;

extern VCore* dut;
extern VerilatedFstC* tfp;
extern CORE_state core;

void sim_init(int argc, char** argv);
void sim_exec(uint64_t n);
void sim_exit();

#endif //__EMULATOR_SIMULATE_H__