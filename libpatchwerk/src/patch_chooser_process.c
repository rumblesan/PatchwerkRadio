#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <lua5.4/lua.h>
#include <lua5.4/lauxlib.h>
#include <lua5.4/lualib.h>
#include <ck_ring.h>

#include <bclib/bstrlib.h>
#include <bclib/dbg.h>

#include "patch_chooser_process.h"

#include "file_utils.h"
#include "logging.h"
#include "messages.h"

PatchChooserProcessConfig *patch_chooser_config_create(
    bstring pattern, bstring script_path,
    int play_time, int filenumber, int thread_sleep_seconds,
    int *status_var, ck_ring_t *pipe_out, ck_ring_buffer_t *pipe_out_buffer) {

  PatchChooserProcessConfig *cfg = malloc(sizeof(PatchChooserProcessConfig));
  check_mem(cfg);

  check(pattern != NULL, "PatchChooser: Invalid patch search pattern");
  cfg->pattern = pattern;

  check(script_path != NULL, "PatchChooser: Invalid script path");
  cfg->script_path = script_path;

  cfg->filenumber = filenumber;
  cfg->thread_sleep_seconds = thread_sleep_seconds;
  cfg->play_time = (double)play_time;

  check(status_var != NULL, "PatchChooser: Invalid status var");
  cfg->status_var = status_var;

  lua_State *L = luaL_newstate();
  luaL_openlibs(L);

  cfg->lua = L;

  check(pipe_out != NULL, "PatchChooser: Invalid pipe out");
  cfg->pipe_out = pipe_out;
  check(pipe_out_buffer != NULL, "PatchChooser: Invalid pipe out buffer");
  cfg->pipe_out_buffer = pipe_out_buffer;

  return cfg;
error:
  return NULL;
}

void patch_chooser_config_destroy(PatchChooserProcessConfig *cfg) {
  check(cfg != NULL, "PatchChooser: Invalid cfg");
  free(cfg);
  return;
error:
  err_logger("PatchChooser", "Could not destroy cfg");
}

void *start_patch_chooser(void *_cfg) {

  PatchChooserProcessConfig *cfg = _cfg;

  *(cfg->status_var) = 1;
  lua_State *L = cfg->lua;

  logger("PathcChooser", "Starting");

  lua_createtable(L, 0, 2);

  lua_pushstring(L, "pattern");
  lua_pushlstring(L, bdata(cfg->pattern), cfg->pattern->slen);
  lua_settable(L, -3);

  lua_pushstring(L, "count");
  lua_pushinteger(L, cfg->filenumber);
  lua_settable(L, -3);

  lua_setglobal(L, "chooser_config");

  logger("PatchChooser", "Loading script %s", bdata(cfg->script_path));
  if (luaL_dofile(L, bdata(cfg->script_path)) == LUA_OK) {
      lua_pop(L, lua_gettop(L));
  }

  logger("PatchChooser", "Finished");
  *(cfg->status_var) = 0;
  patch_chooser_config_destroy(cfg);
  logger("PatchChooser", "Cleaned up");
  pthread_exit(NULL);
  return NULL;
}
