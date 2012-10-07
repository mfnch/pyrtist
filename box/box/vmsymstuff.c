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

#include <assert.h>

#include "types.h"
#include "mem.h"
#include "vm_priv.h"
#include "vmalloc.h"
#include "vmsym.h"
#include "vmsymstuff.h"
#include "container.h"
#include "callable.h"


/*** jumps ******************************************************************/

typedef struct {
  int conditional;
  BoxInt sheet_id;
  BoxInt position;
} VMSymLabelRef;

static BoxTask Assemble_Jmp(BoxVM *vmp, BoxVMSymID sym_id, BoxUInt sym_type,
                            int defined, void *def, size_t def_size,
                            void *ref, size_t ref_size) {
  BoxInt rel_position = 0;
  BoxOp asm_code = BOXOP_JC_I;
  VMSymLabelRef *label_ref = ref;

  assert(sym_type == BOXVMSYMTYPE_COND_JMP);
  assert(ref_size == sizeof(VMSymLabelRef));
  assert(ref);

  if (defined && def != NULL) {
    BoxInt def_position, ref_position;
    assert(def_size == sizeof(VMSymLabel));
    def_position = ((VMSymLabel *) def)->position;
    ref_position = label_ref->position;
    rel_position = def_position - ref_position;
  }

  asm_code = (label_ref->conditional) ? BOXOP_JC_I : BOXOP_JMP_I;
  BoxVM_Assemble_Long(vmp, asm_code, BOXCONTCATEG_IMM, rel_position);
  return BOXTASK_OK;
}

BoxVMSymID BoxVMSym_New_Label(BoxVM *vmp) {
  return BoxVMSym_New(vmp, BOXVMSYMTYPE_COND_JMP, sizeof(VMSymLabel));
}

BoxTask BoxVMSym_Def_Label(BoxVM *vmp, BoxVMSymID sym_id,
                      BoxInt sheet_id, BoxInt position) {
  VMSymLabel label;
  label.sheet_id = sheet_id;
  label.position = position;
  return BoxVMSym_Define(vmp, sym_id, & label);
}

/* Same as VM_Label_Define, but sheet_id is the current active sheet and
 * position is the current position in that sheet.
 */
BoxTask BoxVMSym_Def_Label_Here(BoxVM *vmp, BoxVMSymID label_sym_id) {
  BoxInt sheet_id, position;
  sheet_id = vmp->proc_table.target_proc_num;
  position = BoxArr_Num_Items(& vmp->proc_table.target_proc->code);
  return BoxVMSym_Def_Label(vmp, label_sym_id, sheet_id, position);
}

/* Same as VM_Label_New, but sheet_id is the current active sheet and
 * position is the current position in that sheet.
 */
BoxTask BoxVMSym_New_Label_Here(BoxVM *vmp, BoxVMSymID *label_sym_id) {
  *label_sym_id = BoxVMSym_New_Label(vmp);
  return BoxVMSym_Def_Label_Here(vmp, *label_sym_id);
}

BoxTask VM_Sym_Jmp_Generic(BoxVM *vmp, BoxVMSymID sym_id, int conditional) {
  VMSymLabelRef label_ref;
  label_ref.conditional = conditional;
  label_ref.sheet_id = vmp->proc_table.target_proc_num;
  label_ref.position = BoxArr_Num_Items(& vmp->proc_table.target_proc->code);
  return BoxVMSym_Code_Ref(vmp, sym_id, Assemble_Jmp,
                           & label_ref, sizeof(VMSymLabelRef));
}

BoxTask BoxVMSym_Jc(BoxVM *vmp, BoxVMSymID sym_id) {
  return VM_Sym_Jmp_Generic(vmp, sym_id, 1);
}

BoxTask BoxVMSym_Jmp(BoxVM *vmp, BoxVMSymID sym_id) {
  return VM_Sym_Jmp_Generic(vmp, sym_id, 0);
}

BoxTask BoxVMSym_Release_Label(BoxVM *vmp, BoxVMSymID sym_id) {
  return BOXTASK_OK;
}

/*** procedure headers ******************************************************/

typedef struct {
  BoxInt num_var[NUM_TYPES];
  BoxInt num_reg[NUM_TYPES];
} ProcHead;

/* This is the function which assembles the first code in procedures
 * (the one which allocates registers and variables).
 * Since at the beginning of a procedure the number of allocated registers
 * and variables is unknown, we use the symbol manager to adjust
 * automatically the header of procedures when the data is known.
 */
static BoxTask Assemble_Proc_Head(BoxVM *vmp, BoxVMSymID sym_id, BoxUInt sym_type,
                                  int defined, void *def, size_t def_size,
                                  void *ref, size_t ref_size) {
  ProcHead *ph = (ProcHead *) def;
  static BoxInt asm_code[NUM_TYPES] = {BOXOP_NEWC_II, BOXOP_NEWI_II,
                                    BOXOP_NEWR_II, BOXOP_NEWP_II,
                                    BOXOP_NEWO_II};
  int i;

  assert(sym_type == BOXVMSYMTYPE_PROC_HEAD);
  assert(def_size == sizeof(ProcHead));
  assert(def != NULL);

  for(i = 0; i < NUM_TYPES; i++) {
    BoxInt nv = ph->num_var[i], nr = ph->num_reg[i];
    assert(nv >= 0 && nr >= 0);
    BoxVM_Assemble_Long(vmp, asm_code[i],
                        BOXCONTCATEG_IMM, nv, BOXCONTCATEG_IMM, nr);
    /* ^^^ should use BoxVM_Assemble_Long for more than 127 regs/vars */
  }
  return BOXTASK_OK;
}

BoxTask BoxVMSym_Assemble_Proc_Head(BoxVM *vm, BoxVMSymID *sym_id) {
  *sym_id = BoxVMSym_New(vm, BOXVMSYMTYPE_PROC_HEAD, sizeof(ProcHead));
  return BoxVMSym_Code_Ref(vm, *sym_id, Assemble_Proc_Head, NULL, 0);
}

BoxTask BoxVMSym_Def_Proc_Head(BoxVM *vmp, BoxVMSymID sym_id,
                               BoxInt *num_var, BoxInt *num_reg) {
  ProcHead ph;
  int i;
  for(i = 0; i < NUM_TYPES; i++) {
    ph.num_var[i] = num_var[i];
    ph.num_reg[i] = num_reg[i];
  }
  return BoxVMSym_Define(vmp, sym_id, & ph);
}

/*** callables **************************************************************/

/* Temporary, very ugly stuff... */

/**
 * @brief Reference to a procedure.
 */
typedef struct {
  BoxVMCallNum call_num;
} MyProcRef;

/**
 * @brief Data necessary to define a procedure.
 */
typedef struct {
  BoxCallable *callable;
  BoxCCallOld fn_ptr;
} MyProcDef;

BoxBool
BoxVMSym_Proc_Is_Implemented(BoxVM *vm, BoxVMSymID sym_id,
                             const char **c_name) {
  MyProcDef *proc_def = BoxVMSym_Get_Definition(vm, sym_id);
  assert(proc_def);
  *c_name = BoxCallable_Get_Uid(proc_def->callable);
  return BoxCallable_Is_Implemented(proc_def->callable);
}

BoxBool
BoxVMSym_Define_Proc(BoxVM *vm, BoxVMSymID sym_id, BoxCCallOld fn_ptr) {
  MyProcDef *proc_def = BoxVMSym_Get_Definition(vm, sym_id);
  assert(proc_def);

  if (fn_ptr)
    proc_def->fn_ptr = fn_ptr;

  if (BoxVMSym_Define(vm, sym_id, NULL) != BOXTASK_OK)
    return BOXBOOL_FALSE;
  return BOXBOOL_TRUE;
}

static BoxTask
My_Resolve_Proc_Ref(BoxVM *vm, BoxVMSymID sym_id, BoxUInt sym_type,
                    int defined, void *def, size_t def_size,
                    void *ref, size_t ref_size) {
  MyProcRef *proc_ref = ref;
  MyProcDef *proc_def = def;

  assert(sym_type == BOXVMSYMTYPE_PROC && proc_ref && proc_def && defined);
  
  if (BoxCallable_Is_Implemented(proc_def->callable))
    return BOXTASK_OK;

  if (BoxVM_Install_Proc_CCode(vm, proc_ref->call_num, proc_def->fn_ptr))
    return BOXTASK_OK;

  return BOXTASK_FAILURE;
}

/*
 */
BoxBool
BoxVMSym_Reference_Proc(BoxVM *vm, BoxVMCallNum cn, BoxCallable *cb) {
  MyProcRef proc_ref;
  MyProcDef proc_def;
  const char *name = BoxCallable_Get_Uid(cb);

  assert(vm);

  //if (BoxCallable_Is_Implemented(cb))
  //  return BOXBOOL_TRUE;

  proc_def.callable = BoxCallable_Link(cb);

  /* Create a new PROC symbol. */
  BoxVMSymID sym_id = BoxVMSym_Create(vm, BOXVMSYMTYPE_PROC,
                                      & proc_def, sizeof(MyProcDef));
  assert(sym_id != BOXVMSYMID_NONE);

  if (name)
    BoxVMSym_Set_Name(vm, sym_id, name);

  /* Create a new reference to the symbol. */
  proc_ref.call_num = cn;
  BoxVMSym_Ref(vm, sym_id, My_Resolve_Proc_Ref, & proc_ref,
               sizeof(MyProcRef), BOXVMSYM_AUTO);
  return BOXBOOL_TRUE;
}

