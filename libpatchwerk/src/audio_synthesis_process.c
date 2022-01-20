#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "audio_synthesis_process.h"

#include "logging.h"
#include "messages.h"

#include <bclib/dbg.h>
#include <ck_ring.h>
#include <libpd/z_libpd.h>

AudioSynthesisProcessConfig *
audio_synthesis_config_create(bstring patch_directory, bstring patch_file,
                              int samplerate, int channels, int max_push_msgs,
                              int *status_var, ck_ring_t *pipe_out,
                              ck_ring_buffer_t *pipe_out_buffer) {

  AudioSynthesisProcessConfig *cfg =
      malloc(sizeof(AudioSynthesisProcessConfig));
  check_mem(cfg);

  check(patch_directory != NULL, "AudioSynthesis: Invalid patch directory");
  cfg->patch_directory = patch_directory;
  check(patch_file != NULL, "AudioSynthesis: Invalid patch file");
  cfg->patch_file = patch_file;

  check(samplerate > 0, "AudioSynthesis: Invalid samplerate");
  cfg->samplerate = samplerate;
  check(channels > 0, "AudioSynthesis: Invalid channel count");
  cfg->channels = channels;

  check(max_push_msgs > 0, "AudioSynthesis: Invalid max push messages");
  cfg->max_push_msgs = max_push_msgs;

  check(status_var != NULL, "AudioSynthesis: Invalid status var");
  cfg->status_var = status_var;

  check(pipe_out != NULL, "AudioSynthesis: Invalid pipe out ring");
  cfg->pipe_out = pipe_out;
  check(pipe_out_buffer != NULL,
        "AudioSynthesis: Invalid pipe out ring buffer");
  cfg->pipe_out_buffer = pipe_out_buffer;

  return cfg;
error:
  return NULL;
}

void audio_synthesis_config_destroy(AudioSynthesisProcessConfig *cfg) {
  check(cfg != NULL, "AudioSynthesis: Invalid cfg");
  if (cfg->patch_directory != NULL) {
    bdestroy(cfg->patch_directory);
  }
  if (cfg->patch_file != NULL) {
    bdestroy(cfg->patch_file);
  }
  free(cfg);
  return;
error:
  err_logger("AudioSynthesis", "Could not destroy cfg");
}

void *start_audio_synthesis(void *_cfg) {

  AudioSynthesisProcessConfig *cfg = _cfg;

  libpd_init();
  void *pd_file =
      libpd_openfile(bdata(cfg->patch_file), bdata(cfg->patch_directory));
  check(pd_file != NULL, "Could not open pd patch: %s/%s",
        bdata(cfg->patch_directory), bdata(cfg->patch_file));
  cfg->pd_file = pd_file;

  int blocksize = libpd_blocksize();
  int pd_init = libpd_init_audio(0, cfg->channels, cfg->samplerate);
  check(pd_init == 0, "Could not initialise PD: %d", pd_init);

  check(!libpd_start_message(16), "Could not allocate space for PD message");
  libpd_add_float(1);
  libpd_finish_message("pd", "dsp");

  float *in_buffer = interleaved_audio(cfg->channels, blocksize);
  float *out_buffer = interleaved_audio(cfg->channels, blocksize);

  int pushed_msgs = 0;

  *(cfg->status_var) = 1;

  struct timespec tim, tim2;
  tim.tv_sec = 0;
  tim.tv_nsec = 10;

  logger("AudioSynthesis", "Started");

  PatchInfo *info =
      patch_info_create(bfromcstr("rumblesan"), bfromcstr("test patch"));
  Message *new_patch_msg = new_patch_message(info);
  check(
      ck_ring_enqueue_spsc(cfg->pipe_out, cfg->pipe_out_buffer, new_patch_msg),
      "Could not send new patch message");

  bool running = true;
  while (running) {
    if (ck_ring_size(cfg->pipe_out) < (ck_ring_capacity(cfg->pipe_out) - 1) &&
        pushed_msgs < cfg->max_push_msgs) {
      pushed_msgs += 1;

      check(true, "stand in check");

      int audio_check = libpd_process_float(1, in_buffer, out_buffer);
      check(audio_check == 0, "PD could not create audio");
      AudioBuffer *out_audio = audio_buffer_from_float(
          out_buffer, cfg->channels, cfg->channels * blocksize);

      Message *msg = audio_buffer_message(out_audio);
      if (!ck_ring_enqueue_spsc(cfg->pipe_out, cfg->pipe_out_buffer, msg)) {
        err_logger("Encoder", "Could not send Audio message");
        err_logger("Encoder", "size %d    capacity %d",
                   ck_ring_size(cfg->pipe_out),
                   ck_ring_capacity(cfg->pipe_out));
      }
    } else {
      if (pushed_msgs > 0) {
        // logger("AudioSynthesis", "Pushed %d messages", pushed_msgs);
      }
      pushed_msgs = 0;
      sched_yield();
      nanosleep(&tim, &tim2);
    }
  }

error:
  logger("AudioSynthesis", "Finished");
  *(cfg->status_var) = 0;
  audio_synthesis_config_destroy(cfg);
  logger("AudioSynthesis", "Cleaned up");
  pthread_exit(NULL);
  return NULL;
}
