/****************************************************************************
 * Copyright (C) 2008 by Matteo Franchin                                    *
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

/* $Id$ */

#include <assert.h>

#include "types.h"
#include "virtmach.h"
#include "vmalloc.h"
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
  VM_Assemble_Long(vmp, ASM_CALL_I, CAT_IMM, call_num);
  return Success;
}

Task VM_Sym_New_Call(VMProgram *vmp, UInt *sym_num) {
  return VM_Sym_New(vmp, sym_num, VM_SYM_CALL, sizeof(UInt));
}

Task VM_Sym_Def_Call(VMProgram *vmp, UInt sym_num, UInt proc_num) {
  return VM_Sym_Def(vmp, sym_num, & proc_num);
}

Task VM_Sym_Call(VMProgram *vmp, UInt sym_num) {
  return VM_Sym_Code_Ref(vmp, sym_num, Assemble_Call, NULL, 0);
}

/*** basic method registration **********************************************/
/* Some methods are special and need to be registered separately using
 * the vm allocator. These methods (constructors, destructors, ... of types)
 * are called automatically by the allocator during construction/destruction
 * of objects, rather than being called with an "ASM_CALL_I" instruction.
 * Here we are not generating VM code, we are just calling
 * VM_Alloc_Method_Set.
 */

typedef struct {
  Type type;
  Type method;
} VMSymMethod;

/* This is the function registers the method, if it is known. */
static Task Register_Call(VMProgram *vmp, UInt sym_num, UInt sym_type,
                          int defined, void *def, UInt def_size,
                          void *ref, UInt ref_size) {
  assert(sym_type == VM_SYM_CALL);
  if (defined && def != NULL) {
    VMSymMethod *m = (VMSymMethod *) ref;
    UInt call_num;
    assert(def_size == sizeof(UInt) && ref_size == sizeof(VMSymMethod));
    call_num = *((UInt *) def);
    return VM_Alloc_Method_Set(vmp, m->type, m->method, call_num);
  }
  return Success;
}

Task VM_Sym_Alloc_Method_Register(VMProgram *vmp, UInt sym_num,
                                  Type type, Type method) {
  VMSymMethod m;
  m.type = type;
  m.method = method;
  return VM_Sym_Ref(vmp, sym_num, Register_Call,
                    & m, sizeof(VMSymMethod), VM_SYM_UNRESOLVED);
}

/*** jumps ******************************************************************/

typedef struct {
  int conditional;
  Int sheet_id;
  Int position;
} VMSymLabelRef;

static Task Assemble_Jmp(VMProgram *vmp, UInt sym_num, UInt sym_type,
                         int defined, void *def, UInt def_size,
                         void *ref, UInt ref_size) {
  Int sheet_id, rel_position = 0;
  AsmCode asm_code = ASM_JC_I;
  VMSymLabelRef *label_ref = ref;

  assert(sym_type == VM_SYM_COND_JMP);
  assert(ref_size == sizeof(VMSymLabelRef));
  assert(ref != NULL);

  if (defined && def != NULL) {
    Int def_position, ref_position;
    assert(def_size == sizeof(VMSymLabel));
    sheet_id = ((VMSymLabel *) def)->sheet_id;
    def_position = ((VMSymLabel *) def)->position;
    ref_position = label_ref->position;
    rel_position = def_position - ref_position;
  }

  asm_code = (label_ref->conditional) ? ASM_JC_I : ASM_JMP_I;
  VM_Assemble_Long(vmp, asm_code, CAT_IMM, rel_position);
  return Success;
}

Task VM_Sym_New_Label(VMProgram *vmp, UInt *sym_num) {
  return VM_Sym_New(vmp, sym_num, VM_SYM_COND_JMP, sizeof(VMSymLabel));
}

Task VM_Sym_Def_Label(VMProgram *vmp, UInt sym_num,
                      Int sheet_id, Int position) {
  VMSymLabel label;
  label.sheet_id = sheet_id;
  label.position = position;
  return VM_Sym_Def(vmp, sym_num, & label);
}

/* Same as VM_Label_Define, but sheet_id is the current active sheet and
 * position is the current position in that sheet.
 */
Task VM_Sym_Def_Label_Here(VMProgram *vmp, UInt label_sym_num) {
  Int sheet_id, position;
  sheet_id = vmp->proc_table.target_proc_num;
  position = Arr_NumItem(vmp->proc_table.target_proc->code);
  return VM_Sym_Def_Label(vmp, label_sym_num, sheet_id, position);
}

/* Same as VM_Label_New, but sheet_id is the current active sheet and
 * position is the current position in that sheet.
 */
Task VM_Sym_New_Label_Here(VMProgram *vmp, UInt *label_sym_num) {
  TASK( VM_Sym_New_Label(vmp, label_sym_num) );
  return VM_Sym_Def_Label_Here(vmp, *label_sym_num);
}

Task VM_Sym_Jmp_Generic(VMProgram *vmp, UInt sym_num, int conditional) {
  VMSymLabelRef label_ref;
  label_ref.conditional = conditional;
  label_ref.sheet_id = vmp->proc_table.target_proc_num;
  label_ref.position = Arr_NumItem(vmp->proc_table.target_proc->code);
  return VM_Sym_Code_Ref(vmp, sym_num, Assemble_Jmp,
                         & label_ref, sizeof(VMSymLabelRef));
}

Task VM_Sym_Jc(VMProgram *vmp, UInt sym_num) {
  return VM_Sym_Jmp_Generic(vmp, sym_num, 1);
}

Task VM_Sym_Jmp(VMProgram *vmp, UInt sym_num) {
  return VM_Sym_Jmp_Generic(vmp, sym_num, 0);
}

Task VM_Sym_Destroy_Label(VMProgram *vmp, UInt sym_num) {
  return Success;
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
                               int defined, void *def, UInt def_size,
                               void *ref, UInt ref_size) {
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
    /* ^^^ should use VM_Assemble_Long for more than 127 regs/vars */
  }
  return Success;
}

Task VM_Sym_Proc_Head(VMProgram *vmp, UInt *sym_num) {
  TASK( VM_Sym_New(vmp, sym_num, VM_SYM_PROC_HEAD, sizeof(ProcHead)) );
  return VM_Sym_Code_Ref(vmp, *sym_num, Assemble_Proc_Head, NULL, 0);
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
