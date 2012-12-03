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
 * @brief Datastructure used to parse one single instruction argument.
 */
typedef struct {
  int       in_kind;  /**< Input argument kind. */
  BoxInt    in_value; /**< Input argument value. */
  union {
    BoxChar val_char; /**< Value for Char registers. */
    BoxInt  val_int;  /**< Value for Int registers. */
    BoxReal val_real; /**< Value for Real registers. */
    BoxPtr  val_ptr;  /**< Value for Ptr registers. */
  }         arg_data; /**< Data associated to the argument. */
} BoxOpArgGetter;

/**
 * @brief Datastructure used to execute one VM instruction.
 */
typedef struct {
  BoxVMX         *vmx;
  BoxOpArgGetter arg[2];
} BoxOpExecutor;

/**
 * Prototype of function which implements a VM instruction.
 */
typedef void (*BoxVMOpExecutor)(BoxVMX *vmx);

#endif /* _BOX_VMEXEC_PRIV_H */
