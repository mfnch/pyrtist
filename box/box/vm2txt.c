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

#include <stdio.h>
#include <assert.h>

#include "types.h"
#include "vm_priv.h"
#include "vmdasm_priv.h"


/**
 * Structure used to pass arguments to My_Op_Dasm.
 */
typedef struct {
  FILE *output;
  char *arg[VM_MAX_NUMARGS];

} MyDasmData;

static BoxTask My_Op_Dasm(BoxVMDasm *dasm, void *pass) {
  MyDasmData *data = pass;
  FILE *output = data->output;
  char **arg = data->arg;
  const char *op_name;
  int nargs;

  if (dasm->op_desc)
  {
    op_name = dasm->op_desc->name;   /* op name */
    nargs = dasm->op_desc->numargs;  /* num. of args */

    /* Call the disassembly function to read the bytecode and interpret it */
    if (nargs > 0)
      dasm->op_desc->disasm(dasm, arg);

    /* Exit if required by the disassembly function. */
    if (dasm->flags.exit_now)
      return BOXTASK_FAILURE;

  } else {
    op_name = "???";
    dasm->op_size = 1;
    nargs = 0;
  }

  if (dasm->flags.report_error) {
    fprintf(output, SUInt "\t"BoxVMWord_Fmt"x\tError!",
            (UInt) (dasm->op_pos * sizeof(BoxVMWord)), *dasm->op_ptr);
    
  } else {
    BoxVMWord *i_pos2 = dasm->op_ptr;
    
    /* Stampo l'istruzione e i suoi argomenti */
    fprintf(output, SUInt "\t", (BoxUInt) (dasm->op_pos * sizeof(BoxVMWord)));
    if (dasm->vm->attr.hexcode)
      fprintf(output, BoxVMWord_Fmt"\t", *(i_pos2++));
    fprintf(output, "%s", op_name);
    
    if (nargs > 0) {
      UInt n;

      assert(nargs <= VM_MAX_NUMARGS);

      fprintf(output, " %s", arg[0]);
      for (n = 1; n < nargs; n++)
        fprintf(output, ", %s", arg[n]);
    }
    fprintf(output, "\n");

    /* Stampo i restanti codici dell'istruzione in esadecimale */
    if (dasm->vm->attr.hexcode) {
      size_t i;
      for (i = 1; i < dasm->op_size; i++)
        fprintf(output, "\t"BoxVMWord_Fmt"\n", *(i_pos2++));
    }
  }

  return BOXTASK_OK;
}

/* Traduce il codice binario della VM, in formato testo.
 * prog e' il puntatore all'inizio del codice, dim e' la dimensione del codice
 * da tradurre (espresso in "numero di BoxVMWord").
 */
BoxTask BoxVM_Disassemble(BoxVM *vm, FILE *output,
                          const void *prog, size_t dim)
{
  MyDasmData data;
  char iarg_buffers[VM_MAX_NUMARGS][64]; /* max 64 characters per argument */

  size_t i;

  data.output = output;
  for (i = 0; i < VM_MAX_NUMARGS; i++)
    data.arg[i] = iarg_buffers[i];

  return BoxVM_Disassemble_Block(vm, prog, dim, My_Op_Dasm, & data);
}
