/****************************************************************************
 * Copyright (C) 2010 by Matteo Franchin                                    *
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

#ifndef _BOX_VMPTR_H
#  define _BOX_VMPTR_H

typedef struct _BoxVM_struct BoxVM;

typedef struct _BoxVMStatus_struct VMStatus;

/* Data type used to write/read binary codes for the instructions */
typedef unsigned char BoxVMByte;
typedef char BoxVMSByte;
typedef unsigned long BoxVMByteX4;
#  define BoxVMByteX4_Fmt "%8.8lx"

#  ifdef BOX_ABBREV
typedef BoxVMByteX4 VMByteX4;
#    define VMByteX4_Fmt BoxVMByteX4_Fmt
#  endif

/** Provided for compatibility */
typedef BoxVM VMProgram;

#endif /* _BOX_VMPTR_H */
