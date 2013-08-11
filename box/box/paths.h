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

/** BoxPaths options. */
typedef enum {
  BOXPATHSFLAG_INC_SCRIPT_DIR=0x1,
  BOXPATHSFLAG_UNSET=0x0,
  BOXPATHSFLAG_SET=0x1
} BoxPathsFlag;

/** A BoxPaths object contains information about where to search for various
 * files during the compilation of a Box program.
 */
typedef struct {
  BoxList libraries, /**< List of libraries */
          lib_dirs,  /**< Paths for libraries */
          inc_dirs,  /**< Paths for Box headers */
          inc_exts;  /**< Possible extensions of Box headers */
  BoxPathsFlag
          flags;     /**< Always add the script directory as an include
                          directory. */
} BoxPaths;

/** Initialize a BoxPaths object. */
BOXEXPORT void BoxPaths_Init(BoxPaths *bp);

/** Finalize a BoxPaths object. */
BOXEXPORT void BoxPaths_Finish(BoxPaths *bp);

/** Set BoxPaths flags. */
BOXEXPORT void BoxPaths_Set_Flags(BoxPaths *bp,
                                  BoxPathsFlag mask, BoxPathsFlag val);

/** Returns into '*pkg_path' and '*lib_path' two newly allocated strings
 * containing the path to the packages and the path to the library files.
 * NOTE: These two strings must be allocated by the caller with Box_Mem_Free.
 */
BOXEXPORT void Box_Get_Bltin_Pkg_And_Lib_Paths(char **pkg_path,
                                               char **lib_path);
BOXEXPORT void BoxPaths_Set_All_From_Env(BoxPaths *bp);

BOXEXPORT void BoxPaths_Add_Lib(BoxPaths *bp, const char *lib);

BOXEXPORT void BoxPaths_Add_Lib_Dir(BoxPaths *bp, const char *path);

BOXEXPORT void BoxPaths_Add_Pkg_Dir(BoxPaths *bp, const char *path);

/** Add the directory containing the currently executed script to the list
 * of directories to be searched when including a file with "include".
 * 'script_path' is the name of the script as passed to Box through
 * the command line.
 */
BOXEXPORT void BoxPaths_Add_Script_Path_To_Inc_Dir(BoxPaths *bp,
                                                   const char *script_path);

/** Locate the file with name 'file' scanning the directory 'current_dir'
 * (if not NULL) and the directories specified in the BoxPaths object 'bp'.
 * The first file which is found is opened with fopen using 'mode' for the
 * opening mode. The FILE pointer is returned.
 */
BOXEXPORT char *BoxPaths_Find_Inc_File(BoxPaths *bp, const char *current_dir,
                                       const char *file);

/** Return the list of directories to be used when searching for libraries.
 * NOTE: the original list is returned (not a copy). This means that the user
 *   must not free the BoxList object, as it is still under the responsibility
 *   of the parent BoxPaths object.
 */
BOXEXPORT BoxList *BoxPaths_Get_Lib_Dir(BoxPaths *bp);

/** Return the list of libraries to be used when linking the program.
 * NOTE: the original list is returned (not a copy). This means that the user
 *   must not free the BoxList object, as it is still under the responsibility
 *   of the parent BoxPaths object.
 */
BOXEXPORT BoxList *BoxPaths_Get_Libs(BoxPaths *bp);

#endif
