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
 * @file strutils.h
 * @brief Extra functions to deal with strings (used only by the compiler).
 *
 * This file contains some functions used to manipulate strings.
 * These functions are used only by the compiler and has nothing to do with
 * the implementation of the built-in Box Str object, which can be found
 * inside bltinstr.c
 */

#ifndef _BOX_STRUTILS_H
#  define _BOX_STRUTILS_H

#  include <box/types.h>

/**
 * @brief Return an abbreviation of the input string.
 *
 * Abbreviate a string when its length is above a provided maximum value.
 * Abbreviation is performed by dropping characters and replacing them with
 * "...".
 *
 * @param s Input null-terminated string.
 * @param max_length Maximum length at which abbreviation is triggered.
 * @param start_percent Position (in percentage) at which characters should
 *   be omitted in order to achieve the abbreviation. <= 0 means that leading
 *   characters should be dropped. >= 100 means that trailing characters should
 *   be dropped.
 * @return A null-terminated string not larger than @p max_length characters.
 */
BOXEXPORT char *
Box_Abbrev_Str(const char *s, size_t max_length, int start_percent);

/**
 * @brief Return the numerical value of a character when interpreted as hex.
 *
 * @param digit Input character, to be interpreter as an hexadecimal digit.
 * @return Value from 0 to 15 corresponding to the input character or -1 if the
 *   character is not a hex digit.
 */
BOXEXPORT int
Box_Hex_Digit_To_Int(char digit);

/**
 * @brief Return the hex digit (a char) corresponding to the given int value.
 */
BOXEXPORT char
Box_Hex_Digit_From_Int(int v);

/**
 * @brief Return the numerical value of an integer.
 *
 * @param s Pointer to the input string.
 * @param s_length Length of the input string.
 * @param out Where to put the output number.
 * @return @c NULL is returned if the operation succeeds. Otherwise, an error
 *   message is returned.
 */
BOXEXPORT const char *
Box_Str_To_Int(const char *s, BoxUInt s_length, BoxInt *out);

/**
 * @brief Convert a string of a hex number to a BoxInt.
 *
 * Same parameters and usage as Box_Str_To_Int().
 */
BOXEXPORT const char *
Box_Hex_Str_To_Int(const char *s, size_t s_length, BoxInt *out);

/**
 * @brief Convert a string of to a BoxReal number.
 *
 * Same parameters and usage as Box_Str_To_Int().
 */
BOXEXPORT const char *
Box_Str_To_Real(const char *s, size_t s_length, BoxReal *out);

/**
 * @brief Expand the given escaped character.
 *
 * @param s Input escaped string.
 * @param s_length Length of input string @p s.
 * @param out Where the output character is stored.
 * @return @c NULL on success, otherwise a string containing the error message.
 */
BOXEXPORT const char *
Box_Expand_Escaped_Char(const char *s, size_t s_length, char *out);

/**
 * @brief Expand the given escaped string.
 *
 * @param s The given input string.
 * @param s_length The length of the input string.
 * @param out Where to store the output string (allocated with Box_Mem_Alloc()).
 * @param out_length Where to store the length of the output string.
 * @return @c NULL on success, otherwise return a string containing the error
 *   message.
 */
BOXEXPORT const char *
Box_Expand_Escaped_Str(const char *s, size_t s_length,
                       char **out, size_t *out_length);

BOXEXPORT void *
Box_Mem_Dup(const void *src, unsigned int length);

/**
 * @brief Check whether a given string terminates with another given string.
 *
 * @param src Input string.
 * @param term Termination string.
 * @return Whether @p src terminates with @p term.
 */
BOXEXPORT int
Box_CStr_Ends_With(const char *src, const char *term);

#endif /* _BOX_STRUTILS_H */