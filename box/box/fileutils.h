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

/* $Id$ */

/** @file fileutils.h
 * @brief Some utilities for searching files (used just by the compiler).
 *
 *
 */

#ifndef _FILEUTILS_H
#  define _FILEUTILS_H

int File_Exist(const char *file_name);

void File_Find(BoxList **found_files, const char *file_name,
               BoxList *prefixes, BoxList *suffixes);

void File_Find_First(char **found_file, const char *file_name,
                     BoxList *prefixes, BoxList *suffixes);

/** Split the given path in 'full_path' into its directory component,
 * which is a string allocated in '*dir', and its file component,
 * a string allocated in '*file'. If there is no directory component
 * returns NULL. If 'dir' or 'file' are null, the corresponding string
 * is not allocated/returned. Examples:
 *
 *  File_Path_Split(dir, file, "/dira/dirb/file.ext") -->
 *     *dir = BoxMem_Strdup("/dira/dirb/"); *file = BoxMem_Strdup("file.ext");
 *
 *  File_Path_Split(dir, file, "/") -->
 *                       *dir = BoxMem_Strdup("/"); *file = BoxMem_Strdup("");
 *
 *  File_Path_Split(dir, file, "") --> *dir = NULL; *file = BoxMem_Strdup("");
 */
void File_Path_Split(char **dir, char **file, const char *full_path);

#endif
