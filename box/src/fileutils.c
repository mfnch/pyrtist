/***************************************************************************
 *   Copyright (C) 2007 by Matteo Franchin                                 *
 *   fnch@libero.it                                                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

/* $Id$ */

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
  if (f != (FILE *) NULL) (void) fclose(f);
  return fopen_successful;
#endif
}

typedef struct {
  int only_first;
  const char *file_name;
  char *first_file;
  List *found_files;
} FindFileData;

static Task Find_File_Iterator(void **tuple, void *pass) {
  char *prefix = (char *) tuple[0],
       *suffix = (char *) tuple[1];
  FindFileData *ffd = (FindFileData *) pass;
  const char *file = print("%s/%s%s", prefix, ffd->file_name, suffix);
  if (File_Exist(file)) {
    if (ffd->only_first) {
      ffd->first_file = Mem_Strdup(file);
      return Failed;
    } else {
      assert(file != (char *) NULL);
      List_Append_String(ffd->found_files, file);
    }
  }
  return Success;
}

void File_Find(List **found_files, const char *file_name,
               List *prefixes, List *suffixes)
{
  List *l;
  FindFileData ffd;
  ffd.only_first = 0;
  ffd.file_name = file_name;
  List_New(& ffd.found_files, 0);
  List_New(& l, sizeof(List *));
  List_Append(l, & prefixes);
  List_Append(l, & suffixes);
  (void) List_Product_Iter(l, Find_File_Iterator, (void *) & ffd);
  List_Destroy(l);
  *found_files = ffd.found_files;
}

/** Returns the full path of the file in '*found_file'.
 * This is a C-string which needs to be freed by the user.
 */
void File_Find_First(char **found_file, const char *file_name,
               List *prefixes, List *suffixes)
{
  List *l;
  FindFileData ffd;
  ffd.only_first = 1;
  ffd.first_file = (char *) NULL;
  ffd.file_name = file_name;
  List_New(& l, sizeof(List *));
  List_Append(l, & prefixes);
  List_Append(l, & suffixes);
  (void) List_Product_Iter(l, Find_File_Iterator, (void *) & ffd);
  List_Destroy(l);
  *found_file = ffd.first_file;
}

#if 0
int main(void) {
  List *paths, *extensions, *found_files;
  List_New(& paths, 0);
  List_Append_String(paths, "/tmp/");
  List_Append_String(paths, "/home/fnch/");
  List_New(& extensions, 0);
  List_Append_String(extensions, ".txt");
  List_Append_String(extensions, ".dat");
  File_Find(& found_files, "removeme", paths, extensions);
  List_Destroy(paths);
  List_Destroy(extensions);
  return 0;
}
#endif
