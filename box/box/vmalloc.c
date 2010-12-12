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
#include <assert.h>

#include "types.h"
#include "defaults.h"
#include "mem.h"
#include "messages.h"
#include "virtmach.h"
#include "vmalloc.h"

#if 0
#define DEBUG_VMALLOC
#define DEBUG_VMALLOC_REFCHECK
#endif

typedef struct {
  BoxVMAllocID id;
  BoxInt       references;
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
 *  - 'data pointer' the pointer received by the user (points to 'data');
 *  - 'object' an object of type 'Obj', i.e. an extended pointer made of two
 *    pointers: the pointer to the 'data' and the pointer to the 'block'.
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
#define GET_DPTR_FROM_BPTR(bptr) \
  ((void *) (bptr) + sizeof(VMAllocHead))

void BoxObj_Set_To_Null(BoxObj *o) {
  o->ptr = o->block = NULL;
}

int BoxObj_Is_Null(BoxObj *o) {
  return o->ptr == NULL;
}

void BoxObj_Add_To_Ptr(BoxObj *item, size_t addr) {
  item->ptr += addr;
}

/** Allocate size bytes and returns the pointer to that region.
 * The memory region is associated with the provided data 'type'
 * and has a initial reference counter equal to 1.
 */
void BoxVM_Alloc(BoxVM *vm, BoxPtr *obj, size_t size, BoxVMAllocID id) {
#ifdef DEBUG_VMALLOC
  static int num_alloc = 0;
#endif

  obj->block = BoxMem_Alloc(SIZE_OF_BLOCK(size));
  obj->ptr = NULL;
  if (obj->block != (void *) NULL) {
    VMAllocHead *head = (VMAllocHead *) obj->block;
    obj->ptr = GET_DPTR_FROM_BPTR(obj->block);
    head->id = id;
    head->references = 1;
#ifdef DEBUG_VMALLOC
    printf("BoxVM_Alloc(%d): allocated object with id "SInt" at %p.\n",
           num_alloc, id, obj->block);
    ++num_alloc;
#endif
    return;
  }

#ifdef DEBUG_VMALLOC
  printf("BoxVM_Alloc: failed to allocate object with ID "SInt" at %p.\n",
         id, obj->block);
#endif
}

/** Increase the reference counter for the given object. */
void BoxVM_Link(Obj *obj) {
  VMAllocHead *head = (VMAllocHead *) obj->block;
  if (BoxPtr_Is_Detached(obj))
    return;
  ++head->references;
}

/** Decrease the reference counter for the given object and proceed
 * with destroying it, if it has reached zero.
 */
void BoxVM_Unlink(BoxVM *vmp, BoxPtr *obj) {
  VMAllocHead *head = (VMAllocHead *) obj->block;;
  BoxInt references;
#ifdef DEBUG_VMALLOC
  static int num_dealloc = 0;
#endif

  if (BoxPtr_Is_Detached(obj))
    return;

  references = --head->references;
  if (references > 0)
    return;

  else if (references == 0 && 0) {
    if (head->id >= 1) {
      BoxVMMethodTable *mt = BoxVMMethodTable_From_Alloc_ID(vmp, head->id);
#ifdef DEBUG_VMALLOC
      if (mt == NULL) {
        printf("BoxVM_Unlink: no method table associated to the ID "SInt
               " for object at %p.\n", head->id, obj->block);
      }
#endif

      if (mt != NULL) {
        BoxVMCallNum finalizer = mt->finalizer;
#ifdef DEBUG_VMALLOC
        printf("BoxVM_Unlink: calling destructor (call "SInt") for ID "
               SInt" at %p.\n", finalizer, head->id, obj->block);
#endif
        BoxPtr save_current = *vmp->box_vm_current;
        *vmp->box_vm_current = *obj;
        (void) BoxVM_Module_Execute(vmp, finalizer);
        *vmp->box_vm_current = save_current;
      }
    }

#ifdef DEBUG_VMALLOC
    printf("BoxVM_Unlink(%d): Deallocating object of type "SInt" at %p.\n",
           num_dealloc, id, obj->block);
    ++num_dealloc;
#endif

#ifndef DEBUG_VMALLOC_REFCHECK
    BoxMem_Free(obj->block);
#endif
    obj->block = NULL;
    return;

  } else {
#ifdef DEBUG_VMALLOC_REFCHECK
    printf("BoxVM_Unlink: reference count is "SInt"!\n", references);
#endif
    return;
  }
}

Task BoxVM_Alloc_Init(BoxVM *vm) {
  BoxHT_Init_Default(& vm->id_from_mettab, BOXVM_METHOD_HT_SIZE);
  BoxArr_Init(& vm->mettab_from_id, sizeof(BoxVMMethodTable),
              BOXVM_METHOD_HT_SIZE);
  vm->next_alloc_id = 1;
  return Success;
}

void BoxVM_Alloc_Destroy(BoxVM *vm) {
  BoxHT_Finish(& vm->id_from_mettab);
  BoxArr_Finish(& vm->mettab_from_id);
}

BoxVMAllocID BoxVMAllocID_From_Method_Table(BoxVM *vm, BoxVMMethodTable *mt) {
  BoxHTItem *ht_item;
  if (BoxHT_Find(& vm->id_from_mettab, mt, sizeof(BoxVMMethodTable),
                 & ht_item)) {
    return *((BoxVMAllocID *) ht_item->key);

  } else {
    BoxVMMethodTable *new_mt =
      (BoxVMMethodTable *) BoxArr_Push(& vm->mettab_from_id, mt);
    BoxVMAllocID alloc_id = BoxArr_Num_Items(& vm->mettab_from_id);

    if (NULL != BoxHT_Insert_Obj(& vm->id_from_mettab,
                                 new_mt, sizeof(BoxVMMethodTable),
                                 & alloc_id, sizeof(BoxVMAllocID))) {
      MSG_WARNING("BoxVM_Get_Alloc_ID: Failure in hashtable.");
    }

    return alloc_id;
  }
}

BoxVMMethodTable *BoxVMMethodTable_From_Alloc_ID(BoxVM *vm, BoxVMAllocID id) {
  if (id >= 1 && id < BoxArr_Num_Items(& vm->mettab_from_id))
    return (BoxVMMethodTable *) BoxArr_Item_Ptr(& vm->mettab_from_id, id);
  else
    return (BoxVMMethodTable *) NULL;
}
