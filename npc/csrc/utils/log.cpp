#include "constant.h"

extern uint64_t guest_inst;

bool log_file_enable = false;

FILE *log_fp = NULL;

void log_init(const char *log_file) {
  log_fp = stdout;
  if (log_file != NULL) {
    FILE *fp = fopen(log_file, "w");
    Assert(fp, "Can not open '%s'", log_file);
    log_fp = fp;
    log_file_enable = true;
  }
  Log("Log is written to %s", log_file ? log_file : "stdout");
}

bool log_enable() {
  return MUXDEF(CONFIG_LOG, (guest_inst >= CONFIG_ITRACE_START) &&
         (guest_inst <= CONFIG_ITRACE_LIMIT) && log_file_enable, false);
}
