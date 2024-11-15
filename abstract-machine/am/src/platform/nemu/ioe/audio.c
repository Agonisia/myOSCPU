#include <am.h>
#include <nemu.h>

#define AUDIO_FREQ_ADDR      (AUDIO_ADDR + 0x00)
#define AUDIO_CHANNELS_ADDR  (AUDIO_ADDR + 0x04)
#define AUDIO_SAMPLES_ADDR   (AUDIO_ADDR + 0x08)
#define AUDIO_SBUF_SIZE_ADDR (AUDIO_ADDR + 0x0c)
#define AUDIO_INIT_ADDR      (AUDIO_ADDR + 0x10)
#define AUDIO_COUNT_ADDR     (AUDIO_ADDR + 0x14)

void __am_audio_init() {
}

/*  read the presence flag [present] and size of the stream buffer [bufsize]  */
void __am_audio_config(AM_AUDIO_CONFIG_T *cfg) {
  cfg->present = true;
  cfg->bufsize = inl(AUDIO_SBUF_SIZE_ADDR);
}

/*  initialize according to the written [freq], [channels] and [samples]  */
void __am_audio_ctrl(AM_AUDIO_CTRL_T *ctrl) {
  outl(AUDIO_FREQ_ADDR, ctrl->freq);
  outl(AUDIO_CHANNELS_ADDR, ctrl->channels);
  outl(AUDIO_SAMPLES_ADDR, ctrl->samples);
  outl(AUDIO_INIT_ADDR, 1);
}

/*  read the size of current stream buffer already in use [count] */
void __am_audio_status(AM_AUDIO_STATUS_T *stat) {
  stat->count = inl(AUDIO_COUNT_ADDR);
}

/*  write the content of the interval [buf.start, buf.end) into the stream buffer as audio data 
    if the free space in the stream buffer is less than the amount of audio data to be written, 
    the write will wait until there is enough free space to write all the audio data  */
void __am_audio_play(AM_AUDIO_PLAY_T *ctl) {
  int audio_len = ctl->buf.end - ctl->buf.start;
  int buffer_size = io_read(AM_AUDIO_CONFIG).bufsize;
  int in_use_size = inl(AUDIO_COUNT_ADDR);
  int free_space = buffer_size - in_use_size;

  // not enough free space
  while (free_space < audio_len) {
    // maybe need some busy-wait
    // refresh current buffer useage
    in_use_size = inl(AUDIO_COUNT_ADDR);
    free_space = buffer_size - in_use_size;
  }

  // use AUDIO_SBUF_ADDR as the base address of stream buffer
  uint8_t *sbuf_start = (uint8_t *)AUDIO_SBUF_ADDR;
  // copy the audio data from ctl->buf.start to the buffer at AUDIO_SBUF_ADDR
  // memcpy(sbuf_start + in_use_size, ctl->buf.start, audio_len); // easy implement
  uint8_t *buf_start = (uint8_t *)ctl->buf.start;
  
  // write 4 bytes at one time
  for (int i = 0; i < audio_len / 4; i++) {
    outl((uintptr_t)(sbuf_start + in_use_size + i * 4), *((uint32_t *)(buf_start + i * 4)));
  }

  // write remaining bytes
  for (int i = audio_len / 4 * 4; i < audio_len; i++) {
    outb((uintptr_t)(sbuf_start + in_use_size + i), buf_start[i]);
  }

  // update the count with the new size
  outl(AUDIO_COUNT_ADDR, in_use_size + audio_len);
}
