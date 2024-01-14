#pragma once

#include <lua5.4/lua.h>

#include <bclib/bstrlib.h>

typedef struct APIProcessConfig {

  bstring host;
  bstring port;
  bstring script_path;

  lua_State *lua;

} APIProcessConfig;

APIProcessConfig * api_process_config_create(bstring script_path, bstring host, bstring port);

void api_process_config_destroy(APIProcessConfig *cfg);

void *start_api(void *_cfg);
