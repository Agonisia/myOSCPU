#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

/*  The strlen() function calculates the length of the string pointed to by s, excluding the terminating null byte ('\0').  */
size_t strlen(const char *s) {
  assert(s != NULL);
  size_t len = 0;
  while (s[len] != '\0') {
    len++;
  }

  return len;
}

/*  These  functions copy the string pointed to by src, into a string at the buffer pointed to by dst.  
    The programmer is responsible for allocating a destination buffer large enough, that is, strlen(src) + 1. */
char *strcpy(char *dst, const char *src) {
  assert(dst != NULL && src != NULL);
  size_t len = strlen(src);
  memmove(dst, src, len + 1);

  return dst;
}

/*  These  functions copy non-null bytes from the string pointed to by src into the array pointed to by dst.  
    If the source has too few non-null bytes to fill the destination, the functions pad the destination with trailing null bytes.  
    If the destination buffer, limited by its  size, isn't  large  enough to hold the copy, the resulting character sequence is truncated.  
    For the difference between the two functions, see RETURN VALUE.  */
char *strncpy(char *dst, const char *src, size_t n) {
  assert(dst != NULL && src != NULL);
  size_t len = strlen(src);
  if (len < n) {
    memmove(dst, src, len);
    memset(dst + len, '\0', n - len);
  } else {
    memmove(dst, src, n);
  }

  return dst;
}

/*  This  function  catenates  the  string pointed to by src, after the string pointed to by dst (overwriting its terminating null byte).
    The programmer is responsible for allocating a destination buffer large enough, that is, strlen(dst) + strlen(src) + 1. */
char *strcat(char *dst, const char *src) {
  assert(dst != NULL && src != NULL);
  size_t len_dst = strlen(dst);
  size_t len_src = strlen(src);
  memmove(dst + len_dst, src, len_src + 1);

  return dst;
}

/*  The  strcmp()  function  compares  the two strings s1 and s2.  
    The locale is not taken into account (for a locale-aware comparison, see str‐
    coll(3)).  The comparison is done using unsigned characters.
    strcmp() returns an integer indicating the result of the comparison, as follows:
    •  0, if the s1 and s2 are equal;
    •  a negative value if s1 is less than s2;
    •  a positive value if s1 is greater than s2.*/
int strcmp(const char *s1, const char *s2) {
  assert(s1 != NULL && s2 != NULL);
  size_t len1 = strlen(s1);
  size_t len2 = strlen(s2);
  size_t len_min = (len1 < len2) ? len1 : len2;
  int result = memcmp(s1, s2, len_min);
  if (result == 0) {
    return len1 - len2;
  }

  return result;
}

/*  The strncmp() function compares only the first (at most) n bytes of s1 and s2.  */
int strncmp(const char *s1, const char *s2, size_t n) {
  assert(s1 != NULL && s2 != NULL);
  return memcmp(s1, s2, n);
}

/*  The memset() function fills the first n bytes of the memory area pointed to by s with the constant byte c.  */
void *memset(void *s, int c, size_t n) {
  assert(s != NULL);
  unsigned char *ptr = s; // dont change the original pointer location
  for (size_t i = 0; i < n; i++) {
    *ptr++ = (unsigned char)c;
  }

  return s;
}

/*  The memmove() function copies n bytes from memory area src to memory area dest.  
    The memory areas may overlap: copying takes place as though the bytes in src are first copied into a temporary array that does not overlap src or dest, 
    and the bytes are then copied from the temporary array to dest. */
void *memmove(void *dst, const void *src, size_t n) {
  assert(dst != NULL && src != NULL);
  const unsigned char *ptr_src = src;
  unsigned char *ptr_dst = dst;
  if (ptr_src < ptr_dst && ptr_dst < ptr_src + n) {
    // overlap, move it from back to front
    for (ptr_src += n, ptr_dst += n; n != 0; n--) {
      *(--ptr_dst) = *(--ptr_src);
    }
  } else {
    // move from front to back
    for (; n != 0; n--) {
      *ptr_dst++ = *ptr_src++;
    }
  }

  return dst;  
}

/*  The  memcpy()  function  copies n bytes from memory area in to memory area out.  
    The memory areas must not overlap.  
    Use memmove(3) if the memory areas do overlap.  */
void *memcpy(void *out, const void *in, size_t n) {
  assert(in != NULL && out != NULL);
  const unsigned char *ptr_in = in;
  unsigned char *ptr_out = out;
  for (size_t i = 0; i < n; i++) {
    *ptr_out++ = *ptr_in++;
  }

  return out;
}

/*  The memcmp() function compares the first n bytes (each interpreted as unsigned char) of the memory areas s1 and s2. 
    For a nonzero return value, the sign is determined by the sign of the difference between the first pair of bytes  (interpreted  as  unsigned
    char) that differ in s1 and s2.
    If n is zero, the return value is zero. */
int memcmp(const void *s1, const void *s2, size_t n) {
  assert(s1 != NULL && s2 != NULL);
  const unsigned char *ptr1 = s1;
  const unsigned char *ptr2 = s2;
  for (size_t i = 0; i < n; ++i) {
    if (*ptr1 != *ptr2) {
      return *ptr1 - *ptr2;
    }
    ptr1++;
    ptr2++;
  }

  return 0;
}

#endif
