#ifndef __TYPES_H__
#define __TYPES_H__

#include <stdint.h>

#include "config.h"
#include "macro.h"

#if CONFIG_MBASE + CONFIG_MSIZE > 0x100000000ul
#define PMEM64 1
#endif

typedef MUXDEF(CONFIG_ISA_32, uint32_t, uint64_t) word_t;
typedef MUXDEF(CONFIG_ISA_32, int32_t, int64_t)  sword_t;
#define FMT_WORD MUXDEF(CONFIG_ISA_32, "0x%08" PRIx32, "0x%016" PRIx64)

typedef word_t vaddr_t;
typedef MUXDEF(PMEM64, uint64_t, uint32_t) paddr_t;
#define FMT_PADDR MUXDEF(PMEM64, "0x%016" PRIx64, "0x%08" PRIx32)
typedef uint16_t ioaddr_t;

#endif //__TYPES_H__
