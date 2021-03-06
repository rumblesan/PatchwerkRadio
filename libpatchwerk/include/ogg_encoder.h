#pragma once

#include <vorbis/vorbisenc.h>

#include "audio_buffer.h"
#include "filechunk.h"
#include "messages.h"

typedef struct OggEncoderState {

  ogg_stream_state os;

  vorbis_comment vc;

  vorbis_info vi;

  vorbis_dsp_state vd;
  vorbis_block vb;

} OggEncoderState;

OggEncoderState *ogg_encoder_state(long channels, long samplerate,
                                   float quality);

void cleanup_encoder(OggEncoderState *encoder);

void set_metadata(OggEncoderState *encoder, PatchInfo *info);

void set_headers(OggEncoderState *encoder);

int write_headers(OggEncoderState *encoder, FileChunk *chunk);

int add_audio(OggEncoderState *encoder, AudioBuffer *audio);

int file_finished(OggEncoderState *encoder);

int write_audio(OggEncoderState *encoder, FileChunk *chunk);
