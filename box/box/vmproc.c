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
#include "vm_private.h"
#include "vmproc.h"
#include "srcpos.h"
#include "vmalloc.h"
#include "vmproc_priv.h"

static void Procedure_Destroy(void *s) {
  BoxArr_Finish(& ((BoxVMProc *) s)->code);
  BoxSrcPosTable_Finish(& ((BoxVMProc *) s)->pos_table);
}

static void My_Destroy_Installed_Procedure(void *s) {
  BoxVMProcInstalled *p = (BoxVMProcInstalled *) s;
  if (p->type == BOXVMPROCKIND_FOREIGN)
    (void) BoxCallable_Unlink(p->code.foreign);
  BoxMem_Free(p->name);
  BoxMem_Free(p->desc);
}

void BoxVM_Proc_Init(BoxVM *vmp) {
  BoxVMProcTable *pt = & vmp->proc_table;
  BoxOcc_Init(& pt->uninstalled, sizeof(BoxVMProc), VMPROC_UNINST_CLC_SIZE);
  BoxOcc_Set_Finalizer(& pt->uninstalled, Procedure_Destroy);
  BoxArr_Init(& pt->installed, sizeof(BoxVMProcInstalled), VMPROC_INST_ARR_SIZE);
  BoxArr_Set_Finalizer(& pt->installed, My_Destroy_Installed_Procedure);
  pt->target_proc_num = 0;
  pt->target_proc = (BoxVMProc *) NULL;
  pt->tmp_proc = BoxVM_Proc_Code_New(vmp);
}

void BoxVM_Proc_Finish(BoxVM *vmp) {
  BoxVMProcTable *pt = & vmp->proc_table;
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
  BoxVMProcTable *pt = & vmp->proc_table;
  if (pt->target_proc_num)
    BoxVM_Proc_Target_Set(vmp, pt->target_proc_num);
}

BoxVMProcID BoxVM_Proc_Code_New(BoxVM *vm) {
  BoxVMProcTable *pt = & vm->proc_table;
  BoxVMProc procedure;
  BoxVMProcID n;

  procedure.status.error = 0;
  procedure.status.inhibit = 0;
  BoxArr_Init(& procedure.code, sizeof(BoxVMWord), VM_PROC_CODE_SIZE);
  BoxSrcPosTable_Init(& procedure.pos_table);
  n = BoxOcc_Occupy(& pt->uninstalled, & procedure);
  My_Target_Proc_Refresh(vm);
  return n;
}

void BoxVM_Proc_Code_Destroy(BoxVM *vm, BoxVMProcID proc_id) {
  BoxVMProcTable *pt = & vm->proc_table;
  BoxOcc_Release(& pt->uninstalled, proc_id);
  My_Target_Proc_Refresh(vm);
}

BoxVMProcInstalled *My_Get_Proc_From_Num(BoxVM *vmp, BoxVMCallNum call_num) {
  BoxVMProcTable *pt = & vmp->proc_table;
  if (call_num < 1 || call_num > BoxArr_Num_Items(& pt->installed)) {
    MSG_ERROR("The procedure %d is not installed!", call_num);
    return NULL;
  }

  return (BoxVMProcInstalled *) BoxArr_Item_Ptr(& pt->installed, call_num);
}

char *BoxVM_Proc_Get_Description(BoxVM *vm, BoxVMCallNum call_num) {
  BoxVMProcInstalled *p = My_Get_Proc_From_Num(vm, call_num);
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
  BoxVMProcTable *pt = & vm->proc_table;
  BoxVMProcID previous_target = pt->target_proc_num;
  pt->target_proc_num = proc_id;
  pt->target_proc =
    (BoxVMProc *) ((proc_id > 0) ?
                   BoxOcc_Item_Ptr(& pt->uninstalled, proc_id) : NULL);
  return previous_target;
}

BoxVMProcID BoxVM_Proc_Target_Get(BoxVM *vmp) {
  return vmp->proc_table.target_proc_num;
}

void BoxVM_Proc_Empty(BoxVM *vmp, BoxVMProcID id) {
  BoxVMProcTable *pt = & vmp->proc_table;
  BoxVMProc *p = (BoxVMProc *) BoxOcc_Item_Ptr(& pt->uninstalled, id);
  BoxArr_Clear(& p->code);
  BoxSrcPosTable_Clear(& p->pos_table);
}

size_t BoxVM_Proc_Get_Size(BoxVM *vm, BoxVMProcID id) {
  BoxVMProcTable *pt = & vm->proc_table;
  BoxVMProc *p = (BoxVMProc *) BoxOcc_Item_Ptr(& pt->uninstalled, id);
  return BoxArr_Num_Items(& p->code);
}

/**
 * @brief Get the pointer to an installed procedure structure.
 *
 * @param pt The procedure table.
 * @param cn The call number of the required procedure.
 * @param inst_proc Where to put the pointer of the installed procedure
 *   structure.
 * @return Return the kind of the installed procedure associated to @p cn
 *   or @c BOXVMPROCKIND_UNDEFINED.
 */
static BoxVMProcKind
My_Get_Inst_Proc_Desc(BoxVMProcTable *pt, BoxVMCallNum cn,
                      BoxVMProcInstalled **inst_proc) {
  if (cn != BOXVMCALLNUM_NONE) {
    if (cn > BoxArr_Get_Num_Items(& pt->installed))
      return BOXVMPROCKIND_UNDEFINED;
        
    *inst_proc = (BoxVMProcInstalled *) BoxArr_Item_Ptr(& pt->installed, cn);
    return (*inst_proc)->type;
  }

  return BOXVMPROCKIND_UNDEFINED;
}

/* Install a procedure from VM code. */
BoxBool
BoxVM_Install_Proc_Code(BoxVM *vm, BoxVMCallNum cn, BoxVMProcID id) {
  BoxVMProcTable *pt = & vm->proc_table;
  BoxVMProc *p = (BoxVMProc *) BoxOcc_Item_Ptr(& pt->uninstalled, id);
  BoxVMProcInstalled *inst_proc;

  if (My_Get_Inst_Proc_Desc(pt, cn, & inst_proc) != BOXVMPROCKIND_RESERVED)
    return BOXBOOL_FALSE;

  /* Reduce the memory occupied by the procedure */
  BoxSrcPosTable_Compactify(& p->pos_table);
  BoxArr_Compactify(& p->code);

  inst_proc->type = BOXVMPROCKIND_VM_CODE;
  inst_proc->name = NULL;
  inst_proc->desc = NULL;
  inst_proc->code.proc_id = id;
  return BOXBOOL_TRUE;
}

/* Installs the given C function as a new procedure. */
BoxBool
BoxVM_Install_Proc_CCode(BoxVM *vm, BoxVMCallNum cn, BoxVMCCode c_proc) {
  BoxVMProcTable *pt = & vm->proc_table;
  BoxVMProcInstalled *inst_proc;

  if (My_Get_Inst_Proc_Desc(pt, cn, & inst_proc) != BOXVMPROCKIND_RESERVED)
    return BOXBOOL_FALSE;

  inst_proc->type = BOXVMPROCKIND_C_CODE;
  inst_proc->name = NULL;
  inst_proc->desc = NULL;
  inst_proc->code.c = (Task (*)(void *)) c_proc;
  return BOXBOOL_TRUE;
}

/* Installs the given callable as a new procedure. */
BoxBool
BoxVM_Install_Proc_Callable(BoxVM *vm, BoxVMCallNum cn, BoxCallable *cb) {
  BoxVMProcTable *pt = & vm->proc_table;
  BoxVMProcInstalled *inst_proc;

  if (My_Get_Inst_Proc_Desc(pt, cn, & inst_proc) != BOXVMPROCKIND_RESERVED)
    return BOXBOOL_FALSE;

  inst_proc->type = BOXVMPROCKIND_FOREIGN;
  inst_proc->name = NULL;
  inst_proc->desc = NULL;
  inst_proc->code.foreign = BoxCallable_Link(cb);
  return BOXBOOL_TRUE;
}

/* Set the names for an installed procedure. */
BoxBool
BoxVM_Set_Proc_Names(BoxVM *vm, BoxVMCallNum cn,
                     const char *name, const char *desc) {
  BoxVMProcTable *pt = & vm->proc_table;
  BoxVMProcInstalled *inst_proc;

  if (My_Get_Inst_Proc_Desc(pt, cn, & inst_proc) == BOXVMPROCKIND_UNDEFINED)
    return BOXBOOL_FALSE;

  if (name)
    inst_proc->name = BoxMem_Strdup(name);

  if (desc)
    inst_proc->desc = BoxMem_Strdup(desc);

  return BOXBOOL_TRUE;
}

/* Allocate a new call number. */
BoxVMCallNum
BoxVM_Allocate_CallNum(BoxVM *vm) {
  BoxArr *inst_procs = & vm->proc_table.installed;
  BoxVMProcInstalled *inst_proc = BoxArr_Push(inst_procs, NULL);

  if (inst_proc) {
    inst_proc->type = BOXVMPROCKIND_RESERVED;
    inst_proc->name = NULL;
    inst_proc->desc = NULL;
    return BoxArr_Get_Num_Items(inst_procs);
  } else
    return BOXVMCALLNUM_NONE;
}

/* Deallocate the most recently allocated call number. */
BoxBool
BoxVM_Deallocate_CallNum(BoxVM *vm, BoxVMCallNum num) {
  BoxArr *inst_procs = & vm->proc_table.installed;

  if (num == BOXVMCALLNUM_NONE)
    return BOXBOOL_TRUE;

  if (BoxArr_Get_Num_Items(inst_procs) == num) {
    BoxVMProcInstalled *inst_proc = BoxArr_Get_Last_Item_Ptr(inst_procs);
    if (inst_proc->type != BOXVMPROCKIND_RESERVED)
      return BOXBOOL_FALSE;

    BoxArr_Pop(inst_procs, NULL);
    return BOXBOOL_TRUE;
  }

  return BOXBOOL_FALSE;
}

/* Return the procedure kind for the given call number. */
BoxVMProcKind BoxVM_Get_Proc_Kind(BoxVM *vm, BoxVMCallNum cn) {
  BoxArr *inst_procs = & vm->proc_table.installed;
  if (cn > 0 && cn <= BoxArr_Num_Items(inst_procs)) {
    BoxVMProcInstalled *inst_proc = BoxArr_Item_Ptr(inst_procs, cn);
    return inst_proc->type;
  }

  return BOXVMPROCKIND_UNDEFINED;
}

/* Get the callable associated to a foreign procedure. */
BoxBool 
BoxVM_Get_Callable_Implem(BoxVM *vm, BoxVMCallNum cn, BoxCallable **code) {
  BoxArr *inst_procs = & vm->proc_table.installed;
  if (cn > 0 && cn <= BoxArr_Num_Items(inst_procs)) {
    BoxVMProcInstalled *inst_proc = BoxArr_Item_Ptr(inst_procs, cn);
    if (inst_proc->type == BOXVMPROCKIND_FOREIGN)
      *code = inst_proc->code.foreign;
  }

  return BOXBOOL_FALSE;
}

/* Take note that a given call number is used and needs to be resolved. */
void
BoxVM_Reference_Proc(BoxVM *vm, BoxVMCallNum cn) {
  
}

BoxVMProcID BoxVM_Proc_Get_ID(BoxVM *vm, BoxVMCallNum call_num) {
  BoxVMProcInstalled *p = My_Get_Proc_From_Num(vm, call_num);
  if (p->type == BOXVMPROCKIND_VM_CODE)
    return p->code.proc_id;
  return BOXVMPROCID_NONE;
}

void BoxVM_Proc_Get_Ptr_And_Length(BoxVM *vmp, BoxVMWord **ptr,
                                   UInt *length, BoxVMProcID proc_id) {
  BoxVMProcTable *pt = & vmp->proc_table;
  BoxVMProc *procedure =
    (BoxVMProc *) BoxOcc_Item_Ptr(& pt->uninstalled, proc_id);
  BoxArr *code = & procedure->code;
  if (length != NULL) *length = BoxArr_Num_Items(code);
  if (ptr != NULL) *ptr = (BoxVMWord *) BoxArr_First_Item_Ptr(code);
}

Task BoxVM_Proc_Disassemble(BoxVM *vmp, FILE *out, BoxVMProcID proc_id) {
  BoxVMWord *ptr;
  UInt length;
  BoxVM_Proc_Get_Ptr_And_Length(vmp, & ptr, & length, proc_id);
  return BoxVM_Disassemble(vmp, out, ptr, length);
}

Task BoxVM_Proc_Disassemble_One(BoxVM *vmp, FILE *out,
                                BoxVMCallNum call_num) {
  BoxVMProcInstalled *p = My_Get_Proc_From_Num(vmp, call_num);
  char *p_name, *p_desc, *p_type;

  if (p == NULL)
    return BOXTASK_FAILURE;

  p_name = (p->name) ? p->name : "(undef)";
  p_desc = (p->desc) ? p->desc : "(undef)";
  switch (p->type) {
  case BOXVMPROCKIND_RESERVED: p_type = "unresolved"; break;
  case BOXVMPROCKIND_VM_CODE: p_type = "VM"; break;
  case BOXVMPROCKIND_FOREIGN:  p_type = "foreign"; break;
  case BOXVMPROCKIND_C_CODE:  p_type = "C"; break;
  default: p_type = "(broken?)"; break;
  }

  fprintf(out, "%s procedure "SUInt"; name=%s; desc=%s\n",
          p_type, (UInt) call_num, p_name, p_desc);

  if (p->type == BOXVMPROCKIND_VM_CODE) {
    fprintf(out, "\n");
    Task t = BoxVM_Proc_Disassemble(vmp, out, p->code.proc_id);
    fprintf(out, "----------------------------------------\n");
    return t;
  }
  return BOXTASK_OK;
}

Task BoxVM_Proc_Disassemble_All(BoxVM *vmp, FILE *out) {
  BoxVMProcTable *pt = & vmp->proc_table;
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
    BOXTASK( BoxVM_Proc_Disassemble_One(vmp, out, n) );
  }
  return BOXTASK_OK;
}

void BoxVM_Proc_Associate_Source(BoxVM *vm, BoxVMProcID id, BoxSrcPos *sp) {
  BoxVMProcTable *pt = & vm->proc_table;
  BoxVMProc *p = (BoxVMProc *) BoxOcc_Item_Ptr(& pt->uninstalled, id);
  BoxOutPos op = sizeof(BoxVMWord)*BoxArr_Num_Items(& p->code);
  BoxSrcPosTable_Associate(& p->pos_table, op, sp);
}

BoxSrcPos *BoxVM_Proc_Get_Source_Of(BoxVM *vm, BoxVMProcID id, BoxOutPos op) {
  BoxVMProcTable *pt = & vm->proc_table;
  BoxVMProc *p = (BoxVMProc *) BoxOcc_Item_Ptr(& pt->uninstalled, id);
  return BoxSrcPosTable_Get_Src_Of(& p->pos_table, op);
}
