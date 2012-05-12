/****************************************************************************
 * Copyright (C) 2010-2012 by Matteo Franchin                               *
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
 * @file vm.h
 * @brief Box virtual machine public API.
 */

#ifndef _BOX_VM_H
#  define _BOX_VM_H

typedef struct _BoxVM_struct BoxVM;

typedef struct _BoxVMStatus_struct VMStatus;

/* Data type used to write/read binary codes for the instructions */
typedef unsigned char BoxVMByte;
typedef char BoxVMSByte;
typedef unsigned long BoxVMWord;
#  define BoxVMWord_Fmt "%8.8lx"

/** Allocate space for a BoxVM object and initialise it with BoxVM_Init.
 * You'll need to call BoxVM_Destroy to destroy the object.
 * @see BoxVM_Destroy, BoxVM_Init
 */
BOXEXPORT BoxVM *BoxVM_Create(void);

/** Destroy a BoxVM object created with BoxVM_Create
 * @see BoxVM_Create
 */
BOXEXPORT void BoxVM_Destroy(BoxVM *vm);

#endif /* _BOX_VM_H */
