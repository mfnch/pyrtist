/****************************************************************************
 * Copyright (C) 2008 by Matteo Franchin                                    *
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

#include "version.h"

#ifndef __WINDOWS__
#  if defined WIN32 || defined _WIN32
#    define __WINDOWS__
#  endif
#endif

#ifdef __WINDOWS__
#  include "windows.h"
#endif

BoxList libraries;
BoxList lib_dirs;
BoxList inc_dirs;
BoxList inc_exts;

void Path_Init(void) {
  BoxList_Init(& libraries, 0);
  BoxList_Init(& lib_dirs, 0);
  BoxList_Init(& inc_dirs, 0);
  BoxList_Init(& inc_exts, 0);
  BoxList_Append_String(& inc_exts, ".bxh");
  BoxList_Append_String(& inc_exts, ".box");
  BoxList_Append_String(& inc_exts, "");
}

void Path_Destroy(void) {
  BoxList_Finish(& libraries);
  BoxList_Finish(& lib_dirs);
  BoxList_Finish(& inc_dirs);
  BoxList_Finish(& inc_exts);
}

static void Add_Env_To_List(const char *env_var, BoxList *list) {
#ifdef HAVE_GETENV
  char *strings = getenv(env_var);
  if (strings != (char *) NULL)
    BoxList_Append_Strings(list, strings, PATH_SEPARATOR);
#endif
}

char *Path_Get_Exec_Path(void) {
#ifdef __WINDOWS__
  char *out = (char *) BoxMem_Alloc(MAX_PATH);
  if (out != NULL) {
    char *bn;
    GetModuleFileName(NULL, out, MAX_PATH);
    bn = strrchr(out, '\\');
    if (bn != NULL)
      *bn = '\0';
  }
  return out;

#else
  return NULL;
#endif
}

void Path_Get_Bltin_Pkg_And_Lib_Paths(char **pkg_path, char **lib_path) {
  *pkg_path = NULL;
  *lib_path = NULL;

#ifdef __WINDOWS__
  char *exec_path = Path_Get_Exec_Path();
  if (exec_path != NULL) {
    *pkg_path = printdup("%s\\..\\lib\\box"BOX_VERSTR_ROUGH
                         "\\pkg", exec_path);
    *lib_path = printdup("%s\\..\\lib\\box"BOX_VERSTR_ROUGH
                         "\\lib", exec_path);
    BoxMem_Free(exec_path);
  }

#else
#  ifdef BUILTIN_PKG_PATH
  *pkg_path = strdup(BUILTIN_PKG_PATH);
#  endif
#  ifdef BUILTIN_LIBRARY_PATH
  *lib_path = strdup(BUILTIN_LIBRARY_PATH);
#  endif
#endif
}

void Path_Set_All_From_Env(void) {
  Add_Env_To_List(BOX_LIBRARY_PATH, & lib_dirs);
  Add_Env_To_List(BOX_INCLUDE_PATH, & inc_dirs);
  Add_Env_To_List(BOX_DEFAULT_LIBS, & libraries);

#ifdef __WINDOWS__
  {
    char *pkg_path, *lib_path;
    Path_Get_Bltin_Pkg_And_Lib_Paths(& pkg_path, & lib_path);
    if (pkg_path != NULL && lib_path != NULL) {
      Path_Add_Pkg_Dir(pkg_path);
      Path_Add_Lib_Dir(lib_path);

    } else {
      MSG_WARNING("Cannot add default paths for libraries and headers.");
    }

    BoxMem_Free(pkg_path);
    BoxMem_Free(lib_path);
  }
#else
#  ifdef BUILTIN_LIBRARY_PATH
  Path_Add_Lib_Dir(BUILTIN_LIBRARY_PATH);
#  endif

#  ifdef BUILTIN_PKG_PATH
  Path_Add_Pkg_Dir(BUILTIN_PKG_PATH);
#  endif
#endif
}

void Path_Add_Lib(char *lib) {
  BoxList_Append_String(& libraries, lib);
}

void Path_Add_Lib_Dir(char *path) {
  BoxList_Append_String(& lib_dirs, path);
}

void Path_Add_Pkg_Dir(char *path) {
  BoxList_Append_String(& inc_dirs, path);
}

void Path_Add_Script_Path_To_Inc_Dir(const char *script_path) {
  if (script_path != NULL) {
    char *script_dir;
    File_Path_Split(& script_dir, NULL, script_path);
    if (script_dir != NULL) {
      BoxList_Append_String(& inc_dirs, script_dir);
      BoxMem_Free(script_dir);
      return;
    }
  }

  BoxList_Append_String(& inc_dirs, ".");
}

FILE *Path_Open_Inc_File(const char *file, const char *mode) {
  char *full_path;
  File_Find_First(& full_path, file, & inc_dirs, & inc_exts);
  if (full_path != (char *)NULL) {
    FILE *fd = fopen(full_path, mode);
    BoxMem_Free(full_path);
    return fd;

  } else
    return (FILE *) NULL;
}
