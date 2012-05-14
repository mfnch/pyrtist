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
#include "vmdasm.h"

/*****************************************************************************
 * Functions used to disassemble the instructions (see BoxVM_Disassemble)    *
 *****************************************************************************/

/* Questa funzione serve a disassemblare gli argomenti di
 * un'istruzione di tipo GLPI-GLPI.
 * iarg e' una tabella di puntatori alle stringhe che corrisponderanno
 * agli argomenti disassemblati.
 */
static void My_D_GLPI_GLPI(BoxVM *vmp, char **out) {
  BoxVMX *vmcur = vmp->vmcur;
  UInt n, na = vmcur->idesc->numargs;
  UInt iaform[2] = {vmcur->arg_type & 3, (vmcur->arg_type >> 2) & 3};
  Int iaint[2];

  assert(na <= 2);

  /* Recupero i numeri (interi) di registro/puntatore/etc. */
  switch (na) {
  case 1:
    if (vmcur->flags.is_long)
      BOXVM_READ_LONGOP_1ARG(vmcur->i_pos, vmcur->i_eye, iaint[0]);
    else
      BOXVM_READ_SHORTOP_1ARG(vmcur->i_pos, vmcur->i_eye, iaint[0]);
    break;

  case 2:
    if (vmcur->flags.is_long)
      BOXVM_READ_LONGOP_2ARGS(vmcur->i_pos, vmcur->i_eye, iaint[0], iaint[1]);
    else
      BOXVM_READ_SHORTOP_2ARGS(vmcur->i_pos, vmcur->i_eye, iaint[0], iaint[1]);
    break;
  }

  for (n = 0; n < na; n++) {
    UInt iaf = iaform[n];
    UInt iat = vmcur->idesc->t_id;

    assert(iaf < 4);

    {
      Int iai = iaint[n], uiai = iai;
      char rc, tc;
      const char typechars[NUM_TYPES] = "cirpo";

      tc = typechars[iat];
      if (uiai < 0) {uiai = -uiai; rc = 'v';} else rc = 'r';
      switch(iaf) {
        case BOXCONTCATEG_GREG:
          sprintf(out[n], "g%c%c" SInt, rc, tc, uiai);
          break;
        case BOXCONTCATEG_LREG:
          sprintf(out[n], "%c%c" SInt, rc, tc, uiai);
          break;
        case BOXCONTCATEG_PTR:
          if ( iai < 0 )
            sprintf(out[n], "%c[ro0 - " SInt "]", tc, uiai);
          else if ( iai == 0 )
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
void My_D_CALL(BoxVM *vmp, char **out) {
  BoxVMX *vmcur = vmp->vmcur;
  UInt na = vmcur->idesc->numargs;

  assert(na == 1);

  if ((vmcur->arg_type & 3) == BOXCONTCATEG_IMM) {
    UInt iat = vmcur->idesc->t_id;
    Int call_num;

    if ( vmcur->flags.is_long )
      BOXVM_READ_LONGOP_1ARG(vmcur->i_pos, vmcur->i_eye, call_num);
    else
      BOXVM_READ_SHORTOP_1ARG(vmcur->i_pos, vmcur->i_eye, call_num);

    if (iat == TYPE_CHAR) call_num = (Int) ((Char) call_num);
    {
      BoxVMProcTable *pt = & vmp->proc_table;
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
    My_D_GLPI_GLPI(vmp, out);
  }
}

/* Analoga alla precedente, ma per istruzioni di salto (jmp, jc). */
void My_D_JMP(BoxVM *vmp, char **out) {
  BoxVMX *vmcur = vmp->vmcur;
  UInt na = vmcur->idesc->numargs;

  assert(na == 1);

  if ((vmcur->arg_type & 3) == BOXCONTCATEG_IMM) {

    UInt iat = vmcur->idesc->t_id;
    Int m_num;
    Int position;

    if (vmcur->flags.is_long)
      BOXVM_READ_LONGOP_1ARG(vmcur->i_pos, vmcur->i_eye, m_num);
    else
      BOXVM_READ_SHORTOP_1ARG(vmcur->i_pos, vmcur->i_eye, m_num);

    if (iat == TYPE_CHAR) m_num = (Int) ((Char) m_num);

    position = (vmp->dasm_pos + m_num)*sizeof(BoxVMWord);
    sprintf(out[0], SInt, position);

  } else {
    My_D_GLPI_GLPI(vmp, out);
  }
}

/* Analoga alla precedente, ma per istruzioni del tipo GLPI-Imm. */
void My_D_GLPI_Imm(BoxVM *vmp, char **out) {
  BoxVMX *vmcur = vmp->vmcur;
  UInt iaf = vmcur->arg_type & 3, iat = vmcur->idesc->t_id;
  Int iai;
  BoxVMWord *arg2;

  assert(vmcur->idesc->numargs == 2);
  assert(iat < 4);

  /* Recupero il numero (intero) di registro/puntatore/etc. */
  if (vmcur->flags.is_long)
    BOXVM_READ_LONGOP_1ARG(vmcur->i_pos, vmcur->i_eye, iai);
  else
    BOXVM_READ_SHORTOP_1ARG(vmcur->i_pos, vmcur->i_eye, iai);

  arg2 = vmcur->i_pos;

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
    case TYPE_CHAR:
      sprintf(out[1], SChar, *((Char *) arg2));
      break;
    case TYPE_INT:
      sprintf(out[1], SInt, *((Int *) arg2));
      break;
    case TYPE_REAL:
      sprintf(out[1], SReal, *((Real *) arg2));
      break;
    case TYPE_POINT:
      sprintf(out[1], SPoint,
               ((Point *) arg2)->x, ((Point *) arg2)->y);
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
BoxTask BoxVM_Disassemble(BoxVM *vmp, FILE *output,
                          const void *prog, size_t dim)
{
  const BoxVMInstrDesc *exec_table = vmp->exec_table;
  BoxVMWord *i_pos = (BoxVMWord *) prog;
  BoxVMX vm;
  UInt pos, nargs;
  const char *iname;
  char iarg_buffers[VM_MAX_NUMARGS][64], /* max 64 characters per argument */
       *iarg[VM_MAX_NUMARGS];
  size_t i;

  for (i = 0; i < VM_MAX_NUMARGS; i++)
    iarg[i] = iarg_buffers[i];

  vmp->vmcur = & vm;
  vm.flags.exit = vm.flags.error = 0;
  for (pos = 0; pos < dim;) {
    BoxVMWord i_eye;
    int is_long;

    vm.i_pos = i_pos;
    vmp->dasm_pos = pos;

    /* Leggo i dati fondamentali dell'istruzione: tipo e lunghezza. */
    BOXVM_READ_OP_FORMAT(vm.i_pos, i_eye, is_long);
    vm.flags.is_long = is_long;
    if (is_long)
      BOXVM_READ_LONGOP_HEADER(vm.i_pos, i_eye, vm.i_type, vm.i_len,
                               vm.arg_type);
    else
      BOXVM_READ_SHORTOP_HEADER(vm.i_pos, i_eye, vm.i_type, vm.i_len,
                                vm.arg_type);
    vm.i_eye = i_eye;

#ifdef DEBUG_VM_D_EVERY_ONE
    printf("Instruction at position "SUInt": "
           "{is_long = %d, length = "SUInt", type = "SUInt
           ", arg_type = "SUInt")\n",
           pos, vm.flags.is_long, vm.i_len, vm.i_type, vm.arg_type);
#endif

    if (vm.i_type < 0 || vm.i_type >= BOX_NUM_OPS) {
      iname = "???";
      vm.i_len = 1;
      nargs = 0;

    } else {
      /* Trovo il descrittore di istruzione */
      vm.idesc = & exec_table[vm.i_type];
      iname = vm.idesc->name;

      /* Localizza in memoria gli argomenti */
      nargs = vm.idesc->numargs;

      vm.idesc->disasm(vmp, iarg);
      if (vm.flags.exit)
        return BOXTASK_FAILURE;
    }

    if (vm.flags.error) {
      fprintf(output, SUInt "\t"BoxVMWord_Fmt"x\tError!",
              (UInt) (pos * sizeof(BoxVMWord)), *i_pos);

    } else {
      int i;
      BoxVMWord *i_pos2 = i_pos;

      /* Stampo l'istruzione e i suoi argomenti */
      fprintf(output, SUInt "\t", (UInt) (pos * sizeof(BoxVMWord)));
      if (vmp->attr.hexcode)
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
      if (vmp->attr.hexcode) {
        for (i = 1; i < vm.i_len; i++)
          fprintf(output, "\t"BoxVMWord_Fmt"\n", *(i_pos2++));
      }
    }

    /* Passo alla prossima istruzione */
    if (vm.i_len < 1)
      return BOXTASK_FAILURE;

    vm.i_pos = (i_pos += vm.i_len);
    pos += vm.i_len;
  }
  return BOXTASK_OK;
}
