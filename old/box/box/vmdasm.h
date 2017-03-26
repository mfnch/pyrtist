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
 * @file vmdasm.h
 * @brief Code dealing with reading (disassembling) VM code.
 */

#ifndef _BOX_VMDASM_H
#  define _BOX_VMDASM_H

#  include <stdlib.h>

#  include <box/types.h>
#  include <box/vm.h>

/**
 * Object used to control the disassembling of VM code.
 */
typedef struct BoxVMDasm_struct BoxVMDasm;

/**
 * Iterator called on every instruction by BoxVM_Disassemble_Block.
 * @param dasm the BoxVMDasm structure.
 * @param pass the argument passed to BoxVM_Disassemble_Block.
 * @return a success value to be processed by BoxVM_Disassemble_Block.
 */
typedef BoxTask (*BoxVMDasmIter)(BoxVMDasm *dasm, void *pass);

/**
 * Disassemble a block of memory and call the given iterator for every
 * instruction.
 * @param vm the VM to which the block of code belongs.
 * @param prog the pointer to the block of code.
 * @param dim the size of the block of code in BoxVMWord.
 * @param iter the iterator to call on every disassembled op.
 * @param pass a pointer to pass to the iterator.
 * @return the first value returned by "iter" which is different from
 *   BOXTASK_OK. If "iter" returned all BOXTASK_OK values, or if "iter" was
 *   never called, BOXTASK_OK is returned.
 */
BOXEXPORT BoxTask
  BoxVM_Disassemble_Block(BoxVM *vm, const void *prog, size_t dim,
                          BoxVMDasmIter iter, void *pass);

/* What follows should disappear after refactoring... */

/**
 * Prototype of function which disassembles a VM instruction.
 */
typedef void (*BoxVMOpDisasm)(BoxVMDasm *dasm, char **out);

#endif /* _BOX_VMDASM_H */
