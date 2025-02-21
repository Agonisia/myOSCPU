#ifndef __EMULATOR_DIFFTEST_H__
#define __EMULATOR_DIFFTEST_H__

#include <dlfcn.h>
#include "emulator/simulate.h"

void difftest_init(char *ref_so_file, long img_size, int port);
void difftest_skip_ref();

#endif //__EMULATOR_DIFFTEST_H__