#include <stdlib.h>

#include <bclib/dbg.h>

#include "audio_buffer.h"
#include "logging.h"

AudioBuffer *audio_buffer_create(int channels, int size) {
  AudioBuffer *audio = malloc(sizeof(AudioBuffer));
  check_mem(audio);
  audio->size = size;
  audio->channels = channels;

  audio->buffers = malloc(sizeof(float *) * channels);
  check_mem(audio->buffers);
  for (int c = 0; c < channels; c++) {
    if (size > 0) {
      audio->buffers[c] = calloc(size, sizeof(float *));
      check_mem(audio->buffers[c]);
    } else {
      audio->buffers[c] = NULL;
    }
  }

  return audio;
error:
  return NULL;
}

float *interleaved_audio(int channels, int size) {
  int total_length = channels * size;
  float *audio = calloc(total_length, sizeof(float *));
  return audio;
}

AudioBuffer *audio_buffer_from_float(float *audio, int channels, int size) {
  int chanlen = size / channels;
  AudioBuffer *ab = audio_buffer_create(channels, chanlen);
  check_mem(ab);
  for (int i = 0; i < channels; i++) {
    for (int j = 0; j < chanlen; j++) {
      int pos = (j * channels) + i;
      ab->buffers[i][j] = audio[pos];
    }
  }
  return ab;
error:
  return NULL;
}

float *audio_buffer_to_float(AudioBuffer *buffer) {
  int size = buffer->size * buffer->channels;
  float *audio = malloc(size * sizeof(float));
  check_mem(audio);

  for (int i = 0; i < buffer->channels; i++) {
    for (int j = 0; j < buffer->size; j++) {
      int pos = (j * buffer->channels) + i;
      audio[pos] = buffer->buffers[i][j];
    }
  }
  return audio;
error:
  return NULL;
}

void audio_buffer_destroy(AudioBuffer *audio) {
  check(audio != NULL, "Invalid audio");
  check(audio->buffers != NULL, "Invalid audio buffers");
  for (int c = 0; c < audio->channels; c++) {
    check(audio->buffers[c] != NULL, "Invalid audio buffer #%d", c);
    free(audio->buffers[c]);
  }
  free(audio->buffers);
  free(audio);
  return;
error:
  debug("Error cleaning up audio");
}
