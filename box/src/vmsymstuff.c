/***************************************************************************
 *   Copyright (C) 2006 by Matteo Franchin                                 *
 *   fnch@libero.it                                                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

/* $Id$ */

#include <assert.h>

#include "types.h"
#include "virtmach.h"
#include "vmsym.h"
#include "vmsymstuff.h"

/*** call references ********************************************************/
/* This is the function which assembles the code for the function call */
static Task Assemble_Call(VMProgram *vmp, UInt sym_num, UInt sym_type,
                          int defined, void *def, UInt def_size,
                          void *ref, UInt ref_size)
{
  UInt call_num = 0;
  assert(sym_type == VM_SYM_CALL);
  if (defined && def != NULL) {
    assert(def_size == sizeof(UInt));
    call_num = *((UInt *) def);
  }
  VM_Assemble(vmp, ASM_CALL_I, CAT_IMM, call_num);
  return Success;
}

Task VM_Sym_New_Call(VMProgram *vmp, UInt *sym_num) {
  return VM_Sym_New(vmp, sym_num, VM_SYM_CALL, sizeof(UInt));
}

Task VM_Sym_Def_Call(VMProgram *vmp, UInt sym_num, UInt proc_num) {
  return VM_Sym_Def(vmp, sym_num, & proc_num);
}

Task VM_Sym_Call(VMProgram *vmp, UInt sym_num) {
  return VM_Sym_Code_Ref(vmp, sym_num, Assemble_Call);
}

/*** jumps ******************************************************************/
static Task Assemble_Cond_Jmp(VMProgram *vmp, UInt sym_num, UInt sym_type,
                              int defined, void *def, UInt def_size,
                              void *ref, UInt ref_size)
{
  Int sheet_id, position=0;
  assert(sym_type == VM_SYM_COND_JMP);
  if (defined && def != NULL) {
    assert(def_size == sizeof(VMSymLabel));
    sheet_id = ((VMSymLabel *) def)->sheet_id;
    position = ((VMSymLabel *) def)->position;
  }
  VM_Assemble(vmp, ASM_JC_I, CAT_IMM, position);
  return Success;
}

Task VM_Sym_New_Cond_Jmp(VMProgram *vmp, UInt *sym_num) {
  return VM_Sym_New(vmp, sym_num, VM_SYM_COND_JMP, sizeof(VMSymLabel));
}

Task VM_Sym_Def_Cond_Jmp(VMProgram *vmp, UInt sym_num,
                         Int sheet_id, Int position) {
  VMSymLabel label;
  label.sheet_id = sheet_id;
  label.position = position;
  return VM_Sym_Def(vmp, sym_num, & label);
}

Task VM_Sym_Cond_Jmp(VMProgram *vmp, UInt sym_num) {
  return VM_Sym_Code_Ref(vmp, sym_num, Assemble_Cond_Jmp);
}

/*** procedure headers ******************************************************/

typedef struct {
  Int num_var[NUM_TYPES];
  Int num_reg[NUM_TYPES];
} ProcHead;


/* This is the function which assembles the first code in procedures
 * (the one which allocates registers and variables).
 * Since at the beginning of a procedure the number of allocated registers
 * and variables is unknown, we use the symbol manager to adjust
 * automatically the header of procedures when the data is known.
 */
static Task Assemble_Proc_Head(VMProgram *vmp, UInt sym_num, UInt sym_type,
 int defined, void *def, UInt def_size, void *ref, UInt ref_size) {
  ProcHead *ph = (ProcHead *) def;
  static Int asm_code[NUM_TYPES] = {ASM_NEWC_II, ASM_NEWI_II, ASM_NEWR_II,
   ASM_NEWP_II, ASM_NEWO_II};
  int i;

  assert(sym_type == VM_SYM_PROC_HEAD);
  assert(def_size == sizeof(ProcHead));
  assert(def != NULL);

  for(i = 0; i < NUM_TYPES; i++) {
    Int nv = ph->num_var[i], nr = ph->num_reg[i];
    assert(nv >= 0 && nr >= 0);
    VM_Assemble(vmp, asm_code[i], CAT_IMM, nv, CAT_IMM, nr);
  }
  return Success;
}

Task VM_Sym_Proc_Head(VMProgram *vmp, UInt *sym_num) {
  TASK( VM_Sym_New(vmp, sym_num, VM_SYM_PROC_HEAD, sizeof(ProcHead)) );
  return VM_Sym_Code_Ref(vmp, *sym_num, Assemble_Proc_Head);
}

Task VM_Sym_Def_Proc_Head(VMProgram *vmp, UInt sym_num,
 Int *num_var, Int *num_reg) {
  ProcHead ph;
  int i;
  for(i = 0; i < NUM_TYPES; i++) {
    ph.num_var[i] = num_var[i];
    ph.num_reg[i] = num_reg[i];
  }
  return VM_Sym_Def(vmp, sym_num, & ph);
}
