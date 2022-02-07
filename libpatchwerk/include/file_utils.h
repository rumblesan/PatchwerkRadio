#pragma once

#include <stdbool.h>

#include "bclib/bstrlib.h"

#include "messages.h"

bool is_regular_file(const char *path);

bstring get_random_patch(bstring pattern);

PatchInfo *path_to_patchinfo(bstring path);

