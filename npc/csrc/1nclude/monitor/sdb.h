#ifndef __MONITOR_SDB_H
#define __MONITOR_SDB_H

#include "constant.h"
#include "emulator/simulate.h"
#include "emulator/reg.h"

void sdb_init();
void sdb_mainloop();

word_t expr(char *e, bool *success);

#endif //__MONITOR_SDB_H
