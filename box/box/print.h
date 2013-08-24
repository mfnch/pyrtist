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

/**
 * @file print.h
 * @brief A simplified version of sprintf with automatic memory management.
 *
 * This file provides the function print, which is a simplified version
 * of the function sprintf, but offers a number of advantages over it.
 * Memory is managed automatically, the Name data type is supported
 * with the specified %N, string deallocation is allowed with %~s.
 */

#ifndef _BOX_PRINT_H
#  define _BOX_PRINT_H

#  include <stdarg.h>

#  include <box/mem.h>

#  define BOX_PRINT_BUF_SIZE 512

/**
 * To be called when exiting, to release memory allocated by Box_Print().
 */
BOXEXPORT void
Box_Print_Finish(void);

/**
 * @brief An improved version of sprintf used internally by the compiler.
 *
 * An improved version of sprintf, with a number of desirable features:
 *
 * - handles memory in a nice way: the user does not need to allocate/free
 *   the memory or to worry about buffer overflow when using the %s
 *   specifier to write substrings. Usage is as follows:
 *
 * @code
 * const char *msg = Box_Print("string = %s, number = %d\n", "Hi!", 12);
 * // No need to call the free(...) function. The user mustn't do it!
 * @endcode
 *
 * - has additional format specifiers to handle objects like #BoxName or
 *   #BoxType.
 *
 * - has %~s to print a string and deallocate it with free(...).
 *
 * @code
 * msg = Box_Print("%~s", Box_Mem_Strdup("allocated string"));
 * @endcode
 */
BOXEXPORT const char *
Box_Print(const char *fmt, ...);

#  define printdup(...) Box_Mem_Strdup(Box_Print(__VA_ARGS__))
#  define Box_SPrintF(...) Box_Mem_Strdup(Box_Print(__VA_ARGS__))

#endif /* _BOX_PRINT_H */
