#pragma once

#include <ck_ring.h>

typedef enum {
  WAITINGFORFILE,
  ENCODINGFILE,
  CLOSINGSTREAM,
  ENCODERERROR,
} EncoderState;

typedef struct EncoderProcessConfig {

  int channels;
  int samplerate;
  int format;
  double quality;

  int thread_sleep;
  int max_push_msgs;

  int *status_var;

  ck_ring_buffer_t *pipe_in_buffer;
  ck_ring_t *pipe_in;

  ck_ring_buffer_t *pipe_out_buffer;
  ck_ring_t *pipe_out;

} EncoderProcessConfig;

EncoderProcessConfig *
encoder_config_create(int channels, int samplerate, int format, double quality,
                      int thread_sleep, int max_push_msgs, int *status_var,
                      ck_ring_t *pipe_in, ck_ring_buffer_t *pipe_in_buffer,
                      ck_ring_t *pipe_out, ck_ring_buffer_t *pipe_out_buffer
                      );

void encoder_config_destroy(EncoderProcessConfig *cfg);

void *start_encoder(void *_cfg);
