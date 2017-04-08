/***************************************************************************
 *   Copyright (C) 2007-2013 by Matteo Franchin                            *
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

/* Allocate in blocks of size 4, to make Valgrind happy! */
#define ALLOCATE_INT32_BLOCKS 1

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "types.h"
#include "messages.h"
#include "mem.h"


size_t Box_Mem_Align_Offset(size_t offset, size_t alignment)
{
  assert(alignment > 0);
  return ((offset + alignment - 1)/alignment)*alignment;
}

#if 0
/** Box-C interface relies on Box aligning datastructures the same way it is
 * done by the C compiler which has been used to compile it. In general, it may
 * pretty difficult to get this right. Fortunately, many copilers (including
 * gcc) provide ``__alignof__'', a way to obtain the alignment for a given
 * type.
 */
size_t Box_Mem_Size_Align(size_t offset, size_t align)
{
  size_t align_block;
  if (size <= sizeof(void *))
    align_block = size;
  else
    align_block = sizeof(void *);
  return ((n + align_block - 1)/align_block)*align_block;
}
#endif

/* Undefine macros in case they have been defined in mem.h:
 *   such macros are typically useful for debugging, but we don't need them
 *   here!
 */
#ifdef BOXMEM_DEBUG_MACROS
#  undef Box_Mem_Safe_Alloc
#  undef Box_Mem_Alloc
#  undef Box_Mem_Free
#  undef Box_Mem_Realloc
#  undef Box_Mem_Strdup
#  undef Box_Mem_Strndup
#  undef Box_Mem_Str_Merge
#  undef Box_Mem_Str_Merge_With_Len
#endif

void *Box_Mem_Safe_Alloc(size_t size)
{
#if ALLOCATE_INT32_BLOCKS == 1
  size_t alloc_size = (size + 3) & ~((size_t) 3);
  void *ptr = (alloc_size >= size) ? malloc(alloc_size) : NULL;
#else
  void *ptr = malloc(size);
#endif

  if (!ptr)
    BOX_FATAL_ERROR();

#ifdef DEBUG_MEM
  printf("Box_Mem_Safe_Alloc: returning %p\n", ptr);
#endif
  return ptr;
}

void *Box_Mem_Alloc(size_t size)
{
  return Box_Mem_Safe_Alloc(size);
}

void Box_Mem_Free(void *ptr)
{
#ifdef DEBUG_MEM
  printf("Box_Mem_Free: freeing %p\n", ptr);
  fflush(stdout);
#endif
  free(ptr);
}

void *Box_Mem_Realloc(void *ptr, size_t size)
{
  if (ptr == NULL) return Box_Mem_Alloc(size);
#ifdef DEBUG_MEM
  printf("Box_Mem_Realloc: returning %p\n", ptr);
#endif
  return realloc(ptr, size);
}

char *Box_Mem_Strdup(const char *s)
{
  size_t s_length = strlen(s) + 1;
  char *s_copy = Box_Mem_Alloc(s_length);
  if (!s_copy)
    return NULL;

  (void) memcpy(s_copy, s, s_length);
  return s_copy;
}

char *Box_Mem_Strndup(const char *s, size_t length)
{
  size_t size;
  if (Box_Mem_x_Plus_y(& size, length, 1)) {
    char *sd = Box_Mem_Safe_Alloc(size);
    if (length > 0)
      (void) memcpy(sd, s, length);
    sd[length] = '\0';
    return sd;
  }
  return NULL;
}

char *Box_Mem_Str_Merge_With_Len(const char *str1, size_t l1,
                                 const char *str2, size_t l2)
{
  size_t l, ltot;
  char *s;
  int ok1 = Box_Mem_x_Plus_y(& l, l1, l2),
      ok2 = Box_Mem_x_Plus_y(& ltot, l, 1);
  if (ok1 && ok2) {
    s = Box_Mem_Alloc(ltot*sizeof(char));
    if (l1 > 0) memcpy(s, str1, l1);
    if (l2 > 0) memcpy(s + l1, str2, l2);
    s[l] = '\0';
    return s;
  }
  Box_Mem_Exit("Box_Mem_Str_Merge_With_Len: integer overflow.");
  return NULL;
}

char *Box_Mem_Str_Merge(const char *str1, const char *str2)
{
  size_t l1 = strlen(str1), l2 = strlen(str2);
  return Box_Mem_Str_Merge_With_Len(str1, l1, str2, l2);
}

void Box_Mem_Exit(const char *msg)
{
  MSG_FATAL("Box_Mem_Exit: %s", msg);
  abort();
}

void Box_Fatal_Error(const char *file, unsigned long line_no)
{
  fprintf(stderr, "Fatal error detected at '%s', line %ld.\n", file, line_no);
  abort();
}

/** Executes *r = a*x, returning 0 in case of integer overflow, 1 otherwise. */
int Box_Mem_AX(size_t *r, size_t a, size_t x)
{
 *r = a*x;
 return 1;
}

/** Executes *r = x + y, returning 0 in case of integer overflow, 1 otherwise.
 */
int Box_Mem_x_Plus_y(size_t *r, size_t x, size_t y)
{
 *r = x + y;
 return 1;
}

/** Executes *r = a*x + b*y, returning 0 in case of integer overflow,
 *  1 otherwise.
 */
int Box_Mem_AX_Plus_BY(size_t *r, size_t a, size_t x, size_t b, size_t y)
{
 *r = a*x + b*y;
 return 1;
}

void *Box_Mem_RC_Alloc(size_t s)
{
  size_t total;
  if (Box_Mem_x_Plus_y(& total, sizeof(BoxRefCount), s)) {
    void *whole = Box_Mem_Alloc(total);
    if (whole != NULL) {
      BoxRefCount *rc = whole;
      void *ptr = whole + sizeof(BoxRefCount);
      *rc = (BoxRefCount) 1;
      return ptr;
    }
  }

  return NULL;
}

void *Box_Mem_RC_Safe_Alloc(size_t s)
{
  void *ptr = Box_Mem_RC_Alloc(s);
  assert(ptr != NULL);
  return ptr;
}

void Box_Mem_RC_Link(void *ptr)
{
  void *whole = ptr - sizeof(BoxRefCount);
  BoxRefCount *rc = whole;
  ++*rc;
}

/* Get the number of references to this object. */
BoxRefCount Box_Mem_RC_Get_Num_Refs(void *ptr)
{
  void *whole = ptr - sizeof(BoxRefCount);
  BoxRefCount *prc = whole;
  return *prc;
}

int Box_Mem_RC_Unlink(void *ptr)
{
  void *whole = ptr - sizeof(BoxRefCount);
  BoxRefCount *prc = whole, rc = *prc;
  if (rc == 1) {
    Box_Mem_Free(whole);
    return 1;

  } else {
    assert(rc > 0);
    /* ^^^ this only makes sense if BoxRefCount is a signed integer. */
    --*prc;
    return 0;
  }
}