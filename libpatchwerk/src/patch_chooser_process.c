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
  start = clock();

  logger("PatchChooser", "Started");
  bstring new_patch = NULL;

  while (true) {
    now = clock();
    elapsed_seconds = ((double)now - start) / CLOCKS_PER_SEC;
    if (elapsed_seconds > cfg->thread_sleep_seconds) {
      new_patch = get_random_file(cfg->pattern);
      logger("PatchChooser", "next patch: %s", bdata(new_patch));
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
