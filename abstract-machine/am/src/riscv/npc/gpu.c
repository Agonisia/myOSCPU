#include <am.h>
#include "include/npc.h"

#define SYNC_ADDR (VGACTL_ADDR + 4)

void __am_gpu_init() {
}

void __am_gpu_config(AM_GPU_CONFIG_T *cfg) {
  uint32_t screen_width = inl(VGACTL_ADDR) >> 16;
  uint32_t screen_height = inl(VGACTL_ADDR) & 0xFFFF;
  uint32_t vmem_size = screen_width * screen_height * sizeof(uint32_t);
  *cfg = (AM_GPU_CONFIG_T) {
    .present = true, .has_accel = false,
    .width = screen_width, .height = screen_height,
    .vmemsz = vmem_size
  };
}

void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl) {
  // see am-kernels/tests/am-tests/src/tests/video.c
  int x = ctl->x, y = ctl->y;
  int width = ctl->w, height = ctl->h;
  if (!ctl->sync && (width == 0 || height == 0)) {
    return;
  }
  
  uint32_t *pixels = ctl->pixels;
  uint32_t screen_width = inl(VGACTL_ADDR) >> 16;
  uint32_t *frame_buffer = (uint32_t *)(uintptr_t)FB_ADDR;
  // loop through each pixel's area and write to framebuffer
  for (int j = 0; j < height; j++) {
    for (int i = 0; i < width; i++) {
      // get the framebuffer index for current pixel
      uint32_t index = (y + j) * screen_width + (x + i);
      // write the pixel to the framebuffer
      frame_buffer[index] = pixels[j * width + i];
    }
  }

  if (ctl->sync) {
    outl(SYNC_ADDR, 1); // stl[0] = ADDR, stl[1] = is_sync
  }
}

void __am_gpu_status(AM_GPU_STATUS_T *status) {
  status->ready = true;
}
