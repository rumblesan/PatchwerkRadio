#include <stdlib.h>

#include "audio_buffer.h"
#include "filechunk.h"
#include "messages.h"

#include "bclib/bstrlib.h"
#include "bclib/dbg.h"

Message *file_chunk_message(FileChunk *chunk) {
  check(chunk != NULL, "Invalid FileChunk");
  Message *message = message_create(FILECHUNK, chunk);
  check(message != NULL, "Could not create message");
  return message;
error:
  return NULL;
}

Message *audio_buffer_message(AudioBuffer *buffer) {
  check(buffer != NULL, "Invalid AudioBuffer");
  Message *message = message_create(AUDIOBUFFER, buffer);
  check(message != NULL, "Could not create message");
  return message;
error:
  return NULL;
}

Message *new_patch_message(PatchInfo *info) {
  check(info != NULL, "Could not create patch info");
  Message *message = message_create(NEWPATCH, info);
  check(message != NULL, "Could not create message");
  return message;
error:
  return NULL;
}

Message *patch_finished_message() {
  Message *message = message_create(PATCHFINISHED, NULL);
  check(message != NULL, "Could not create message");
  return message;
error:
  return NULL;
}
Message *stream_finished_message() {
  Message *message = message_create(STREAMFINISHED, NULL);
  check(message != NULL, "Could not create message");
  return message;
error:
  return NULL;
}

PatchInfo *patch_info_create(bstring creator, bstring title) {
  PatchInfo *info = malloc(sizeof(PatchInfo));
  check_mem(info);
  info->creator = creator;
  info->title = title;
  return info;
error:
  return NULL;
}
void patch_info_destroy(PatchInfo *info) {
  check(info->creator != NULL, "Invalid creator");
  check(info->title != NULL, "Invalid title");
  bdestroy(info->creator);
  bdestroy(info->title);
  free(info);
  return;
error:
  return;
}

Message *message_create(MessageType type, void *payload) {
  Message *message = malloc(sizeof(Message));
  check_mem(message);

  message->type = type;
  // No check here as finished messages can have NULL payloads
  message->payload = payload;

  return message;
error:
  return NULL;
}

void message_destroy(Message *message) {
  check(message != NULL, "Invalid message");
  switch (message->type) {
  case AUDIOBUFFER:
    audio_buffer_destroy(message->payload);
    break;
  case FILECHUNK:
    file_chunk_destroy(message->payload);
    break;
  case NEWPATCH:
    patch_info_destroy(message->payload);
    break;
  case PATCHFINISHED:
    break;
  case STREAMFINISHED:
    break;
  }
  free(message);
error:
  return;
}

const char *msg_type(Message *message) {
  check(message != NULL, "Invalid message");
  switch (message->type) {
  case AUDIOBUFFER:
    return "AudioBuffer";
  case FILECHUNK:
    return "FileChunk";
  case NEWPATCH:
    return "NewPatch";
  case PATCHFINISHED:
    return "PatchFinished";
  case STREAMFINISHED:
    return "StreamFinished";
  }
error:
  return "Invalid Message";
}
