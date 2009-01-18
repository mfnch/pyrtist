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
Task Str_ToChar(char *s, Int l, char *c);
Task Str_ToInt(char *s, UInt l, Int *i);
Task Str_Hex_To_Int(char *s, UInt l, Int *out);
Task Str_ToReal(char *s, UInt l, Real *r);
char *Str_ToString(char *s, Int l, Int *new_length);
Name *Name_Empty(void);
const char *Name_Str(Name *n);
char *Name_To_Str(Name *n);
void Name_From_Str(Name *dest, char *src);
void Name_Free(Name *n);
Name *Name_Dup(Name *n);
Task Name_Cat(Name *nm, Name *nm1, Name *nm2, int free_args);
void *BoxMem_Dup(const void *src, unsigned int length);

/** Similar to BoxMem_Dup, but allocate extra space in the destination */
void *BoxMem_Dup_Larger(const void *src, Int src_size, Int dest_size);

#define Name_Cat_And_Free(nm, nm1, nm2) Name_Cat(nm, nm1, nm2, 1)

#  ifndef HAVE_STRNDUP
char *strndup(const char *s, int n);
#  endif

#endif
