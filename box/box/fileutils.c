/****************************************************************************
 * Copyright (C) 2007-2012 by Matteo Franchin                               *
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

int Box_File_Exist(const char *file_name) {
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
  int        only_first;
  const char *file_name,
             *prefix;
  char       *first_file;
  BoxList    *found_files;
} MyFindFileData;

static BoxTask My_Find_File(MyFindFileData *ffd,
                            const char *prefix, const char *suffix) {
  const char *file = Box_Print("%s%c%s%s",
                               (char *) prefix, DIR_SEPARATOR,
                               ffd->file_name, (char *) suffix);
  if (Box_File_Exist(file)) {
    if (ffd->only_first) {
      ffd->first_file = BoxMem_Strdup(file);
      return BOXTASK_FAILURE;

    } else {
      assert(file != (char *) NULL);
      BoxList_Append_String(ffd->found_files, file);
      return BOXTASK_OK;
    }
  }

  return BOXTASK_OK;  
}

static BoxTask My_Find_File_Prod_Iterator(void **tuple, void *pass) {
  char *prefix = (char *) tuple[0],
       *suffix = (char *) tuple[1];
  MyFindFileData *ffd = (MyFindFileData *) pass;
  return My_Find_File(ffd, prefix, suffix);
}

void Box_Find_Files_In_Dirs(BoxList **found_files, const char *file_name,
                            BoxList *prefixes, BoxList *suffixes) {
  BoxList l;
  MyFindFileData ffd;
  ffd.only_first = 0;
  ffd.file_name = file_name;
  ffd.found_files = BoxList_New(0);
  BoxList_Init(& l, sizeof(BoxList *));
  BoxList_Append(& l, & prefixes);
  BoxList_Append(& l, & suffixes);
  (void) BoxList_Product_Iter(& l, My_Find_File_Prod_Iterator, & ffd);
  BoxList_Finish(& l);
  *found_files = ffd.found_files;
}

/** Returns the full path of the file in '*found_file'.
 * This is a C-string which needs to be freed by the user.
 */
void Box_Find_File_In_Dirs(char **found_file, const char *file_name,
                           BoxList *prefixes, BoxList *suffixes) {
  BoxList l;
  MyFindFileData ffd;
  ffd.only_first = 1;
  ffd.first_file = (char *) NULL;
  ffd.file_name = file_name;
  BoxList_Init(& l, sizeof(BoxList *));
  BoxList_Append(& l, & prefixes);
  BoxList_Append(& l, & suffixes);
  (void) BoxList_Product_Iter(& l, My_Find_File_Prod_Iterator, & ffd);
  BoxList_Finish(& l);
  *found_file = ffd.first_file;
}

static BoxTask My_Find_File_Iterator(void *item, void *pass) {
  MyFindFileData *ffd = (MyFindFileData *) pass;
  char *suffix = (char *) item;
  assert(ffd->only_first);
  BoxTask t = My_Find_File(ffd, ffd->prefix, suffix);
  return t;
}

void Box_Find_File_In_Dir(char **found_file, const char *file_name,
                          const char *prefix, BoxList *suffixes) {
  MyFindFileData ffd;
  ffd.only_first = 1;
  ffd.first_file = (char *) NULL;
  ffd.prefix = prefix;
  ffd.file_name = file_name;
  (void) BoxList_Iter(suffixes, My_Find_File_Iterator, & ffd);
  *found_file = ffd.first_file;
}

void Box_Split_Path(char **dir, char **file, const char *full_path) {
  const char *basename = strrchr(full_path, DIR_SEPARATOR);
  assert(full_path != NULL);
  if (basename == NULL) {
    if (dir != NULL)
      *dir = NULL;
    if (file != NULL)
      *file = BoxMem_Strdup(full_path);

  } else {
    size_t i = (basename - full_path) + 1;
    /* ^^^ cast from ptrdiff_t */
    if (file != NULL)
      *file = BoxMem_Strdup(basename + 1);
    if (dir != NULL) {
      *dir = memcpy(BoxMem_Safe_Alloc(sizeof(char)*(i + 1)),
                    full_path, i);
      (*dir)[i] = '\0';
    }
  }
}

/* This functions transforms the input Unix path in a way that it can be
 * understood in the current platform.
 */
char *Box_Normalize_Path(char *unix_path) {
#ifdef __WINDOWS__
  char *c;
  for (c = unix_path; *c != '\0'; c++) {
    if (c == '/')
      *c = '\\';
  }
  return unix_path;

#else
  return unix_path;
#endif
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
  Box_Find_Files_In_Dirs(& found_files, "removeme", paths, extensions);
  BoxList_Destroy(paths);
  BoxList_Destroy(extensions);
  return 0;
}
#endif
