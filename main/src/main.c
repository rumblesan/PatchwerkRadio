#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

#include <sndfile.h>

#include "audio_synthesis_process.h"
#include "encoder_process.h"
#include "broadcast_process.h"
#include "config.h"
#include "logging.h"
#include "messages.h"

#include "bclib/dbg.h"
#include "bclib/bstrlib.h"

int cleanup_pipe(RingBuffer *pipe, const char *pipename) {
  Message *pipe_msg = NULL;
  check(pipe != NULL, "Invalid pipe passed in");
  while (!rb_empty(pipe)) {
    pipe_msg = rb_pop(pipe);
    check(pipe_msg != NULL, "Null message whilst emptying %s pipe", pipename);
    message_destroy(pipe_msg);
  }
  rb_destroy(pipe);
  return 0;
 error:
  return 1;
}

int main (int argc, char *argv[]) {

  RadioInputCfg *radio_config = NULL;

  RingBuffer *audio2encode = NULL;
  RingBuffer *encode2broadcast = NULL;

  AudioSynthesisProcessConfig *audio_synth_cfg = NULL;
  EncoderProcessConfig *encoder_cfg = NULL;
  BroadcastProcessConfig *broadcast_cfg = NULL;

  pthread_t audio_synth_thread;
  pthread_attr_t audio_synth_thread_attr;
  pthread_t encoder_thread;
  pthread_attr_t encoder_thread_attr;
  pthread_t broadcast_thread;
  pthread_attr_t broadcast_thread_attr;
  int audio_synth_status = 0;
  int encoder_status = 0;
  int broadcast_status = 0;

  startup_log("PatchwerkRadio", "Hello, Patchwerk Radio");

  check(argc == 2, "Need to give config file path argument");

  char *config_path = argv[1];

  radio_config = read_config(config_path);
  check(radio_config != NULL, "Could not read config file");

  audio2encode = rb_create(100);
  check(audio2encode != NULL, "Couldn't create coms from audio synthesis to encoder");
  encode2broadcast = rb_create(100);
  check(encode2broadcast != NULL, "Couldn't create coms from encoder to broadcaster");

  audio_synth_cfg = audio_synthesis_config_create(
                                          radio_config->puredata.patch_directory,
                                          radio_config->puredata.patch_file,
                                          radio_config->audio.samplerate,
                                          radio_config->audio.channels,
                                          radio_config->system.max_push_messages,
                                          &audio_synth_status,
                                          audio2encode);
  check(audio_synth_cfg != NULL, "Couldn't create stretcher process config");

  encoder_cfg = encoder_config_create(radio_config->audio.channels,
                                      radio_config->audio.samplerate,
                                      SF_FORMAT_OGG | SF_FORMAT_VORBIS,
                                      radio_config->encoder.quality,
                                      radio_config->system.thread_sleep,
                                      radio_config->system.max_push_messages,
                                      &encoder_status,
                                      audio2encode, encode2broadcast);
  check(encoder_cfg != NULL, "Couldn't create encoder process config");

  broadcast_cfg = broadcast_config_create(radio_config->broadcast.host,
                                          radio_config->broadcast.port,
                                          radio_config->broadcast.source,
                                          radio_config->broadcast.password,
                                          radio_config->broadcast.mount,
                                          radio_config->broadcast.name,
                                          radio_config->broadcast.description,
                                          radio_config->broadcast.genre,
                                          radio_config->broadcast.url,
                                          SHOUT_PROTOCOL_HTTP,
                                          SHOUT_FORMAT_OGG,
                                          &broadcast_status,
                                          encode2broadcast);
  check(broadcast_cfg != NULL, "Couldn't create broadcast process config");


  check(!pthread_attr_init(&audio_synth_thread_attr),
        "Error setting audio synth thread attributes");
  check(!pthread_attr_setdetachstate(&audio_synth_thread_attr, PTHREAD_CREATE_DETACHED),
        "Error setting audio synth thread detach state");
  check(!pthread_create(&audio_synth_thread,
                        &audio_synth_thread_attr,
                        &start_audio_synthesis,
                        audio_synth_cfg),
        "Error creating stretcher thread");

  check(!pthread_attr_init(&encoder_thread_attr),
        "Error setting encoder thread attributes");
  check(!pthread_attr_setdetachstate(&encoder_thread_attr, PTHREAD_CREATE_DETACHED),
        "Error setting encoder thread detach state");
  check(!pthread_create(&encoder_thread,
                        &encoder_thread_attr,
                        &start_encoder,
                        encoder_cfg),
        "Error creating encoder thread");

  check(!pthread_attr_init(&broadcast_thread_attr),
        "Error setting broadcast thread attributes");
  check(!pthread_attr_setdetachstate(&broadcast_thread_attr, PTHREAD_CREATE_DETACHED),
        "Error setting broadcast thread detach state");
  check(!pthread_create(&broadcast_thread,
                        &broadcast_thread_attr,
                        &start_broadcast,
                        broadcast_cfg),
        "Error creating broadcasting thread");

  int as2enc_msgs = 0;
  int enc2brd_msgs = 0;
  while (1) {
    sleep(radio_config->system.stats_interval);
    if (audio_synth_status == 0) {
      err_logger("SlowRadio", "Stopped Synthesising!");
      break;
    }
    if (encoder_status == 0) {
      err_logger("SlowRadio", "Stopped Encoding!");
      break;
    }
    if (broadcast_status == 0) {
      err_logger("SlowRadio", "Stopped Broadcasting!");
      break;
    }
    as2enc_msgs = rb_size(audio2encode);
    enc2brd_msgs = rb_size(encode2broadcast);
    logger("SlowRadio", "Messages: audio synth %d encoder %d broadcast", as2enc_msgs, enc2brd_msgs);
  }

  logger("SlowRadio", "Stopping");
 error:
  logger("SlowRadio", "Cleaning up");
  if (radio_config != NULL) destroy_config(radio_config);

  cleanup_pipe(audio2encode, "Audio to Encode");
  cleanup_pipe(encode2broadcast, "Encode to Broadcast");

  if (audio_synth_cfg != NULL) audio_synthesis_config_destroy(audio_synth_cfg);
  if (encoder_cfg != NULL) encoder_config_destroy(encoder_cfg);
  if (broadcast_cfg != NULL) broadcast_config_destroy(broadcast_cfg);

  // Always exit with non-zero as this is meant to be
  // a never-ending process
  return 1;
}
