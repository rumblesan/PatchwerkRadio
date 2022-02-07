#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <ck_ring.h>

#include <bclib/bstrlib.h>
#include <bclib/dbg.h>

#include "patch_chooser_process.h"

#include "file_utils.h"
#include "logging.h"
#include "messages.h"

PatchChooserProcessConfig *patch_chooser_config_create(
    bstring pattern, double play_time, int filenumber, int thread_sleep_seconds,
    int *status_var, ck_ring_t *pipe_out, ck_ring_buffer_t *pipe_out_buffer) {

  PatchChooserProcessConfig *cfg = malloc(sizeof(PatchChooserProcessConfig));
  check_mem(cfg);

  check(pattern != NULL, "PatchChooser: Invalid patch search pattern");
  cfg->pattern = pattern;

  cfg->filenumber = filenumber;
  cfg->thread_sleep_seconds = thread_sleep_seconds;
  cfg->play_time = play_time;

  check(status_var != NULL, "PatchChooser: Invalid status var");
  cfg->status_var = status_var;

  check(pipe_out != NULL, "PatchChooser: Invalid pipe out");
  cfg->pipe_out = pipe_out;
  check(pipe_out_buffer != NULL, "PatchChooser: Invalid pipe out buffer");
  cfg->pipe_out_buffer = pipe_out_buffer;

  return cfg;
error:
  return NULL;
}

void patch_chooser_config_destroy(PatchChooserProcessConfig *cfg) {
  check(cfg != NULL, "PatchChooser: Invalid cfg");
  free(cfg);
  return;
error:
  err_logger("PatchChooser", "Could not destroy cfg");
}

void *start_patch_chooser(void *_cfg) {

  PatchChooserProcessConfig *cfg = _cfg;

  *(cfg->status_var) = 1;

  struct timespec tim, tim2;
  tim.tv_sec = cfg->thread_sleep_seconds;
  tim.tv_nsec = 0;

  clock_t start, now;
  double elapsed_seconds;
  start = -(cfg->thread_sleep_seconds *
            CLOCKS_PER_SEC); // start at negative so we immediately
                             // load a new patch

  logger("PatchChooser", "Started");

  check(true, "stand in check");

  while (true) {
    now = clock();
    elapsed_seconds = ((double)now - start) / CLOCKS_PER_SEC;
    if (elapsed_seconds > cfg->thread_sleep_seconds &&
        ck_ring_size(cfg->pipe_out) < (ck_ring_capacity(cfg->pipe_out) - 1)) {
      bstring next_patch_path = get_random_patch(cfg->pattern);
      logger("PatchChooser", "next patch path %s", bdata(next_patch_path));
      PatchInfo *pi = path_to_patchinfo(next_patch_path);
      logger("PatchChooser", "next patch: %s - %s", bdata(pi->creator),
             bdata(pi->title));
      Message *msg = load_patch_message(pi);
      if (!ck_ring_enqueue_spsc(cfg->pipe_out, cfg->pipe_out_buffer, msg)) {
        err_logger("PatchChooser", "Could not send New Patch message");
        err_logger("PatchChooser", "size %d    capacity %d",
                   ck_ring_size(cfg->pipe_out),
                   ck_ring_capacity(cfg->pipe_out));
        message_destroy(msg);
      }

      start = clock();
    }
    sched_yield();
    nanosleep(&tim, &tim2);
  }

error:
  logger("PatchChooser", "Finished");
  *(cfg->status_var) = 0;
  patch_chooser_config_destroy(cfg);
  logger("PatchChooser", "Cleaned up");
  pthread_exit(NULL);
  return NULL;
}
