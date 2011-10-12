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

#ifdef __WINDOWS__
#  include "windows.h"
#endif


void BoxPaths_Init(BoxPaths *bp) {
#ifdef __WINDOWS__
  const char *init_suffix = "\\init.box";
#else
  const char *init_suffix = "/init.box";
#endif
  BoxList_Init(& bp->libraries, 0);
  BoxList_Init(& bp->lib_dirs, 0);
  BoxList_Init(& bp->inc_dirs, 0);
  BoxList_Init(& bp->inc_exts, 0);
  BoxList_Append_String(& bp->inc_exts, init_suffix);
  BoxList_Append_String(& bp->inc_exts, ".bxh");
  BoxList_Append_String(& bp->inc_exts, ".box");
  BoxList_Append_String(& bp->inc_exts, "");
  bp->flags = BOXPATHSFLAG_INC_SCRIPT_DIR;
}

void BoxPaths_Finish(BoxPaths *bp) {
  BoxList_Finish(& bp->libraries);
  BoxList_Finish(& bp->lib_dirs);
  BoxList_Finish(& bp->inc_dirs);
  BoxList_Finish(& bp->inc_exts);
}

void BoxPaths_Set_Flags(BoxPaths *bp, BoxPathsFlag mask, BoxPathsFlag val) {
  mask &= BOXPATHSFLAG_SET;
  bp->flags &= ~mask;
  bp->flags |= (val & mask);
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

void Box_Get_Bltin_Pkg_And_Lib_Paths(char **pkg_path, char **lib_path) {
  *pkg_path = NULL;
  *lib_path = NULL;

#ifdef __WINDOWS__
  char *exec_path = Path_Get_Exec_Path();
  if (exec_path != NULL) {
    *pkg_path = Box_SPrintF("%s\\..\\lib\\box"BOX_VERSTR_ROUGH
                            "\\pkg", exec_path);
    *lib_path = Box_SPrintF("%s\\..\\lib\\box"BOX_VERSTR_ROUGH
                            "\\lib", exec_path);
    BoxMem_Free(exec_path);
  }

#else
#  ifdef BUILTIN_PKG_PATH
  *pkg_path = BoxMem_Strdup(BUILTIN_PKG_PATH);
#  endif
#  ifdef BUILTIN_LIBRARY_PATH
  *lib_path = BoxMem_Strdup(BUILTIN_LIBRARY_PATH);
#  endif
#endif
}

void BoxPaths_Set_All_From_Env(BoxPaths *bp) {
  Add_Env_To_List(BOX_LIBRARY_PATH, & bp->lib_dirs);
  Add_Env_To_List(BOX_INCLUDE_PATH, & bp->inc_dirs);
  Add_Env_To_List(BOX_DEFAULT_LIBS, & bp->libraries);

#ifdef __WINDOWS__
  {
    char *pkg_path, *lib_path;
    Box_Get_Bltin_Pkg_And_Lib_Paths(& pkg_path, & lib_path);
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
  BoxPaths_Add_Lib_Dir(bp, BUILTIN_LIBRARY_PATH);
#  endif

#  ifdef BUILTIN_PKG_PATH
  BoxPaths_Add_Pkg_Dir(bp, BUILTIN_PKG_PATH);
#  endif
#endif
}

void BoxPaths_Add_Lib(BoxPaths *bp, char *lib) {
  BoxList_Append_String(& bp->libraries, lib);
}

void BoxPaths_Add_Lib_Dir(BoxPaths *bp, char *path) {
  BoxList_Append_String(& bp->lib_dirs, path);
}

void BoxPaths_Add_Pkg_Dir(BoxPaths *bp, char *path) {
  BoxList_Append_String(& bp->inc_dirs, path);
}

void BoxPaths_Add_Script_Path_To_Inc_Dir(BoxPaths *bp,
                                         const char *script_path) {
  if (script_path != NULL) {
    char *script_dir;
    Box_Split_Path(& script_dir, NULL, script_path);
    if (script_dir != NULL) {
      BoxList_Append_String(& bp->inc_dirs, script_dir);
      BoxMem_Free(script_dir);
      return;
    }
  }

  BoxList_Append_String(& bp->inc_dirs, ".");
}

char *BoxPaths_Find_Inc_File(BoxPaths *bp, const char *current_dir,
                             const char *file) {
  char *full_path = NULL;

  if (bp->flags & BOXPATHSFLAG_INC_SCRIPT_DIR) {
    if (current_dir != NULL)
      Box_Find_File_In_Dir(& full_path, file, current_dir, & bp->inc_exts);
  }

  if (full_path == NULL)
    Box_Find_File_In_Dirs(& full_path, file, & bp->inc_dirs, & bp->inc_exts);

  return full_path;
}

BoxList *BoxPaths_Get_Lib_Dir(BoxPaths *bp) {
  return & bp->lib_dirs;
}

BoxList *BoxPaths_Get_Libs(BoxPaths *bp) {
  return & bp->libraries;
}
