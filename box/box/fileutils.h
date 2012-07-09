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

/** @file fileutils.h
 * @brief Some utilities for searching files (used just by the compiler).
 */

#ifndef _FILEUTILS_H
#  define _FILEUTILS_H

#  include <box/types.h>
#  include <box/list.h>

BOXEXPORT int Box_File_Exist(const char *file_name);

BOXEXPORT void Box_Find_Files_In_Dirs(BoxList **found_files,
                                      const char *file_name,
                                      BoxList *prefixes, BoxList *suffixes);

/** Search a file with the given name (file_name) combining together all the
 * given prefixes and suffixes. Note that the function only returns the first
 * found file, searching sequentially in the provided directories and
 * considering sequentially the given extensions. The order of the element of
 * the two BoxList object matters for that.
 * @param found_file a pointer to the found file
 * @param file_name the name of the file to find
 * @param prefixes the directories (or prefixes) to consider
 * @param suffixes the extensions (or suffixes) to consider
 */
BOXEXPORT void Box_Find_File_In_Dirs(char **found_file, const char *file_name,
                                     BoxList *prefixes, BoxList *suffixes);

/**
 * Similar to Box_Find_File_In_Dirs, but search only inside one directory.
 * This is why this function only takes a string (prefix) rather than a
 * BoxList (prefixes).
 */
BOXEXPORT void Box_Find_File_In_Dir(char **found_file, const char *file_name,
                                    const char *prefix, BoxList *suffixes);

/**
 * Split the given path in 'full_path' into its directory component,
 * which is a string allocated in '*dir', and its file component,
 * a string allocated in '*file'. If there is no directory component
 * returns NULL. If 'dir' or 'file' are null, the corresponding string
 * is not allocated/returned. Examples:
 *
 *  Box_Split_Path(dir, file, "/dira/dirb/file.ext") -->
 *     *dir = BoxMem_Strdup("/dira/dirb/"); *file = BoxMem_Strdup("file.ext");
 *
 *  Box_Split_Path(dir, file, "/") -->
 *                       *dir = BoxMem_Strdup("/"); *file = BoxMem_Strdup("");
 *
 *  Box_Split_Path(dir, file, "") --> *dir = NULL; *file = BoxMem_Strdup("");
 */
BOXEXPORT void Box_Split_Path(char **dir, char **file, const char *full_path);

/**
 * This functions transforms the input Unix path in a way that it can be
 * understood in the current platform. In particular, when running on Windows
 * this function transforms slashes to backslashes.
 * @param srcpath A string allocated with BoxMem_Alloc containing the source
 *   path in Unix style.
 * @return This function tries to do a in-place transformation of the source
 *   string. When possible, it just returns srcpath. If this is not possible,
 *   then a new string is allocated and returned, while the original srcpath
 *   is freed with BoxMem_Free. In other words, the user passes allocation
 *   responsibility for the source string and receives allocation
 *   responsibility for the returned string.
 */
BOXEXPORT char *Box_Normalize_Path(char *unix_path);

#endif
