#include "minunit.h"

#include "file_utils.h"
#include "messages.h"

#include <bclib/bstrlib.h>
#include <bclib/dbg.h>
#include <bclib/ringbuffer.h>

char *test_path_to_patch_info() {
  bstring path = bfromcstr("./patterns/creator/patch/main.pd");
  PatchInfo *pi = path_to_patchinfo(path);

  mu_assert(biseq(pi->creator, bfromcstr("creator")), "creator incorrect");
  mu_assert(biseq(pi->title, bfromcstr("patch")), "title incorrect");
  mu_assert(biseq(pi->filepath, path), "path incorrect");

  patch_info_destroy(pi);
  return NULL;
}

char *all_tests() {
  mu_suite_start();

  mu_run_test(test_path_to_patch_info);

  return NULL;
}

RUN_TESTS(all_tests);
