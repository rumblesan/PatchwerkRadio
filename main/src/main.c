#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <sndfile.h>

#include "audio_synthesis_process.h"
#include "broadcast_process.h"
#include "config.h"
#include "encoder_process.h"
#include "logging.h"
#include "messages.h"
#include "patch_chooser_process.h"
#include "pipe_utils.h"

#include "bclib/bstrlib.h"
#include "bclib/dbg.h"
#include "ck_ring.h"

int main(int argc, char *argv[]) {

  RadioInputCfg *radio_config = NULL;

  PatchChooserProcessConfig *patch_chooser_cfg = NULL;
  AudioSynthesisProcessConfig *audio_synth_cfg = NULL;
  EncoderProcessConfig *encoder_cfg = NULL;
  BroadcastProcessConfig *broadcast_cfg = NULL;

  pthread_t patch_chooser_thread;
  pthread_attr_t patch_chooser_thread_attr;
  pthread_t audio_synth_thread;
  pthread_attr_t audio_synth_thread_attr;
  pthread_t encoder_thread;
  pthread_attr_t encoder_thread_attr;
  pthread_t broadcast_thread;
  pthread_attr_t broadcast_thread_attr;
  int patch_chooser_status = 0;
  int audio_synth_status = 0;
  int encoder_status = 0;
  int broadcast_status = 0;

  ck_ring_t *chooser2audio = NULL;
  ck_ring_buffer_t *chooser2audio_buffer = NULL;

  ck_ring_t *audio2encode = NULL;
  ck_ring_buffer_t *audio2encode_buffer = NULL;

  ck_ring_t *encode2broadcast = NULL;
  ck_ring_buffer_t *encode2broadcast_buffer = NULL;

  startup_log("PatchwerkRadio", "Hello, Patchwerk Radio");
  time_t t = time(NULL);
  logger("PatchwerkRadio", "%s", asctime(gmtime(&t)));
  srand((unsigned)t);

  check(argc == 2, "Need to give config file path argument");

  char *config_path = argv[1];

  radio_config = read_config(config_path);
  check(radio_config != NULL, "Could not read config file");

  check(create_pipe(&chooser2audio, &chooser2audio_buffer, 16),
        "Could not create patch chooser to audio ring buffer");
  check(create_pipe(&audio2encode, &audio2encode_buffer, 32),
        "Could not create audio to encoder ring buffer");
  check(create_pipe(&encode2broadcast, &encode2broadcast_buffer, 32),
        "Could not create audio to encoder ring buffer");

  patch_chooser_cfg = patch_chooser_config_create(
      radio_config->chooser.pattern, radio_config->chooser.play_time,
      radio_config->chooser.filenumber,
      radio_config->chooser.thread_sleep_seconds, &patch_chooser_status,
      chooser2audio, chooser2audio_buffer);
  check(patch_chooser_cfg != NULL,
        "Couldn't create patch chooser process config");

  audio_synth_cfg = audio_synthesis_config_create(
      radio_config->audio.samplerate, radio_config->audio.channels,
      radio_config->audio.fadetime, radio_config->system.max_push_messages,
      &audio_synth_status, chooser2audio, chooser2audio_buffer, audio2encode,
      audio2encode_buffer);
  check(audio_synth_cfg != NULL, "Couldn't create audio synth process config");

  encoder_cfg = encoder_config_create(
      radio_config->audio.channels, radio_config->audio.samplerate,
      SF_FORMAT_OGG | SF_FORMAT_VORBIS, radio_config->encoder.quality,
      radio_config->system.thread_sleep, radio_config->system.max_push_messages,
      &encoder_status, audio2encode, audio2encode_buffer, encode2broadcast,
      encode2broadcast_buffer);
  check(encoder_cfg != NULL, "Couldn't create encoder process config");

  broadcast_cfg = broadcast_config_create(
      radio_config->broadcast.host, radio_config->broadcast.port,
      radio_config->broadcast.source, radio_config->broadcast.password,
      radio_config->broadcast.mount, radio_config->broadcast.name,
      radio_config->broadcast.description, radio_config->broadcast.genre,
      radio_config->broadcast.url, SHOUT_PROTOCOL_HTTP, SHOUT_FORMAT_OGG,
      &broadcast_status, encode2broadcast, encode2broadcast_buffer);
  check(broadcast_cfg != NULL, "Couldn't create broadcast process config");

  check(!pthread_attr_init(&patch_chooser_thread_attr),
        "Error setting patch chooser thread attributes");
  check(!pthread_attr_setdetachstate(&patch_chooser_thread_attr,
                                     PTHREAD_CREATE_DETACHED),
        "Error setting patch chooser thread detach state");
  check(!pthread_create(&patch_chooser_thread, &patch_chooser_thread_attr,
                        &start_patch_chooser, patch_chooser_cfg),
        "Error creating audio synth thread");

  check(!pthread_attr_init(&audio_synth_thread_attr),
        "Error setting audio synth thread attributes");
  check(!pthread_attr_setdetachstate(&audio_synth_thread_attr,
                                     PTHREAD_CREATE_DETACHED),
        "Error setting audio synth thread detach state");
  check(!pthread_create(&audio_synth_thread, &audio_synth_thread_attr,
                        &start_audio_synthesis, audio_synth_cfg),
        "Error creating audio synth thread");

  check(!pthread_attr_init(&encoder_thread_attr),
        "Error setting encoder thread attributes");
  check(!pthread_attr_setdetachstate(&encoder_thread_attr,
                                     PTHREAD_CREATE_DETACHED),
        "Error setting encoder thread detach state");
  check(!pthread_create(&encoder_thread, &encoder_thread_attr, &start_encoder,
                        encoder_cfg),
        "Error creating encoder thread");

  check(!pthread_attr_init(&broadcast_thread_attr),
        "Error setting broadcast thread attributes");
  check(!pthread_attr_setdetachstate(&broadcast_thread_attr,
                                     PTHREAD_CREATE_DETACHED),
        "Error setting broadcast thread detach state");
  check(!pthread_create(&broadcast_thread, &broadcast_thread_attr,
                        &start_broadcast, broadcast_cfg),
        "Error creating broadcasting thread");

  int ch2as_msgs = 0;
  int as2enc_msgs = 0;
  int enc2brd_msgs = 0;
  while (1) {
    sleep(radio_config->system.stats_interval);
    if (patch_chooser_status == 0) {
      err_logger("SlowRadio", "Stopped Patch Chooser!");
      break;
    }
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
    ch2as_msgs = ck_ring_size(chooser2audio);
    as2enc_msgs = ck_ring_size(audio2encode);
    enc2brd_msgs = ck_ring_size(encode2broadcast);
    logger("SlowRadio",
           "Messages: chooser %d audio synth %d encoder %d broadcast",
           ch2as_msgs, as2enc_msgs, enc2brd_msgs);
  }

  logger("SlowRadio", "Stopping");
error:
  logger("SlowRadio", "Cleaning up");
  if (radio_config != NULL)
    destroy_config(radio_config);

  cleanup_pipe(chooser2audio, chooser2audio_buffer, "Chooser to Audio");
  cleanup_pipe(audio2encode, audio2encode_buffer, "Audio to Encode");
  cleanup_pipe(encode2broadcast, encode2broadcast_buffer,
               "Encode to Broadcast");

  if (patch_chooser_cfg != NULL)
    patch_chooser_config_destroy(patch_chooser_cfg);
  if (audio_synth_cfg != NULL)
    audio_synthesis_config_destroy(audio_synth_cfg);
  if (encoder_cfg != NULL)
    encoder_config_destroy(encoder_cfg);
  if (broadcast_cfg != NULL)
    broadcast_config_destroy(broadcast_cfg);

  // Always exit with non-zero as this is meant to be
  // a never-ending process
  return 1;
}
