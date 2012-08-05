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

/** @file str.h
 * @brief Extra functions to deal with strings (used only by the compiler).
 *
 * This file contains some functions used to manipulate strings.
 * These functions are used only by the compiler and has nothing to do with
 * the implementation of the built-in Box Str object, which can be found
 * inside bltinstr.c
 */

#ifndef _STR_H
#  define _STR_H

#  include <box/types.h>

Task Str_Eq(char *a, char *b);
Task Str_Eq2(char *s1, UInt l1, char *s2, UInt l2);
Task Str_CaseEq2(char *s1, UInt l1, char *s2, UInt l2);
char *Str_DupLow(char *s, UInt leng);
char *Str_Dup(const char *s, UInt leng);
char *Str_Cut(const char *s, UInt maxleng, Int start);
char *Str__Cut(const char *s, UInt leng, UInt maxleng, Int start);
unsigned char oct_digit(unsigned char c, int *status);
unsigned char hex_digit(unsigned char c, int *status);

Task Str_ToInt(char *s, UInt l, Int *i);

/** Return the int (0 to 15) corresponding to the given hex digit (a char). */
int Box_Hex_Digit_To_Int(char digit);

/** Return the hex digit (a char) corresponding to the given int value. */
char Box_Hex_Digit_From_Int(int v);

Task Str_Hex_To_Int(char *s, UInt l, Int *out);
Task Str_ToReal(char *s, UInt l, Real *r);

/** Reads the escaped sequence in 's' whose length is 'l' and return
 * the corresponding char value in '*c'.
 * If 's' is a just a normal (unescaped) character, then this will be copied
 * to '*c'.
 */
BoxTask Box_Reduce_Esc_Char(const char *s, size_t l, char *c);

/** Reads the string 's' (with length 'l') and returns a string where the
 * escaped sequences are replaced by the corresponding characters.
 * The length of this new string is put into '*new_length'.
 */
char *Box_Reduce_Esc_String(const char *s, size_t l, size_t *new_length);

BoxName *BoxName_Empty(void);
const char *BoxName_Str(BoxName *n);
char *BoxName_To_Str(BoxName *n);
void BoxName_From_Str(BoxName *dest, char *src);
void BoxName_Free(BoxName *n);
BoxName *BoxName_Dup(BoxName *n);
Task BoxName_Cat(BoxName *nm, BoxName *nm1, BoxName *nm2, int free_args);
void *BoxMem_Dup(const void *src, unsigned int length);

/** Similar to BoxMem_Dup, but allocate extra space in the destination */
void *BoxMem_Dup_Larger(const void *src, Int src_size, Int dest_size);

/** Returns whether the string 'src' ends with 'end'. Both strings are
 * supposed to be NUL-terminated.
 */
int Box_CStr_Ends_With(const char *src, const char *end);

#define BoxName_Cat_And_Free(nm, nm1, nm2) \
  BoxName_Cat(nm, nm1, nm2, 1)

#endif

