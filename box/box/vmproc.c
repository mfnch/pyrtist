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

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "types.h"
#include "messages.h"
#include "mem.h"
#include "virtmach.h"
#include "vmproc.h"
#include "srcpos.h"

static void Procedure_Destroy(void *s) {
  BoxArr_Finish(& ((BoxVMProc *) s)->code);
  BoxSrcPosTable_Finish(& ((BoxVMProc *) s)->pos_table);
}

static void Installed_Procedure_Destroy(void *s) {
  VMProcInstalled *p = (VMProcInstalled *) s;
  BoxMem_Free(p->name);
  BoxMem_Free(p->desc);
}

void BoxVM_Proc_Init(BoxVM *vmp) {
  VMProcTable *pt = & vmp->proc_table;
  BoxOcc_Init(& pt->uninstalled, sizeof(BoxVMProc), VMPROC_UNINST_CLC_SIZE);
  BoxOcc_Set_Finalizer(& pt->uninstalled, Procedure_Destroy);
  BoxArr_Init(& pt->installed, sizeof(VMProcInstalled), VMPROC_INST_ARR_SIZE);
  BoxArr_Set_Finalizer(& pt->installed, Installed_Procedure_Destroy);
  pt->target_proc_num = 0;
  pt->target_proc = (BoxVMProc *) NULL;
  pt->tmp_proc = BoxVM_Proc_Code_New(vmp);
}

void BoxVM_Proc_Finish(BoxVM *vmp) {
  VMProcTable *pt = & vmp->proc_table;
  BoxOcc_Finish(& pt->uninstalled);
  BoxArr_Finish(& pt->installed);
}

/* This function should be called whenever the Collection pt->uninstalled
 * is "touched": i.e. a new uninstalled procedure is inserted or removed.
 * These operations could, indeed, change the address of the target procedure
 * (whose number is pt->target_proc_num), therefore we must re-calculate
 * this address and update pt->target_proc.
 * Yes, I know. This is a great source of nasty bugs...
 */
static void My_Target_Proc_Refresh(BoxVM *vmp) {
  VMProcTable *pt = & vmp->proc_table;
  if (pt->target_proc_num)
    BoxVM_Proc_Target_Set(vmp, pt->target_proc_num);
}

BoxVMProcID BoxVM_Proc_Code_New(BoxVM *vm) {
  VMProcTable *pt = & vm->proc_table;
  BoxVMProc procedure;
  BoxVMProcID n;

  procedure.status.error = 0;
  procedure.status.inhibit = 0;
  BoxArr_Init(& procedure.code, sizeof(VMByteX4), VM_PROC_CODE_SIZE);
  BoxSrcPosTable_Init(& procedure.pos_table);
  n = BoxOcc_Occupy(& pt->uninstalled, & procedure);
  My_Target_Proc_Refresh(vm);
  return n;
}

void BoxVM_Proc_Code_Destroy(BoxVM *vm, BoxVMProcID proc_id) {
  VMProcTable *pt = & vm->proc_table;
  BoxOcc_Release(& pt->uninstalled, proc_id);
  My_Target_Proc_Refresh(vm);
}

VMProcInstalled *My_Get_Proc_From_Num(BoxVM *vmp, BoxVMCallNum call_num) {
  VMProcTable *pt = & vmp->proc_table;
  if (call_num < 1 || call_num > BoxArr_Num_Items(& pt->installed)) {
    MSG_ERROR("The procedure %d is not installed!", call_num);
    return NULL;
  }

  return (VMProcInstalled *) BoxArr_Item_Ptr(& pt->installed, call_num);
}

char *BoxVM_Proc_Get_Description(BoxVM *vm, BoxVMCallNum call_num) {
  VMProcInstalled *p = My_Get_Proc_From_Num(vm, call_num);
  if (p != NULL) {
    switch ((p->desc == NULL) | (p->name == NULL) << 1) {
    case 0: return printdup("%s \"%s\"", p->desc, p->name);
    case 1: return BoxMem_Strdup(p->name);
    case 2: return BoxMem_Strdup(p->desc);
    case 3: return BoxMem_Strdup("(undef)");
    default: return BoxMem_Strdup("(error)");
    }
  }
  return BoxMem_Strdup("(unknown)");
}

BoxVMProcID BoxVM_Proc_Target_Set(BoxVM *vm, BoxVMProcID proc_id) {
  VMProcTable *pt = & vm->proc_table;
  BoxVMProcID previous_target = pt->target_proc_num;
  pt->target_proc_num = proc_id;
  if (proc_id > 0)
    pt->target_proc = (BoxVMProc *) BoxOcc_Item_Ptr(& pt->uninstalled, proc_id);
  else
    pt->target_proc = (BoxVMProc *) NULL;
  return previous_target;
}

BoxVMProcID BoxVM_Proc_Target_Get(BoxVM *vmp) {
  return vmp->proc_table.target_proc_num;
}

void BoxVM_Proc_Empty(BoxVM *vmp, BoxVMProcID id) {
  VMProcTable *pt = & vmp->proc_table;
  BoxVMProc *p = (BoxVMProc *) BoxOcc_Item_Ptr(& pt->uninstalled, id);
  BoxArr_Clear(& p->code);
  BoxSrcPosTable_Clear(& p->pos_table);
}

size_t BoxVM_Proc_Get_Size(BoxVM *vm, BoxVMProcID id) {
  VMProcTable *pt = & vm->proc_table;
  BoxVMProc *p = (BoxVMProc *) BoxOcc_Item_Ptr(& pt->uninstalled, id);
  return BoxArr_Num_Items(& p->code);
}


static VMProcInstalled *My_Get_Inst_Proc_Desc(VMProcTable *pt,
                                              BoxVMCallNum *cn_out,
                                              BoxVMCallNum cn_in) {
  if (cn_in > 0) {
    VMProcInstalled *procedure_inst =
      (VMProcInstalled *) BoxArr_Item_Ptr(& pt->installed, cn_in);

    if (procedure_inst->type == BOXVMPROC_IS_RESERVED) {
      *cn_out = cn_in;
      return procedure_inst;

    } else {
      MSG_FATAL("BoxVM_Proc_Install_CCode: Double procedure installation");
      assert(0);
      *cn_out = BOXVMCALLNUM_NONE;
      return (VMProcInstalled *) NULL;
    }

  } else {
    VMProcInstalled *procedure_inst =
      (VMProcInstalled *) BoxArr_Push(& pt->installed, NULL);
    *cn_out = BoxArr_Num_Items(& pt->installed);
    return procedure_inst;
  }
}

BoxVMCallNum BoxVM_Proc_Install_Code(BoxVM *vm,
                                     BoxVMCallNum required_call_num,
                                     BoxVMProcID id,
                                     const char *name, const char *desc) {
  VMProcTable *pt = & vm->proc_table;
  BoxVMProc *p = (BoxVMProc *) BoxOcc_Item_Ptr(& pt->uninstalled, id);
  BoxVMCallNum cn;
  VMProcInstalled *procedure_inst =
    My_Get_Inst_Proc_Desc(pt, & cn, required_call_num);

  /* Reduce the memory occupied by the procedure */
  BoxSrcPosTable_Compactify(& p->pos_table);
  BoxArr_Compactify(& p->code);

  procedure_inst->type = BOXVMPROC_IS_VM_CODE;
  procedure_inst->name = (name != NULL) ? BoxMem_Strdup(name) : NULL;
  procedure_inst->desc = (desc != NULL) ? BoxMem_Strdup(desc) : NULL;
  procedure_inst->code.proc_id = id;
  return cn;
}

BoxVMCallNum BoxVM_Proc_Install_CCode(BoxVM *vm,
                                      BoxVMCallNum required_call_num,
                                      BoxVMCCode c_proc,
                                      const char *name, const char *desc) {
  VMProcTable *pt = & vm->proc_table;
  BoxVMCallNum cn;
  VMProcInstalled *procedure_inst =
    My_Get_Inst_Proc_Desc(pt, & cn, required_call_num);

  assert(procedure_inst != NULL);
  procedure_inst->type = BOXVMPROC_IS_C_CODE;
  procedure_inst->name = (name != NULL) ? BoxMem_Strdup(name) : NULL;
  procedure_inst->desc = (desc != NULL) ? BoxMem_Strdup(desc) : NULL;
  procedure_inst->code.c = (Task (*)(void *)) c_proc;
  return cn;
}

BoxVMCallNum BoxVM_Proc_Install_Undefined(BoxVM *vm) {
  VMProcTable *pt = & vm->proc_table;
  VMProcInstalled procedure_inst;

  procedure_inst.type = BOXVMPROC_IS_RESERVED;
  procedure_inst.name = (char *) NULL;
  procedure_inst.desc = (char *) NULL;
  BoxArr_Push(& pt->installed, & procedure_inst);
  return BoxArr_Num_Items(& pt->installed);
}

BoxVMProcState BoxVM_Is_Installed(BoxVM *vm, BoxVMCallNum call_num) {
  VMProcTable *pt = & vm->proc_table;
  if (call_num > 0 && call_num <= BoxArr_Num_Items(& pt->installed)) {
    VMProcInstalled *procedure_inst =
      (VMProcInstalled *) BoxArr_Item_Ptr(& pt->installed, call_num);
    return procedure_inst->type;

  } else
    return BOXVMPROC_IS_UNDEFINED;
}

BoxVMCallNum BoxVM_Proc_Next_Call_Num(BoxVM *vm) {
  return BoxArr_Num_Items(& vm->proc_table.installed) + 1;
}

BoxVMProcID BoxVM_Proc_Get_ID(BoxVM *vm, BoxVMCallNum call_num) {
  VMProcInstalled *p = My_Get_Proc_From_Num(vm, call_num);
  if (p->type == BOXVMPROC_IS_VM_CODE)
    return p->code.proc_id;

  else
    return BOXVMPROCID_NONE;
}

void BoxVM_Proc_Get_Ptr_And_Length(BoxVM *vmp, VMByteX4 **ptr,
                                   UInt *length, BoxVMProcID proc_id) {
  VMProcTable *pt = & vmp->proc_table;
  BoxVMProc *procedure = (BoxVMProc *) BoxOcc_Item_Ptr(& pt->uninstalled, proc_id);
  BoxArr *code = & procedure->code;
  if (length != NULL) *length = BoxArr_Num_Items(code);
  if (ptr != NULL) *ptr = (VMByteX4 *) BoxArr_First_Item_Ptr(code);
}

Task BoxVM_Proc_Disassemble(BoxVM *vmp, FILE *out, BoxVMProcID proc_id) {
  VMByteX4 *ptr;
  UInt length;
  BoxVM_Proc_Get_Ptr_And_Length(vmp, & ptr, & length, proc_id);
  return BoxVM_Disassemble(vmp, out, ptr, length);
}

Task BoxVM_Proc_Disassemble_One(BoxVM *vmp, FILE *out,
                                BoxVMCallNum call_num) {
  VMProcInstalled *p = My_Get_Proc_From_Num(vmp, call_num);
  char *p_name, *p_desc, *p_type;

  if (p == NULL)
    return Failed;

  p_name = (p->name != NULL) ? p->name : "(undef)";
  p_desc = (p->desc != NULL) ? p->desc : "(undef)";
  switch(p->type) {
  case BOXVMPROC_IS_VM_CODE: p_type = "VM"; break;
  case BOXVMPROC_IS_C_CODE:  p_type = "C"; break;
  default: p_type = "(broken?)"; break;
  }

  fprintf(out, "%s procedure "SUInt"; name=%s; desc=%s\n",
          p_type, (UInt) call_num, p_name, p_desc);

  if (p->type == BOXVMPROC_IS_VM_CODE) {
    fprintf(out, "\n");
    Task t = BoxVM_Proc_Disassemble(vmp, out, p->code.proc_id);
    fprintf(out, "----------------------------------------\n");
    return t;
  }
  return Success;
}

Task BoxVM_Proc_Disassemble_All(BoxVM *vmp, FILE *out) {
  VMProcTable *pt = & vmp->proc_table;
  UInt n, proc_id;
  char *s;

  BoxVM_Data_Display(vmp, out);
  fprintf(out, "\n");

  fprintf(out, "*** OBJECT DESCRIPTOR TABLE ***\n");
  s = BoxVM_ObjDesc_Table_To_Str(vmp);
  fprintf(out, "%s", s);
  BoxMem_Free(s);
  fprintf(out, "*** END OF OBJECT DESCRIPTOR TABLE ***\n\n");


  proc_id = BoxArr_Num_Items(& pt->installed);
  for(n = 1; n <= proc_id; n++) {
    TASK( BoxVM_Proc_Disassemble_One(vmp, out, n) );
  }
  return Success;
}

void BoxVM_Proc_Associate_Source(BoxVM *vm, BoxVMProcID id, BoxSrcPos *sp) {
  VMProcTable *pt = & vm->proc_table;
  BoxVMProc *p = (BoxVMProc *) BoxOcc_Item_Ptr(& pt->uninstalled, id);
  BoxOutPos op = sizeof(VMByteX4)*BoxArr_Num_Items(& p->code);
  BoxSrcPosTable_Associate(& p->pos_table, op, sp);
}

BoxSrcPos *BoxVM_Proc_Get_Source_Of(BoxVM *vm, BoxVMProcID id, BoxOutPos op) {
  VMProcTable *pt = & vm->proc_table;
  BoxVMProc *p = (BoxVMProc *) BoxOcc_Item_Ptr(& pt->uninstalled, id);
  return BoxSrcPosTable_Get_Src_Of(& p->pos_table, op);
}
