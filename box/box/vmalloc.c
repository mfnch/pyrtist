/****************************************************************************
 * Copyright (C) 2008-2010 by Matteo Franchin                               *
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

#define DEBUG_VMALLOC 0
#define DEBUG_VMALLOC_REFCHECK 0

typedef struct {
  BoxVMAllocID id;
  BoxInt       references;
} VMAllocHead;

#define BoxPtr_Get_Head(p) ((VMAllocHead *) (p)->block)

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

void BoxObj_Add_To_Ptr(BoxObj *item, size_t addr) {
  item->ptr += addr;
}

void BoxVMMethodTable_Set(BoxVMMethodTable *mt,
                          size_t size,
                          BoxVMCallNum initializer,
                          BoxVMCallNum finalizer) {
  mt->size = size;
  mt->initializer = initializer;
  mt->finalizer = finalizer;
  mt->copier = BOXVMCALLNUM_NONE;
  mt->mover = BOXVMCALLNUM_NONE;
}

/** Allocate size bytes and returns the pointer to that region.
 * The memory region is associated with the provided data 'type'
 * and has a initial reference counter equal to 1.
 */
void BoxVM_Alloc(BoxVM *vm, BoxPtr *obj, size_t size, BoxVMAllocID id) {
#if DEBUG_VMALLOC == 1
  static int num_alloc = 0;
#endif

  obj->block = BoxMem_Alloc(SIZE_OF_BLOCK(size));
  obj->ptr = NULL;
  if (obj->block != NULL) {
    VMAllocHead *head = (VMAllocHead *) obj->block;
    obj->ptr = GET_DPTR_FROM_BPTR(obj->block);
    head->id = id;
    head->references = 1;
#if DEBUG_VMALLOC == 1
    printf("BoxVM_Alloc(%d): allocated object with id "SInt" at %p.\n",
           num_alloc, id, obj->block);
    ++num_alloc;
#endif
    return;
  }

#if DEBUG_VMALLOC == 1
  printf("BoxVM_Alloc: failed to allocate object with ID "SInt" at %p.\n",
         id, obj->block);
#endif
}

/** Increase the reference counter for the given object. */
void BoxVM_Link(Obj *obj) {
  VMAllocHead *head = BoxPtr_Get_Head(obj);
  if (BoxPtr_Is_Detached(obj))
    return;
  ++head->references;
}

/** Decrease the reference counter for the given object and proceed
 * with destroying it, if it has reached zero.
 */
void BoxVM_Unlink(BoxVM *vmp, BoxPtr *obj) {
  VMAllocHead *head = BoxPtr_Get_Head(obj);
  BoxInt references;
#if DEBUG_VMALLOC == 1
  static int num_dealloc = 0;
#endif

  if (BoxPtr_Is_Detached(obj))
    return;

  //references = --head->references;
  if (references > 0)
    return;

  else if (references == 0) {
    if (head->id >= 1) {
      BoxVMMethodTable *mt = BoxVMMethodTable_From_Alloc_ID(vmp, head->id);
#if DEBUG_VMALLOC == 1
      if (mt == NULL) {
        printf("BoxVM_Unlink: no method table associated to the ID "SInt
               " for object at %p.\n", head->id, obj->block);
      }
#endif

      if (mt != NULL) {
        BoxVMCallNum finalizer = mt->finalizer;
#if DEBUG_VMALLOC == 1
        printf("BoxVM_Unlink: calling destructor (call "SInt") for ID "
               SInt" at %p.\n", finalizer, head->id, obj->block);
#endif
        BoxPtr save_current = *vmp->box_vm_current;
        *vmp->box_vm_current = *obj;
        (void) BoxVM_Module_Execute(vmp, finalizer);
        *vmp->box_vm_current = save_current;
      }
    }

#if DEBUG_VMALLOC == 1
    printf("BoxVM_Unlink(%d): Deallocating object of type "SInt" at %p.\n",
           num_dealloc, head->id, obj->block);
    ++num_dealloc;
#endif

#if DEBUG_VMALLOC_REFCHECK == 0
    BoxMem_Free(obj->block);
#endif
    obj->block = NULL;
    return;

  } else {
#if DEBUG_VMALLOC_REFCHECK == 1
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

int BoxVMMethodTable_Is_Empty(BoxVMMethodTable *mt) {
  return (   mt->initializer == BOXVMCALLNUM_NONE
          && mt->finalizer == BOXVMCALLNUM_NONE
          && mt->copier == BOXVMCALLNUM_NONE
          && mt->mover == BOXVMCALLNUM_NONE);
}

BoxVMAllocID BoxVMAllocID_From_Method_Table(BoxVM *vm, BoxVMMethodTable *mt) {
  BoxHTItem *ht_item;

  /* If all the methods are BOXVMCALLNUM_NONE, then we return the ID 0, which
   * is signals that the considered type is "simple" (does not require special
   * handling for finalization, etc).
   */
  if (BoxVMMethodTable_Is_Empty(mt))
    return BOXVMALLOCID_NONE;

  if (BoxHT_Find(& vm->id_from_mettab, mt, sizeof(BoxVMMethodTable),
                 & ht_item)) {
    return *((BoxVMAllocID *) ht_item->object);

  } else {
    BoxVMMethodTable *new_mt =
      (BoxVMMethodTable *) BoxArr_Push(& vm->mettab_from_id, mt);
    BoxVMAllocID alloc_id = BoxArr_Num_Items(& vm->mettab_from_id);

    if (NULL == BoxHT_Insert_Obj(& vm->id_from_mettab,
                                 new_mt, sizeof(BoxVMMethodTable),
                                 & alloc_id, sizeof(BoxVMAllocID))) {
      MSG_WARNING("BoxVM_Get_Alloc_ID: Failure in hashtable.");
    }

    return alloc_id;
  }
}

BoxVMMethodTable *BoxVMMethodTable_From_Alloc_ID(BoxVM *vm, BoxVMAllocID id) {
  if (id >= 1 && id <= BoxArr_Num_Items(& vm->mettab_from_id))
    return (BoxVMMethodTable *) BoxArr_Item_Ptr(& vm->mettab_from_id, id);
  else
    return (BoxVMMethodTable *) NULL;
}

BoxTask BoxVM_Obj_Create(BoxVM *vm, BoxPtr *obj, BoxVMAllocID id) {
  BoxVMMethodTable *mt = BoxVMMethodTable_From_Alloc_ID(vm, id);

  if (mt == NULL)
    return BOXTASK_FAILURE;

  else {
    BoxVMCallNum initializer = mt->initializer;
    BoxVM_Alloc(vm, obj, mt->size, id);

    if (initializer != BOXVMCALLNUM_NONE) {
      BoxTask success;
      BoxPtr save_current = *vm->box_vm_current;
#if DEBUG_VMALLOC == 1
      printf("BoxVM_Obj_Create: calling initializer (call "SInt") for ID "
             SInt" at %p.\n", initializer, id, obj->block);
#endif
      *vm->box_vm_current = *obj;
      success = BoxVM_Module_Execute(vm, initializer);
      *vm->box_vm_current = save_current;
      return success;

    } else
      return BOXTASK_OK;
  }
}

BoxTask BoxVM_Obj_Copy(BoxVM *vm, BoxVMAllocID id,
                       BoxPtr *dest, BoxPtr *src) {

#if DEBUG_VMALLOC == 1
    printf("BoxVM_Obj_Copy: %p:%p --[id:%d]--> %p:%p"
           src->block, src->ptr, (int) id,
           dest->block, dest->ptr,);
#endif

  BoxVMMethodTable *mt = BoxVMMethodTable_From_Alloc_ID(vm, id);
  if (mt == NULL) {
    MSG_ERROR("BoxVM_Obj_Copy: invalid alloc-ID.");
    return BOXTASK_ERROR;
  }

  if (mt->copier != BOXVMCALLNUM_NONE) {
    /* Custom copy */
#if DEBUG_VMALLOC == 1
    printf("BoxVM_Obj_Copy: calling custom copier: "SInt, mt->copier);
#endif
    return BoxVM_Module_Execute_With_Args(vm, mt->copier, dest, src);

  } else {
    /* Generic copy */
    (void) memcpy(dest->ptr, src->ptr, mt->size);
    return BOXTASK_OK;
  }
}






BoxVMObjDesc *BoxVMObjDesc_From_Alloc_ID(BoxVM *vm, BoxVMAllocID id) {
  assert(0);
  return 0;
}

typedef BoxTask (*BoxSubObjIterator)(BoxVM *vm, BoxVMObjDesc *desc,
                                     BoxPtr *sub, size_t addr, void *pass);

static BoxTask My_Obj_Iter(BoxVM *vm, BoxVMObjDesc *desc, BoxPtr *obj,
                           BoxSubObjIterator iter, void *pass) {
  BoxPtr sub;
  size_t parent_size = desc->size, i;

  sub.block = obj->block;
  for (i = 0; i < desc->num_subs; i++) {
    size_t addr = desc->subs[i].position;
    BoxVMAllocID id = desc->subs[i].alloc_id;

    assert(addr < parent_size);
    sub.ptr = obj->ptr + addr;

    BoxVMObjDesc *sub_desc = BoxVMObjDesc_From_Alloc_ID(vm, id);
    if (iter(vm, sub_desc, & sub, addr, pass))
      return BOXTASK_FAILURE;
  }

  return BOXTASK_OK;
}




static BoxTask My_Obj_Init(BoxVM *vm, BoxVMObjDesc *desc,
                           BoxPtr *obj, size_t addr, void *pass) {
  BoxVMCallNum initializer = desc->initializer;

  /* First all the sub-objects are initialized */
  if (My_Obj_Iter(vm, desc, obj, My_Obj_Init, NULL))
    return BOXTASK_FAILURE;

  /* Now the parent object is initialized */
  if (initializer == BOXVMCALLNUM_NONE)
    return BOXTASK_OK;
  else
    return BoxVM_Module_Execute_With_Args(vm, initializer, obj, NULL);
}

BoxTask BoxVM_Obj_Init(BoxVM *vm, BoxPtr *obj, BoxVMAllocID id) {
  BoxVMObjDesc *desc = BoxVMObjDesc_From_Alloc_ID(vm, id);
  if (desc == NULL)
    return BOXTASK_OK;
  else
    return My_Obj_Init(vm, desc, obj, 0, NULL);
}

static BoxTask My_Obj_Finish(BoxVM *vm, BoxVMObjDesc *desc,
                             BoxPtr *obj, size_t addr, void *pass) {
  BoxVMCallNum finalizer = desc->finalizer;

  /* First we finalize the parent, then all its children */
  if (finalizer != BOXVMCALLNUM_NONE)
    return BoxVM_Module_Execute_With_Args(vm, finalizer, obj, NULL);

  return My_Obj_Iter(vm, desc, obj, My_Obj_Finish, NULL);
}

BoxTask BoxVM_Obj_Finish(BoxVM *vm, BoxPtr *obj, BoxVMAllocID id) {
  BoxVMObjDesc *desc = BoxVMObjDesc_From_Alloc_ID(vm, id);
  if (desc == NULL)
    return BOXTASK_OK;
  else
    return My_Obj_Finish(vm, desc, obj, 0, NULL);
}

static BoxTask My_Obj_Copy(BoxVM *vm, BoxVMObjDesc *desc,
                           BoxPtr *dest, size_t addr, void *pass) {
  BoxPtr src;
  BoxVMCallNum copier = desc->copier;

  assert(desc != NULL && pass != NULL);

  src.block = ((BoxPtr *) pass)->block;
  src.ptr = ((BoxPtr *) pass)->ptr + addr;

  if (copier != BOXVMCALLNUM_NONE)
    return BoxVM_Module_Execute_With_Args(vm, copier, dest, & src);
  else
    return My_Obj_Iter(vm, desc, dest, My_Obj_Copy, & src);
}

BoxTask BoxVM_Obj_Copy2(BoxVM *vm, BoxPtr *dest, BoxPtr *src,
                       BoxVMAllocID id) {
  BoxVMObjDesc *desc = BoxVMObjDesc_From_Alloc_ID(vm, id);
  if (desc != NULL) {
    /* Copy the raw data */
    (void) memcpy(dest->ptr, src->ptr, desc->size);

    /* Copy objects with special copier */
    return My_Obj_Copy(vm, desc, dest, 0, src);

  } else {
    MSG_ERROR("BoxVM_Obj_Copy: unknown object id (%d).", (int) id);
    return BOXTASK_ERROR;
  }
}

BoxTask BoxVM_Obj_Relocate(BoxVM *vm, BoxPtr *dest, BoxPtr *src) {
  assert(!BoxPtr_Is_Null(dest) && !BoxPtr_Is_Null(src));
  /* ^^^ For now we assume non detached pointers */

  BoxVMMethodTable *mt;
  VMAllocHead *dest_head = BoxPtr_Get_Head(dest),
              *src_head = BoxPtr_Get_Head(src);
  BoxVMAllocID id = dest_head->id;

  /* If the two objects are the same, then clear the src pointer and exit */
  if (src->ptr == dest->ptr) {
    assert(src->block == dest->block);
    BoxPtr_Nullify(src);
    return BOXTASK_OK;
  }

  if (src_head->id != id) {
    MSG_ERROR("Cannot move object: source kind does not match "
              "destination kind.");
    return BOXTASK_ERROR;
  }

  mt = BoxVMMethodTable_From_Alloc_ID(vm, id);
  if (mt == NULL) {
    MSG_ERROR("BoxVM_Obj_Move: don't know how to move objects with "
              "alloc-id=%I", id);
    return BOXTASK_ERROR;
  }

  assert(mt != NULL && mt->size > 0);

  /* Call finalizer on destination (since it will be overwritten) */
  if (mt->finalizer ) {
    BoxVMCallNum finalizer = mt->finalizer;
#if DEBUG_VMALLOC == 1
    printf("BoxVM_Obj_Move: calling destructor on destination (call "SInt") "
           "for ID "SInt" at %p.\n", finalizer, id, dest->block);
#endif
    BoxPtr save_current = *vm->box_vm_current;
    *vm->box_vm_current = *dest;
    (void) BoxVM_Module_Execute(vm, finalizer);
    *vm->box_vm_current = save_current;
  }

  if (mt->mover != BOXVMCALLNUM_NONE) {
    MSG_FATAL("BoxVM_Obj_Move: custom object movers are not supported, yet!");
    assert(0);
    return BOXTASK_FAILURE;
  }

  /* Depending on whether the source is referenced somewhere else in the Box
   * program we act in two different ways:
   */
  if (src_head->references == 1) {
    /* We relocate the object in memory: we move the content of the object
     * from src to dest and dallocate src (only the container, we don't call
     * the destructor!)
     */
    (void) memcpy(dest->ptr, src->ptr, mt->size);

    /* Deallocate the source object */
    src_head->id = 0;
    BoxVM_Unlink(vm, src);
    return BOXTASK_OK;

  } else {
    /* We cannot move the object in memory, as there are other parts of the
     * code referring to it. We then duplicate the object.
     */
    assert(src_head->references > 1);
    MSG_FATAL("BoxVM_Obj_Move: not implemented, yet!");
    return BOXTASK_FAILURE;
  }
}

#if 0
typedef struct {
  BoxVMCallNum initializer, /**< Method to initialize the object */
               finalizer,   /**< Method to finalize the object */
               copier,      /**< Method to copy the object */
               mover;       /**< Method to move the object */
  size_t       size,        /**< Size of the object */
               num_subs;    /**< Number of sub-objects */
  BoxVMSubObj  subs[];      /**< Subobjects */
} BoxVMObjDesc;
#endif



