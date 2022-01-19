#include "minunit.h"
#include "audio_synthesis_process.h"
#include "messages.h"

#include <bclib/ringbuffer.h>
#include <bclib/bstrlib.h>
#include <bclib/dbg.h>


char *test_audio_synthesis_config_create() {
  bstring patch_directory = bfromcstr("/some/dir/here");
  bstring patch_file = bfromcstr("my_test_patch.pd");
  int samplerate = 44100;
  int channels = 2;
  int max_push_msgs = 10;
  int audio_synth_status;

  RingBuffer *pipe_out = rb_create(100);

  EncoderProcessConfig *cfg = audio_synthesis_config_create(
      patch_directory,
      patch_file,
      samplerate,
      channels,
      max_push_msgs,
      &audio_synth_status,
      pipe_out
  );

  mu_assert(cfg != NULL, "Could not create audio synthesis process config");

  audio_synthesis_config_destroy(cfg);
  rb_destroy(pipe_out);
  return NULL;
}

char *all_tests() {
  mu_suite_start();

  mu_run_test(test_audio_synthesis_config_create);

  return NULL;
}

RUN_TESTS(all_tests);
