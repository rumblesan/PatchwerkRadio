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
    bstring patch_folder, bstring script_path,
    int play_time, int filenumber, int thread_sleep_seconds,
    int *status_var, ck_ring_t *pipe_out, ck_ring_buffer_t *pipe_out_buffer) {

  PatchChooserProcessConfig *cfg = malloc(sizeof(PatchChooserProcessConfig));
  check_mem(cfg);

  check(patch_folder != NULL, "PatchChooser: Invalid patch folder path");
  cfg->patch_folder = patch_folder;

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

int patch_chooser_send_load_patch(lua_State* L) {
  PatchChooserLuaUserdata *pclud = (PatchChooserLuaUserdata *)lua_touserdata(L, 1);
  luaL_argcheck(L, pclud != NULL, 1, "PatchChooser user data expected");
  luaL_checktype(L, 2, LUA_TTABLE);
  lua_getfield(L, 2, "creator");
  lua_getfield(L, 2, "title");
  lua_getfield(L, 2, "path");
  const char *creator = lua_tostring(L, -3);
  const char *title = lua_tostring(L, -2);
  const char *path = lua_tostring(L, -1);
  PatchInfo *pi = patch_info_create(bfromcstr(creator), bfromcstr(title), bfromcstr(path));
  lua_pop(L, 3);
  logger("PatchChooser", "next patch: %s - %s -> %s", bdata(pi->creator),
         bdata(pi->title), bdata(pi->filepath));
  Message *msg = load_patch_message(pi);
  if (!ck_ring_enqueue_spsc(pclud->pipe_out, pclud->pipe_out_buffer, msg)) {
    err_logger("PatchChooser", "Could not send New Patch message");
    err_logger("PatchChooser", "size %d    capacity %d",
               ck_ring_size(pclud->pipe_out),
               ck_ring_capacity(pclud->pipe_out));
    message_destroy(msg);
  }
  return 0;
}

void *start_patch_chooser(void *_cfg) {

  PatchChooserProcessConfig *cfg = _cfg;

  *(cfg->status_var) = 1;
  lua_State *L = cfg->lua;

  logger("PathcChooser", "Starting");

  lua_pushcfunction(L, patch_chooser_send_load_patch);
  lua_setglobal(L, "send_load_patch");

  lua_createtable(L, 0, 3);

  lua_pushstring(L, "patch_folder");
  lua_pushlstring(L, bdata(cfg->patch_folder), cfg->patch_folder->slen);
  lua_settable(L, -3);

  lua_pushstring(L, "load_count");
  lua_pushinteger(L, cfg->filenumber);
  lua_settable(L, -3);

  lua_pushstring(L, "state");
  PatchChooserLuaUserdata *pclud = (PatchChooserLuaUserdata *)lua_newuserdata(L, sizeof(PatchChooserLuaUserdata));
  pclud->pipe_out = cfg->pipe_out;
  pclud->pipe_out_buffer = cfg->pipe_out_buffer;
  lua_settable(L, -3);

  lua_setglobal(L, "AppConfig");

  logger("PatchChooser", "Loading script %s", bdata(cfg->script_path));
  if (luaL_dofile(L, bdata(cfg->script_path)) == LUA_OK) {
      logger("PatchChooser", "Finished");
      lua_pop(L, lua_gettop(L));
  } else {
      err_logger("PatchChooser", "Error from Lua: %s", lua_tostring(L, -1));
  }

  *(cfg->status_var) = 0;
  patch_chooser_config_destroy(cfg);
  logger("PatchChooser", "Cleaned up");
  pthread_exit(NULL);
  return NULL;
}
