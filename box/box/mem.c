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

/* Allocate in blocks of size sizeof(uint32_t), to make Valgrind happy! */
#define DO_ALIGN_SIZE

#include <stdlib.h>
#include <string.h>

/* I'm not really sure here if SIZEOF_BLOCK should always be sizeof(uint32_t).
 * This is why the following code here may seem a little bit non-sense.
 */
#ifdef HAVE_STDINT_H
#  include <stdint.h>
#  define SIZEOF_BLOCK (sizeof(uint32_t))
#else
#  define SIZEOF_BLOCK 4
#endif

#include "types.h"
#include "messages.h"
#include "mem.h"

size_t BoxMem_Size_Align(size_t n) {
  return ((n + SIZEOF_BLOCK - 1)/SIZEOF_BLOCK)*SIZEOF_BLOCK;
}

#include <stdio.h>

void *BoxMem_Alloc(size_t size) {
  void *ptr = malloc(BoxMem_Size_Align(size));
  if (ptr == NULL) BoxMem_Exit("malloc failed!");
#ifdef DEBUG_MEM
  printf("BoxMem_Alloc: returning %p\n", ptr);
#endif
  return ptr;
}

void BoxMem_Free(void *ptr) {
#ifdef DEBUG_MEM
  printf("BoxMem_Free: freeing %p\n", ptr);
  fflush(stdout);
#endif
  free(ptr);
}

void *BoxMem_Realloc(void *ptr, size_t size) {
  if (ptr == NULL) return BoxMem_Alloc(size);
#ifdef DEBUG_MEM
  printf("BoxMem_Realloc: returning %p\n", ptr);
#endif
  return realloc(ptr, size);
}

char *BoxMem_Strdup(const char *s) {
  size_t sl = strlen(s) + 1;
  char *sd = BoxMem_Alloc(sl);
  if (sd == (char *) NULL) BoxMem_Exit("strdup failed!");
  (void) memcpy(sd, s, sl);
  return sd;
}

void BoxMem_Exit(const char *msg) {
  MSG_FATAL("BoxMem_Exit: %s", msg);
  exit(EXIT_FAILURE);
}

/** Executes *r = a*x, returning 0 in case of integer overflow, 1 otherwise. */
int BoxMem_ax(size_t *r, size_t a, size_t x) {
 *r = a*x;
 return 1;
}

/** Executes *r = x + y, returning 0 in case of integer overflow, 1 otherwise.
 */
int BoxMem_x_Plus_y(size_t *r, size_t x, size_t y) {
 *r = x + y;
 return 1;
}

/** Executes *r = a*x + b*y, returning 0 in case of integer overflow,
 *  1 otherwise.
 */
int BoxMem_ax_Plus_by(size_t *r, size_t a, size_t x, size_t b, size_t y) {
 *r = a*x + b*y;
 return 1;
}
