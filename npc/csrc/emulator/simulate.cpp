#include "emulator/simulate.h"
#include "emulator/reg.h"
#include "emulator/dpic.h"

VCore* dut = nullptr;
VerilatedFstC* tfp = nullptr;

CORE_state core = {};
uint64_t sim_count = 0;
uint64_t guest_inst = 0;
static uint64_t sim_time = 0; // unit: us

extern "C" void disasm_init(const char *triple);
void device_update();
void difftest_step(vaddr_t pc);
void inst_trace(CORE_state core);
void print_ring_buffer();

static void statistic() {
  #define NUMBERIC_FMT MUXDEF(CONFIG_TARGET_AM, "%", "%'") PRIu64
  Log("host time spent = " NUMBERIC_FMT " us", sim_time);
  Log("total guest instructions = " NUMBERIC_FMT, guest_inst);
  if (sim_time > 0) {
    Log("simulation frequency = " NUMBERIC_FMT " inst/s", guest_inst * 1000000 / sim_time);
  } else {
    Log("Finish running in less than 1 us and can not calculate the simulation frequency");
  }
}

void assert_fail_msg() {
  statistic();
  reg_display();
  print_ring_buffer();
}

void single_step() {
  dut->clock ^= 1;
  dut->eval();
  tfp->dump(sim_count);
  sim_count++;
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

  uint64_t timer_start = get_time();

  for (;n > 0; n --) {
    exec_once();
    guest_inst++;
    if (sim_state.state != SIM_RUNNING) {
      break;
    }
    IFONE(CONFIG_DEVICE, device_update()); 
  }

  uint64_t timer_end = get_time();
  sim_time += timer_end - timer_start;

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
