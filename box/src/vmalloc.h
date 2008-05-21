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

#ifndef _VMALLOC_H
#  define _VMALLOC_H

#  include "types.h"
#  include "virtmach.h"

/** Kind of method: VM_ALC_DESTRUCTOR is called by VM_Unlink before
 * deallocating an object. VM_ALC_CONSTRUCTOR is still unused.
 */
typedef enum {
  VM_ALC_CONSTRUCTOR,
  VM_ALC_DESTRUCTOR
} VMAlcMethod;

/** Allocate size bytes and returns the pointer to that region.
 * The memory region is associated with the provided data 'type'
 * and has a initial reference counter equal to 1.
 */
void *VM_Alloc(size_t size, Int type);

/** Increase the reference counter for the given object. */
void VM_Link(void *uptr);

/** Decrease the reference counter for the given object and proceed
 * with destroying it, if it has reached zero.
 */
void VM_Unlink(VMProgram *vmp, void *uptr);

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
Task VM_Alloc_Method_Set(VMProgram *vmp, Int type, VMAlcMethod m, Int m_num);

/** Find the method of kind 'm' associated with type 'type' in the scope
 * of the virtual machine 'vmp'
 */
Int VM_Alloc_Method_Get(VMProgram *vmp, Int type, VMAlcMethod m);

#endif
