/****************************************************************************
 * Copyright (C) 2008-2012 by Matteo Franchin                               *
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
 * @file vmx.h
 * @brief The Virtual Machine eXecutor.
 */

#ifndef _BOX_VMX_H
#  define _BOX_VMX_H

#  include <stdio.h>

#  include <box/types.h>

/**
 * Initialize a new VM executor in the given portion of memory.
 */
BOXEXPORT void BoxVMX_Init(BoxVMX *vmx);

/**
 * Finalize the given VM executor.
 */
BOXEXPORT void BoxVMX_Finish(BoxVMX *vmx);

/**
 * Get the last failure message.
 * @param vmx the VM executor.
 * @param steal if set to BOXBOOL_TRUE, steal the string (and allocation
 *   responsibility) and remove the failure message from the executor state.
 * @return A string which must be deallocated by the user iff
 *   steal == BOXBOOL_TRUE.
 */
BOXEXPORT char *BoxVMX_Get_Fail_Msg(BoxVMX *vmx, BoxBool steal);

/**
 * Set a failure message.
 * @param vmx the VM executor.
 * @param msg a string representing an error condition. If this is NULL, then
 *   the error condition for the VM executor is reset.
 */
BOXEXPORT void BoxVMX_Set_Fail_Msg(BoxVMX *vmx, const char *msg);

#endif /* _BOX_VMX_H */
