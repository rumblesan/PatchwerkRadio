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

AudioSynthesisProcessConfig *audio_synthesis_config_create(
    int samplerate, int channels, int max_push_msgs, int *status_var,
    ck_ring_t *pipe_in, ck_ring_buffer_t *pipe_in_buffer, ck_ring_t *pipe_out,
    ck_ring_buffer_t *pipe_out_buffer) {

  AudioSynthesisProcessConfig *cfg =
      malloc(sizeof(AudioSynthesisProcessConfig));
  check_mem(cfg);

  cfg->pd_ready = false;
  cfg->patch_file = NULL;

  cfg->blocksize = 0;

  check(samplerate > 0, "AudioSynthesis: Invalid samplerate");
  cfg->samplerate = samplerate;
  check(channels > 0, "AudioSynthesis: Invalid channel count");
  cfg->channels = channels;

  check(max_push_msgs > 0, "AudioSynthesis: Invalid max push messages");
  cfg->max_push_msgs = max_push_msgs;

  check(status_var != NULL, "AudioSynthesis: Invalid status var");
  cfg->status_var = status_var;

  check(pipe_in != NULL, "AudioSynthesis: Invalid pipe in ring");
  cfg->pipe_in = pipe_in;
  check(pipe_in_buffer != NULL, "AudioSynthesis: Invalid pipe in ring buffer");
  cfg->pipe_in_buffer = pipe_in_buffer;

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
  check(cfg != NULL, "AudioSynthesis: Invalid config");
  free(cfg);
  return;
error:
  err_logger("AudioSynthesis", "Could not destroy cfg");
}

int handle_input_message(AudioSynthesisProcessConfig *cfg, Message *input_msg) {
  check(cfg != NULL, "AudioSynthesis: Invalid config");
  check(input_msg != NULL, "AudioSynthesis: Invalid input message");

  if (input_msg->type != LOADPATCH) {
    err_logger("AudioSynthesis", "Can't handle message type %s",
               msg_type(input_msg));
    return 0;
  }

  if (cfg->patch_file != NULL) {
    libpd_closefile(cfg->patch_file);
    // only send patch finished message if there was a previous patch loaded
    Message *patch_finished_msg = patch_finished_message();
    check(ck_ring_enqueue_spsc(cfg->pipe_out, cfg->pipe_out_buffer,
                               patch_finished_msg),
          "Could not send patch finished message");
  }

  PatchInfo *pinfo = input_msg->payload;
  logger("AudioSynthesis", "Loading file %s", bdata(pinfo->filepath));
  void *pd_file = libpd_openfile(bdata(pinfo->filepath), "./");
  check(pd_file != NULL, "Could not open pd patch: ./%s",
        bdata(pinfo->filepath));
  cfg->patch_file = pd_file;

  CreatorInfo *info =
      creator_info_create(bstrcpy(pinfo->creator), bstrcpy(pinfo->title));
  Message *new_patch_msg = new_patch_message(info);
  check(
      ck_ring_enqueue_spsc(cfg->pipe_out, cfg->pipe_out_buffer, new_patch_msg),
      "Could not send new patch message");

  message_destroy(input_msg);

  return 0;
error:
  if (input_msg != NULL) {
    message_destroy(input_msg);
  }
  return 1;
}

int setup_pd(AudioSynthesisProcessConfig *cfg) {
  check(cfg != NULL, "AudioSynthesis: Invalid config");
  logger("AudioSynthesis", "initing pd");
  libpd_init();
  logger("AudioSynthesis", "done initing pd");

  int pd_init = libpd_init_audio(0, cfg->channels, cfg->samplerate);
  check(pd_init == 0, "Could not initialise PD audio: %d", pd_init);

  check(!libpd_start_message(16), "Could not allocate space for PD message");
  libpd_add_float(1);
  libpd_finish_message("pd", "dsp");

  cfg->blocksize = libpd_blocksize();
  logger("AudioSynthesis", "got block size: %d", cfg->blocksize);
  return 0;
error:
  return 1;
}

int wait_for_pd_ready(AudioSynthesisProcessConfig *cfg) {
  check(cfg != NULL, "AudioSynthesis: Invalid config");

  struct timespec tim, tim2;
  tim.tv_sec = 1;
  tim.tv_nsec = 0;

  Message *input_msg = NULL;

  while (!cfg->pd_ready) {

    if (ck_ring_size(cfg->pipe_in) > 0 &&
        ck_ring_dequeue_spsc(cfg->pipe_in, cfg->pipe_in_buffer, &input_msg)) {
      if (handle_input_message(cfg, input_msg) == 0) {
        logger("AudioSynthesis", "first patch loaded");
        cfg->pd_ready = 1;
      }
    } else {
      sched_yield();
      nanosleep(&tim, &tim2);
    }
  }

  return 0;
error:
  return 1;
}

void *start_audio_synthesis(void *_cfg) {

  AudioSynthesisProcessConfig *cfg = _cfg;

  *(cfg->status_var) = 1;

  check(!setup_pd(cfg), "Could not setup PureData");
  logger("AudioSynthesis", "PD setup");
  check(!wait_for_pd_ready(cfg), "PureData not ready");

  float *in_buffer = interleaved_audio(cfg->channels, cfg->blocksize);
  float *out_buffer = interleaved_audio(cfg->channels, cfg->blocksize);

  int pushed_msgs = 0;

  struct timespec tim, tim2;
  tim.tv_sec = 0;
  tim.tv_nsec = 2000;

  logger("AudioSynthesis", "Started");

  // free message spaces to leave in output
  // pipe to allow for patch finished messages
  int out_msg_buffer_count = 5;

  Message *input_msg = NULL;
  bool running = true;
  while (running) {
    if (ck_ring_size(cfg->pipe_out) <
            (ck_ring_capacity(cfg->pipe_out) - out_msg_buffer_count) &&
        pushed_msgs < cfg->max_push_msgs) {
      pushed_msgs += 1;

      check(true, "stand in check");

      int audio_check = libpd_process_float(1, in_buffer, out_buffer);
      check(audio_check == 0, "PD could not create audio");
      AudioBuffer *out_audio = audio_buffer_from_float(
          out_buffer, cfg->channels, cfg->channels * cfg->blocksize);

      Message *msg = audio_buffer_message(out_audio);
      if (!ck_ring_enqueue_spsc(cfg->pipe_out, cfg->pipe_out_buffer, msg)) {
        // TODO memory leak!?
        err_logger("Encoder", "Could not send Audio message");
        err_logger("Encoder", "size %d    capacity %d",
                   ck_ring_size(cfg->pipe_out),
                   ck_ring_capacity(cfg->pipe_out));
      }
    } else if (ck_ring_size(cfg->pipe_in) > 0) {
      check(ck_ring_dequeue_spsc(cfg->pipe_in, cfg->pipe_in_buffer, &input_msg),
            "Encoder: Could not get input message from queue");
      logger("AudioSynthesis", "Incoming message");
      handle_input_message(cfg, input_msg);
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
