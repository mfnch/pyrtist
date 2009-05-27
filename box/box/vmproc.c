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

#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "messages.h"
#include "mem.h"
#include "virtmach.h"
#include "vmproc.h"

static void Procedure_Destroy(void *s) {
  BoxArr *code = & ((VMProc *) s)->code;
  if (code != NULL) BoxArr_Finish(code);
}

static void Installed_Procedure_Destroy(void *s) {
  VMProcInstalled *p = (VMProcInstalled *) s;
  BoxMem_Free(p->name);
  BoxMem_Free(p->desc);
}

Task VM_Proc_Init(VMProgram *vmp) {
  VMProcTable *pt = & vmp->proc_table;
  BoxOcc_Init(& pt->uninstalled, sizeof(VMProc), VMPROC_UNINST_CLC_SIZE);
  BoxOcc_Set_Finalizer(& pt->uninstalled, Procedure_Destroy);
  BoxArr_Init(& pt->installed, sizeof(VMProcInstalled), VMPROC_INST_ARR_SIZE);
  BoxArr_Set_Finalizer(& pt->installed, Installed_Procedure_Destroy);
  pt->target_proc_num = 0;
  pt->target_proc = (VMProc *) NULL;
  TASK( VM_Proc_Code_New(vmp, & pt->tmp_proc) );
  return Success;
}

void VM_Proc_Destroy(VMProgram *vmp) {
  VMProcTable *pt = & vmp->proc_table;
  BoxOcc_Finish(& pt->uninstalled);
  BoxArr_Finish(& pt->installed);
}

/* This function whenever the Collection pt->uninstalled is touched:
 * a new uninstalled procedure is inserted or removed.
 * These operations could change the address of the target procedure
 * (whose number is pt->target_proc_num), therefore we must re-calculate
 * this address and update pt->target_proc.
 * Yes, I know. This is a great source of nasty bugs...
 */
static void target_proc_refresh(VMProgram *vmp) {
  VMProcTable *pt = & vmp->proc_table;
  if (pt->target_proc_num)
    BoxVM_Proc_Target_Set(vmp, pt->target_proc_num);
}

Task VM_Proc_Code_New(VMProgram *vmp, UInt *proc_num) {
  VMProcTable *pt = & vmp->proc_table;
  VMProc procedure;
  UInt n;

  procedure.status.error = 0;
  procedure.status.inhibit = 0;
  BoxArr_Init(& procedure.code, sizeof(VMByteX4), VM_PROC_CODE_SIZE);
  n = BoxOcc_Occupy(& pt->uninstalled, & procedure);
  if (proc_num != NULL) *proc_num = n;
  target_proc_refresh(vmp);
  return Success;
}

Task VM_Proc_Code_Destroy(VMProgram *vmp, UInt proc_num) {
  VMProcTable *pt = & vmp->proc_table;
  BoxOcc_Release(& pt->uninstalled, proc_num);
  target_proc_refresh(vmp);
  return Success;
}

BoxVMProcID BoxVM_Proc_Target_Set(BoxVM *vm, BoxVMProcID proc_id) {
  VMProcTable *pt = & vm->proc_table;
  BoxVMProcID previous_target = pt->target_proc_num;
  pt->target_proc_num = proc_id;
  if (proc_id > 0)
    pt->target_proc = (VMProc *) BoxOcc_Item_Ptr(& pt->uninstalled, proc_id);
  else
    pt->target_proc = (VMProc *) NULL;
  return previous_target;
}

BoxVMProcID BoxVM_Proc_Target_Get(VMProgram *vmp) {
  return vmp->proc_table.target_proc_num;
}

void VM_Proc_Empty(VMProgram *vmp, UInt proc_num) {
  VMProcTable *pt = & vmp->proc_table;
  VMProc *procedure = (VMProc *) BoxOcc_Item_Ptr(& pt->uninstalled, proc_num);
  BoxArr_Clear(& procedure->code);
}

void VM_Proc_Install_Code(VMProgram *vmp, UInt *call_num,
                          UInt proc_num, const char *name,
                          const char *desc) {
  VMProcTable *pt = & vmp->proc_table;
  VMProcInstalled procedure_inst;

  procedure_inst.type = VMPROC_IS_VM_CODE;
  procedure_inst.name = BoxMem_Strdup(name);
  procedure_inst.desc = BoxMem_Strdup(desc);
  procedure_inst.code.proc_num = proc_num;
#if 0
  TASK( Clc_Object_Ptr(pt->uninstalled, & procedure, proc_num) );
  procedure_inst.code.vm.size = Arr_NumItem(((VMProc *) procedure)->code);;
  TASK( Arr_Data_Only(((VMProc *) procedure)->code, & code_ptr) );
  procedure_inst.code.vm.ptr = code_ptr;
#endif

  *call_num = BoxArr_Num_Items(& pt->installed) + 1;
  BoxArr_Push(& pt->installed, & procedure_inst);
}

void VM_Proc_Install_CCode(VMProgram *vmp, UInt *call_num,
                           VMCCode c_proc, const char *name,
                           const char *desc) {
  VMProcTable *pt = & vmp->proc_table;
  VMProcInstalled procedure_inst;

  procedure_inst.type = VMPROC_IS_C_CODE;
  procedure_inst.name = BoxMem_Strdup(name);
  procedure_inst.desc = BoxMem_Strdup(desc);
  procedure_inst.code.c = (Task (*)(void *)) c_proc;

  *call_num = BoxArr_Num_Items(& pt->installed) + 1;
  BoxArr_Push(& pt->installed, & procedure_inst);
}

UInt VM_Proc_Install_Number(VMProgram *vmp) {
  return BoxArr_Num_Items(& vmp->proc_table.installed) + 1;
}

void VM_Proc_Ptr_And_Length(VMProgram *vmp, VMByteX4 **ptr,
                            UInt *length, int proc_num) {
  VMProcTable *pt = & vmp->proc_table;
  VMProc *procedure = (VMProc *) BoxOcc_Item_Ptr(& pt->uninstalled, proc_num);
  BoxArr *code = & procedure->code;
  if (length != NULL) *length = BoxArr_Num_Items(code);
  if (ptr != NULL) *ptr = (VMByteX4 *) BoxArr_First_Item_Ptr(code);
}

Task VM_Proc_Disassemble(VMProgram *vmp, FILE *out, UInt proc_num) {
  VMByteX4 *ptr;
  UInt length;
  VM_Proc_Ptr_And_Length(vmp, & ptr, & length, proc_num);
  return BoxVM_Disassemble(vmp, out, ptr, length);
}

Task VM_Proc_Disassemble_One(VMProgram *vmp, FILE *out, UInt call_num) {
  VMProcTable *pt = & vmp->proc_table;
  VMProcInstalled *p;
  char *mod_type;
  int print_code;

  if (call_num < 1 || call_num > BoxArr_Num_Items(& pt->installed)) {
    MSG_ERROR("The procedure %d is not installed!", call_num);
    return Failed;
  }

  p = (VMProcInstalled *) BoxArr_Item_Ptr(& pt->installed, call_num);
  fprintf(out, "\n----------------------------------------\n");
  fprintf(out, "Procedure number: "SUInt"\n", (UInt) call_num);
  if ( p->name != NULL )
    fprintf(out, "Name: '%s'\n", p->name);
  else
    fprintf(out, "Name: (undefined)\n");

  if ( p->desc != NULL )
    fprintf(out, "Description: '%s'\n", p->desc);
  else
    fprintf(out, "Description: (undefined)\n");

  print_code = 0;
  switch(p->type) {
    case VMPROC_IS_VM_CODE: mod_type = "BOX-VM code"; print_code = 0; break;
    case VMPROC_IS_C_CODE:  mod_type = "external C-function"; break;
    default:  mod_type = "Broken procedure, Aarrggg..."; break;
  }
  fprintf(out, "Type: %s\n", mod_type);

  if (p->type == VMPROC_IS_VM_CODE) {
/*   if ( print_code ) */
    return VM_Proc_Disassemble(vmp, out, p->code.proc_num);
  }
  return Success;
}

Task VM_Proc_Disassemble_All(VMProgram *vmp, FILE *out) {
  VMProcTable *pt = & vmp->proc_table;
  UInt n, proc_num;

  BoxVM_Data_Display(vmp, out);
  fprintf(out, "\n");

  proc_num = BoxArr_Num_Items(& pt->installed);
  for(n = 1; n <= proc_num; n++) {
    TASK( VM_Proc_Disassemble_One(vmp, out, n) );
  }
  return Success;
}

#if 0

/******************************************************************************
 * Functions to handle sheets: a sheet is a place where you can put temporary *
 * code, which can then be istalled as a new module or handled in other ways. *
 ******************************************************************************/

/* This function returns the module-number which will be
 * associated with the next module that will be installed.
 */
Int VM_Module_Next(VMProgram *vmp) {
  if ( vmp->vm_modules_list == NULL ) return 1;
  return Arr_NumItem(vmp->vm_modules_list) + 1;
}

#endif
