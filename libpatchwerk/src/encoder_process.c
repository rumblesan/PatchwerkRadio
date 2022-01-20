#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include "ck_ring.h"

#include "encoder_process.h"

#include "filechunk.h"
#include "logging.h"
#include "messages.h"
#include "ogg_encoder.h"

#include "bclib/dbg.h"

EncoderProcessConfig *
encoder_config_create(int channels, int samplerate, int format, double quality,
                      int thread_sleep, int max_push_msgs, int *status_var,
                      ck_ring_t *pipe_in, ck_ring_buffer_t *pipe_in_buffer,
                      ck_ring_t *pipe_out, ck_ring_buffer_t *pipe_out_buffer) {

  EncoderProcessConfig *cfg = malloc(sizeof(EncoderProcessConfig));
  check_mem(cfg);

  check(pipe_in != NULL, "Invalid msg in queue passed");
  cfg->pipe_in = pipe_in;
  check(pipe_in_buffer != NULL, "Invalid msg in ring buffer passed");
  cfg->pipe_in_buffer = pipe_in_buffer;

  check(pipe_out != NULL, "Invalid msg out queue passed");
  cfg->pipe_out = pipe_out;
  check(pipe_out_buffer != NULL, "Invalid msg out ring buffer passed");
  cfg->pipe_out_buffer = pipe_out_buffer;

  cfg->channels = channels;
  cfg->samplerate = samplerate;
  cfg->format = format;
  cfg->quality = quality;

  cfg->status_var = status_var;

  cfg->thread_sleep = thread_sleep;
  cfg->max_push_msgs = max_push_msgs;

  return cfg;
error:
  return NULL;
}

void encoder_config_destroy(EncoderProcessConfig *cfg) { free(cfg); }

EncoderState waiting_for_file_state(EncoderProcessConfig *cfg,
                                    OggEncoderState **encoderP,
                                    Message *input_msg) {

  if (input_msg->type == NEWPATCH) {
    logger("Encoder", "New Patch received. Creating new encoder");

    OggEncoderState *new_encoder =
        ogg_encoder_state(cfg->channels, cfg->samplerate, cfg->quality);
    check(new_encoder != NULL, "Could not create new encoder state");
    *encoderP = new_encoder;

    PatchInfo *tinfo = input_msg->payload;
    check(tinfo != NULL, "Could not get patch info from message");

    set_metadata(new_encoder, tinfo);

    FileChunk *headers = file_chunk_create();
    check(headers != NULL, "Could not create headers file chunk");
    write_headers(new_encoder, headers);
    Message *output_msg = file_chunk_message(headers);
    check(output_msg != NULL, "Could not create headers message");

    if (!ck_ring_enqueue_spsc(cfg->pipe_out, cfg->pipe_out_buffer,
                              output_msg)) {
      err_logger("Encoder", "Could not send NewPatch message");
    }

    message_destroy(input_msg);
    logger("Encoder", "Changing to encoding state");
    return ENCODINGFILE;
  } else if (input_msg->type == STREAMFINISHED) {
    logger("Encoder", "Stream Finished message received");
    message_destroy(input_msg);
    return CLOSINGSTREAM;
  } else {
    err_logger("Encoder",
               "Received message of type %s but waiting for new patch",
               msg_type(input_msg));
    message_destroy(input_msg);
    return ENCODERERROR;
  }
error:
  return ENCODERERROR;
}

EncoderState encoding_file_state(EncoderProcessConfig *cfg,
                                 OggEncoderState **encoderP,
                                 Message *input_msg) {
  FileChunk *audio_data = NULL;
  Message *output_msg = NULL;

  check(encoderP != NULL, "Invalid encoder passed");
  OggEncoderState *encoder = *encoderP;
  check(encoder != NULL, "Invalid encoder passed");

  if (input_msg->type == AUDIOBUFFER) {
    AudioBuffer *audio = input_msg->payload;
    check(audio != NULL, "Received invalid audio");
    add_audio(encoder, audio);

    audio_data = file_chunk_create();
    check(audio_data != NULL, "Could not create audio data");
    write_audio(encoder, audio_data);

    if (audio_data->data == NULL) {
      free(audio_data);
    } else {
      output_msg = file_chunk_message(audio_data);
      check(output_msg != NULL, "Could not create audio message");
      if (!ck_ring_enqueue_spsc(cfg->pipe_out, cfg->pipe_out_buffer,
                                output_msg)) {
        err_logger("Encoder", "Could not send AudioBuffer message");
      }
    }

    message_destroy(input_msg);
    return ENCODINGFILE;

  } else if (input_msg->type == STREAMFINISHED) {
    logger("Encoder", "Stream Finished message received");
    message_destroy(input_msg);
    return CLOSINGSTREAM;
  } else if (input_msg->type == PATCHFINISHED) {
    logger("Encoder", "Patch Finished message received");

    // We need to make sure the ogg stream is emptied
    file_finished(encoder);
    audio_data = file_chunk_create();
    check(audio_data != NULL, "Could not create audio data");

    bool oggfinished = false;
    while (!oggfinished) {
      oggfinished = write_audio(encoder, audio_data);
    }

    if (audio_data->data == NULL) {
      free(audio_data);
    } else {
      output_msg = file_chunk_message(audio_data);
      check(output_msg != NULL, "Could not create audio message");
      if (!ck_ring_enqueue_spsc(cfg->pipe_out, cfg->pipe_out_buffer,
                                output_msg)) {
        err_logger("Encoder", "Could not send PatchFinished message");
      }
    }

    cleanup_encoder(encoder);
    *encoderP = NULL;
    message_destroy(input_msg);
    logger("Encoder", "Changing to waiting for file state");
    return WAITINGFORFILE;
  } else {
    err_logger("Encoder",
               "Received invalid %s message whilst in encoding state",
               msg_type(input_msg));
    message_destroy(input_msg);
    return ENCODERERROR;
  }

error:
  return ENCODERERROR;
}

void *start_encoder(void *_cfg) {
  EncoderProcessConfig *cfg = _cfg;

  OggEncoderState *encoder = NULL;

  EncoderState state = WAITINGFORFILE;
  int pushed_msgs = 0;

  check(cfg != NULL, "Encoder: Invalid config data passed");

  struct timespec tim, tim2;
  tim.tv_sec = 0;
  tim.tv_nsec = cfg->thread_sleep;

  logger("Encoder", "Starting");
  *(cfg->status_var) = 1;
  bool running = true;

  Message *input_msg = NULL;

  while (running) {

    if (state == CLOSINGSTREAM || state == ENCODERERROR) {
      running = false;
    }

    if (ck_ring_size(cfg->pipe_in) > 0 &&
        ck_ring_size(cfg->pipe_out) < (ck_ring_capacity(cfg->pipe_out) - 1) &&
        pushed_msgs < cfg->max_push_msgs) {
      pushed_msgs += 1;

      check(ck_ring_dequeue_spsc(cfg->pipe_in, cfg->pipe_in_buffer, &input_msg),
            "Encoder: Could not get input message from queue");
      check(input_msg != NULL, "Encoder: Could not get input message");

      switch (state) {
      case WAITINGFORFILE:
        state = waiting_for_file_state(cfg, &encoder, input_msg);
        break;
      case ENCODINGFILE:
        state = encoding_file_state(cfg, &encoder, input_msg);
        break;
      case CLOSINGSTREAM:
        running = false;
        break;
      case ENCODERERROR:
        running = false;
        break;
      }

    } else {
      pushed_msgs = 0;
      sched_yield();
      nanosleep(&tim, &tim2);
    }
  }

error:
  logger("Encoder", "Finished");
  *(cfg->status_var) = 0;
  if (cfg != NULL)
    encoder_config_destroy(cfg);
  if (encoder != NULL)
    cleanup_encoder(encoder);
  logger("Encoder", "Cleaned up");
  return NULL;
}
