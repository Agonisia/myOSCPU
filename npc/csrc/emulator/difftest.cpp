#include "emulator/difftest.h"

void (*ref_difftest_memcpy)(paddr_t addr, void *buf, size_t n, bool direction) = NULL;
void (*ref_difftest_regcpy)(void *dut, bool direction) = NULL;
void (*ref_difftest_exec)(uint64_t n) = NULL;
void (*ref_difftest_raise_intr)(uint64_t NO) = NULL;

void assert_fail_msg();

static bool is_skip_ref = false;
enum { DIFFTEST_TO_DUT, DIFFTEST_TO_REF };
CORE_state ref;

void difftest_skip_ref() {
  is_skip_ref = true;
}

static void checkregs(CORE_state *ref, vaddr_t pc) {
  
  for (int i = 0; i < ARRLEN(core.gpr); i++) {
    if (core.gpr[i] != ref->gpr[i]) {
      Log("mismatch gpr[%d]: \
        DUT = " ANSI_FMT(FMT_WORD, ANSI_FG_RED) ", REF = " ANSI_FMT(FMT_WORD, ANSI_FG_RED), \
        i, core.gpr[i], ref->gpr[i] \
      );
      sim_state.state = SIM_ABORT;
      sim_state.halt_pc = core.pc;
      assert_fail_msg();
      return;
    }
  }

  for (int i = 0; i < csr_num; i++) {
    if (core.csr[i] != ref->csr[i]) {
      Log("mismatch csr[%d]: \
        DUT = " ANSI_FMT(FMT_WORD, ANSI_FG_RED) ", REF = " ANSI_FMT(FMT_WORD, ANSI_FG_RED), \
        i, core.csr[i], ref->csr[i] \
      );
      sim_state.state = SIM_ABORT;
      sim_state.halt_pc = core.pc;
      assert_fail_msg();
      return;
    }
  }
}

void difftest_step(vaddr_t pc) {
  // printf("pc in difftest_step = " FMT_WORD "\n", pc);
  // printf("core.pc in difftest_step = " FMT_WORD "\n", core.pc);
  // printf("ref.pc in difftes t_step = " FMT_WORD "\n", ref.pc);

  if (is_skip_ref) {
    // to skip the checking of an instruction, just copy the reg state to reference design
    ref_difftest_regcpy(&core, DIFFTEST_TO_REF);
    is_skip_ref = false;
    return;
  }

  ref_difftest_exec(1);
  ref_difftest_regcpy(&ref, DIFFTEST_TO_DUT);

  checkregs(&ref, pc);
}

void difftest_init(char *ref_so_file, long img_size, int port) {
  IFONE(CONFIG_DIFFTEST, 
    assert(ref_so_file != NULL);

    void *handle;
    handle = dlopen(ref_so_file, RTLD_LAZY);
    assert(handle);

    ref_difftest_memcpy = (void (*)(paddr_t, void*, size_t, bool))dlsym(handle, "difftest_memcpy");
    assert(ref_difftest_memcpy);

    ref_difftest_regcpy = (void (*)(void*, bool))dlsym(handle, "difftest_regcpy");
    assert(ref_difftest_regcpy);

    ref_difftest_exec = (void (*)(uint64_t))dlsym(handle, "difftest_exec");
    assert(ref_difftest_exec);

    ref_difftest_raise_intr = (void (*)(uint64_t))dlsym(handle, "difftest_raise_intr");
    assert(ref_difftest_raise_intr);

    void (*ref_difftest_init)(int) = (void (*)(int))dlsym(handle, "difftest_init");
    assert(ref_difftest_init);

    Log("The result of every instruction will be compared with %s. ", ref_so_file);

    ref_difftest_init(port);
    ref_difftest_memcpy(RESET_VECTOR, guest_to_host(RESET_VECTOR), img_size, DIFFTEST_TO_REF);
    ref_difftest_regcpy(&core, DIFFTEST_TO_REF);
  );
}
