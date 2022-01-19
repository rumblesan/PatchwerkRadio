#pragma once

#include <libpd/z_libpd.h>

#include "bclib/bstrlib.h"
#include "bclib/ringbuffer.h"

typedef struct AudioSynthesisProcessConfig {

  bstring patch_directory;
  bstring patch_file;

  int samplerate;
  int channels;

  int max_push_msgs;
  int *status_var;
  void *pd_file;

  RingBuffer *pipe_out;

} AudioSynthesisProcessConfig;

AudioSynthesisProcessConfig *
audio_synthesis_config_create(bstring patch_directory, bstring patch_file,
                              int samplerate, int channels, int max_push_msgs,
                              int *status_var, RingBuffer *pipe_out);

void audio_synthesis_config_destroy(AudioSynthesisProcessConfig *cfg);

void *start_audio_synthesis(void *_cfg);
