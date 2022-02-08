#pragma once

#include <shout/shout.h>
#include <ck_ring.h>

#include <bclib/bstrlib.h>

typedef struct BroadcastProcessConfig {

  bstring host;
  int port;

  bstring user;
  bstring pass;

  bstring stream_name;
  bstring stream_description;
  bstring stream_genre;
  bstring stream_url;

  bstring mount;
  int protocol;
  int format;

  int msg_buffer_count;

  int *status_var;

  ck_ring_t *pipe_in;
  ck_ring_buffer_t *pipe_in_buffer;

} BroadcastProcessConfig;

BroadcastProcessConfig *
broadcast_config_create(bstring host, int port, bstring user, bstring pass,
                        bstring mount, bstring name, bstring description,
                        bstring genre, bstring url, int protocol, int format,
                        int *status_var, ck_ring_t *pipe_in, ck_ring_buffer_t *pipe_in_buffer);

void broadcast_config_destroy(BroadcastProcessConfig *cfg);

void *start_broadcast(void *_cfg);
