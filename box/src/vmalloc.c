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
#include "virtmach.h"
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
void VM_Unlink(VMProgram *vmp, void *uptr) {
  VMAllocHead *head = GET_HEAD_FROM_UPTR(uptr);
  Int references = --head->references;
  if (references > 0)
    return;

  else if (references == 0) {
    void *bptr = GET_BPTR_FROM_UPTR(uptr);
    Int method_num = VM_Alloc_Method_Get(vmp, head->type, VM_ALC_DESTRUCTOR);
    if (method_num >= 0) {
      VMStatus *vmcur;
      void *save_arg1;
#ifdef DEBUG_VMALLOC
      printf("VM_Unlink: calling destructor (call %d) for type "
             SInt" at %p.\n", method_num, head->type, bptr);
#endif
      vmcur = vmp->vmcur;
      save_arg1 = vmcur->arg1;
      vmcur->arg1 = uptr;
      (void) VM_Module_Execute(vmp, method_num);
      vmcur->arg1 = save_arg1;
    }
#ifdef DEBUG_VMALLOC
    printf("VM_Unlink: Deallocating object of type "SInt" at %p.\n",
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

Task VM_Alloc_Init(VMProgram *vmp) {
  HT(& vmp->method_table, VM_METHOD_HT_SIZE);
  return Success;
}

void VM_Alloc_Destroy(VMProgram *vmp) {
  HT_Destroy(vmp->method_table);
}

typedef struct {
  Int type;
  VMAlcMethod method;
} Key;

Task VM_Alloc_Method_Set(VMProgram *vmp, Int type, VMAlcMethod m, Int m_num) {
  Key k;
  k.type = type;
  k.method = m;
  /* Should I check for re-definition and how should I deal with that? */
  if (!HT_Insert_Obj(vmp->method_table,
                     & k, sizeof(Key),
                     & m_num, sizeof(Int)))
    return Failed;
  return Success;
}

Int VM_Alloc_Method_Get(VMProgram *vmp, Int type, VMAlcMethod m) {
  HashItem *found;
  Key k;
  k.type = type;
  k.method = m;
  if (HT_Find(vmp->method_table, & k, sizeof(Key), & found))
    return *((Int *) found->object);
  return -1;
}
