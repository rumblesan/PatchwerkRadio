#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>

#include <lua5.4/lua.h>
#include <lua5.4/lauxlib.h>
#include <lua5.4/lualib.h>

#include <bclib/dbg.h>

#include "api_process.h"
#include "logging.h"

APIProcessConfig *api_process_config_create(bstring script_path, bstring host, bstring port) {
  APIProcessConfig *cfg = calloc(1, sizeof(APIProcessConfig));
  check_mem(cfg);

  check(script_path != NULL, "API: Invalid script_path passed");
  check(host != NULL, "API: Invalid host passed");
  check(port != NULL, "API: Invalid port passed");
  cfg->script_path = script_path;
  cfg->host = host;
  cfg->port = port;

  lua_State *L = luaL_newstate();
  check_mem(L);

  luaL_openlibs(L);

  cfg->lua = L;

  return cfg;
error:
  return NULL;
}

void api_process_config_destroy(APIProcessConfig *cfg) {
  check(cfg != NULL, "Invalid API process config");

  // Strings all get cleaned up by config code
  lua_close(cfg->lua);
  free(cfg);
  return;
error:
  log_err("Could not clean up API process config");
}

void *start_api(void *_cfg) {
  APIProcessConfig *cfg = _cfg;

  lua_State *L = cfg->lua;

  logger("API", "Starting API");

  lua_createtable(L, 0, 2);

  lua_pushstring(L, "HOST");
  lua_pushlstring(L, bdata(cfg->host), cfg->host->slen);
  lua_settable(L, -3);

  lua_pushstring(L, "PORT");
  lua_pushlstring(L, bdata(cfg->port), cfg->port->slen);
  lua_settable(L, -3);

  lua_setglobal(L, "api_config");

  logger("API", "Loading script %s", bdata(cfg->script_path));
  if (luaL_dofile(L, bdata(cfg->script_path)) == LUA_OK) {
      lua_pop(L, lua_gettop(L));
  }

  err_logger("API", "Issue starting api server");

  return NULL;
}
