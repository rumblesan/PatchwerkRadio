#include "messages.h"
#include "minunit.h"
#include "patch_chooser_process.h"
#include "pipe_utils.h"

#include <bclib/bstrlib.h>
#include <bclib/dbg.h>
#include <bclib/ringbuffer.h>

char *test_patch_chooser_config_create() {
  bstring patch_file_pattern = bfromcstr("somepatternhere");
  double play_time = 20;
  int filenumber = -1;
  int queue_size = 1024;
  int thread_sleep_seconds = 1;
  int patch_chooser_status;

  ck_ring_t *pipe_out = NULL;
  ck_ring_buffer_t *pipe_out_buffer = NULL;

  create_pipe(&pipe_out, &pipe_out_buffer, queue_size);

  PatchChooserProcessConfig *cfg = patch_chooser_config_create(
      patch_file_pattern, play_time, filenumber, thread_sleep_seconds,
      &patch_chooser_status, pipe_out, pipe_out_buffer);

  mu_assert(cfg != NULL, "Could not create patch chooser process config");

  patch_chooser_config_destroy(cfg);

  cleanup_pipe(pipe_out, pipe_out_buffer, "pipeout");
  return NULL;
}

char *all_tests() {
  mu_suite_start();

  mu_run_test(test_patch_chooser_config_create);

  return NULL;
}

RUN_TESTS(all_tests);
