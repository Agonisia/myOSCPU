#include "monitor/tracer.h"

#define MAX_SIZE_RING_BUFFER 16

typedef struct {
  char buffer[MAX_SIZE_RING_BUFFER][128]; // store the log
  int head;  // point to the newest writting 
  int size;  // the num of log inside
} RingBuffer;
RingBuffer ring_buffer = {{{0}}, 0, 0};

void print_ring_buffer() {
  printf("Error occurred, recent instructions:\n");

  for (int i = 0; i < ring_buffer.size; i++) {
    int index = (ring_buffer.head - ring_buffer.size + i + MAX_SIZE_RING_BUFFER) % MAX_SIZE_RING_BUFFER;
    if (i == ring_buffer.size - 1) { // the latest instruction
      printf("--> %s\n", ring_buffer.buffer[index]); // the error instruction
    } else {
      printf("    %s\n", ring_buffer.buffer[index]);
    }
  }
}

void write_ring_buffer(const char *logbuf) {
  // ring_buffer.buffer => array, ring_buffer.head => current index 
  // combine them => specific element at current index in array
  snprintf(ring_buffer.buffer[ring_buffer.head], sizeof(ring_buffer.buffer[ring_buffer.head]), "%s", logbuf);
  ring_buffer.head = (ring_buffer.head + 1) % MAX_SIZE_RING_BUFFER; // ring loop
  if (ring_buffer.size < MAX_SIZE_RING_BUFFER) {
    ring_buffer.size++;
  }
}

void inst_trace(CORE_state core) {
  char *p = core.logbuf;
  uint32_t inst = core.inst;

  // output the pc
  p += snprintf(p, sizeof(core.logbuf), FMT_WORD ":", core.pc); // write current pc into logbuf and move to next

  // output the instruction 
  int ilen = (inst & 0x3) == 0x3 ? 4 : 2; // get length of the instruction, default is 4 but 2 for compressed
  // output the instruction in hex
  uint8_t *inst_bytes = (uint8_t *)&inst; // interpret inst as a byte array
  for (int i = ilen - 1; i >= 0; i--) {
    p += snprintf(p, 4, " %02x", inst_bytes[i]); // output each byte in hex, low to high
  }

  // output align
  const int ilen_max = 4;
  const int space_len = (ilen_max - ilen) * 3 + 2; // calculate space length
  p += snprintf(p, space_len + 1, "%*s", space_len, ""); // add spaces

  // disassemble the instrucion 
  disassemble(p, core.logbuf + sizeof(core.logbuf) - p, core.pc, inst_bytes, ilen);

  // output the log buffer
  printf("%s\n", core.logbuf);

  // write the log buffer into ring buffer
  write_ring_buffer(core.logbuf);

  // write the log buffer into log file
  if (CONFIG_LOG) {
    log_write("%s\n", core.logbuf);
  }
}