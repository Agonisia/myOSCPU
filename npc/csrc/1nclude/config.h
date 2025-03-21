#ifndef __CONFIG_H__  // or #pragma once
#define __CONFIG_H__

#define CONFIG_ISA "RV32-E"
#define CONFIG_ISA_32 1

// 8000_0000 to 87FF_FFFF, 128MB
#define CONFIG_MBASE 0x80000000
#define CONFIG_MSIZE 0x8000000
#define CONFIG_PC_RESET_OFFSET 0x0

// log
#define CONFIG_LOG 1

// difftest
#define CONFIG_DIFFTEST 1

// tracer
#define CONFIG_ITRACE 0
#define CONFIG_ITRACE_START 0
#define CONFIG_ITRACE_LIMIT 10000

#define CONFIG_MTRACE 0

#define CONFIG_FTRACE 0

#define CONFIG_DTRACE 0

#define CONFIG_ETRACE 1

// device 
#define CONFIG_DEVICE 1 
#define CONFIG_DEVICE_BASE 0xa0000000
#define CONFIG_MMIO_BASE   0xa0000000

#define CONFIG_HAS_SERIAL 1
#define CONFIG_SERIAL_ADDR (CONFIG_DEVICE_BASE + 0x00003f8)

#define CONFIG_HAS_RTC 1
#define CONFIG_RTC_ADDR (CONFIG_DEVICE_BASE + 0x0000048)

#define CONFIG_HAS_VGA 0
#define CONFIG_VGA_ADDR (CONFIG_DEVICE_BASE + 0x0000100)
#define CONFIG_FRAME_BUFFER_ADDR (CONFIG_DEVICE_BASE + 0x1000000)

#define CONFIG_VGA_SIZE_800x600 1

#endif // __CONFIG_H__