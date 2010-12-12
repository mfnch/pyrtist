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

#  include <box/types.h>
#  include <box/vmproc.h>
#  include <box/vmptr.h>

/** This is a table of methods for a particular object type.
 * Typically, we need to allocate one of such tables for each different object
 * type. Obviously, many objects can have the same type and hence share the
 * same table.
 */
typedef struct {
  BoxVMCallNum finalizer; /**< Method to be called to finalize the object */
} BoxVMMethodTable;

/** Every installed BoxVMMethodTable has a unique identifier, which is just an
 * integer number of this type. This is the number used in the malloc VM
 * instruction.
 */
typedef BoxInt BoxVMAllocID;

/** Set an extended pointer to Null. */
void BoxObj_Set_To_Null(BoxObj *o);

/** Return 0 if the point is valid, 1 otherwise. */
int BoxObj_Is_Null(BoxObj *o);

/** Increase the extended pointer item by the given integer (in bytes).
 * (the block pointer is not changed).
 */
void BoxObj_Add_To_Ptr(BoxObj *item, size_t addr);

/** Allocate size bytes and returns the corresponding object in 'obj'.
 * The memory region is associated with the provided data 'type'
 * and has a initial reference counter equal to 1.
 */
void BoxVM_Alloc(BoxVM *vm, BoxPtr *obj, size_t size, BoxVMAllocID id);

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

/** Install the method table and returns its ID (an integer number).
 * If an equivalent table has been installed already, then return that
 * rather than installing a new one. This way, object of the same type
 * should end up sharing the same method table.
 */
BoxVMAllocID BoxVMAllocID_From_Method_Table(BoxVM *vm, BoxVMMethodTable *mt);

/** Return the method table for the given allocation ID. If there is no table
 * associated to the ID, then return the NULL pointer.
 */
BoxVMMethodTable *BoxVMMethodTable_From_Alloc_ID(BoxVM *vm, BoxVMAllocID id);

#endif
