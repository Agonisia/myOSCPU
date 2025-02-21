#include "constant.h"
#include "utils.h"
#include "device/alarm.h"
#include <SDL2/SDL.h>

void map_init();
void serial_init();
void rtc_init();
void alarm_init();
void vga_init();

void vga_update_screen();

void device_update() {
  static uint64_t last = 0;
  uint64_t now = get_time();
  if (now - last < 1000000 / TIMER_HZ) {
    return;
  }
  last = now;

  IFONE(CONFIG_HAS_VGA, vga_update_screen());

  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_QUIT:
        sim_state.state = SIM_QUIT;
        break;
#ifdef CONFIG_HAS_KEYBOARD
      // If a key was pressed
      case SDL_KEYDOWN:
      case SDL_KEYUP: {
        uint8_t k = event.key.keysym.scancode;
        bool is_keydown = (event.key.type == SDL_KEYDOWN);
        send_key(k, is_keydown);
        break;
      }
#endif
      default: break;
    }
  }
}

void sdl_clear_event_queue() {
  SDL_Event event;
  while (SDL_PollEvent(&event));
}

void device_init() {
  map_init();

  IFONE(CONFIG_HAS_SERIAL, serial_init());
  IFONE(CONFIG_HAS_RTC, rtc_init());
  IFONE(CONFIG_HAS_VGA, vga_init());

  alarm_init();
}
