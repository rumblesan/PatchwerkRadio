#include "minunit.h"

#include "audio_buffer.h"
#include "filechunk.h"
#include "messages.h"

char *test_stream_finish_message_create() {
  Message *message = stream_finished_message();
  mu_assert(message != NULL, "Could not create stream finished message");
  message_destroy(message);
  return NULL;
}

char *test_patch_finish_message_create() {
  Message *message = patch_finished_message();
  mu_assert(message != NULL, "Could not create patch finished message");
  message_destroy(message);
  return NULL;
}

char *test_load_patch_message_create() {
  PatchInfo *patch_info = patch_info_create(
      bfromcstr("creator"), bfromcstr("title"), bfromcstr("path"));
  Message *message = load_patch_message(patch_info);
  mu_assert(message != NULL, "Could not create load patch message");
  message_destroy(message);
  return NULL;
}

char *test_new_patch_message_create() {
  CreatorInfo *creator_info =
      creator_info_create(bfromcstr("creator"), bfromcstr("title"));
  Message *message = new_patch_message(creator_info);
  mu_assert(message != NULL, "Could not create new patch message");
  message_destroy(message);
  return NULL;
}

char *test_file_chunk_message_create() {
  FileChunk *chunk = file_chunk_create();
  mu_assert(chunk != NULL, "Could not create file chunk");

  int addlength = 1000;

  unsigned char *addition1 = calloc(addlength, sizeof(unsigned char));
  FileChunk *out1 = file_chunk_extend(chunk, addition1, addlength);
  mu_assert(out1 != NULL, "Invalid response returned from extend");
  mu_assert(out1->length == addlength, "Did not extend length");
  mu_assert(chunk->length == addlength, "Did not extend length");
  free(addition1);

  Message *message = file_chunk_message(chunk);
  mu_assert(message != NULL, "Could not create file chunk message");
  message_destroy(message);
  return NULL;
}

char *test_audio_buffer_message_create() {
  AudioBuffer *audio = audio_buffer_create(2, 1024);
  mu_assert(audio != NULL, "Could not create audio buffer");

  Message *message = audio_buffer_message(audio);
  mu_assert(message != NULL, "Could not create audio buffer message");
  message_destroy(message);
  return NULL;
}

char *all_tests() {
  mu_suite_start();

  mu_run_test(test_stream_finish_message_create);
  mu_run_test(test_patch_finish_message_create);
  mu_run_test(test_load_patch_message_create);
  mu_run_test(test_new_patch_message_create);
  mu_run_test(test_file_chunk_message_create);
  mu_run_test(test_audio_buffer_message_create);

  return NULL;
}

RUN_TESTS(all_tests);
