#pragma once

#include "audio_buffer.h"
#include "filechunk.h"

#include "bclib/bstrlib.h"

typedef enum {
  AUDIOBUFFER,
  FILECHUNK,
  LOADPATCH,
  NEWPATCH,
  PATCHFINISHED,
  STREAMFINISHED
} MessageType;

typedef struct CreatorInfo {
  bstring creator;
  bstring title;
} CreatorInfo;

typedef struct PatchInfo {
  bstring creator;
  bstring title;
  bstring filepath;
} PatchInfo;

typedef struct Message {
  MessageType type;
  void *payload;
} Message;

Message *message_create(MessageType type, void *payload);

void message_destroy(Message *message);

PatchInfo *patch_info_create(bstring creator, bstring title, bstring filepath);
void patch_info_destroy(PatchInfo *info);

CreatorInfo *creator_info_create(bstring creator, bstring title);
void creator_info_destroy(CreatorInfo *info);

Message *file_chunk_message(FileChunk *chunk); /*  */
Message *audio_buffer_message(AudioBuffer *buffer);
Message *load_patch_message(PatchInfo *info);
Message *new_patch_message(CreatorInfo *info);
Message *patch_finished_message();
Message *stream_finished_message();

const char *msg_type(Message *message);
