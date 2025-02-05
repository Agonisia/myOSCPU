#include "emulator/simulate.h"
#include "emulator/reg.h"
#include "emulator/dpic.h"

VCore* dut = nullptr;
VerilatedFstC* tfp = nullptr;

uint32_t sim_time = 0;
uint64_t guest_inst = 0;
CORE_state core = {};

extern "C" void disasm_init(const char *triple);
void difftest_skip_ref();
void difftest_step(vaddr_t pc);
void inst_trace(CORE_state core);
void print_ring_buffer();

static void statistic() {
  #define NUMBERIC_FMT MUXDEF(CONFIG_TARGET_AM, "%", "%'") PRIu64
  Log("total guest instructions = " NUMBERIC_FMT, guest_inst);
}

void assert_fail_msg() {
  statistic();
  reg_display();
  print_ring_buffer();
}

void single_step() {
  dut->clock ^= 1;
  dut->eval();
  tfp->dump(sim_time);
  sim_time++;
}

void one_cycle() {
  single_step();
  single_step();
}

void enable_update() { 
  dut->io_enable = 1;
  one_cycle();
  one_cycle();
  dut->io_enable = 0;    
}

void exec_once() {
  vaddr_t pc_curr = core.pc;
  enable_update();  
  IFONE(CONFIG_ITRACE, inst_trace(core)); // before pc update
  one_cycle();
  one_cycle();
  IFONE(CONFIG_DIFFTEST, difftest_step(pc_curr)); // after calcuation but need pc before update
}

void sim_exec(uint64_t n) {
  switch (sim_state.state) {
    case SIM_END:
    case SIM_ABORT:
      printf("Program execution has ended. To restart the program, exit and run again.\n");
      return;
    default: 
      sim_state.state = SIM_RUNNING;
  }

  for (;n > 0; n --) {
    exec_once();
    guest_inst++;
    if (sim_time > SIM_MAX_LIMIT ||
        (sim_state.state == SIM_END) || 
        (sim_state.state == SIM_ABORT)) {
      break;
    } 
  }

  switch (sim_state.state) {
    case SIM_RUNNING:
      // running to break;
      sim_state.state = SIM_STOP;
      break;
    case SIM_END:
    case SIM_ABORT:
      // abort: set red and say abort 
      // end1: halt_ret == 0 -> set green and say good trap
      // end2: halt_ret != 0 -> set red and say bad trap
      const char *trap_type;
      if (sim_state.state == SIM_ABORT) {
        trap_type = ANSI_FMT("ABORT", ANSI_FG_RED);
      } else if (sim_state.halt_ret == 0) {
        trap_type = ANSI_FMT("HIT GOOD TRAP", ANSI_FG_GREEN);
      } else {
        trap_type = ANSI_FMT("HIT BAD TRAP", ANSI_FG_RED);
      }
      Log("Rock Bottom: %s at PC = " FMT_WORD, trap_type, sim_state.halt_pc);
    case SIM_QUIT:
      // fall through
      statistic();
  }
  
}

void sim_reset() {
  dut->reset = 1;
  single_step();
  single_step();
  dut->reset = 0;
  single_step();
  single_step();
}

void sim_exit() {
  delete tfp;
  delete dut;
}

void sim_init(int argc, char** argv) {
  Verilated::commandArgs(argc, argv); 
  dut = new VCore;
  Verilated::traceEverOn(true);
  tfp = new VerilatedFstC;
  
  dut->trace(tfp, 99);
  tfp->open("sim_record.fst");

  dut->clock = 1;
  sim_reset();

  disasm_init("riscv32-pc-linux-gnu");
}
