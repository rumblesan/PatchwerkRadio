#pragma once

typedef struct FileChunk {
  unsigned char *data;
  int length;
} FileChunk;

FileChunk *file_chunk_create();

FileChunk *file_chunk_extend(FileChunk *chunk, unsigned char *addition,
                             int addlength);

void file_chunk_destroy(FileChunk *chunk);
