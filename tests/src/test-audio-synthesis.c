#include "audio_synthesis_process.h"
#include "messages.h"
#include "minunit.h"
#include "pipe_utils.h"

#include <bclib/bstrlib.h>
#include <bclib/dbg.h>
#include <bclib/ringbuffer.h>

char *test_audio_synthesis_config_create() {
  int samplerate = 44100;
  int channels = 2;
  double fadetime = 10.0;
  int queue_size = 1024;
  int max_push_msgs = 10;
  int audio_synth_status;

  ck_ring_t *pipe_in = NULL;
  ck_ring_buffer_t *pipe_in_buffer = NULL;

  ck_ring_t *pipe_out = NULL;
  ck_ring_buffer_t *pipe_out_buffer = NULL;

  create_pipe(&pipe_in, &pipe_in_buffer, queue_size);
  create_pipe(&pipe_out, &pipe_out_buffer, queue_size);

  AudioSynthesisProcessConfig *cfg = audio_synthesis_config_create(
      samplerate, channels, fadetime, max_push_msgs, &audio_synth_status,
      pipe_in, pipe_in_buffer, pipe_out, pipe_out_buffer);

  mu_assert(cfg != NULL, "Could not create audio synthesis process config");

  audio_synthesis_config_destroy(cfg);

  cleanup_pipe(pipe_in, pipe_in_buffer, "pipein");
  cleanup_pipe(pipe_out, pipe_out_buffer, "pipeout");
  return NULL;
}

char *all_tests() {
  mu_suite_start();

  mu_run_test(test_audio_synthesis_config_create);

  return NULL;
}

RUN_TESTS(all_tests);
