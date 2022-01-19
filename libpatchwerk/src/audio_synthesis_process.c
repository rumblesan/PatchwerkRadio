#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>

#include "audio_synthesis_process.h"

#include "messages.h"
#include "logging.h"

#include <bclib/dbg.h>
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

  int pushed_msgs = 0;

  *(cfg->status_var) = 1;

  struct timespec tim, tim2;
  tim.tv_sec = 0;
  tim.tv_nsec = 10;


  logger("AudioSynthesis", "Started");
  bool running = true;
  while (running) {
    if (!rb_full(cfg->pipe_out) && pushed_msgs < cfg->max_push_msgs) {
      pushed_msgs += 1;

      check(true, "stand in check");

      AudioBuffer *buf = audio_buffer_create(2, 64);
      Message *msg = audio_buffer_message(buf);
      rb_push(cfg->pipe_out, msg);
    } else {
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
