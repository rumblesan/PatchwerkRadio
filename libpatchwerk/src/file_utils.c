#include <glob.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "file_utils.h"

#include "bclib/bstrlib.h"
#include "bclib/dbg.h"
#include "bclib/list.h"

bool is_regular_file(const char *path) {
  struct stat path_stat;
  stat(path, &path_stat);
  return S_ISREG(path_stat.st_mode);
}

bstring get_random_patch(bstring pattern) {
  glob_t globbuf;

  List *filelist = NULL;

  check(!glob(bdata(pattern), GLOB_NOSORT, NULL, &globbuf),
        "Could not glob folder");

  filelist = list_create();
  check(filelist != NULL, "Could not create file list");

  for (size_t i = 0; i < globbuf.gl_pathc; i += 1) {
    char *name = globbuf.gl_pathv[i];
    if (is_regular_file(name)) {
      list_push(filelist, bfromcstr(name));
    }
  }
  if (globbuf.gl_pathc > 0)
    globfree(&globbuf);

  int randpos = floor((rand() / (float)RAND_MAX) * list_count(filelist));
  bstring fname = bstrcpy(list_get(filelist, randpos));

  LIST_FOREACH(filelist, first, next, cur) { bdestroy(cur->value); }
  list_destroy(filelist);
  return fname;
error:
  if (filelist != NULL)
    list_destroy(filelist);
  return bfromcstr("");
}

PatchInfo *path_to_patchinfo(bstring path) {
  struct bstrList *l = bsplit(path, '/');

  if (l->qty < 3) {
    bstrListDestroy(l);
    return NULL;
  }

  bstring artist = bstrcpy(l->entry[l->qty - 3]);
  bstring name = bstrcpy(l->entry[l->qty - 2]);

  PatchInfo *pi = patch_info_create(artist, name, path);

  bstrListDestroy(l);

  return pi;
}
