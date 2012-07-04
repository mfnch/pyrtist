/****************************************************************************
 * Copyright (C) 2008-2011 by Matteo Franchin                               *
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
#include "vm_private.h"
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

void BoxPtr_Add_To_Ptr(BoxPtr *item, size_t addr) {
  item->ptr += addr;
}

/** Structure used to store the object descriptors and their names */
typedef struct {
  char         *name;
  BoxVMObjDesc *desc;
} MyDescEntry;

/** Finalizer for the MyDescEntry structures */
static void My_DescEntry_Finalizer(void *entry) {
  BoxMem_Free(((MyDescEntry *) entry)->name);
  BoxMem_Free(((MyDescEntry *) entry)->desc);
}

/** Initialises the table of object descriptors */
BoxTask BoxVM_Alloc_Init(BoxVM *vm) {
  /* Create an hash table which copies the keys, but not the objects */
  BoxHT_Init_Default(& vm->id_from_desc, BOXVM_DESC_HT_SIZE);
  BoxHT_Set_Attr(& vm->id_from_desc, BOXHTATTR_COPY_KEYS, 0);

  BoxArr_Init(& vm->desc_from_id, sizeof(MyDescEntry), BOXVM_DESC_HT_SIZE);
  BoxArr_Set_Finalizer(& vm->desc_from_id, My_DescEntry_Finalizer);
  return Success;
}

/** Finalises the table of object descriptors */
void BoxVM_Alloc_Destroy(BoxVM *vm) {
  BoxHT_Finish(& vm->id_from_desc);
  BoxArr_Finish(& vm->desc_from_id);
}

/** Whether an object descriptor is empty */
int BoxVMObjDesc_Is_Empty(BoxVMObjDesc *od) {
  return (   od->initializer == BOXVMCALLNUM_NONE
          && od->finalizer == BOXVMCALLNUM_NONE
          && od->copier == BOXVMCALLNUM_NONE
          && od->mover == BOXVMCALLNUM_NONE
          && od->num_subs == 0);
}

static char *My_ObjDesc_To_Str(BoxVM *vm, BoxVMAllocID id, char *indentation) {
  MyDescEntry *de = (MyDescEntry *) BoxArr_Item_Ptr(& vm->desc_from_id, id);
  BoxVMObjDesc *od = de->desc;
  char *s =
    Box_SPrintF("%s: size=%d, n.subs=%d, has=%s%s%s%s, "
                "I=%d, F=%d, C=%d, R=%d\n",
                de->name, (int) od->size, (int) od->num_subs,
                (od->has.initializer) ? "I" : "i",
                (od->has.finalizer) ? "F" : "f",
                (od->has.copier) ? "C" : "c",
                (od->has.mover) ? "R" : "r",
                (int) od->initializer,
                (int) od->finalizer,
                (int) od->copier,
                (int) od->mover);
  char *indentation2 = Box_SPrintF("  %s", indentation);
  BoxVMSubObj *sub = & od->subs[0];
  size_t i;

  for (i = 0; i < od->num_subs; i++, sub++) {
    char *ss = My_ObjDesc_To_Str(vm, sub->alloc_id, indentation2);
    s = Box_SPrintF("%~s%s%d (%d): %~s",
                    s, indentation2, i, sub->position, ss);
  }

  BoxMem_Free(indentation2);
  return s;
}

char *BoxVMObjDesc_To_Str(BoxVM *vm, BoxVMAllocID id) {
  if (id >= 1 && id <= BoxArr_Num_Items(& vm->desc_from_id))
    return My_ObjDesc_To_Str(vm, id, "");

  else
    return (char *) NULL;
}

char *BoxVM_ObjDesc_Table_To_Str(BoxVM *vm) {
  char *s = NULL;
  size_t id, n = BoxArr_Num_Items(& vm->desc_from_id);
  for (id = 1; id <= n; id++) {
    if (s == NULL)
      s = BoxVMObjDesc_To_Str(vm, id);
    else
      s = Box_SPrintF("%~s%~s", s, BoxVMObjDesc_To_Str(vm, id));
  }

  return (s != NULL) ? s : Box_SPrintF("");
}

BoxVMAllocID BoxVMAllocID_From_ObjDesc(BoxVM *vm, BoxVMObjDesc **od_ptr) {
  BoxHTItem *ht_item;
  BoxVMObjDesc *od = *od_ptr;

  /* If all the methods are BOXVMCALLNUM_NONE, then we return the ID 0, which
   * signals that the considered type is "simple" (does not require special
   * handling for finalization, etc).
   */
  if (BoxVMObjDesc_Is_Empty(od))
    return BOXVMALLOCID_NONE;

  if (BoxHT_Find(& vm->id_from_desc, od, BOXVMOBJDESC_SIZE(od), & ht_item)) {
    return *((BoxVMAllocID *) ht_item->object);

  } else {
    MyDescEntry *de = (MyDescEntry *) BoxArr_Push(& vm->desc_from_id, NULL);
    BoxVMAllocID alloc_id = BoxArr_Num_Items(& vm->desc_from_id);
    de->name = (char *) NULL;
    de->desc = od;
    *od_ptr = (BoxVMObjDesc *) NULL; /* To communicate we stole the pointer */

    if (NULL == BoxHT_Insert_Obj(& vm->id_from_desc,
                                 od, BOXVMOBJDESC_SIZE(od),
                                 & alloc_id, sizeof(BoxVMAllocID))) {
      MSG_WARNING("BoxVMAllocID_From_ObjDesc: Failure in hashtable.");
    }

    return alloc_id;
  }
}

BoxVMObjDesc *BoxVMObjDesc_From_Alloc_ID(BoxVM *vm, BoxVMAllocID id) {
  if (id >= 1 && id <= BoxArr_Num_Items(& vm->desc_from_id))
    return ((MyDescEntry *) BoxArr_Item_Ptr(& vm->desc_from_id, id))->desc;
  else
    return (BoxVMObjDesc *) NULL;
}

const char *BoxVM_Get_Obj_Name(BoxVM *vm, BoxVMAllocID id) {
  if (id >= 1 && id <= BoxArr_Num_Items(& vm->desc_from_id))
    return ((MyDescEntry *) BoxArr_Item_Ptr(& vm->desc_from_id, id))->name;
  else
    return (char *) NULL;
}

void BoxVM_Set_Obj_Name(BoxVM *vm, BoxVMAllocID id, char *name) {
  if (id >= 1 && id <= BoxArr_Num_Items(& vm->desc_from_id)) {
    MyDescEntry *entry = BoxArr_Item_Ptr(& vm->desc_from_id, id);
    BoxMem_Free(entry->name);
    entry->name = name;
  }
}

/** Function type used to iterate over the subobjects of an object
 * (given a descriptor).
 */
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

    if (iter(vm, sub_desc, & sub, addr, pass) != BOXTASK_OK)
      return BOXTASK_FAILURE;
  }

  return BOXTASK_OK;
}

/* XXX NOTE: we have to revisit the code below. Exceptions during
 * initialization or finalization may lead to memory leaks.
 */

static BoxTask My_Obj_Init(BoxVM *vm, BoxVMObjDesc *desc,
                           BoxPtr *obj, size_t addr, void *pass) {
  BoxVMCallNum initializer = desc->initializer;

  /* First all the sub-objects are initialized */
  if (desc->num_subs > 0) {
    BoxTask t = My_Obj_Iter(vm, desc, obj, My_Obj_Init, NULL);
    if (t != BOXTASK_OK)
      return t;
  }

  /* Now the parent object is initialized */
  if (initializer == BOXVMCALLNUM_NONE)
    return BOXTASK_OK;
  else
    return BoxVM_Module_Execute_With_Args(vm->vmcur, initializer, obj, NULL);
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
  if (finalizer != BOXVMCALLNUM_NONE) {
    BoxTask t = BoxVM_Module_Execute_With_Args(vm->vmcur, finalizer, obj, NULL);
    if (t != BOXTASK_OK)
      return t;
  }

  if (desc->num_subs > 0)
    return My_Obj_Iter(vm, desc, obj, My_Obj_Finish, NULL);
  else
    return BOXTASK_OK;
}

BoxTask BoxVM_Obj_Finish(BoxVM *vm, BoxPtr *obj, BoxVMAllocID id) {
  BoxVMObjDesc *desc = BoxVMObjDesc_From_Alloc_ID(vm, id);
  if (desc == NULL)
    return BOXTASK_OK;
  else
    return My_Obj_Finish(vm, desc, obj, 0, NULL);
}



/** Allocate size bytes and returns the pointer to that region.
 * The memory region is associated with the provided data 'type'
 * and has a initial reference counter equal to 1.
 */
void BoxVM_Obj_Alloc(BoxVM *vm, BoxPtr *obj, size_t size, BoxVMAllocID id) {
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
    printf("BoxVM_Obj_Alloc(%d): allocated object with id "SInt" at %p.\n",
           num_alloc, id, obj->block);
    ++num_alloc;
#endif
    return;
  }

#if DEBUG_VMALLOC == 1
  printf("BoxVM_Obj_Alloc: failed to allocate object with ID "SInt" at %p.\n",
         id, obj->block);
#endif
}

/** Increase the reference counter for the given object. */
void BoxVM_Obj_Link(BoxPtr *obj) {
  VMAllocHead *head = BoxPtr_Get_Head(obj);
  if (BoxPtr_Is_Detached(obj))
    return;
  ++head->references;
  assert(head->references >= 2);
}

/** Decrease the reference counter for the given object and proceed
 * with destroying it, if it has reached zero.
 */
void BoxVM_Obj_Unlink(BoxVM *vmp, BoxPtr *obj) {
  VMAllocHead *head = BoxPtr_Get_Head(obj);
  BoxInt references;
#if DEBUG_VMALLOC == 1
  static int num_dealloc = 0;
#endif

  if (BoxPtr_Is_Detached(obj))
    return;

  references = head->references - 1;
  if (references > 0) {
    head->references = references;
    return;

  } else if (references == 0) {
    if (head->id > 0)
      (void) BoxVM_Obj_Finish(vmp, obj, head->id);

#if DEBUG_VMALLOC == 1
    printf("BoxVM_Obj_Unlink(%d): deallocated object of id "SInt" at %p.\n",
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
    printf("BoxVM_Obj_Unlink: reference count is "SInt"!\n", references);
#endif
    return;
  }
}

BoxTask BoxVM_Obj_Create(BoxVM *vm, BoxPtr *obj, BoxVMAllocID id) {
  BoxVMObjDesc *od = BoxVMObjDesc_From_Alloc_ID(vm, id);
  if (od == NULL)
    return BOXTASK_FAILURE;

  else {
    BoxVM_Obj_Alloc(vm, obj, od->size, id);
    return (obj->block != NULL) ?
           My_Obj_Init(vm, od, obj, 0, NULL) : BOXTASK_FAILURE;
  }
}

typedef struct {
  BoxPtr src, dest;
  size_t gap_offs;
  size_t container_offs;

} MyCopyState;

static BoxTask My_Obj_Copy(BoxVM *vm, BoxVMObjDesc *desc,
                           BoxPtr *dest, size_t relative_offs, void *pass) {
  BoxVMCallNum copier = desc->copier;
  MyCopyState *state = pass;
  size_t absolute_offs = state->container_offs + relative_offs;

  assert(desc != NULL && pass != NULL);

  if (copier != BOXVMCALLNUM_NONE) {
    size_t gap_offs = state->gap_offs;

    BoxPtr src;
    src.block = state->src.block;
    src.ptr = state->src.ptr + absolute_offs;
    
    /* Copy the gap, if there is one. */
    if (absolute_offs > gap_offs)
      (void) memcpy(state->dest.ptr + gap_offs,
                    state->src.ptr + gap_offs,
                    absolute_offs - gap_offs);

    /* Update the offset for the next gap.
     * FIXME: note that here we may end up copying the dummy space introduced
     *   in order to aligning the subobject in the parent object.
     *   This is something we would like to avoid!
     */
    state->gap_offs = absolute_offs + desc->size;

    return BoxVM_Module_Execute_With_Args(vm->vmcur, copier, dest, & src);

  } else {
    size_t container_offs = state->container_offs;
    state->container_offs = absolute_offs;
    BoxTask t = My_Obj_Iter(vm, desc, dest, My_Obj_Copy, state);
    state->container_offs = container_offs;
    return t;
  }
}

BoxTask BoxVM_Obj_Copy(BoxVM *vm, BoxPtr *dest, BoxPtr *src,
                       BoxVMAllocID id) {
  MyCopyState state;
  BoxVMObjDesc *desc = BoxVMObjDesc_From_Alloc_ID(vm, id);

  state.src = *src;
  state.dest = *dest;
  state.gap_offs = 0;
  state.container_offs = 0;

  if (desc != NULL) {
    BoxTask t = My_Obj_Copy(vm, desc, dest, 0, & state);

    if (t == BOXTASK_OK && state.gap_offs < desc->size) {
      size_t offs = state.gap_offs;
      (void) memcpy(dest->ptr + offs,
                    src->ptr + offs,
                    desc->size - offs);
    }

    return t;

  } else {
    MSG_ERROR("BoxVM_Obj_Copy: unknown object id (%d).", (int) id);
    return BOXTASK_ERROR;
  }
}

BoxTask BoxVM_Obj_Relocate(BoxVM *vm, BoxPtr *dest, BoxPtr *src,
                           BoxVMAllocID id) {
  //assert(!BoxPtr_Is_Null(dest) && !BoxPtr_Is_Null(src));
  /* ^^^ For now we assume non detached pointers */
  if (src->ptr != dest->ptr) {
#if 1
    return BoxVM_Obj_Copy(vm, dest, src, id);

#else
    VMAllocHead *src_head = BoxPtr_Get_Head(src);

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
#endif

  } else {
    /* If the two objects are the same, then clear the src pointer and exit */
    BoxPtr_Nullify(src);
    return BOXTASK_OK;
  }
}
