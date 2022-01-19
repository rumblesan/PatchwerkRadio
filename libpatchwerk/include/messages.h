#pragma once

#include "filechunk.h"
#include "audio_buffer.h"

#include "bclib/bstrlib.h"

typedef enum {
  AUDIOBUFFER,
  FILECHUNK,
  NEWTRACK,
  TRACKFINISHED,
  STREAMFINISHED
} MessageType;

typedef struct TrackInfo {
  bstring artist;
  bstring title;
} TrackInfo;

typedef struct Message {
  MessageType type;
  void *payload;
} Message;

Message *message_create(MessageType type, void *payload);

void message_destroy(Message *message);

TrackInfo *track_info_create(bstring artist, bstring title);
void track_info_destroy(TrackInfo *info);

Message *file_chunk_message(FileChunk *chunk); /*  */
Message *audio_buffer_message(AudioBuffer *buffer);
Message *new_track_message(TrackInfo *info);
Message *track_finished_message();
Message *stream_finished_message();

const char *msg_type(Message *message);
