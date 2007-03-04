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

#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "messages.h"
#include "mem.h"
#include "virtmach.h"
#include "vmproc.h"

static Task Procedure_Destroy(void *s) {
  Array *code = ((VMProc *) s)->code;
  if ( code != (Array *) NULL ) Arr_Destroy( code );
  return Success;
}

static Task Installed_Procedure_Destroy(void *s) {
  VMProcInstalled *p = (VMProcInstalled *) s;
  free(p->name);
  free(p->desc);
  return Success;
}

Task VM_Proc_Init(VMProgram *vmp) {
  VMProcTable *pt = & vmp->proc_table;
  TASK( Clc_New(& pt->uninstalled, sizeof(VMProc), VMPROC_UNINST_CLC_SIZE) );
  Clc_Destructor(pt->uninstalled, Procedure_Destroy);
  TASK( Arr_New(& pt->installed, sizeof(VMProcInstalled),
                VMPROC_INST_ARR_SIZE) );
  Arr_Destructor(pt->installed, Installed_Procedure_Destroy);
  pt->target_proc_num = 0;
  pt->target_proc = (VMProc *) NULL;
  TASK( VM_Proc_Code_New(vmp, & pt->tmp_proc) );
  return Success;
}

void VM_Proc_Destroy(VMProgram *vmp) {
  VMProcTable *pt = & vmp->proc_table;
  Clc_Destroy(pt->uninstalled);
  Arr_Destroy(pt->installed);
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
    (void) VM_Proc_Target_Set(vmp, pt->target_proc_num);
}

Task VM_Proc_Code_New(VMProgram *vmp, UInt *proc_num) {
  VMProcTable *pt = & vmp->proc_table;
  VMProc procedure;
  procedure.status.error = 0;
  procedure.status.inhibit = 0;
  TASK( Arr_New(& procedure.code, sizeof(VMByteX4), VM_PROC_CODE_SIZE) );
  TASK( Clc_Occupy(pt->uninstalled, & procedure, proc_num) );
  target_proc_refresh(vmp);
  return Success;
}

Task VM_Proc_Code_Destroy(VMProgram *vmp, UInt proc_num) {
  VMProcTable *pt = & vmp->proc_table;
  TASK( Clc_Release(pt->uninstalled, proc_num) );
  target_proc_refresh(vmp);
  return Success;
}

Task VM_Proc_Target_Set(VMProgram *vmp, UInt proc_num) {
  VMProcTable *pt = & vmp->proc_table;
  void *procedure;
  TASK( Clc_Object_Ptr(pt->uninstalled, & procedure, proc_num) );
  pt->target_proc_num = proc_num;
  pt->target_proc = (VMProc *) procedure;
  return Success;
}

UInt VM_Proc_Target_Get(VMProgram *vmp) {
  return vmp->proc_table.target_proc_num;
}

Task VM_Proc_Empty(VMProgram *vmp, UInt proc_num) {
  VMProcTable *pt = & vmp->proc_table;
  void *procedure;
  TASK( Clc_Object_Ptr(pt->uninstalled, & procedure, proc_num) );
  return Arr_Empty(((VMProc *) procedure)->code);
}

Task VM_Proc_Install_Code(VMProgram *vmp, UInt *call_num,
                          UInt proc_num, const char *name,
                          const char *desc) {
  VMProcTable *pt = & vmp->proc_table;
  VMProcInstalled procedure_inst;

  procedure_inst.type = VMPROC_IS_VM_CODE;
  procedure_inst.name = Mem_Strdup(name);
  procedure_inst.desc = Mem_Strdup(desc);
  procedure_inst.code.proc_num = proc_num;
#if 0
  TASK( Clc_Object_Ptr(pt->uninstalled, & procedure, proc_num) );
  procedure_inst.code.vm.size = Arr_NumItem(((VMProc *) procedure)->code);;
  TASK( Arr_Data_Only(((VMProc *) procedure)->code, & code_ptr) );
  procedure_inst.code.vm.ptr = code_ptr;
#endif

  *call_num = Arr_NumItem(pt->installed) + 1;
  TASK( Arr_Push(pt->installed, & procedure_inst) );
  return Success;
}

Task VM_Proc_Install_CCode(VMProgram *vmp, UInt *call_num,
                           VMCCode c_proc, const char *name,
                           const char *desc) {
  VMProcTable *pt = & vmp->proc_table;
  VMProcInstalled procedure_inst;

  procedure_inst.type = VMPROC_IS_C_CODE;
  procedure_inst.name = Mem_Strdup(name);
  procedure_inst.desc = Mem_Strdup(desc);
  procedure_inst.code.c = (Task (*)(void *)) c_proc;

  *call_num = Arr_NumItem(pt->installed) + 1;
  TASK( Arr_Push(pt->installed, & procedure_inst) );
  return Success;
}

UInt VM_Proc_Install_Number(VMProgram *vmp) {
  return Arr_NumItem(vmp->proc_table.installed) + 1;
}

Task VM_Proc_Ptr_And_Length(VMProgram *vmp, VMByteX4 **ptr,
                            UInt *length, int proc_num) {
  VMProcTable *pt = & vmp->proc_table;
  void *procedure;
  Array *code;
  TASK( Clc_Object_Ptr(pt->uninstalled, & procedure, proc_num) );
  code = ((VMProc *) procedure)->code;
  if (length != NULL) *length = Arr_NumItem(code);
  if (ptr != NULL) *ptr = Arr_FirstItemPtr(code, VMByteX4);
  return Success;
}

Task VM_Proc_Disassemble(VMProgram *vmp, FILE *out, UInt proc_num) {
  VMByteX4 *ptr;
  UInt length;
  TASK( VM_Proc_Ptr_And_Length(vmp, & ptr, & length, proc_num) );
  return VM_Disassemble(vmp, out, ptr, length);
}

Task VM_Proc_Disassemble_One(VMProgram *vmp, FILE *out, UInt call_num)
{
  VMProcTable *pt = & vmp->proc_table;
  VMProcInstalled *p;
  char *mod_type;
  int print_code;

  if (call_num < 1 || call_num > Arr_NumItem(pt->installed)) {
    MSG_ERROR("The procedure %d is not installed!", call_num);
    return Failed;
  }

  p = Arr_ItemPtr( pt->installed, VMProcInstalled, call_num );
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

  proc_num = Arr_NumItem(pt->installed);
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
Intg VM_Module_Next(VMProgram *vmp) {
  if ( vmp->vm_modules_list == NULL ) return 1;
  return Arr_NumItem(vmp->vm_modules_list) + 1;
}

#endif
