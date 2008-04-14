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

#include <stdlib.h>

#include "types.h"
#include "defaults.h"
#include "vmalloc.h"

/* #define DEBUG_VMALLOC */

typedef struct {
  Int type;
  Int references;
} VMAllocHead;

/* When the user asks to allocate 'size' bytes, we actually allocate
 * more than that. Indeed, we attach to every VM allocated area an extra
 * head, which contains a description about the allocated data.
 * This description contains: the number of references to the data
 * which have been made so far and the type of data.
 * The latter is important for destruction of objects: the VM is aware
 * about how objects should be destroyed, so that, when the reference
 * counter reaches zero, it knows what VM destructor function to call
 * before calling free().
 * We use the following nomenclature:
 *  - 'data' is the piece of allocated memory actually accessible to the user;
 *  - 'head' is the piece not accessible to the user, containing the reference
 *           count and other information;
 *  - 'block' is all the allocated data: 'head' plus 'data';
 *  - 'block pointer' the pointer to the 'block' memory area;
 *  - 'user pointer' the pointer received by the user (point to 'data');
 */

/** Get the size of the block containing 'data' of size 'dsize' */
#define SIZE_OF_BLOCK(dsize) ((dsize) + sizeof(VMAllocHead))

/** Get the 'head' from the 'user pointer' */
#define GET_HEAD_FROM_UPTR(uptr) \
  ((VMAllocHead *) ((void *) (uptr) - sizeof(VMAllocHead)))

/** Get the 'head' from the 'block pointer' */
#define GET_HEAD_FROM_BPTR(bptr) \
  ((VMAllocHead *) (bptr))

/** Get the 'head' from the 'user pointer' */
#define GET_BPTR_FROM_UPTR(uptr) \
  ((void *) (uptr) - sizeof(VMAllocHead))

/** Get the 'user pointer' from the 'block pointer' */
#define GET_UPTR_FROM_BPTR(bptr) \
  ((void *) (bptr) + sizeof(VMAllocHead))

/** Allocate size bytes and returns the pointer to that region.
 * The memory region is associated with the provided data 'type'
 * and has a initial reference counter equal to 1.
 */
void *VM_Alloc(size_t size, Int type) {
  void *bptr = malloc(SIZE_OF_BLOCK(size));
  VMAllocHead *head = GET_HEAD_FROM_BPTR(bptr);
  void *uptr = GET_UPTR_FROM_BPTR(bptr);
  if (bptr == (void *) NULL) return (void *) NULL;
  head->type = type;
  head->references = 1;
  return uptr;
}

/** Increase the reference counter for the given object. */
void VM_Link(void *uptr) {
  VMAllocHead *head = GET_HEAD_FROM_UPTR(uptr);
  ++head->references;
}

/** Decrease the reference counter for the given object and proceed
 * with destroying it, if it has reached zero.
 */
void VM_Unlink(void *uptr) {
  VMAllocHead *head = GET_HEAD_FROM_UPTR(uptr);
  Int references = --head->references;
  if (references > 0)
    return;

  else if (references == 0) {
    void *bptr = GET_BPTR_FROM_UPTR(uptr);
#ifdef DEBUG_VMALLOC
    printf("VM_Unlink: Deallocating object of type "SInt" at %p\n",
           head->type, bptr);
#endif
    free(bptr);
    return;

  } else {
#ifdef DEBUG_VMALLOC
    printf("VM_Unlink: shouldn't happen!\n");
#endif
    return;
  }
}
