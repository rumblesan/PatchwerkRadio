#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>

#include "audio_synthesis_process.h"

#include "messages.h"
#include "logging.h"

#include <bclib/dbg.h>
#include <libpd/z_libpd.h>
#include <bclib/ringbuffer.h>

AudioSynthesisProcessConfig *audio_synthesis_config_create(
                                                int max_push_msgs,
                                                int *status_var,
                                                RingBuffer *pipe_out
                                                ) {

  AudioSynthesisProcessConfig *cfg = malloc(sizeof(AudioSynthesisProcessConfig));
  check_mem(cfg);

  check(pipe_out != NULL, "AudioSynthesis: Invalid pipe out");
  cfg->pipe_out = pipe_out;

  cfg->status_var = status_var;

  cfg->max_push_msgs = max_push_msgs;

  return cfg;
 error:
  return NULL;
}

void audio_synthesis_config_destroy(AudioSynthesisProcessConfig *cfg) {
  check(cfg != NULL, "AudioSynthesis: Invalid cfg");
  free(cfg);
  return;
 error:
  err_logger("AudioSynthesis", "Could not destroy cfg");
}

void *start_audio_synthesis(void *_cfg) {

  AudioSynthesisProcessConfig *cfg = _cfg;

  libpd_init();
  const char *directory = "/opt/patchwerk/patches";
  const char *patch = "test.pd";
  void *pd_file = libpd_openfile(patch, directory);
  check(pd_file != NULL, "Could not open pd patch");
  cfg->pd_file = pd_file;

  int blocksize = libpd_blocksize();
  int channel_num = 2;
  int pd_init = libpd_init_audio(0, channel_num, 44100);
  check(pd_init == 0, "Could not initialise PD: %d", pd_init);

  check(!libpd_start_message(16), "Could not allocate space for PD message");
  libpd_add_float(1);
  libpd_finish_message("pd", "dsp");

  float *in_buffer = interleaved_audio(channel_num, blocksize);
  float *out_buffer = interleaved_audio(channel_num, blocksize);

  int pushed_msgs = 0;

  *(cfg->status_var) = 1;

  struct timespec tim, tim2;
  tim.tv_sec = 0;
  tim.tv_nsec = 10;


  logger("AudioSynthesis", "Started");

  PatchInfo *info = patch_info_create(bfromcstr("rumblesan"), bfromcstr("test patch"));
  Message *new_patch_msg = new_patch_message(info);
  rb_push(cfg->pipe_out, new_patch_msg);

  bool running = true;
  while (running) {
    if (!rb_full(cfg->pipe_out) && pushed_msgs < cfg->max_push_msgs) {
      pushed_msgs += 1;

      check(true, "stand in check");

      int audio_check = libpd_process_float(1, in_buffer, out_buffer);
      check(audio_check == 0, "PD could not create audio");
      AudioBuffer *out_audio = audio_buffer_from_float(out_buffer, channel_num, channel_num * blocksize);

      Message *msg = audio_buffer_message(out_audio);
      rb_push(cfg->pipe_out, msg);
    } else {
      if (pushed_msgs > 0) {
        //logger("AudioSynthesis", "Pushed %d messages", pushed_msgs);
      }
      pushed_msgs = 0;
      sched_yield();
      nanosleep(&tim, &tim2);
    }
  }


 error:
  logger("AudioSynthesis", "Finished");
  *(cfg->status_var) = 0;
  audio_synthesis_config_destroy(cfg);
  logger("AudioSynthesis", "Cleaned up");
  pthread_exit(NULL);
  return NULL;
}
