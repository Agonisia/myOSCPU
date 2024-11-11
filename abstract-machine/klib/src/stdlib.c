#include <am.h>
#include <klib.h>
#include <klib-macros.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
static unsigned long int next = 1;
static bool is_init = false;
static char* addr;

int rand(void) {
  // RAND_MAX assumed to be 32767
  next = next * 1103515245 + 12345;
  return (unsigned int)(next/65536) % 32768;
}

void srand(unsigned int seed) {
  next = seed;
}

int abs(int x) {
  return (x < 0 ? -x : x);
}

int atoi(const char* nptr) {
  int x = 0;
  while (*nptr == ' ') { nptr ++; }
  while (*nptr >= '0' && *nptr <= '9') {
    x = x * 10 + *nptr - '0';
    nptr ++;
  }
  return x;
}

/*  The  malloc()  function allocates size bytes and returns a pointer to the allocated memory.  The memory is not initialized.
    If size is 0, then malloc() returns a unique pointer value that can later be successfully passed  to  free(). 

    The malloc(),functions return a pointer to the allocated memory, which is suitably
    aligned for any type that fits into the requested size or less.  On error, these functions return NULL and set errno.   At‐
    tempting  to allocate more than PTRDIFF_MAX bytes is considered an error, as an object that large could cause later pointer
    subtraction to overflow.  */
void *malloc(size_t size) {
  // On native, malloc() will be called during initializaion of C runtime.
  // Therefore do not call panic() here, else it will yield a dead recursion:
  //   panic() -> putchar() -> (glibc) -> malloc() -> panic()
  if (size == 0) {
    return NULL;
  }
  if(!is_init) {
  	addr = (void*)ROUNDUP(heap.start, 8);
	  is_init = true;
  }
  size  = (size_t)ROUNDUP(size, 8);
  char *head = addr;
  addr += size;
  assert((uintptr_t)heap.start <= (uintptr_t)addr && (uintptr_t)addr < (uintptr_t)heap.end);
  memset(head, 0, size);  
  return head;
}


/*  The  free() function frees the memory space pointed to by ptr, which must have been returned by a previous call to malloc()
    or related functions.  Otherwise, or if ptr has already been freed, undefined behavior occurs.  If ptr is NULL,  no  opera‐
    tion is performed. 

    The free() function returns no value, and preserves errno.  */
void free(void *ptr) {
}

#endif
