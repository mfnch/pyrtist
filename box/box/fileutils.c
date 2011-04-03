/****************************************************************************
 * Copyright (C) 2007, 2008 by Matteo Franchin                              *
 *                                                                          *
 * This file is part of Box.                                                *
 *                                                                          *
 *   Box is free software: you can redistribute it and/or modify it         *
 *   under the terms of the GNU Lesser General Public License as published  *
 *   by the Free Software Foundation, either version 3 of the License, or   *
 *   (at your option) any later version.                                    *
 *                                                                          *
 *   Box is distributed in the hope that it will be useful,                 *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Lesser General Public License for more details.                    *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with Box.  If not, see <http://www.gnu.org/licenses/>.   *
 ****************************************************************************/

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef HAVE_SYS_TYPES_H
#  include <sys/types.h>
#endif

#ifdef HAVE_SYS_STAT_H
#  include <sys/stat.h>
#endif

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#include <stdio.h>
#include <assert.h>

#include "types.h"
#include "defaults.h"
#include "messages.h"
#include "list.h"
#include "fileutils.h"

int File_Exist(const char *file_name) {
#ifdef HAVE_STAT
  /* I don't know if this implementation is strictly
   * correct/optimal. For now I don't care!
   */
  struct stat s;
  return stat(file_name, & s) == 0;
#else
  /* I feel this is not a good idea... */
  FILE *f = fopen(file_name, "r");
  int fopen_successful = (f != (FILE *) NULL);
  if (fopen_successful) (void) fclose(f);
  return fopen_successful;
#endif
}

typedef struct {
  int only_first;
  const char *file_name;
  char *first_file;
  BoxList *found_files;
} FindFileData;

static Task Find_File_Iterator(void **tuple, void *pass) {
  char *prefix = (char *) tuple[0],
       *suffix = (char *) tuple[1];
  FindFileData *ffd = (FindFileData *) pass;
  const char *file = Box_Print("%s%c%s%s", prefix, DIR_SEPARATOR,
                               ffd->file_name, suffix);
  if (File_Exist(file)) {
    if (ffd->only_first) {
      ffd->first_file = BoxMem_Strdup(file);
      return Failed;
    } else {
      assert(file != (char *) NULL);
      BoxList_Append_String(ffd->found_files, file);
    }
  }
  return Success;
}

void File_Find(BoxList **found_files, const char *file_name,
               BoxList *prefixes, BoxList *suffixes) {
  BoxList l;
  FindFileData ffd;
  ffd.only_first = 0;
  ffd.file_name = file_name;
  ffd.found_files = BoxList_New(0);
  BoxList_Init(& l, sizeof(BoxList *));
  BoxList_Append(& l, & prefixes);
  BoxList_Append(& l, & suffixes);
  (void) BoxList_Product_Iter(& l, Find_File_Iterator, (void *) & ffd);
  BoxList_Finish(& l);
  *found_files = ffd.found_files;
}

/** Returns the full path of the file in '*found_file'.
 * This is a C-string which needs to be freed by the user.
 */
void File_Find_First(char **found_file, const char *file_name,
                     BoxList *prefixes, BoxList *suffixes) {
  BoxList l;
  FindFileData ffd;
  ffd.only_first = 1;
  ffd.first_file = (char *) NULL;
  ffd.file_name = file_name;
  BoxList_Init(& l, sizeof(BoxList *));
  BoxList_Append(& l, & prefixes);
  BoxList_Append(& l, & suffixes);
  (void) BoxList_Product_Iter(& l, Find_File_Iterator, (void *) & ffd);
  BoxList_Finish(& l);
  *found_file = ffd.first_file;
}

void File_Path_Split(char **dir, char **file, const char *full_path) {
  const char *basename = strrchr(full_path, DIR_SEPARATOR);
  assert(full_path != NULL);
  if (basename == NULL) {
    if (dir != NULL) *dir = NULL;
    if (file != NULL) *file = BoxMem_Strdup(full_path);
    return;

  } else {
    size_t i = (basename - full_path) + 1;
    /* ^^^ cast from ptrdiff_t */
    if (file != NULL) *file = BoxMem_Strdup(basename + 1);
    if (dir != NULL) {
      *dir = memcpy(BoxMem_Alloc(sizeof(char)*(i + 1)), full_path, i);
      (*dir)[i] = '\0';
    }
  }
}

#if 0
int main(void) {
  BoxList *paths, *extensions, *found_files;
  BoxList_New(& paths, 0);
  BoxList_Append_String(paths, "/tmp/");
  BoxList_Append_String(paths, "/home/fnch/");
  BoxList_New(& extensions, 0);
  BoxList_Append_String(extensions, ".txt");
  BoxList_Append_String(extensions, ".dat");
  File_Find(& found_files, "removeme", paths, extensions);
  BoxList_Destroy(paths);
  BoxList_Destroy(extensions);
  return 0;
}
#endif
