#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__) // while macro is defined, using those func in stdio, stdlib and string

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
  size_t total_len = 0; // // total string length without truncation and padding, for the return value
  bool precision_flag = false;
  int precision = -1;
  bool padding_flag = false;
  int padding = 0;

  while (*fmt) {
    if (*fmt == '%' && *(fmt + 1)) {
      ++fmt; // skip % sign, using symbol behind it

      // reset flag
      precision_flag = false;
      precision = -1;
      padding_flag = false;
      padding = 0;

      // check for padding (e.g., %02d)
      if (*fmt == '0') { // Padding with '0' detected
        padding_flag = true;
        ++fmt; // skip the zero
        // read the width value (e.g., 2 in %02d)
        while (*fmt >= '0' && *fmt <= '9') {
          padding = padding * 10 + (*fmt - '0');
          ++fmt;
        }
      }        

      // check for precision (e.g., %.5d or %.3s)
      if (*fmt == '.') {
        precision_flag = true;
        ++fmt; // skip the dot
        precision = 0;
        // read the precision value (e.g., 3 in %.3s)
        while (*fmt >= '0' && *fmt <= '9') {
          precision = precision * 10 + (*fmt - '0');
          ++fmt;
        }
      }

      // handle the type (s, d, etc.)
      switch (*fmt) {
        case 'd': {
          // handle integer case
          int num = va_arg(ap, int);
          char buf[20];
          itoa(num, buf, 10); // convert integer to string
          size_t num_len = strlen(buf);
          total_len += num_len;

          // adjust width for precision
          int width = (precision > num_len) ? precision - num_len + 1 : 0;
          int total_width = (padding > num_len) ? padding : num_len;
          total_width = (precision_flag && precision > num_len) ? precision : total_width;

          // add padding for width
          width = (total_width > num_len) ? total_width - num_len - width : 0;
          for (int i = 0; i < width && len < n - 1; i++) {
            *out++ = (padding_flag ? '0' : ' ');
            len++;
          }

          // copy the number to output buffer
          size_t to_copy = (num_len > n - len - 1) ? n - len - 1 : num_len;
          if (len < n - 1) {
              memcpy(out, buf, to_copy);
              out += to_copy;
              len += to_copy;
          }
          break;
        }

        case 's': {
          // handle string case
          const char *str = va_arg(ap, const char *);
          assert(str != NULL);
          size_t str_len = strlen(str); 
          total_len += str_len;

          // adjust for precision
          size_t to_copy = precision_flag ? (precision < str_len ? precision : str_len) : str_len;
          size_t copy_len = (to_copy > n - len - 1) ? n - len - 1 : to_copy;
          if (len < n - 1) {
            memcpy(out, str, copy_len);
            out += copy_len;
            len += copy_len;
          }
          break;
        }

        default: {
          // handle unknown specifiers
          if (len < n - 1) { // make sure to write inside buffer
            *out++ = *fmt;
            len++;
          }
          total_len++;
          break;
        }
      }
    } else {
      // read next character
      if (len < n - 1) {
        *out++ = *fmt;
        len++;
      }
      total_len++;
    }
    fmt++;
  }

  *out = '\0'; // NULL terminator
  return total_len; // return the total number of characters written
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
