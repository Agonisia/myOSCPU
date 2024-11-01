#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

/*  Upon successful return, these functions return the number of bytes printed (excluding the null byte used to end output to strings).

    The  functions  snprintf() and vsnprintf() do not write more than size bytes (including the terminating null byte ('\0')).  
    If the output was truncated due to this limit, then the return value is the number of characters (excluding the terminating null byte) 
    which would have been written to the final string if enough space had been available.  
    Thus, a return value of size or more means that the output was truncated.
    
    If an output error is encountered, a negative value is returned.  */

/*  iota convert int and return its in string */
static char* itoa(int value, char* str, int base) {
  assert(base <= 16);

  static char digit[] = "0123456789abcdef";
  char* wstr = str;
  size_t sign = 0;
  size_t len = 0;

  // nagative case
  if (value < 0 && base == 10) {
    sign = 1;
    value = -value;
  } 

  // turn int to string 
  while (value) {
    wstr[len++] = digit[abs(value % base)];
    value /= base;
  }

  // add minus sign
  if (sign) {
    wstr[len++] = '-';
  }

  // add terminator
  wstr[len] = '\0';

  // reverse the string 
  for (size_t i = 0; i < len / 2; i++) {
    char temp = str[i];
    str[i] = str[len - 1 - i];
    str[len - 1 - i] = temp;
  }

  return str;
}

/*  vsnprintf formats a string with a va_list and stores it in a buffer with size limits.*/
int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  if (n == 0) {
    return 0; // write nothing if n == 0
  }
  
  size_t len = 0;

  while (*fmt && len < n - 1) {
    if (*fmt == '%') {
      ++fmt; // skip % sign, using symbol behind it
      switch (*fmt) {
        case 's': {
          // string case
          const char *str = va_arg(ap, const char *);
          assert(str != NULL);
          size_t str_len = strlen(str); // total string length
          size_t to_copy = (str_len > n - len - 1) ? n - len - 1 : str_len; // detect len of string need to be copied
          memcpy(out, str, to_copy);
          out += to_copy; // move to end of the string 
          len += to_copy; // update num of total written string
          break;
        }
        case 'd': {
          // decimal num case 
          int num = va_arg(ap, int);
          char buf[20];
          itoa(num, buf, 10);
          size_t num_len = strlen(buf);
          size_t to_copy = (num_len > n - len - 1) ? n - len - 1 : num_len;
          memcpy(out, buf, to_copy);
          out += to_copy;
          len += to_copy;
          break;
        }
        default: {
          // unknown type
          *out++ = *fmt; // just copy it
          len++;
          break;
        }
      }
    } else {
      *out++ = *fmt;
      len++;
    }
    fmt++;
  }

  *out = '\0'; // add teminator 
  return len; // return num actally written
}

/*  vsprintf formats a string and stores it in a buffer using a va_list of arguments. */
int vsprintf(char *out, const char *fmt, va_list ap) {
  return vsnprintf(out, SIZE_MAX, fmt, ap);
}

/*  snprintf formats a string with size limits and stores it in a buffer, taking a format string and a variable number of arguments. */
int snprintf(char *out, size_t n, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int result = vsnprintf(out, n, fmt, ap);
    va_end(ap);
    return result;
}

/*  sprintf formats a string and stores it in a buffer, taking a format string and a variable number of arguments directly. */
int sprintf(char *out, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int result = vsprintf(out, fmt, ap);
    va_end(ap);
    return result;
}

/*  printf write a formatted string to the standard output stream.  */
int printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char buffer[1024];
    int result = vsnprintf(buffer, sizeof(buffer), fmt, ap);
    va_end(ap);
    putstr(buffer);
    return result;
}

#endif
