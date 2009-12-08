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

/* $Id$ */

/** @file paths.h
 * @brief Handles the paths used to search for Box headers and libraries.
 *
 * Box headers and libraries are searched in some well defined system paths.
 * This files handles this list of paths.
 */

#ifndef _PATHS_H
#  define _PATHS_H

#  include <stdio.h>

#  include "list.h"

extern BoxList libraries;
extern BoxList lib_dirs;
extern BoxList inc_dirs;
extern BoxList inc_exts;

void Path_Init(void);

void Path_Destroy(void);

/** Returns into '*pkg_path' and '*lib_path' two newly allocated strings
 * containing the path to the packages and the path to the library files.
 * NOTE: These two strings must be allocated by the caller with BoxMem_Free.
 */
void Path_Get_Bltin_Pkg_And_Lib_Paths(char **pkg_path, char **lib_path);

void Path_Set_All_From_Env(void);

void Path_Add_Lib(char *lib);

void Path_Add_Lib_Dir(char *path);

void Path_Add_Pkg_Dir(char *path);

/** Add the directory containing the currently executed script to the list
 * of directories to be searched when including a file with "include".
 * 'script_path' is the name of the script as passed to Box through
 * the command line.
 */
void Path_Add_Script_Path_To_Inc_Dir(const char *script_path);

FILE *Path_Open_Inc_File(const char *file, const char *mode);

#endif
