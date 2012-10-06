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
 * @file vmdasm_priv.h
 * @brief Code dealing with reading (disassembling) of VM code.
 */

#ifndef _BOX_VMDASM_PRIVATE_H
#  define _BOX_VMDASM_PRIVATE_H

#  include <stdlib.h>

#  include <box/types.h>
#  include <box/vm.h>
#  include <box/vmdasm.h>

/**
 * Object used to control the disassembling of VM code.
 */
struct BoxVMDasm_struct {
  struct {
    unsigned int exit_now     :1, /**< Exit from the disassembly loop. */
                 report_error :1, /**< Trigger the error condition. */
                 op_is_long   :1; /**< Whether the instruction is long. */

  }              flags;

  BoxVM          *vm;             /**< VM which is being processed. */

  BoxVMWord      *op_ptr;         /**< Pointer to the current word. */
  BoxVMWord      op_word;         /**< The current word. */
  size_t         op_pos;          /**< Position in the buffer. */
  size_t         op_size;         /**< Size of the instruction. */
  const BoxVMInstrDesc
                 *op_desc;        /**< Descriptor for current instruction. */
  BoxUInt        op_arg_type;
};

#endif /* _BOX_VMDASM_PRIVATE_H */
