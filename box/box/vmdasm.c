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

#define DEBUG_VM_D_EVERY_ONE 0

/*****************************************************************************
 * Functions used to disassemble the instructions (see BoxVM_Disassemble)    *
 *****************************************************************************/

/* Questa funzione serve a disassemblare gli argomenti di
 * un'istruzione di tipo GLPI-GLPI.
 * iarg e' una tabella di puntatori alle stringhe che corrisponderanno
 * agli argomenti disassemblati.
 */
static void My_D_GLPI_GLPI(BoxVMDasm *dasm, char **out) {
  int n, na = dasm->op_desc->numargs;
  int arg_formats[2] = {dasm->op_arg_type & 3, (dasm->op_arg_type >> 2) & 3};
  BoxInt arg_values[2];
  BoxVMWord op_word = dasm->op_word;

  /* Recupero i numeri (interi) di registro/puntatore/etc. */
  if (na == 1) {
    if (dasm->flags.op_is_long)
      BOXVM_READ_LONGOP_1ARG(dasm->op_ptr, op_word, arg_values[0]);
    else
      BOXVM_READ_SHORTOP_1ARG(dasm->op_ptr, op_word, arg_values[0]);

  } else {
    assert(na == 2);
    if (dasm->flags.op_is_long)
      BOXVM_READ_LONGOP_2ARGS(dasm->op_ptr, op_word,
                              arg_values[0], arg_values[1]);
    else
      BOXVM_READ_SHORTOP_2ARGS(dasm->op_ptr, op_word,
                               arg_values[0], arg_values[1]);
  }

  for (n = 0; n < na; n++) {
    int arg_format = arg_formats[n],
        arg_type = dasm->op_desc->t_id;
    BoxInt arg_value = arg_values[n], arg_abs_value = abs(arg_value);
    char reg_char = "vr"[arg_value >= 0],
         type_char = "cirpo"[arg_type];

    switch(arg_format) {
    case BOXCONTCATEG_GREG:
      sprintf(out[n], "g%c%c" SInt, reg_char, type_char, arg_abs_value);
      break;
    case BOXCONTCATEG_LREG:
      sprintf(out[n], "%c%c" SInt, reg_char, type_char, arg_abs_value);
      break;
    case BOXCONTCATEG_PTR:
      if (arg_value < 0)
        sprintf(out[n], "%c[ro0 - " SInt "]", type_char, arg_abs_value);
      else if (arg_value == 0)
        sprintf(out[n], "%c[ro0]", type_char);
      else
        sprintf(out[n], "%c[ro0 + " SInt "]", type_char, arg_abs_value);
      break;
    case BOXCONTCATEG_IMM:
      if (arg_type == BOXTYPEID_CHAR)
        arg_value = (BoxInt) ((BoxChar) arg_value);
      sprintf(out[n], SInt, arg_value);
      break;
    default:
      abort();
    }
  }
}

/* Analoga alla precedente, ma per istruzioni CALL. */
void My_D_CALL(BoxVMDasm *dasm, char **out) {
  BoxUInt na = dasm->op_desc->numargs;
  BoxVMWord op_word = dasm->op_word;

  assert(na == 1);

  if ((dasm->op_arg_type & 3) == BOXCONTCATEG_IMM) {
    int iat = dasm->op_desc->t_id;
    BoxInt call_num;

    if (dasm->flags.op_is_long)
      BOXVM_READ_LONGOP_1ARG(dasm->op_ptr, op_word, call_num);
    else
      BOXVM_READ_SHORTOP_1ARG(dasm->op_ptr, op_word, call_num);

    if (iat == TYPE_CHAR)
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

  if ((dasm->op_arg_type & 3) == BOXCONTCATEG_IMM) {
    int iat = dasm->op_desc->t_id;
    BoxInt m_num;
    BoxInt position;
    BoxVMByte op_word = dasm->op_word;

    if (dasm->flags.op_is_long)
      BOXVM_READ_LONGOP_1ARG(dasm->op_ptr, op_word, m_num);
    else
      BOXVM_READ_SHORTOP_1ARG(dasm->op_ptr, op_word, m_num);

    if (iat == TYPE_CHAR)
      m_num = (BoxInt) ((BoxChar) m_num);

    position = (dasm->op_pos + m_num)*sizeof(BoxVMWord);
    sprintf(out[0], SInt, position);

  } else
    My_D_GLPI_GLPI(dasm, out);
}

/* Analoga alla precedente, ma per istruzioni del tipo GLPI-Imm. */
void My_D_GLPI_Imm(BoxVMDasm *dasm, char **out) {
  BoxUInt iaf = dasm->op_arg_type & 3, iat = dasm->op_desc->t_id;
  BoxInt iai;
  BoxVMWord *arg2;
  BoxVMWord op_word = dasm->op_word;

  assert(dasm->op_desc->numargs == 2);
  assert(iat < 4);

  /* Recupero il numero (intero) di registro/puntatore/etc. */
  if (dasm->flags.op_is_long)
    BOXVM_READ_LONGOP_1ARG(dasm->op_ptr, op_word, iai);
  else
    BOXVM_READ_SHORTOP_1ARG(dasm->op_ptr, op_word, iai);

  arg2 = dasm->op_ptr;

  /* Primo argomento */
  {
    BoxInt uiai = iai;
    char rc, tc;
    const char typechars[NUM_TYPES] = "cirpo";

    tc = typechars[iat];
    if (uiai < 0) {uiai = -uiai; rc = 'v';} else rc = 'r';
    switch(iaf) {
    case BOXCONTCATEG_GREG:
      sprintf(out[0], "g%c%c" SInt, rc, tc, uiai);
      break;
    case BOXCONTCATEG_LREG:
      sprintf(out[0], "%c%c" SInt, rc, tc, uiai);
      break;
    case BOXCONTCATEG_PTR:
      if (iai < 0)
        sprintf(out[0], "%c[ro0 - " SInt "]", tc, uiai);
      else if (iai == 0)
        sprintf(out[0], "%c[ro0]", tc);
      else
        sprintf(out[0], "%c[ro0 + " SInt "]", tc, uiai);
      break;
    case BOXCONTCATEG_IMM:
      sprintf(out[0], SInt, iai);
      break;
    }
  }

  /* Secondo argomento */
  switch (iat) {
  case BOXTYPEID_CHAR:
    sprintf(out[1], SChar, *((BoxChar *) arg2));
    break;
  case BOXTYPEID_INT:
    sprintf(out[1], SInt, *((BoxInt *) arg2));
    break;
  case BOXTYPEID_REAL:
    sprintf(out[1], SReal, *((BoxReal *) arg2));
    break;
  case BOXTYPEID_POINT:
    sprintf(out[1], SPoint,
            ((BoxPoint *) arg2)->x, ((BoxPoint *) arg2)->y);
    break;
  }
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

/**
 * Disassemble a block of memory and call the given iterator for every
 * instruction.
 */
BoxTask BoxVM_Disassemble_Block(BoxVM *vm, const void *prog, size_t dim,
                                BoxVMDasmIter iter, void *pass)
{
  BoxVMDasm dasm;
  const BoxVMInstrDesc *exec_table = vm->exec_table;

  dasm.vm = vm;
  dasm.flags.exit_now = 0;
  dasm.flags.report_error = 0;

  for (dasm.op_pos = 0; dasm.op_pos < dim;)
  {
    BoxUInt op_type, op_arg_type, op_size;
    BoxTask outcome;

    dasm.op_ptr = & ((BoxVMWord *) prog)[dasm.op_pos];

    /* Leggo i dati fondamentali dell'istruzione: tipo e lunghezza. */
    BOXVM_READ_OP_HEADER(dasm.op_ptr, dasm.op_word, op_type, op_size,
                         op_arg_type, dasm.flags.op_is_long);

    dasm.op_size = op_size;
    dasm.op_arg_type = op_arg_type;

#if DEBUG_VM_D_EVERY_ONE
    printf("Instruction at position "SUInt" (%p): "
           "{is_long = %d, length = "SUInt", type = "SUInt
           ", arg_type = "SUInt")\n",
           dasm.op_pos, dasm.op_ptr, dasm.flags.op_is_long,
           op_size, op_type, op_arg_type);
#endif

    dasm.op_desc = ((op_type >= 0 || op_type < BOX_NUM_OPS) ?
                    & exec_table[op_type] : NULL);
 
    outcome = iter(& dasm, pass);
    if (outcome != BOXTASK_OK)
      return outcome;

    /* Move to the next instruction */
    if (dasm.op_size < 1)
      return BOXTASK_FAILURE;

    dasm.op_pos += dasm.op_size;
  }

  return BOXTASK_OK;
}
