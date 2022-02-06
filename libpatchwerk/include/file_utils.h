#pragma once

#include <stdbool.h>

#include "bclib/bstrlib.h"

bool is_regular_file(const char *path);

bstring get_random_file(bstring pattern);
