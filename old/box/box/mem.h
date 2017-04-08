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

/**
 * @file mem.h
 * @brief Wrappings for the system calls to allocate memory (malloc & co.).
 */

#ifndef _BOX_MEM_H
#  define _BOX_MEM_H

#  include <stdlib.h> /* defines size_t */

#  include <box/types.h>

/** Integer type used to count references */
typedef size_t BoxRefCount;

BOX_BEGIN_DECLS

/** Return the lowest multiple of sizeof(uint32_t) which is greater than n */
BOXEXPORT size_t Box_Mem_Size_Align(size_t n);

/** Align the given offset to the next multiple of 'alignment'. */
BOXEXPORT size_t Box_Mem_Align_Offset(size_t offset, size_t alignment);

/** Increase the given 'size' so that it is multiple of 'alignment'. */
#define Box_Mem_Get_Multiple_Size(size, alignment) \
  Box_Mem_Align_Offset((size), (alignment))

BOXEXPORT void *Box_Mem_Alloc(size_t size);

/** Similar to Box_Mem_Alloc, but never returns NULL (display an error message
 * and abort, instead!).
 */
BOXEXPORT void *Box_Mem_Safe_Alloc(size_t size);

#define Box_Mem_Safe_Alloc_Items(item_size, num_items) \
  Box_Mem_Safe_Alloc((item_size)*(num_items))

BOXEXPORT void *Box_Mem_Realloc(void *ptr, size_t size);

BOXEXPORT void Box_Mem_Free(void *ptr);

BOXEXPORT char *Box_Mem_Strdup(const char *s);

BOXEXPORT char *Box_Mem_Strndup(const char *s, size_t length);

/** Merge the first l1 characters of str1 with the first l2 characters of str2
 * and return a NUL terminated string made by the concatenation of the two
 * parts.
 */
BOXEXPORT char *Box_Mem_Str_Merge_With_Len(const char *str1, size_t l1,
                                           const char *str2, size_t l2);

/**
 * Merge two strings by callng Box_Mem_Str_Merge_With_Len() and using strlen to
 * get the lengths of the two strings (wh)
 */
BOXEXPORT char *Box_Mem_Str_Merge(const char *str1, const char *str2);

BOXEXPORT void Box_Mem_Exit(const char *msg);

/** Called to signal a fatal error condition and exit immediately. */
BOXEXPORT void Box_Fatal_Error(const char *file, unsigned long line_no);

/** Executes *r = a*x, returning 0 in case of integer overflow, 1 otherwise. */
BOXEXPORT int Box_Mem_AX(size_t *r, size_t a, size_t x);

/** Return a multiplied by b and abort in case of integer overflow. */
#define Box_Mem_Safe_AX(a, b) ((a)*(b))

/** Executes *r = x + y, returning 0 in case of integer overflow, 1 otherwise.
 */
BOXEXPORT int Box_Mem_x_Plus_y(size_t *r, size_t x, size_t y);

#define Box_Mem_Sum Box_Mem_x_Plus_y

/** Executes *r = a*x + b*y, returning 0 in case of integer overflow,
 *  1 otherwise.
 */
BOXEXPORT int Box_Mem_AX_Plus_BY(size_t *r, size_t a, size_t x,
                                 size_t b, size_t y);

/**
 * Provides functionality similar to 'malloc' with the difference that the
 * allocation is reference counted. In particular, this function does allocate
 * 's' bytes plus the space for an additional integer. The integer contains the
 * number of references to this block, which is initially set to 1.  The number
 * can be increase with Box_Mem_RC_Link and is decreased with
 * Box_Mem_RC_Unlink.  The block is freed when the reference count reaches
 * zero.
 */
BOXEXPORT void *Box_Mem_RC_Alloc(size_t s);

/**
 * Similar to 'Box_Mem_RC_Alloc' but aborts if the memory request fails.
 */
BOXEXPORT void *Box_Mem_RC_Safe_Alloc(size_t s);

/**
 * Increase the reference count associated with the block 'ptr'.
 */
BOXEXPORT void Box_Mem_RC_Link(void *ptr);

/**
 * Get the number of references to this object.
 * @param ptr Object allocated with Box_Mem_RC_Alloc or Box_Mem_RC_Safe_Alloc.
 * @return Number of references to this object.
 */
BOXEXPORT BoxRefCount Box_Mem_RC_Get_Num_Refs(void *ptr);

/**
 * Decrease the reference count. The block is freed if the reference count
 * reaches zero.
 * @return 1 if the block was freed (reference count reached zero),
 *   0 otherwise.
 */
BOXEXPORT int Box_Mem_RC_Unlink(void *ptr);

BOX_END_DECLS

/*#define BOXMEM_DEBUG_MACROS*/

#ifdef BOXMEM_DEBUG_MACROS
#  define BOXMEM_MACRO1(fn, ...) (fn(__VA_ARGS__) + ((size_t) 0*printf(#fn" in %s (%d) \n", __FILE__, __LINE__)))
#  define BOXMEM_MACRO2(fn, ...) {fn(__VA_ARGS__); (void) printf(#fn" in %s (%d) \n", __FILE__, __LINE__);}

#  define Box_Mem_Safe_Alloc(...) BOXMEM_MACRO1(Box_Mem_Safe_Alloc, __VA_ARGS__)
#  define Box_Mem_Alloc(...) BOXMEM_MACRO1(Box_Mem_Alloc, __VA_ARGS__)
#  define Box_Mem_Realloc(...) BOXMEM_MACRO1(Box_Mem_Realloc, __VA_ARGS__)
#  define Box_Mem_Free(...) BOXMEM_MACRO2(Box_Mem_Free, __VA_ARGS__)
#  define Box_Mem_Strdup(...) BOXMEM_MACRO1(Box_Mem_Strdup, __VA_ARGS__)
#  define Box_Mem_Strndup(...) BOXMEM_MACRO1(Box_Mem_Strndup, __VA_ARGS__)
#  define Box_Mem_Str_Merge(...) BOXMEM_MACRO1(Box_Mem_Str_Merge, __VA_ARGS__)
#  define Box_Mem_Str_Merge_With_Len(...) \
     BOXMEM_MACRO1(Box_Mem_Str_Merge_With_Len, __VA_ARGS__)
#endif
#endif