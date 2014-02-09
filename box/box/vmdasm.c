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

#include <assert.h>
#include <stdlib.h>

#include "types.h"
#include "messages.h"
#include "strutils.h"
#include "container.h"
#include "vm_priv.h"
#include "vmdasm_priv.h"
#include "vmproc_priv.h"
#include "vmop_priv.h"

/**
 * Disassemble a block of memory and call the given iterator for every
 * instruction.
 */
BoxTask BoxVM_Disassemble_Block(BoxVM *vm, const void *prog, size_t dim,
                                BoxVMDasmIter iter, void *pass)
{
  BoxVMDasm dasm;
  const BoxOpDesc *exec_table = vm->exec_table;
  size_t op_pos;

  dasm.vm = vm;
  dasm.flags.exit_now = 0;
  dasm.flags.report_error = 0;

  for (op_pos = 0; op_pos < dim;) {
    BoxVMWord *op_addr = & ((BoxVMWord *) prog)[op_pos];
    BoxTask outcome;

    if (!BoxVMOp_Read(& dasm.op, exec_table, op_addr))
      return BOXTASK_FAILURE;

    dasm.op_pos = op_pos;
    dasm.op_desc = ((dasm.op.id >= 0 || dasm.op.id < BOX_NUM_OPS) ?
                    & exec_table[dasm.op.id] : NULL);

    outcome = iter(& dasm, pass);
    if (outcome != BOXTASK_OK)
      return outcome;

    /* Move to the next instruction */
    if (dasm.op.next < 1)
      return BOXTASK_FAILURE;

    op_pos += dasm.op.next;
  }

  return BOXTASK_OK;
}
