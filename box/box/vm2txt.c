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
  BoxVMWord *bytecode;
  FILE      *output;
} MyDasmData;


static void 
My_Arg_To_Str(char *out, size_t out_size,
              int arg_format, BoxTypeId args_type, BoxInt arg_value) {
  BoxInt arg_abs_value = abs(arg_value);
  char reg_char = "vr"[arg_value >= 0],
       type_char = "cirpo"[args_type];
  switch(arg_format) {
  case BOXCONTCATEG_GREG:
    sprintf(out, "g%c%c" SInt, reg_char, type_char, arg_abs_value);
    break;
  case BOXCONTCATEG_LREG:
    sprintf(out, "%c%c" SInt, reg_char, type_char, arg_abs_value);
    break;
  case BOXCONTCATEG_PTR:
    if (arg_value < 0)
      sprintf(out, "%c[ro0 - " SInt "]", type_char, arg_abs_value);
    else if (arg_value == 0)
      sprintf(out, "%c[ro0]", type_char);
    else
      sprintf(out, "%c[ro0 + " SInt "]", type_char, arg_abs_value);
    break;
  case BOXCONTCATEG_IMM:
    if (args_type == BOXTYPEID_CHAR)
      arg_value = (BoxInt) ((BoxChar) arg_value);
    sprintf(out, SInt, arg_value);
    break;
  default:
    abort();
  }
}

static void My_Data_To_Str(char *out, size_t out_size,
                           BoxTypeId t, void *data) {
  switch (t) {
  case BOXTYPEID_CHAR:
    sprintf(out, SChar, *((BoxChar *) data));
    break;
  case BOXTYPEID_INT:
    sprintf(out, SInt, *((BoxInt *) data));
    break;
  case BOXTYPEID_REAL:
    sprintf(out, SReal, *((BoxReal *) data));
    break;
  case BOXTYPEID_POINT:
    sprintf(out, SPoint,
            ((BoxPoint *) data)->x, ((BoxPoint *) data)->y);
    break;
  default:
    sprintf(out, "???");
    break;
  }
}

static BoxTask My_Op_Dasm(BoxVMDasm *dasm, void *pass) {
  MyDasmData *data = pass;
  FILE *output = data->output;
  BoxOp *op = & dasm->op;
  const char *op_name;
  int num_args;

  if (dasm->op_desc) {
    op_name  = dasm->op_desc->name;      /* op name */
    num_args = dasm->op_desc->num_args;  /* num. of args */

   } else {
    op_name = "???";
    num_args = 0;
  }

  /* Print the instruction and its arguments. */
  fprintf(output, SUInt "\t", (BoxUInt) (dasm->op_pos * sizeof(BoxVMWord)));
  if (dasm->vm->attr.hexcode)
    fprintf(output, BoxVMWord_Fmt"\t", data->bytecode[dasm->op_pos]);
  fprintf(output, "%s", op_name);

  if (num_args > 0) {
    const size_t arg_buffer_size = 64;
    char arg_buffer[arg_buffer_size];
    BoxTypeId t = dasm->op_desc->t_id;
    int i;
    char *sep = " ";

    assert(num_args <= BOX_OP_MAX_NUM_ARGS);
    
    for (i = 0; i < num_args; i++, sep = ", ") {
      My_Arg_To_Str(arg_buffer, arg_buffer_size,
                    (op->args_forms >> (2*i)) & 0x3, t, op->args[i]);
      fprintf(output, "%s%s", sep, arg_buffer);
    }

    if (op->has_data) {
      My_Data_To_Str(arg_buffer, arg_buffer_size, t, op->data);
      fprintf(output, "%s%s", sep, arg_buffer);
    }
  }

  fprintf(output, "\n");

  /* Print the remaining instruction words in hex. */
  if (dasm->vm->attr.hexcode) {
    size_t i;
    for (i = 1; i < dasm->op.next; i++)
      fprintf(output, "\t"BoxVMWord_Fmt"\n",
              data->bytecode[dasm->op_pos + i]);
  }

  return BOXTASK_OK;
}

/* Traduce il codice binario della VM, in formato testo.
 * prog e' il puntatore all'inizio del codice, dim e' la dimensione del codice
 * da tradurre (espresso in "numero di BoxVMWord").
 */
BoxTask BoxVM_Disassemble(BoxVM *vm, FILE *output,
                          const void *prog, size_t dim) {
  MyDasmData data;
  data.output = output;
  data.bytecode = (void *) prog;
  return BoxVM_Disassemble_Block(vm, prog, dim, My_Op_Dasm, & data);
}
