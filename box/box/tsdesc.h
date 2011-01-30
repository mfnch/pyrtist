/***************************************************************************
 *   Copyright (C) 2006-2011 by Matteo Franchin (fnch@libero.it)           *
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

/** @file tsdesc.h
 * @brief Generator of object descriptors (required for memory management).
 *
 * The functions defined in this sub-unit allow to generator type descriptors
 * (BoxVMObjDesc) to be registered with the VM for memory management.
 * An object descriptor contains information about what subobjects are
 * contained inside the given object. It is therefore a tree structure which
 * can be used to initialise, finalise, copy and relocate objects (which
 * is done dynamically by the Box VM).
 */

#ifndef _BOX_TSDESC_H
#  define _BOX_TSDESC_H

#  include <box/types.h>
#  include <box/vmalloc.h>
#  include <box/compiler.h>

/** Build the object descriptor for the given type 't'.
 * 'ts' and 'vm' are the type system and VM object.
 * @return A newly allocated BoxVMObjDesc object (it is responsability of the
 *   caller to deallocate it (with BoxMem_Free).
 *   Return NULL if the object does have a simple structure and does not need
 *   an object descriptor.
 */
BoxVMObjDesc *TS_Get_ObjDesc(BoxCmp *c, BoxType t);

/** Return the alloc ID of the given object 't'.
 * This function internally creates the object descriptor and finds whether
 * it has been already created (to avoid duplication of IDs).
 */
BoxVMAllocID TS_Get_AllocID(BoxCmp *c, BoxType t);

#endif /* _BOX_TSDESC_H */
