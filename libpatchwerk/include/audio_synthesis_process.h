#pragma once

#include <libpd/z_libpd.h>
#include <ck_ring.h>

#include "bclib/bstrlib.h"

typedef struct AudioSynthesisProcessConfig {

  int samplerate;
  int channels;

  bool pd_ready;
  void *patch_file;
  int blocksize;

  int max_push_msgs;
  int *status_var;

  ck_ring_buffer_t *pipe_in_buffer;
  ck_ring_t *pipe_in;

  ck_ring_t *pipe_out;
  ck_ring_buffer_t *pipe_out_buffer;

} AudioSynthesisProcessConfig;

AudioSynthesisProcessConfig *
audio_synthesis_config_create(
                              int samplerate, int channels, int max_push_msgs,
                              int *status_var,
                              ck_ring_t *pipe_in, ck_ring_buffer_t *pipe_in_buffer,
                              ck_ring_t *pipe_out, ck_ring_buffer_t *pipe_out_buffer
                              );

void audio_synthesis_config_destroy(AudioSynthesisProcessConfig *cfg);

void *start_audio_synthesis(void *_cfg);
