
#include "encoder_process.h"
#include "messages.h"
#include "minunit.h"

#include <bclib/bstrlib.h>
#include <bclib/dbg.h>
#include <bclib/ringbuffer.h>

char *test_encoder_config_create() {
  int channels = 2;
  int samplerate = 44100;
  int format = 1;
  double quality = 0.7;
  int max_push_msgs = 10;
  int thread_sleep = 20;
  int encoder_status;

  RingBuffer *pipe_in = rb_create(100);
  RingBuffer *pipe_out = rb_create(100);

  EncoderProcessConfig *cfg =
      encoder_config_create(channels, samplerate, format, quality, thread_sleep,
                            max_push_msgs, &encoder_status, pipe_in, pipe_out);

  mu_assert(cfg != NULL, "Could not create encoder process config");

  encoder_config_destroy(cfg);
  rb_destroy(pipe_in);
  rb_destroy(pipe_out);
  return NULL;
}

char *test_encoder_loop() {
  int channels = 2;
  int samplerate = 44100;
  int format = 1;
  double quality = 0.7;
  int max_push_msgs = 10;
  int thread_sleep = 20;
  int maxmsgs = 5000;
  int read_size = 2048;
  int encoder_status = -1;

  PatchInfo *patch_info =
      patch_info_create(bfromcstr("creator"), bfromcstr("title"));
  Message *input_msg = NULL;
  Message *output_msg = NULL;
  AudioBuffer *audio = NULL;

  RingBuffer *pipe_in = rb_create(maxmsgs + 10);
  RingBuffer *pipe_out = rb_create(maxmsgs + 10);

  EncoderProcessConfig *cfg =
      encoder_config_create(channels, samplerate, format, quality, thread_sleep,
                            max_push_msgs, &encoder_status, pipe_in, pipe_out);

  mu_assert(cfg != NULL, "Could not create encoder process config");

  input_msg = new_patch_message(patch_info);
  rb_push(pipe_in, input_msg);
  for (int m = 0; m < maxmsgs; m += 1) {
    audio = audio_buffer_create(channels, read_size);
    input_msg = audio_buffer_message(audio);
    rb_push(pipe_in, input_msg);
  }
  input_msg = stream_finished_message();
  rb_push(pipe_in, input_msg);

  start_encoder(cfg);

  mu_assert(!rb_empty(pipe_out), "Output pipe should not be empty");

  while (!rb_empty(pipe_out)) {
    output_msg = rb_pop(pipe_out);
    message_destroy(output_msg);
  }

  mu_assert(encoder_status == 0, "Encoder status should be 0");

  rb_destroy(pipe_in);
  rb_destroy(pipe_out);
  return NULL;
}

char *test_multi_loop() {
  int channels = 2;
  int samplerate = 44100;
  int format = 1;
  double quality = 0.7;
  int thread_sleep = 20;
  int max_push_msgs = 10;
  int patchs = 3;
  int patch_msgs = 5000;
  int read_size = 2048;
  PatchInfo *patch_info = NULL;
  int encoder_status = -1;
  Message *input_msg = NULL;
  Message *output_msg = NULL;
  AudioBuffer *audio = NULL;

  RingBuffer *pipe_in = rb_create(patchs * (patch_msgs + 10));
  RingBuffer *pipe_out = rb_create(patchs * (patch_msgs + 10));

  EncoderProcessConfig *cfg =
      encoder_config_create(channels, samplerate, format, quality, thread_sleep,
                            max_push_msgs, &encoder_status, pipe_in, pipe_out);

  mu_assert(cfg != NULL, "Could not create encoder process config");

  for (int t = 0; t < patchs; t += 1) {
    log_info("Patch %d", t + 1);
    patch_info = patch_info_create(bfromcstr("creator"), bfromcstr("title"));
    input_msg = new_patch_message(patch_info);
    rb_push(pipe_in, input_msg);
    for (int m = 0; m < patch_msgs; m += 1) {
      audio = audio_buffer_create(channels, read_size);
      input_msg = audio_buffer_message(audio);
      rb_push(pipe_in, input_msg);
    }
    input_msg = patch_finished_message();
    rb_push(pipe_in, input_msg);
  }

  input_msg = stream_finished_message();
  rb_push(pipe_in, input_msg);

  start_encoder(cfg);

  mu_assert(!rb_empty(pipe_out), "Output pipe should not be empty");

  while (!rb_empty(pipe_out)) {
    output_msg = rb_pop(pipe_out);
    message_destroy(output_msg);
  }

  mu_assert(encoder_status == 0, "Encoder status should be 0");

  rb_destroy(pipe_in);
  rb_destroy(pipe_out);
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
