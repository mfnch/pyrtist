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

/** @file print.h
 * @brief A simplified version of sprintf with automatic memory management.
 *
 * This file provides the function print, which is a simplified version
 * of the function sprintf, but offers a number of advantages over it.
 * Memory is managed automatically, the Name data type is supported
 * with the specified %N, string deallocation is allowed with %~s.
 */

#ifndef _PRINT_H
#  define _PRINT_H

#  include <stdarg.h>

#  define PRINT_BUF_SIZE 512

/** A simplified version of sprintf, with a number of desirable features:
 * - handles memory in a nice way: the user does not need to allocate/free
 *   the memory or to worry about buffer overflow when using the %s
 *   specifier to write substrings. Usage is as follows:
 *       const char *msg = print("string = '%s', number = '%d'\n", "Hi!", 12);
 *       (* No need to call the free(...) function. The user mustn't do it! *)
 * - has %N to handle the Name data type.
 *   ES:
 *       Name my_name = {15, "Matteo Franchin"};
 *       msg = print("My name is %N, nice to meet you!", & my_name);
 * - has %~s to print a string and deallocate it with free(...).
 *   ES:
 *       msg = print("%~s", strdup("allocated string"));
 */
const char *print(const char *fmt, ...);

#endif
