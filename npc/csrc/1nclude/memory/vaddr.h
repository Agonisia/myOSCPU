#ifndef __MEMORY_VADDR_H__
#define __MEMORY_VADDR_H__

#include "constant.h"
#include "memory/host.h"
#include "memory/paddr.h"

word_t vaddr_inst_fetch(vaddr_t addr, int len);
word_t vaddr_read(vaddr_t addr, int len);
void vaddr_write(vaddr_t addr, int len, word_t data);

#define PAGE_SHIFT        12
#define PAGE_SIZE         (1ul << PAGE_SHIFT)
#define PAGE_MASK         (PAGE_SIZE - 1)

#endif //__MEMORY_VADDR_H__
