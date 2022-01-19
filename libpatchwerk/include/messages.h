#pragma once

#include "audio_buffer.h"
#include "filechunk.h"

#include "bclib/bstrlib.h"

typedef enum {
  AUDIOBUFFER,
  FILECHUNK,
  NEWPATCH,
  PATCHFINISHED,
  STREAMFINISHED
} MessageType;

typedef struct PatchInfo {
  bstring creator;
  bstring title;
} PatchInfo;

typedef struct Message {
  MessageType type;
  void *payload;
} Message;

Message *message_create(MessageType type, void *payload);

void message_destroy(Message *message);

PatchInfo *patch_info_create(bstring creator, bstring title);
void patch_info_destroy(PatchInfo *info);

Message *file_chunk_message(FileChunk *chunk); /*  */
Message *audio_buffer_message(AudioBuffer *buffer);
Message *new_patch_message(PatchInfo *info);
Message *patch_finished_message();
Message *stream_finished_message();

const char *msg_type(Message *message);
