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

#define DEBUG_VM_D_EVERY_ONE 0

/*****************************************************************************
 * Functions used to disassemble the instructions (see BoxVM_Disassemble)    *
 *****************************************************************************/

#if 0
/* Analoga alla precedente, ma per istruzioni CALL. */
void My_D_CALL(BoxVMDasm *dasm, char **out) {
  BoxUInt na = dasm->op_desc->numargs;

  assert(na == 1);

  if ((dasm->op.args_type & 3) == BOXCONTCATEG_IMM) {
    int iat = dasm->op_desc->t_id;
    BoxInt call_num;

    if (iat == BOXTYPEID_CHAR)
      call_num = (BoxInt) ((BoxChar) call_num);

    {
      BoxVMProcTable *pt = & dasm->vm->proc_table;
      if (call_num < 1 || call_num > BoxArr_Num_Items(& pt->installed)) {
        sprintf(out[0], SInt, call_num);
        return;

      } else {
        char *call_name, *trunc_name;
        BoxVMProcInstalled *p = BoxArr_Item_Ptr(& pt->installed, call_num);

        call_name = (p->desc) ? p->desc : p->name;
        trunc_name = (call_name) ? Str_Cut(call_name, 40, 85) : NULL;
        sprintf(out[0], SInt"('%.40s')",
                call_num, (trunc_name) ? trunc_name : "?");
        BoxMem_Free(trunc_name);
        return;
      }
    }

  } else {
    My_D_GLPI_GLPI(dasm, out);
  }
}

/* Analoga alla precedente, ma per istruzioni di salto (jmp, jc). */
void My_D_JMP(BoxVMDasm *dasm, char **out) {
  BoxUInt na = dasm->op_desc->numargs;

  assert(na == 1);

  if ((dasm->op.args_type & 3) == BOXCONTCATEG_IMM) {
    int iat = dasm->op_desc->t_id;
    BoxInt m_num;
    BoxInt position;

    if (iat == BOXTYPEID_CHAR)
      m_num = (BoxInt) ((BoxChar) m_num);

    position = (dasm->op_pos + m_num)*sizeof(BoxVMWord);
    sprintf(out[0], SInt, position);

  } else
    My_D_GLPI_GLPI(dasm, out);
}

#define MY_COMBINE_CHARS(c1, c2, c3) \
  (((int) (c3)) | (((int) (c2)) << 8) | (((int) (c1)) << 16))

BoxVMOpDisasm BoxVM_Get_ArgDAsm_From_Str(const char *s) {
  switch (MY_COMBINE_CHARS(s[0], s[1], s[2])) {
  case MY_COMBINE_CHARS('c', '-', '\0'): return My_D_CALL;
  case MY_COMBINE_CHARS('x', 'i', '\0'): return My_D_GLPI_Imm;
  case MY_COMBINE_CHARS('x', 'x', '\0'): return My_D_GLPI_GLPI;
  case MY_COMBINE_CHARS('j', '-', '\0'): return My_D_JMP;
  default:
    MSG_FATAL("My_Disassembler_From_Str: unknown string '%s'", s);
    assert(0);
    return NULL;
  }
}

#endif


/**
 * Disassemble a block of memory and call the given iterator for every
 * instruction.
 */
BoxTask BoxVM_Disassemble_Block(BoxVM *vm, const void *prog, size_t dim,
                                BoxVMDasmIter iter, void *pass) {
  BoxVMDasm dasm;
  const BoxOpDesc *exec_table = vm->exec_table;
  size_t op_pos;

  dasm.vm = vm;
  dasm.flags.exit_now = 0;
  dasm.flags.report_error = 0;

  for (op_pos = 0; op_pos < dim;) {
    BoxVMWord *op_addr = & ((BoxVMWord *) prog)[op_pos];
    BoxTask outcome;

    if (!BoxOp_Read(& dasm.op, vm->vmcur, op_addr))
      return BOXTASK_FAILURE;

#if DEBUG_VM_D_EVERY_ONE
    printf("Instruction at position "SUInt" (%p): "
           "{is_long = %d, length = "SUInt", type = "SUInt
           ", arg_type = "SUInt")\n",
           dasm.op_pos, dasm.op_ptr, dasm.flags.op_is_long,
           op.size, op.type, op.arg_type);
#endif

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
