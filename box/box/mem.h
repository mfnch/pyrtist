/***************************************************************************
 *   Copyright (C) 2007 by Matteo Franchin                                 *
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

/** @file mem.h
 * @brief Wrappings for the system calls to allocate memory (malloc & co.).
 */

#ifndef _BOX_MEM_H
#  define _BOX_MEM_H

#  include <stdlib.h> /* defines size_t */

#  include <box/types.h>

/** Return the lowest multiple of sizeof(uint32_t) which is greater than n */
size_t BoxMem_Size_Align(size_t n);

void *BoxMem_Alloc(size_t size);

void *BoxMem_Realloc(void *ptr, size_t size);

void BoxMem_Free(void *ptr);

char *BoxMem_Strdup(const char *s);

char *BoxMem_Strndup(const char *s, size_t length);

void BoxMem_Exit(const char *msg);

/** Executes *r = a*x, returning 0 in case of integer overflow, 1 otherwise. */
int BoxMem_ax(size_t *r, size_t a, size_t x);

/** Executes *r = x + y, returning 0 in case of integer overflow, 1 otherwise.
 */
int BoxMem_x_Plus_y(size_t *r, size_t x, size_t y);

/** Executes *r = a*x + b*y, returning 0 in case of integer overflow,
 *  1 otherwise.
 */
int BoxMem_ax_Plus_by(size_t *r, size_t a, size_t x, size_t b, size_t y);

#if 0
#  define BoxMem_Alloc(x) (BoxMem_Alloc(x) + 0*printf("BoxMem_Alloc in %s (%d) \n", __FILE__, __LINE__))
#  define BoxMem_Realloc(x, y) (BoxMem_Realloc(x, y) + 0*printf("BoxMem_Relloc in %s (%d) \n", __FILE__, __LINE__))
#  define BoxMem_Free(x) {BoxMem_Free(x); (void) printf("BoxMem_Free in %s (%d) \n", __FILE__, __LINE__);}
#endif
#endif
