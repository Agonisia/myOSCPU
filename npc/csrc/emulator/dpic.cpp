#include "emulator/dpic.h"
#include "memory/paddr.h"
#include "emulator/difftest.h"

void func_call_trace(vaddr_t addr_curr, vaddr_t addr_func);
void func_ret_trace(vaddr_t addr_curr);
void exception_trace();

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
extern "C" int mem_read(paddr_t addr) { // load type 
  vaddr_t addr_aligned = addr & ~0x3u;
  int offset = addr & 0x3u;

  if (addr == CONFIG_SERIAL_ADDR) { // serial
    IFONE(CONFIG_DIFFTEST, difftest_skip_ref());
    return getchar();
  }

  if (addr == CONFIG_RTC_ADDR) { // rtc
    IFONE(CONFIG_DIFFTEST, difftest_skip_ref());
    return (uint32_t)get_time();
  } else if (addr == CONFIG_RTC_ADDR + 4) {
    IFONE(CONFIG_DIFFTEST, difftest_skip_ref());
    return (uint32_t)(get_time() >> 32);
  } 

  word_t data = vaddr_read(addr_aligned, 4);

  if (offset) { // if not aligned
    return (data >> (offset * 8)) & 0xFFFFFFFF; // get the byte based on the offset
  } else {
    return data;
  }
}

/*
  according to the mask, write 4 bytes data to address `addr & ~0x3u`
  each bit in `mask` represents a 1-byte mask in `data`
  e.g. `mask = 0x3` means only the lowest 2 bytes are written, rest of remains unchanged. 
*/
extern "C" void mem_write(paddr_t addr, uint8_t mask, word_t data) { // store type
  vaddr_t addr_aligned = addr & ~0x3u;
  int offset = addr & 0x3u;

  if (addr == CONFIG_SERIAL_ADDR) { // serial
    IFONE(CONFIG_DIFFTEST, difftest_skip_ref());
    putchar(data);
    return;
  } 

  word_t data_origin = paddr_read(addr_aligned, 4);
  word_t mask_shift = 0;
  word_t data_shift = 0;

  for (int i = 0; i < 4; i++) {
    if (mask & (1 << i)) { // if current byte is to be written 
      uint8_t byte = (data >> (i * 8)) & 0xFF; // extract the byte in new data
      mask_shift |= (0xFF << ((i + offset) % 4) * 8); // reconstruct mask based on the offset
      data_shift |= (byte << ((i + offset) % 4) * 8); // reconstruct data based on the offset
    }
  }

  word_t data_final = (data_origin & ~mask_shift) | (data_shift & mask_shift);
  vaddr_write(addr_aligned, 4, data_final);
}

extern "C" void exception_display() {
  IFONE(CONFIG_ETRACE, exception_trace());
}

extern "C" void gprfile_update(
  int gprfile_0, int gprfile_1, int gprfile_2, int gprfile_3,
  int gprfile_4, int gprfile_5, int gprfile_6, int gprfile_7,
  int gprfile_8, int gprfile_9, int gprfile_10, int gprfile_11,
  int gprfile_12, int gprfile_13, int gprfile_14, int gprfile_15
) {
  int gprfile[16] = {
    gprfile_0, gprfile_1, gprfile_2, gprfile_3,
    gprfile_4, gprfile_5, gprfile_6, gprfile_7,
    gprfile_8, gprfile_9, gprfile_10, gprfile_11,
    gprfile_12, gprfile_13, gprfile_14, gprfile_15
  };

  for (int i = 0; i < 16; i++) {
    core.gpr[i] = gprfile[i];
  }
}

extern "C" void csrfile_update(
  int csrfile_mstatus, int csrfile_mtvec, int csrfile_mepc, int csrfile_mcause 
) {
  core.csr[mstatus] = csrfile_mstatus;
  core.csr[mtvec] = csrfile_mtvec;
  core.csr[mepc] = csrfile_mepc;
  core.csr[mcause] = csrfile_mcause;
}