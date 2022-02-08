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
    int samplerate, int channels, double fadetime, int max_push_msgs,
    int *status_var, ck_ring_t *pipe_in, ck_ring_buffer_t *pipe_in_buffer,
    ck_ring_t *pipe_out, ck_ring_buffer_t *pipe_out_buffer) {

  AudioSynthesisProcessConfig *cfg =
      malloc(sizeof(AudioSynthesisProcessConfig));
  check_mem(cfg);

  cfg->patch_file = NULL;

  cfg->blocksize = 0;

  cfg->fadetime = fadetime;
  cfg->fadeamount = 0.0;
  cfg->fadedelta = (1.0 / ((double)cfg->fadetime * (double)samplerate));
  cfg->state = AS_WAITING_FOR_PATCH;
  cfg->next_patch_msg = NULL;

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
  check(cfg != NULL, "Invalid config");
  free(cfg);
  return;
error:
  err_logger("AudioSynthesis", "Could not destroy cfg");
}

int handle_input_message(AudioSynthesisProcessConfig *cfg, Message *input_msg) {
  logger("AudioSynthesis", "Incoming message");
  check(cfg != NULL, "Invalid config");
  check(input_msg != NULL, "Invalid input message");

  if (input_msg->type != LOADPATCH) {
    err_logger("AudioSynthesis", "Can't handle message type %s",
               msg_type(input_msg));
    message_destroy(input_msg);
    return 0;
  }

  logger("AudioSynthesis", "Saved load patch message");
  cfg->next_patch_msg = input_msg;
  cfg->state = AS_FADING_OUT;

  return 0;
error:
  if (input_msg != NULL) {
    message_destroy(input_msg);
  }
  return 1;
}

int load_next_patch(AudioSynthesisProcessConfig *cfg) {
  if (cfg->patch_file != NULL) {
    libpd_closefile(cfg->patch_file);
    // only send patch finished message if there was a previous patch loaded
    Message *patch_finished_msg = patch_finished_message();
    check(ck_ring_enqueue_spsc(cfg->pipe_out, cfg->pipe_out_buffer,
                               patch_finished_msg),
          "Could not send patch finished message");
  }

  PatchInfo *pinfo = cfg->next_patch_msg->payload;
  logger("AudioSynthesis", "Loading file %s", bdata(pinfo->filepath));
  void *pd_file = libpd_openfile(bdata(pinfo->filepath), "./");
  check(pd_file != NULL, "Could not open pd patch: ./%s",
        bdata(pinfo->filepath));
  cfg->patch_file = pd_file;

  CreatorInfo *info =
      creator_info_create(bstrcpy(pinfo->creator), bstrcpy(pinfo->title));
  Message *new_patch_msg = new_patch_message(info);
  logger("AudioSynthesis", "Sent new patch message");
  check(
      ck_ring_enqueue_spsc(cfg->pipe_out, cfg->pipe_out_buffer, new_patch_msg),
      "Could not send new patch message");

  message_destroy(cfg->next_patch_msg);
  cfg->next_patch_msg = NULL;
  cfg->state = AS_FADING_IN;

  return 0;
error:
  if (cfg->next_patch_msg != NULL) {
    message_destroy(cfg->next_patch_msg);
  }
  cfg->state = AS_WAITING_FOR_PATCH;
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

  bool waiting = true;
  while (waiting) {

    if (ck_ring_size(cfg->pipe_in) > 0 &&
        ck_ring_dequeue_spsc(cfg->pipe_in, cfg->pipe_in_buffer, &input_msg)) {
      if (handle_input_message(cfg, input_msg) == 0) {
        logger("AudioSynthesis", "first patch loaded");
        load_next_patch(cfg);
        waiting = false;
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

void fade_audio_out(AudioSynthesisProcessConfig *cfg, AudioBuffer *audio) {
  double delta = cfg->fadedelta;
  double fadeamount = cfg->fadeamount;
  for (int j = 0; j < audio->size; j++) {
    for (int i = 0; i < audio->channels; i++) {
      audio->buffers[i][j] = audio->buffers[i][j] * fadeamount;
    }
    fadeamount -= delta;
  }
  cfg->fadeamount = fadeamount;
  if (cfg->fadeamount <= 0.0) {
    cfg->fadeamount = 0.0;
    cfg->state = AS_LOADING_NEXT_PATCH;
    logger("AudioSynthesis", "loading next patch");
  }
}

void fade_audio_in(AudioSynthesisProcessConfig *cfg, AudioBuffer *audio) {
  double delta = cfg->fadedelta;
  double fadeamount = cfg->fadeamount;
  for (int j = 0; j < audio->size; j++) {
    for (int i = 0; i < audio->channels; i++) {
      audio->buffers[i][j] = audio->buffers[i][j] * fadeamount;
    }
    fadeamount += delta;
  }
  cfg->fadeamount = fadeamount;
  if (cfg->fadeamount >= 1.0) {
    cfg->fadeamount = 1.0;
    cfg->state = AS_PLAYING;
    logger("AudioSynthesis", "finished fading in");
  }
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

      check(!libpd_process_float(1, in_buffer, out_buffer),
            "PD could not create audio");
      AudioBuffer *out_audio = audio_buffer_from_float(
          out_buffer, cfg->channels, cfg->channels * cfg->blocksize);

      if (cfg->state == AS_FADING_OUT) {
        fade_audio_out(cfg, out_audio);
      } else if (cfg->state == AS_FADING_IN) {
        fade_audio_in(cfg, out_audio);
      }

      Message *msg = audio_buffer_message(out_audio);
      if (!ck_ring_enqueue_spsc(cfg->pipe_out, cfg->pipe_out_buffer, msg)) {
        err_logger("AudioSynthesis", "Could not send Audio message");
        message_destroy(msg);
      }

      if (cfg->state == AS_LOADING_NEXT_PATCH) {
        logger("AudioSynthesis", "loading next patch");
        load_next_patch(cfg);
      }

    } else if (ck_ring_size(cfg->pipe_in) > 0) {
      check(ck_ring_dequeue_spsc(cfg->pipe_in, cfg->pipe_in_buffer, &input_msg),
            "Could not get input message from queue");
      handle_input_message(cfg, input_msg);
    } else {
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
