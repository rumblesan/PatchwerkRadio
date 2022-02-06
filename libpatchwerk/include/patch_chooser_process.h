#pragma once

#include <ck_ring.h>

#include <bclib/bstrlib.h>


typedef struct PatchChooserProcessConfig {

  bstring pattern;

  // number of files to read before quitting
  // mostly used for testing
  // -1 means forever
  int filenumber;

  int thread_sleep_seconds;
  double play_time;

  int *status_var;

  ck_ring_t *pipe_out;
  ck_ring_buffer_t *pipe_out_buffer;

} PatchChooserProcessConfig;

PatchChooserProcessConfig *patch_chooser_config_create(
                                                  bstring pattern,
                                                  double play_time,
                                                  int filenumber,
                                                  int thread_sleep_seconds,
                                                  int *status_var,
                                                  ck_ring_t *pipe_out,
                                                  ck_ring_buffer_t *pipe_out_buffer);

void patch_chooser_config_destroy(PatchChooserProcessConfig *cfg);

void *start_patch_chooser(void *_cfg);
