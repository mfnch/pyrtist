/****************************************************************************
 * Copyright (C) 2012 by Matteo Franchin                                    *
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

/**
 * @file vmserial.h
 * @brief Serialization/deserialization of Box VM.
 */

#ifndef _BOX_VMSERIAL_H
#  define _BOX_VMSERIAL_H

#  include <box/types.h>
#  include <box/vm.h>

/**
 * Pre-compute the size of the serialized VM.
 * @param vm the VM for which the computation should be made.
 * @param the size that the serialized VM would have.
 */
BOXEXPORT size_t BoxVM_Get_Serialization_Size(BoxVM *vm);

/**
 * Serialize the given VM.
 * @param vm the VM which should be serialized.
 * @param dest the address where to put the serialized VM.
 * @param dest_size the maximum number of bytes allowed for the serialization.
 * @return BOXTASK_OK for success, BOXTASK_FAILURE if dest_size was not big
 *   enough.
 */
BOXEXPORT BoxTask BoxVM_Serialize(BoxVM *vm, void *dest, size_t dest_size);

/**
 * Deserialize a VM from a block of memory.
 * @param vm initialized (but empty) VM which will be the destination of the
 *   deserialization.
 * @param src the address from which the serialized VM should be read.
 * @param src_size the number of bytes to read from src.
 * @return a new VM, or NULL if the deserialization failed.
 */
BOXEXPORT BoxVM *BoxVM_Deserialize(const void *src, size_t src_size);

#endif /* _BOX_VMSERIAL_H */
