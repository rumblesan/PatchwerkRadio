
#include "audio_buffer.h"
#include "encoder_process.h"
#include "messages.h"
#include "minunit.h"
#include "pipe_utils.h"

#include <bclib/bstrlib.h>
#include <bclib/dbg.h>
#include <ck_ring.h>

char *test_encoder_config_create() {
  int channels = 2;
  int samplerate = 44100;
  int format = 1;
  double quality = 0.7;
  int max_push_msgs = 10;
  int queue_size = 256;
  int thread_sleep = 20;
  int encoder_status;

  ck_ring_t *pipe_in = NULL;
  ck_ring_buffer_t *pipe_in_buffer = NULL;

  ck_ring_t *pipe_out = NULL;
  ck_ring_buffer_t *pipe_out_buffer = NULL;

  create_pipe(&pipe_in, &pipe_in_buffer, queue_size);
  create_pipe(&pipe_out, &pipe_out_buffer, queue_size);

  EncoderProcessConfig *cfg = encoder_config_create(
      channels, samplerate, format, quality, thread_sleep, max_push_msgs,
      &encoder_status, pipe_in, pipe_in_buffer, pipe_out, pipe_out_buffer);

  mu_assert(cfg != NULL, "Could not create encoder process config");

  encoder_config_destroy(cfg);
  cleanup_pipe(pipe_in, pipe_in_buffer, "PipeIn");
  cleanup_pipe(pipe_out, pipe_out_buffer, "PipeOut");
  return NULL;
}

char *test_encoder_loop() {
  int channels = 2;
  int samplerate = 44100;
  int format = 1;
  double quality = 0.7;
  int max_push_msgs = 10;
  int thread_sleep = 20;
  int maxmsgs = 64;
  int queue_size = 256;
  int read_size = 2048;
  int encoder_status = -1;

  CreatorInfo *creator_info =
      creator_info_create(bfromcstr("creator"), bfromcstr("title"));
  Message *input_msg = NULL;
  Message *output_msg = NULL;
  AudioBuffer *audio = NULL;

  ck_ring_t *pipe_in = NULL;
  ck_ring_buffer_t *pipe_in_buffer = NULL;

  ck_ring_t *pipe_out = NULL;
  ck_ring_buffer_t *pipe_out_buffer = NULL;

  create_pipe(&pipe_in, &pipe_in_buffer, queue_size);
  create_pipe(&pipe_out, &pipe_out_buffer, queue_size);

  EncoderProcessConfig *cfg = encoder_config_create(
      channels, samplerate, format, quality, thread_sleep, max_push_msgs,
      &encoder_status, pipe_in, pipe_in_buffer, pipe_out, pipe_out_buffer);

  mu_assert(cfg != NULL, "Could not create encoder process config");

  input_msg = new_patch_message(creator_info);
  mu_assert(ck_ring_enqueue_spsc(pipe_in, pipe_in_buffer, input_msg),
            "Could not add first message to pipe in") for (int m = 0;
                                                           m < maxmsgs;
                                                           m += 1) {
    audio = audio_buffer_create(channels, read_size);
    input_msg = audio_buffer_message(audio);
    mu_assert(ck_ring_enqueue_spsc(pipe_in, pipe_in_buffer, input_msg),
              "Could not add test message to pipe in");
  }
  printf("pipe in size %d\n", ck_ring_size(pipe_in));
  input_msg = stream_finished_message();
  ck_ring_enqueue_spsc(pipe_in, pipe_in_buffer, input_msg);

  start_encoder(cfg);

  mu_assert(ck_ring_size(pipe_out) > 0, "Output pipe should not be empty");

  while (ck_ring_dequeue_spsc(pipe_out, pipe_out_buffer, &output_msg) > 0) {
    message_destroy(output_msg);
  }

  mu_assert(encoder_status == 0, "Encoder status should be 0");

  cleanup_pipe(pipe_in, pipe_in_buffer, "PipeIn");
  cleanup_pipe(pipe_out, pipe_out_buffer, "PipeOut");
  return NULL;
}

char *test_multi_loop() {
  int channels = 2;
  int samplerate = 44100;
  int format = 1;
  double quality = 0.7;
  int thread_sleep = 20;
  int queue_size = 4096;
  int max_push_msgs = 10;
  int patches = 3;
  int patch_msgs = 50;
  int read_size = 2048;
  CreatorInfo *creator_info = NULL;
  int encoder_status = -1;
  Message *input_msg = NULL;
  Message *output_msg = NULL;
  AudioBuffer *audio = NULL;

  ck_ring_t *pipe_in = NULL;
  ck_ring_buffer_t *pipe_in_buffer = NULL;

  ck_ring_t *pipe_out = NULL;
  ck_ring_buffer_t *pipe_out_buffer = NULL;

  create_pipe(&pipe_in, &pipe_in_buffer, queue_size);
  create_pipe(&pipe_out, &pipe_out_buffer, queue_size);

  EncoderProcessConfig *cfg = encoder_config_create(
      channels, samplerate, format, quality, thread_sleep, max_push_msgs,
      &encoder_status, pipe_in, pipe_in_buffer, pipe_out, pipe_out_buffer);

  mu_assert(cfg != NULL, "Could not create encoder process config");

  for (int t = 0; t < patches; t += 1) {
    log_info("Patch %d", t + 1);
    creator_info =
        creator_info_create(bfromcstr("creator"), bfromcstr("title"));
    input_msg = new_patch_message(creator_info);
    ck_ring_enqueue_spsc(pipe_in, pipe_in_buffer, input_msg);
    for (int m = 0; m < patch_msgs; m += 1) {
      audio = audio_buffer_create(channels, read_size);
      input_msg = audio_buffer_message(audio);
      ck_ring_enqueue_spsc(pipe_in, pipe_in_buffer, input_msg);
    }
    input_msg = patch_finished_message();
    ck_ring_enqueue_spsc(pipe_in, pipe_in_buffer, input_msg);
  }

  input_msg = stream_finished_message();
  ck_ring_enqueue_spsc(pipe_in, pipe_in_buffer, input_msg);

  start_encoder(cfg);

  mu_assert(ck_ring_size(pipe_out) > 0, "Output pipe should not be empty");

  while (ck_ring_dequeue_spsc(pipe_out, pipe_out_buffer, &output_msg) > 0) {
    message_destroy(output_msg);
  }

  mu_assert(encoder_status == 0, "Encoder status should be 0");

  cleanup_pipe(pipe_in, pipe_in_buffer, "PipeIn");
  cleanup_pipe(pipe_out, pipe_out_buffer, "PipeOut");
  return NULL;
}

char *all_tests() {
  mu_suite_start();

  mu_run_test(test_encoder_config_create);
  mu_run_test(test_encoder_loop);
  mu_run_test(test_multi_loop);

  return NULL;
}

RUN_TESTS(all_tests);
