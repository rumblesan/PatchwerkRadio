#include <ck_ring.h>

bool create_pipe(ck_ring_t **bus, ck_ring_buffer_t **buffer, uint16_t size);

int cleanup_pipe(ck_ring_t *ring, ck_ring_buffer_t *buffer, const char *pipename);
