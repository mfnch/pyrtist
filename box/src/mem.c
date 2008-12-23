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

void *BoxMem_Alloc(UInt size) {
  void *ptr = malloc(BoxMem_Size_Align(size));
  if (ptr == NULL) BoxMem_Exit("malloc failed!");
  return ptr;
}

void BoxMem_Free(void *ptr) {
  free(ptr);
}

void *BoxMem_Realloc(void *ptr, UInt size) {
  if (ptr == NULL) return BoxMem_Alloc(size);
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
