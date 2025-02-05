#include "emulator/dpic.h"
#include "memory/paddr.h"

void func_call_trace(vaddr_t addr_curr, vaddr_t addr_func);
void func_ret_trace(vaddr_t addr_curr);

extern "C" void ebreak_exit() {
  printf("Simulation terminated by ebreak.\n");
  sim_state.state = SIM_END;
  sim_state.halt_pc = core.pc;
  sim_state.halt_ret = 0;
}

extern "C" void inst_invalid() {
  printf("Simulation terminated by invalid instuction.\n");
  printf("Instrcution at PC: 0x%08x is unsupported.\n", core.pc);
  sim_state.state = SIM_ABORT;
  sim_state.halt_pc = core.pc;
  sim_state.halt_ret = 1;
}

extern "C" int inst_fetch() {
  static vaddr_t addr_prev = 0;
  static int inst_prev = 0;

  if (core.pc == addr_prev) {
    return inst_prev; // prevent duplicated fetch
  }

  word_t inst = vaddr_read(core.pc, 4);
  addr_prev = core.pc;
  inst_prev = inst;
  return inst;
}

extern "C" void inst_display(word_t inst) {
  core.inst = inst;
}

extern "C" void pc_display(vaddr_t pc) {
  core.pc = pc;
}

extern "C" void func_check(int rdest, vaddr_t addr_curr, vaddr_t addr_jump, word_t data_rsrc1) {
  IFONE(CONFIG_FTRACE,
    static vaddr_t addr_prev = 0;
    
    if (addr_curr == addr_prev) {
      return; // prevent duplicated trace
    }

    if (rdest == 1) {
      func_call_trace(addr_curr, addr_jump);
    } else if (rdest == 0 && data_rsrc1 == core.gpr[1]) {
      func_ret_trace(addr_curr);
    }

    addr_prev = addr_curr;
  );
}

/* return 4 bytes read from address `addr & ~0x3u` */
extern "C" int mem_read(paddr_t addr) {
  printf("mem_read: 0x%08x\n", addr);
  vaddr_t addr_aligned = addr & ~0x3u;
  word_t data = paddr_read(addr_aligned, 4);
  return data; 
}

/*
  according to the mask, write 4 bytes data to address `addr & ~0x3u`
  each bit in `mask` represents a 1-byte mask in `data`
  e.g. `mask = 0x3` means only the lowest 2 bytes are written, rest of remains unchanged. 
*/
extern "C" void mem_write(paddr_t addr, uint8_t mask, word_t data) {
  vaddr_t addr_aligned = addr & ~0x3u;

  // update the data byte-by-byte
  for (int i = 0; i < 4; i++) {
    if (mask & (1 << i)) {
      uint8_t new_byte = (data >> (i * 8)) & 0xFF; // get the `i` byte of new data 
      data &= ~(0xFF << (i * 8)); // clear the `i` byte of old data 
      data |= (new_byte << (i * 8)); // write the `i` byte of new data 
    }
  }

  paddr_write(addr_aligned, 4, data);
}


extern "C" void regfile_update(
  int regfile_0, int regfile_1, int regfile_2, int regfile_3,
  int regfile_4, int regfile_5, int regfile_6, int regfile_7,
  int regfile_8, int regfile_9, int regfile_10, int regfile_11,
  int regfile_12, int regfile_13, int regfile_14, int regfile_15
) {
  int regfile[16] = {
    regfile_0, regfile_1, regfile_2, regfile_3,
    regfile_4, regfile_5, regfile_6, regfile_7,
    regfile_8, regfile_9, regfile_10, regfile_11,
    regfile_12, regfile_13, regfile_14, regfile_15
  };

  for (int i = 0; i < 16; i++) {
    core.gpr[i] = regfile[i];
  }
}