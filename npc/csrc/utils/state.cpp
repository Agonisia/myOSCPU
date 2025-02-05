#include "utils.h"

SIM_state sim_state = {.state = SIM_STOP, .halt_ret = 1};

int is_exit_status_bad() {
  // 2 condition: run naturally and hit good map, or someone say quit 
  int good = (sim_state.state == SIM_END && sim_state.halt_ret == 0) ||
    (sim_state.state == SIM_QUIT && sim_state.halt_ret == 0);
  return !good; // good should be 1
}