#pragma once

#include <stdio.h>

#define startup_log(P, M, ...)                                                 \
  fprintf(stderr, "********************\nSTART: [%s] " M "\n", P,              \
          ##__VA_ARGS__) /*  */

#define logger(P, M, ...)                                                      \
  fprintf(stderr, " INFO: [%s] " M "\n", P, ##__VA_ARGS__)

#define err_logger(P, M, ...)                                                  \
  fprintf(stderr, "ERROR: [%s] " M "\n", P, ##__VA_ARGS__)
