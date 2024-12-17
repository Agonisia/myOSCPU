#include <iostream>
#include <map>

#include <verilated.h>
#include <verilated_fst_c.h>

#include <VCore.h>

static VCore* dut = nullptr; 
static VerilatedFstC* tfp = nullptr;

bool sim_run = true;
uint32_t sim_time = 0;
std::map<uint32_t, uint32_t> memory;

void memory_init() {
  memory[0x80000000] = 0x00000013;  // NOP
  memory[0x80000004] = 0x01600093;  // addi x1, x0, 22
  memory[0x80000008] = 0x00b08093;  // addi x1, x1, 11
  memory[0x8000000c] = 0x00100073;  // ebreak
  memory[0x80000010] = 0x06308093;  // addi x1, x1, 99
}

uint32_t pmem_read(uint32_t addr) {
  if (memory.find(addr) != memory.end()) {
    uint32_t instruction = memory[addr];
    return instruction;
  } else {
    std::cerr << "Error: Memory read at invalid address 0x" 
              << std::hex << addr << std::endl;
    exit(EXIT_FAILURE);
  }
}

extern "C" void ebreak_exit() {
  std::cout << "Simulation terminated by ebreak." << std::endl;
  sim_run = false;
}

void sim_init(int argc, char** argv) {
  Verilated::commandArgs(argc, argv); 
  dut = new VCore;
  Verilated::traceEverOn(true);
  tfp = new VerilatedFstC;
  
  dut->trace(tfp, 99);
  tfp->open("sim_record.fst");

  dut->clock = 1;
}

void single_step() {
  dut->clock ^= 1;
  dut->eval();
  tfp->dump(sim_time);
  sim_time++;
}

void sim_reset() {
  dut->reset = 1;
  single_step();
  single_step();
  dut->reset = 0;
  single_step();
}

void sim_exit() {
  single_step();
  delete tfp;
  delete dut;
}

int main(int argc, char** argv) {
  printf("Hello, ysyx!\n");
  memory_init();
  sim_init(argc, argv);
  sim_reset();

  dut->io_testReadAddr = 0b00001;
  
  while(sim_run) {
    uint32_t inst_addr = dut->io_instAddr;
    uint32_t imem_data = pmem_read(inst_addr);
    std::cout << "PC: 0x" << std::hex << inst_addr << std::endl;
    dut->io_imemData = imem_data;
    
    dut->io_imemEnable = 1;
    single_step();
    single_step();
    dut->io_imemEnable = 0;

    std::cout << std::dec << "test read value: " << dut->io_testReadData << std::endl;

    dut->io_pcEnable = 1;
    single_step();
    single_step();
    dut->io_pcEnable = 0;
  }
  sim_exit();
  return 0;
}
