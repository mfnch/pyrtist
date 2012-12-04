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
 * @file vmexec_priv.h
 * @brief Box virtual executor implementation.
 */

#ifndef _BOX_VMEXEC_PRIV_H
#  define _BOX_VMEXEC_PRIV_H

#  include <box/core.h>

/**
 * Prototype of function which implements a VM instruction.
 */
typedef void (*BoxVMOpExecutor)(BoxVMX *vmx);

#endif /* _BOX_VMEXEC_PRIV_H */
