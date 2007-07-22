/***************************************************************************
 *   Copyright (C) 2006 by Matteo Franchin                                 *
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "types.h"
#include "defaults.h"
#include "messages.h"
#include "mem.h"
#include "list.h"
#include "fileutils.h"
#include "paths.h"

List *libraries;
List *lib_dirs;
List *inc_dirs;
List *inc_exts;

void Path_Init(void) {
  List_New(& libraries, 0);
  List_New(& lib_dirs, 0);
  List_New(& inc_dirs, 0);
  List_New(& inc_exts, 0);
  List_Append_String(inc_exts, ".bxh");
  List_Append_String(inc_exts, "");
}

void Path_Destroy(void) {
  List_Destroy(libraries);
  List_Destroy(lib_dirs);
  List_Destroy(inc_dirs);
  List_Destroy(inc_exts);
}

static void Add_Env_To_List(const char *env_var, List *list) {
#ifdef HAVE_GETENV
  char *strings = getenv(env_var);
  if (strings != (char *) NULL)
    List_Append_Strings(list, strings, PATH_SEPARATOR);
#endif
}

void Path_Set_All_From_Env(void) {
  Add_Env_To_List(BOX_LIBRARY_PATH, lib_dirs);
  Add_Env_To_List(BOX_INCLUDE_PATH, inc_dirs);
  Add_Env_To_List(BOX_DEFAULT_LIBS, libraries);

#ifdef BUILTIN_LIBRARY_PATH
  Path_Add_Lib_Dir(BUILTIN_LIBRARY_PATH);
#endif

#ifdef BUILTIN_INCLUDE_PATH
  Path_Add_Inc_Dir(BUILTIN_INCLUDE_PATH);
#endif
}

void Path_Add_Lib(char *lib) {
  List_Append_String(libraries, lib);
}

void Path_Add_Lib_Dir(char *path) {
  List_Append_String(lib_dirs, path);
}

void Path_Add_Inc_Dir(char *path) {
  List_Append_String(inc_dirs, path);
}

FILE *Path_Open_Inc_File(const char *file, const char *mode) {
  char *full_path;
  File_Find_First(& full_path, file, inc_dirs, inc_exts);
  if (full_path != (char *)NULL) {
    FILE *fd = fopen(full_path, mode);
    Mem_Free(full_path);
    return fd;

  } else
    return (char *) NULL;
}
