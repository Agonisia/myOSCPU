#include "device/map.h"
#include "device/alarm.h"
#include "utils.h"

static uint32_t *rtc_port_base = NULL;

static void rtc_io_handler(uint32_t offset, int len, bool is_write) {
  assert(offset == 0 || offset == 4);
  if (!is_write && offset == 4) {
    uint64_t us = get_time();
    rtc_port_base[0] = (uint32_t)us;
    rtc_port_base[1] = us >> 32;
  }
}

static void timer_intr() {
  if (sim_state.state == SIM_RUNNING) {
    extern void dev_raise_intr();
    dev_raise_intr();
  }
}

void rtc_init() {
  rtc_port_base = (uint32_t *)new_space(8);
  add_mmio_map("rtc", CONFIG_RTC_ADDR, rtc_port_base, 8, rtc_io_handler);
  add_alarm_handle(timer_intr);
}