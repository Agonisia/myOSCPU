/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <common.h>
#include <device/map.h>
#include <SDL2/SDL.h>

enum {
  reg_freq,
  reg_channels,
  reg_samples,
  reg_sbuf_size,
  reg_init,
  reg_count,
  nr_reg
};

enum {
  addr_freq = reg_freq * 4,
  addr_channels = reg_channels * 4,
  addr_samples = reg_samples * 4,
  addr_sbuf_size = reg_sbuf_size * 4,
  addr_init = reg_init * 4,
  addr_count = reg_count * 4,
  addr_num_reg = nr_reg * 4
};

static uint8_t *sbuf = NULL;
static uint32_t *audio_base = NULL;
static int sbuf_count = 0;

SDL_AudioSpec s = {};

void audio_callback(void *userdata, Uint8 * stream, int len) {
  SDL_LockAudio(); 
  // from sbuf to SDL stream
  SDL_memset(stream, 0, len);

  // use linear buffer instead of maintaining complex ring buffer
  if (sbuf_count > len) {
    // data could read in sbuf larger than SDL needed
    // move [len] bytes to SDL
    SDL_memcpy(stream, sbuf, len); 
    // move left data in sbuf to the front
    SDL_memmove(sbuf, sbuf + len, sbuf_count - len); // also can use memcpy here
    // update sbuf data count 
    sbuf_count -= len;
  } else {
    // data in sbuf smaller than SDL needed
    // move all the bytes in sbuf to SDL
    SDL_memcpy(stream, sbuf, sbuf_count);
    // set left part in SDL as 0
    SDL_memset(stream + sbuf_count, 0, len - sbuf_count);
    // set sbuf date count as 0
    sbuf_count = 0;
  }
  SDL_UnlockAudio();
}

static void audio_io_handler(uint32_t offset, int len, bool is_write) {
  SDL_LockAudio();
  if (is_write) {
    // write request
    switch (offset) {
      case addr_freq:
        s.freq = audio_base[reg_freq];
        break;
      case addr_channels:
        s.channels = audio_base[reg_channels];
        break;
      case addr_samples:
        s.samples = audio_base[reg_samples];
        break;
      case addr_count:
        sbuf_count = audio_base[reg_count];
        break;
      case addr_init:
        if (audio_base[reg_init] == 1 && s.callback == NULL) {
          audio_base[reg_init] = 0;
          s.format = AUDIO_S16SYS;
          s.userdata = NULL;
          s.size = CONFIG_SB_SIZE;
          s.callback = audio_callback;
          SDL_InitSubSystem(SDL_INIT_AUDIO);
          SDL_OpenAudio(&s, NULL);
          SDL_PauseAudio(0);
        }
        break;
      default:
        printf("write error: offset = 0x%02x\n", offset);
        break;
    }
  } else {
    // read request
    switch (offset) {
      case addr_sbuf_size:
        audio_base[reg_sbuf_size] = CONFIG_SB_SIZE;
        break;
      case addr_count:
        audio_base[reg_count] = sbuf_count;
        break;
      default:
        break;
    }
  }
  SDL_UnlockAudio();
}

void init_audio() {
  uint32_t space_size = sizeof(uint32_t) * nr_reg;
  audio_base = (uint32_t *)new_space(space_size);
#ifdef CONFIG_HAS_PORT_IO
  add_pio_map ("audio", CONFIG_AUDIO_CTL_PORT, audio_base, space_size, audio_io_handler);
#else
  add_mmio_map("audio", CONFIG_AUDIO_CTL_MMIO, audio_base, space_size, audio_io_handler);
#endif

  sbuf = (uint8_t *)new_space(CONFIG_SB_SIZE);
  add_mmio_map("audio-sbuf", CONFIG_SB_ADDR, sbuf, CONFIG_SB_SIZE, NULL);
}
