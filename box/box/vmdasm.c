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
#include <stdio.h>

#include "types.h"
#include "messages.h"
#include "strutils.h"
#include "container.h"
#include "vm_private.h"
#include "vmdasm_private.h"

/*#define DEBUG_VM_D_EVERY_ONE*/

/*****************************************************************************
 * Functions used to disassemble the instructions (see BoxVM_Disassemble)    *
 *****************************************************************************/

/* Questa funzione serve a disassemblare gli argomenti di
 * un'istruzione di tipo GLPI-GLPI.
 * iarg e' una tabella di puntatori alle stringhe che corrisponderanno
 * agli argomenti disassemblati.
 */
static void My_D_GLPI_GLPI(BoxVMDasm *dasm, char **out) {
  UInt n, na = dasm->op_desc->numargs;
  UInt iaform[2] = {dasm->op_arg_type & 3, (dasm->op_arg_type >> 2) & 3};
  Int iaint[2];
  BoxVMWord op_word = dasm->op_word;

  /* Recupero i numeri (interi) di registro/puntatore/etc. */
  switch (na) {
  case 1:
    if (dasm->flags.op_is_long)
      BOXVM_READ_LONGOP_1ARG(dasm->op_ptr, op_word, iaint[0]);
    else
      BOXVM_READ_SHORTOP_1ARG(dasm->op_ptr, op_word, iaint[0]);
    break;

  case 2:
    if (dasm->flags.op_is_long)
      BOXVM_READ_LONGOP_2ARGS(dasm->op_ptr, op_word, iaint[0], iaint[1]);
    else
      BOXVM_READ_SHORTOP_2ARGS(dasm->op_ptr, op_word, iaint[0], iaint[1]);
    break;

  default:
    assert(0);
  }

  for (n = 0; n < na; n++) {
    UInt iaf = iaform[n];
    UInt iat = dasm->op_desc->t_id;

    assert(iaf < 4);

    {
      Int iai = iaint[n], uiai = iai;
      char rc, tc;
      const char typechars[NUM_TYPES] = "cirpo";

      tc = typechars[iat];

      if (uiai < 0) {
        uiai = -uiai;
        rc = 'v';

      } else
        rc = 'r';

      switch(iaf) {
      case BOXCONTCATEG_GREG:
        sprintf(out[n], "g%c%c" SInt, rc, tc, uiai);
        break;
      case BOXCONTCATEG_LREG:
        sprintf(out[n], "%c%c" SInt, rc, tc, uiai);
        break;
      case BOXCONTCATEG_PTR:
        if (iai < 0)
          sprintf(out[n], "%c[ro0 - " SInt "]", tc, uiai);
        else if (iai == 0)
          sprintf(out[n], "%c[ro0]", tc);
        else
          sprintf(out[n], "%c[ro0 + " SInt "]", tc, uiai);
        break;
      case BOXCONTCATEG_IMM:
        if (iat == TYPE_CHAR) iai = (Int) ((Char) iai);
        sprintf(out[n], SInt, iai);
        break;
      }
    }
  }
}

/* Analoga alla precedente, ma per istruzioni CALL. */
void My_D_CALL(BoxVMDasm *dasm, char **out) {
  UInt na = dasm->op_desc->numargs;
  BoxVMWord op_word = dasm->op_word;

  assert(na == 1);

  if ((dasm->op_arg_type & 3) == BOXCONTCATEG_IMM) {
    UInt iat = dasm->op_desc->t_id;
    Int call_num;

    if (dasm->flags.op_is_long)
      BOXVM_READ_LONGOP_1ARG(dasm->op_ptr, op_word, call_num);
    else
      BOXVM_READ_SHORTOP_1ARG(dasm->op_ptr, op_word, call_num);

    if (iat == TYPE_CHAR)
      call_num = (Int) ((Char) call_num);

    {
      BoxVMProcTable *pt = & dasm->vm->proc_table;
      if (call_num < 1 || call_num > BoxArr_Num_Items(& pt->installed)) {
        sprintf(out[0], SInt, call_num);
        return;

      } else {
        char *call_name;
        BoxVMProcInstalled *p;
        p = (BoxVMProcInstalled *) BoxArr_Item_Ptr(& pt->installed, call_num);
        call_name = Str_Cut(p->desc, 40, 85);
        sprintf(out[0], SInt"('%.40s')", call_num, call_name);
        BoxMem_Free(call_name);
        return;
      }
    }

  } else {
    My_D_GLPI_GLPI(dasm, out);
  }
}

/* Analoga alla precedente, ma per istruzioni di salto (jmp, jc). */
void My_D_JMP(BoxVMDasm *dasm, char **out) {
  UInt na = dasm->op_desc->numargs;

  assert(na == 1);

  if ((dasm->op_arg_type & 3) == BOXCONTCATEG_IMM) {
    UInt iat = dasm->op_desc->t_id;
    Int m_num;
    Int position;
    BoxVMByte op_word = dasm->op_word;

    if (dasm->flags.op_is_long)
      BOXVM_READ_LONGOP_1ARG(dasm->op_ptr, op_word, m_num);
    else
      BOXVM_READ_SHORTOP_1ARG(dasm->op_ptr, op_word, m_num);

    if (iat == TYPE_CHAR)
      m_num = (Int) ((Char) m_num);

    position = (dasm->op_pos + m_num)*sizeof(BoxVMWord);
    sprintf(out[0], SInt, position);

  } else
    My_D_GLPI_GLPI(dasm, out);
}

/* Analoga alla precedente, ma per istruzioni del tipo GLPI-Imm. */
void My_D_GLPI_Imm(BoxVMDasm *dasm, char **out) {
  UInt iaf = dasm->op_arg_type & 3, iat = dasm->op_desc->t_id;
  Int iai;
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
    Int uiai = iai;
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
  case BOXTYPE_CHAR:
    sprintf(out[1], SChar, *((BoxChar *) arg2));
    break;
  case BOXTYPE_INT:
    sprintf(out[1], SInt, *((BoxInt *) arg2));
    break;
  case BOXTYPE_REAL:
    sprintf(out[1], SReal, *((BoxReal *) arg2));
    break;
  case BOXTYPE_POINT:
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

/* Traduce il codice binario della VM, in formato testo.
 * prog e' il puntatore all'inizio del codice, dim e' la dimensione del codice
 * da tradurre (espresso in "numero di BoxVMWord").
 */
BoxTask BoxVM_Disassemble(BoxVM *vm, FILE *output,
                          const void *prog, size_t dim)
{
  const BoxVMInstrDesc *exec_table = vm->exec_table;
  UInt nargs;
  const char *iname;
  char iarg_buffers[VM_MAX_NUMARGS][64], /* max 64 characters per argument */
       *iarg[VM_MAX_NUMARGS];
  size_t i;

  BoxVMDasm dasm;

  for (i = 0; i < VM_MAX_NUMARGS; i++)
    iarg[i] = iarg_buffers[i];

  dasm.vm = vm;
  dasm.flags.exit_now = 0;
  dasm.flags.report_error = 0;


  for (dasm.op_pos = 0; dasm.op_pos < dim;) {
    BoxUInt op_type, op_arg_type, op_size;

    dasm.op_ptr = & ((BoxVMWord *) prog)[dasm.op_pos];

    /* Leggo i dati fondamentali dell'istruzione: tipo e lunghezza. */
    BOXVM_READ_OP_HEADER(dasm.op_ptr, dasm.op_word, op_type, op_size,
                         op_arg_type, dasm.flags.op_is_long);

    dasm.op_size = op_size;
    dasm.op_arg_type = op_arg_type;

#ifdef DEBUG_VM_D_EVERY_ONE
    printf("Instruction at position "SUInt" (%p): "
           "{is_long = %d, length = "SUInt", type = "SUInt
           ", arg_type = "SUInt")\n",
           dasm.op_pos, dasm.op_ptr, dasm.flags.op_is_long,
           op_size, op_arg_type, op_arg_type);
#endif

    if (op_type >= 0 || op_type < BOX_NUM_OPS) {
      /* Find the instruction descriptor */
      dasm.op_desc = & exec_table[op_type];
      iname = dasm.op_desc->name;

      /* Localizza in memoria gli argomenti */
      nargs = dasm.op_desc->numargs;

      /* Call the disassembly function to read the bytecode and interpret it */
      if (nargs)
        dasm.op_desc->disasm(& dasm, iarg);

      /* Exit if required by the disassembly function. */
      if (dasm.flags.exit_now)
        return BOXTASK_FAILURE;

    } else {
      iname = "???";
      dasm.op_size = 1;
      nargs = 0;
    }

    if (dasm.flags.report_error) {
      fprintf(output, SUInt "\t"BoxVMWord_Fmt"x\tError!",
              (UInt) (dasm.op_pos * sizeof(BoxVMWord)), *dasm.op_ptr);

    } else {
      BoxVMWord *i_pos2 = dasm.op_ptr;

      /* Stampo l'istruzione e i suoi argomenti */
      fprintf(output, SUInt "\t", (BoxUInt) (dasm.op_pos * sizeof(BoxVMWord)));
      if (vm->attr.hexcode)
        fprintf(output, BoxVMWord_Fmt"\t", *(i_pos2++));
      fprintf(output, "%s", iname);

      if (nargs > 0) {
        UInt n;

        assert(nargs <= VM_MAX_NUMARGS);

        fprintf(output, " %s", iarg[0]);
        for (n = 1; n < nargs; n++)
          fprintf(output, ", %s", iarg[n]);
      }
      fprintf(output, "\n");

      /* Stampo i restanti codici dell'istruzione in esadecimale */
      if (vm->attr.hexcode) {
        size_t i;
        for (i = 1; i < dasm.op_size; i++)
          fprintf(output, "\t"BoxVMWord_Fmt"\n", *(i_pos2++));
      }
    }

    /* Move to the next instruction */
    if (dasm.op_size < 1)
      return BOXTASK_FAILURE;

    dasm.op_pos += dasm.op_size;
  }

  return BOXTASK_OK;
}
