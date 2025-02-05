#ifndef __CONFIG_H__  // or #pragma once
#define __CONFIG_H__

#define CONFIG_ISA "riscv32-e"
#define CONFIG_ISA_32 1
#define CONFIG_DEVICE 0

// 8000_0000 to 87FF_FFFF, 128MB
#define CONFIG_MBASE 0x80000000
#define CONFIG_MSIZE 0x8000000
#define CONFIG_PC_RESET_OFFSET 0x0

// log
#define CONFIG_LOG 1

// difftest
#define CONFIG_DIFFTEST 1

// tracer
#define CONFIG_ITRACE 1
#define CONFIG_ITRACE_START 0
#define CONFIG_ITRACE_LIMIT 10000

#define CONFIG_MTRACE 1

#define CONFIG_FTRACE 0

#endif // __CONFIG_H__