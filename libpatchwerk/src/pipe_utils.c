#include <stdlib.h>

#include <bclib/dbg.h>
#include <ck_ring.h>

#include "messages.h"

bool create_pipe(ck_ring_t **bus, ck_ring_buffer_t **buffer, uint16_t size) {
  *bus = malloc(sizeof(ck_ring_t));
  check_mem(*bus);
  *buffer = malloc(sizeof(ck_ring_buffer_t) * size);
  check_mem(*buffer);
  ck_ring_init(*bus, size);
  return true;
error:
  return false;
}

int cleanup_pipe(ck_ring_t *ring, ck_ring_buffer_t *buffer,
                 const char *pipename) {
  Message *pipe_msg = NULL;
  check(ring != NULL, "Invalid queue ring passed in");
  check(buffer != NULL, "Invalid queue buffer passed in");
  while (ck_ring_dequeue_spsc(ring, buffer, &pipe_msg)) {
    check(pipe_msg != NULL, "Null message whilst emptying %s pipe", pipename);
    message_destroy(pipe_msg);
  }
  free(buffer);
  free(ring);
  return 0;
error:
  return 1;
}
