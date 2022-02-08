#pragma once

#include <libpd/z_libpd.h>
#include <ck_ring.h>

#include "bclib/bstrlib.h"

#include "messages.h"

typedef enum {
  AS_WAITING_FOR_PATCH,
  AS_PLAYING,
  AS_FADING_OUT,
  AS_FADING_IN,
  AS_LOADING_NEXT_PATCH,
} AudioSynthesisState;

typedef struct AudioSynthesisProcessConfig {

  int samplerate;
  int channels;

  void *patch_file;
  int blocksize;

  double fadetime;
  double fadeamount;
  double fadedelta;
  AudioSynthesisState state;
  Message *next_patch_msg;

  int max_push_msgs;
  int *status_var;

  ck_ring_buffer_t *pipe_in_buffer;
  ck_ring_t *pipe_in;

  ck_ring_t *pipe_out;
  ck_ring_buffer_t *pipe_out_buffer;

} AudioSynthesisProcessConfig;

AudioSynthesisProcessConfig *
audio_synthesis_config_create(
                              int samplerate, int channels, double fadetime,
                              int max_push_msgs, int *status_var,
                              ck_ring_t *pipe_in, ck_ring_buffer_t *pipe_in_buffer,
                              ck_ring_t *pipe_out, ck_ring_buffer_t *pipe_out_buffer
                              );

void audio_synthesis_config_destroy(AudioSynthesisProcessConfig *cfg);

void *start_audio_synthesis(void *_cfg);
