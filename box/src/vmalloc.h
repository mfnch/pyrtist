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

#  include "types.h"
#  include "virtmach.h"

/** Kind of method: VM_ALC_DESTRUCTOR is called by VM_Unlink before
 * deallocating an object. VM_ALC_CONSTRUCTOR is still unused.
 */
typedef enum {
  VM_OBJ_METHOD_ERR=-1,
  VM_OBJ_METHOD_OPEN=0,
  VM_OBJ_METHOD_CLOSE,
  VM_OBJ_METHOD_PAUSE,
  VM_OBJ_METHOD_DESTROY
} VMObjMethod;

/** Allocate size bytes and returns the corresponding object in 'obj'.
 * The memory region is associated with the provided data 'type'
 * and has a initial reference counter equal to 1.
 */
void VM_Alloc(Obj *obj, size_t size, Int type);

/** Increase the reference counter for the given object. */
void VM_Link(Obj *obj);

/** Decrease the reference counter for the given object and proceed
 * with destroying it, if it has reached zero.
 */
void VM_Unlink(VMProgram *vmp, Obj *obj);

/** Initialise the memory handling system of the virtual machine 'vmp'
 *  (to be called internally by VM_Init).
 */
Task VM_Alloc_Init(VMProgram *vmp);

/** Finalises the memory handling system of the virtual machine 'vmp'.
 */
void VM_Alloc_Destroy(VMProgram *vmp);

/** Set 'm_num' to be the method of kind 'm' associated with type 'type'
 * in the scope of the virtual machine 'vmp'
 */
Task VM_Alloc_Method_Set(VMProgram *vmp, Int type, VMObjMethod m, Int m_num);

/** Find the method of kind 'm' associated with type 'type' in the scope
 * of the virtual machine 'vmp'. Return -1 if the method was not found.
 */
Int VM_Alloc_Method_Get(VMProgram *vmp, Int type, VMObjMethod m);

#endif
