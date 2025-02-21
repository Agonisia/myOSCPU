#include "constant.h"
#include "device/map.h"
#include <SDL2/SDL.h>

#define SCREEN_W (MUXONE(CONFIG_VGA_SIZE_800x600, 800, 400))
#define SCREEN_H (MUXONE(CONFIG_VGA_SIZE_800x600, 600, 300))

static uint32_t screen_width() {
  return SCREEN_W;
}

static uint32_t screen_height() {
  return SCREEN_H;
}

static uint32_t screen_size() {
  return screen_width() * screen_height() * sizeof(uint32_t);
}

static void *vmem = NULL;
static uint32_t *vgactl_port_base = NULL;

static SDL_Renderer *renderer = NULL;
static SDL_Texture *texture = NULL;

static void screen_init() {
  SDL_Window *window = NULL;
  char title[128];
  sprintf(title, "RockBottom-%s", CONFIG_ISA);
  SDL_Init(SDL_INIT_VIDEO);
  SDL_CreateWindowAndRenderer(
      SCREEN_W,
      SCREEN_H,
      0, &window, &renderer);
  SDL_SetWindowTitle(window, title);
  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
      SDL_TEXTUREACCESS_STATIC, SCREEN_W, SCREEN_H);
  SDL_RenderPresent(renderer);
}

static inline void screen_update() {
  SDL_UpdateTexture(texture, NULL, vmem, SCREEN_W * sizeof(uint32_t));
  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, NULL, NULL);
  SDL_RenderPresent(renderer);
}

void vga_update_screen() {
  // call `screen_update()` when the sync register is non-zero,
  // then zero out the sync register
  uint32_t is_sync = vgactl_port_base[1];
  if (is_sync) {
    screen_update();
    vgactl_port_base[1] = 0;
  }
}

void vga_init() {
  vgactl_port_base = (uint32_t *)new_space(8);
  vgactl_port_base[0] = (screen_width() << 16) | screen_height(); // width and length conbine there
  add_mmio_map("vgactl", CONFIG_VGA_ADDR, vgactl_port_base, 8, NULL);

  vmem = new_space(screen_size());
  add_mmio_map("vmem", CONFIG_FRAME_BUFFER_ADDR, vmem, screen_size(), NULL);
  screen_init();
  memset(vmem, 0, screen_size());
}
