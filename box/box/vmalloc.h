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

/** @file vmalloc.h
 * @brief Memory management system for the VM (with reference counting).
 *
 * The memory management system of Box is based on reference counting.
 * When an object x of size s(x) is allocated, the VM acutally allocates
 * a region which is larger than s(x). In particular it allocates extra space
 * to keep a reference counter (an integer number) and a type identifier
 * (again an integer number). The reference counter keeps the number
 * of references to the object x: every object y which refers to x
 * is associated with an increment 1 of the counter of references for x.
 * If y is destroyed the reference counter for x has to be decreased.
 * x is destroyed when the reference counter reaches 0. In this way many
 * objects can refer to the same object: this will be deallocated only when
 * there are no more objects referring to it.
 * The type identifier is a number which is associated to the object
 * and determines its type. The VM knows how to initialise, create, destroy,
 * ... objects and decides what method has to be called by looking at
 * the type identifier.
 * Objects can contain the full sub-objects, not just pointers to other objects.
 * For example, an array of 1000 objects is not just an array of 1000 pointers
 * to objects which are allocated independently.
 * This mechanism requires to extend the concept of pointer: our pointers are
 * actually tuples of two pointers: one pointer to the raw data and one
 * pointer to the allocated object which contains it (or NULL if the
 * object is static and was not allocated in a "traditional" way).
 */

#ifndef _VMALLOC_H
#  define _VMALLOC_H

#  include <stdlib.h>

#  include <box/types.h>
#  include <box/vmproc.h>
#  include <box/vmptr.h>

/** Used for allocations of blocks of memory without "type". */
#  define  BOXVMALLOCID_NONE ((BoxVMAllocID) 0)
#  define BOXVMALLOCID_ARRAY ((BoxVMAllocID) 1)
#  define   BOXVMALLOCID_PTR ((BoxVMAllocID) 2)
#  define BOXVMALLOCID_FIRST ((BoxVMAllocID) 3)

/** Every installed BoxVMObjDesc has a unique identifier, which is just an
 * integer number of this type. This is the number used in the malloc VM
 * instruction.
 */
typedef BoxInt BoxVMAllocID;

/** Datastructure used to describe a sub-object inside an object. */
typedef struct {
  BoxVMAllocID alloc_id; /**< Type of the object */
  size_t       position; /**< Position in the block */
} BoxVMSubObj;

/** Box Object descriptor. This is a data structure which describes an object
 * and contains information about the size of the object, the position of
 * the sub-objects inside the object and the various methods to initialize it,
 * finalize it, etc.
 */
typedef struct {
  struct {
    unsigned int
               initializer : 1, /**< Object requires special init */
               finalizer   : 1, /**< Object requires special finish */
               copier      : 1, /**< Object requires special copier */
               mover       : 1; /**< Object requires special mover */
  } has;                    /**< Tells whether the object or one of the
                                 subobjects require special treatment */
  BoxVMCallNum initializer, /**< Method to initialize the object */
               finalizer,   /**< Method to finalize the object */
               copier,      /**< Method to copy the object */
               mover;       /**< Method to move the object */
  size_t       size,        /**< Size of the object */
               num_subs;    /**< Number of sub-objects */
  BoxVMSubObj  subs[];      /**< Subobjects */
} BoxVMObjDesc;

/** Macro to compute the size of a BoxVMObjDesc object. */
#define BOXVMOBJDESC_SIZE(od) \
  (sizeof(BoxVMObjDesc) + (od)->num_subs*sizeof(BoxVMSubObj))

/** Increase the extended pointer item by the given integer (in bytes).
 * (the block pointer is not changed).
 */
void BoxObj_Add_To_Ptr(BoxObj *item, size_t addr);

#if 0
/** Function used to populate the a method dable (BoxVMMethodTable).
 * NOTE: The usage of this function is recommended, as it makes it easy to
 *   adjust external code to newer version of the boxcore libary (it makes it
 *   more difficult to forget to set one member of the structure).
 *   This way the BoxVMMethodTable can be treated as an opaque structure.
 */
void BoxVMMethodTable_Set(BoxVMMethodTable *mt, size_t size,
                          BoxVMCallNum initializer, BoxVMCallNum finalizer);
#endif

/** Call the desctructor for the main object and all the subobjects
 * without releasing (BoxMem_Free) the block of memory occupied by the object.
 */
BoxTask BoxVM_Obj_Finish(BoxVM *vm, BoxPtr *obj, BoxVMAllocID id);

/** Allocate size bytes and returns the corresponding object in 'obj'.
 * The memory region is associated with the provided data 'type'
 * and has a initial reference counter equal to 1.
 */
void BoxVM_Obj_Alloc(BoxVM *vm, BoxPtr *obj, size_t size, BoxVMAllocID id);

/** Increase the reference counter for the given object. */
void BoxVM_Link(BoxObj *obj);

/** Decrease the reference counter for the given object and proceed
 * with destroying it, if it has reached zero.
 */
void BoxVM_Unlink(BoxVM *vm, BoxObj *obj);

/** Initialise the memory handling system of the virtual machine 'vmp'
 *  (to be called internally by VM_Init).
 */
BoxTask BoxVM_Alloc_Init(BoxVM *vm);

/** Finalises the memory handling system of the virtual machine 'vmp'.
 */
void BoxVM_Alloc_Destroy(BoxVM *vm);

/** Whether the object descriptor is empty (simple objects). */
int BoxVMObjDesc_Is_Empty(BoxVMObjDesc *od);

/** Return a string representation of the object descriptor associated
 * to the given alloc ID.
 */
char *BoxVMObjDesc_To_Str(BoxVM *vm, BoxVMAllocID id);

/** Return a string description of all the objects known to the VM
 * (i.e. for which an object descriptor exists).
 */
char *BoxVM_ObjDesc_Table_To_Str(BoxVM *vm);

/** Install the object descriptor and returns its ID (an integer number).
 * The function takes a pointer to the pointer to the object descriptor.
 * The reason is that the function will steal the descriptor rather than
 * copying it. If this is the case, then *od_ptr is set to NULL.
 * On the other hand, if an equivalent object descriptor has been installed
 * already, then return that rather than installing a new one. This way,
 * objects of the same type should end up sharing the same descriptor.
 * Note that in the latter case *od_ptr is left untouched. An example of
 * usage may further clarify the behaviour of this function:
 *
 *   BoxVMObjDesc *od = GeneratorOfObjDesc(...); // Here memory is allocated!
 *   BoxVMAllocID id = BoxVMAllocID_From_ObjDesc(vm, & od);
 *   if (od == NULL)
 *     // Object descriptor has been installed: do not need to free
 *     // We still may give a name (just for debugging purposes)
 *     BoxVM_Set_Obj_Name(vm, id, BoxMem_Strdup("NameOfType"));
 *   else
 *     BoxMem_Free(od); // Deallocate object
 *
 */
BoxVMAllocID BoxVMAllocID_From_ObjDesc(BoxVM *vm, BoxVMObjDesc **od_ptr);

/** Return the object descriptor for the given allocation ID. If there is no
 * object descriptor associated to the ID, then return the NULL pointer.
 */
BoxVMObjDesc *BoxVMObjDesc_From_Alloc_ID(BoxVM *vm, BoxVMAllocID id);

/** Get the name of the object descriptor.
 * @see BoxVM_Set_Obj_Name
 */
const char *BoxVM_Get_Obj_Name(BoxVM *vm, BoxVMAllocID id);

/** Set a name for the object descriptor.
 * @see BoxVMAllocID_From_ObjDesc
 */
void BoxVM_Set_Obj_Name(BoxVM *vm, BoxVMAllocID id, char *name);

/** Allocate the space for an object with alloc-ID equal to 'id'.
 * The object is then initialized.
 * If 'id' is not valid return BOXTASK_FAILURE, otherwise return BOXTASK_OK.
 */
BoxTask BoxVM_Obj_Create(BoxVM *vm, BoxPtr *obj, BoxVMAllocID id);

/** Relocate an object in memory: move the object from 'src' to 'dest'.
 */
BoxTask BoxVM_Obj_Relocate(BoxVM *vm, BoxPtr *dest, BoxPtr *src);

#endif
