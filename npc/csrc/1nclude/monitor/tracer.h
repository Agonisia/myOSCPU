#ifndef __MONITOR_TRACER_H
#define __MONITOR_TRACER_H

#include <elf.h>

#include "constant.h"
#include "emulator/simulate.h"

extern "C" void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);

#endif //__MONITOR_TRACER_H