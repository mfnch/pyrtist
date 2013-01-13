/****************************************************************************
 * Copyright (C) 2011 by Matteo Franchin                                    *
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

/** @file str.h
 * @brief Implementation of the Str type in the Box language (C API).
 *
 * A nice description...
 */

#ifndef _BOX_STR_H
#  define _BOX_STR_H

/** @brief The Str object.
 */
typedef struct {
  BoxInt length;      /**< the current length of the string */
  BoxInt buffer_size; /**< the size of the block allocated to contain
                           the string. */
  char   *ptr;        /**< the pointer to the block allocated to contain
                           the string. */
} BoxStr;

/** Just for compatibility */
typedef BoxStr Str;

BOXEXPORT void BoxStr_Init(BoxStr *s);

BOXEXPORT void BoxStr_Finish(BoxStr *s);

/** Set 'dest' with the content of 'src', deleting whatever was in 'dest'
 * before. 
 * NOTE: this requires 'dest' to be a properly initialized BoxStr object.
 */
BOXEXPORT BoxTask BoxStr_Set(BoxStr *dest, const BoxStr *src);

/** Set 'dest' with the content of 'src', deleting whatever was in 'dest'
 * before. This is similar to BoxStr_Set, but uses a C string as source.
 * NOTE: this requires 'dest' to be a properly initialized BoxStr object.
 */
BOXEXPORT BoxTask BoxStr_Set_From_C_String(BoxStr *dest, const char *src);

BOXEXPORT BoxTask BoxStr_Large_Enough(BoxStr *s, BoxInt length);

BOXEXPORT BoxTask BoxStr_Concat(BoxStr *dest, const BoxStr *src);

BOXEXPORT BoxTask BoxStr_Concat_C_String(BoxStr *s, const char *ca);

/** Initialise a string 'new_str' from another existing string 'src'. */
BOXEXPORT BoxTask BoxStr_Init_From(BoxStr *new_str, const BoxStr *src);

/** Create a new C string from a BoxStr object. The string is freshly
 * allocated with Box_Mem_Safe_Alloc.
 */
BOXEXPORT char *BoxStr_To_C_String(BoxStr *s);



/** Return the pointer to the raw data in the string. */
BOXEXPORT char *BoxStr_Get_Ptr(BoxStr *s);

/** Return the size of the data in the string. */
BOXEXPORT size_t BoxStr_Get_Size(BoxStr *s);

/** Compare two strings and return an integer less than, equal to, or greater
 * than zero if s1 is found, respectively, to be less than, to match, or be
 * greater than s2.
 */
BOXEXPORT int BoxStr_Compare(const BoxStr *s1, const BoxStr *s2);

#endif /* _BOX_STR_H */
