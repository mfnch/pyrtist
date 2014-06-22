/****************************************************************************
 * Copyright (C) 2009 by Matteo Franchin                                    *
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

#include <stdarg.h>
#include <assert.h>

#include "types.h"
#include "mem.h"
#include "messages.h"
#include "vm_priv.h"
#include "vmproc.h"
#include "container.h"
#include "registers.h"

#include "vmcode.h"
#include "compiler_priv.h"
#include "vmop_priv.h"


/* Function called to end for BOXVMCODETYPE_MAIN or BOXVMCODETYPE_SUB */
static void
My_Proc_End(BoxVMCode *p)
{
  RegAlloc *ra = BoxVMCode_Get_RegAlloc(p);
  uint32_t num_temps[NUM_REGISTER_TYPES], num_vars[NUM_REGISTER_TYPES];

  /* Global registers are allocated only for main */
  if (p->style == BOXVMCODESTYLE_MAIN) {
    RegAlloc_Get_Global_Nums(ra, num_temps, num_vars);
    ASSERT_TASK( BoxVM_Alloc_Global_Regs(p->cmp->vm, num_vars, num_temps) );
  }

  BoxLIR_Append_Op(p->cmp->lir, BOXOP_RET);
}

void
BoxVMCode_Init(BoxVMCode *p, BoxCmp *c, BoxVMCodeStyle style)
{
  p->style = style;
  p->cmp = c;
  p->have.parent = 0;
  p->have.parent_reg = 0;
  p->have.child = 0;
  p->have.child_reg = 0;
  p->have.reg_alloc = 0;
  p->have.proc_name = 0;
  p->have.alter_name = 0;
  p->have.call_num = 0;
  p->have.installed = 0;

  switch(style) {
  case BOXVMCODESTYLE_SUB:
  case BOXVMCODESTYLE_MAIN:
    /* Force initialisation of the register allocator. */
    (void) BoxVMCode_Get_RegAlloc(p);
    break;
  case BOXVMCODESTYLE_EXTERN:
    break;
  default:
    MSG_FATAL("BoxVMCode_Init: Invalid value for style (BoxVMCodeStyle).");
    abort();
  }

  p->have.callable = 0;
  p->callable = NULL;
}

void
BoxVMCode_Finish(BoxVMCode *p)
{
  if (p->have.callable)
    (void) BoxCallable_Unlink(p->callable);
  if (p->have.proc_name)
    Box_Mem_Free(p->proc_name);
  if (p->have.alter_name)
    Box_Mem_Free(p->alter_name);
  if (p->have.reg_alloc)
    Reg_Finish(& p->reg_alloc);
}

void
BoxVMCode_Set_Callable(BoxVMCode *p, BoxCallable *cb)
{
  if (p->have.callable)
    (void) BoxCallable_Unlink(cb);

  p->callable = BoxCallable_Link(cb);
  p->have.callable = 1;
}

void
BoxVMCode_Set_Prototype(BoxVMCode *p, int have_child, int have_parent)
{
  /* Make sure the prototype is consistent with the previous one. */
  assert(!p->have.parent || have_parent);
  assert(!p->have.child || have_child);

  p->have.parent = have_parent;
  p->have.child = have_child;
}

BoxVMRegNum
BoxVMCode_Get_Parent_Reg(BoxVMCode *p)
{
  assert(p->have.parent);

  if (p->have.parent_reg)
    return p->reg_parent;

  p->reg_parent = Reg_Occupy(& p->reg_alloc, BOXTYPEID_PTR);
  p->have.parent_reg = 1;
  return p->reg_parent;
}

BoxVMRegNum
BoxVMCode_Get_Child_Reg(BoxVMCode *p)
{
  assert(p->have.child);

  if (p->have.child_reg)
    return p->reg_child;

  p->reg_child = Reg_Occupy(& p->reg_alloc, BOXTYPEID_PTR);
  p->have.child_reg = 1;
  return p->reg_child;
}

BoxVMCodeStyle
BoxVMCode_Get_Style(BoxVMCode *p)
{
  return p->style;
}

RegAlloc *
BoxVMCode_Get_RegAlloc(BoxVMCode *p)
{
  if (p->have.reg_alloc)
    return & p->reg_alloc;

  Reg_Init(& p->reg_alloc);
  p->have.reg_alloc = 1;
  return & p->reg_alloc;
}

void
BoxVMCode_Set_Alter_Name(BoxVMCode *p, const char *alter_name)
{
  if (p->have.installed) {
    MSG_FATAL("Too late to set the alternative name \"%s\"! "
              "The procedure has already been installed using \"%s\".",
              alter_name, p->alter_name);
    assert(0);
  }

  if (p->have.alter_name)
    Box_Mem_Free(p->alter_name);

  p->alter_name = Box_Mem_Strdup(alter_name);
  p->have.alter_name = 1;
}

char *
BoxVMCode_Get_Alter_Name(BoxVMCode *p)
{
  /* The description is just a help string to make the ASM more readable
   * (may disappear in the future). For now it is:
   *  - the type of the procedure, if this is known;
   *  - the name of the procedure, if this is known;
   *  - "|unknown|"
   */
  if (p->have.alter_name)
    return Box_Mem_Strdup(p->alter_name);
  else
    return Box_Mem_Strdup((p->have.proc_name) ? p->proc_name : "|unknown|");
}

BoxVMCallNum BoxVMCode_Install(BoxVMCode *p)
{
  BoxVM *vm = p->cmp->vm;
  BoxVMProcID proc_id, prev_proc_id;
  char *alter_name, *proc_name;

  if (p->style == BOXVMCODESTYLE_EXTERN) {
    MSG_FATAL("BoxVMCode_Install: Case BOXVMCODESTYLE_EXTERN "
              "not implemented!");
    return BOXVMCALLNUM_NONE;
  }

  proc_id = BoxVM_Proc_Code_New(p->cmp->vm);
  proc_name = (p->have.proc_name) ? p->proc_name : NULL;

  /* End the procedure. */
  My_Proc_End(p);

  if (!p->have.call_num) {
    p->call_num = BoxVM_Allocate_Call_Num(p->cmp->vm);
    p->have.call_num = 1;
  }

  if (p->call_num == BOXVMCALLNUM_NONE)
    return BOXVMCALLNUM_NONE;

  {
    BoxLIRNodeProc *proc = p->cmp->lir->target;
    BoxLIRNodeOp *op;
    int32_t offset;
    RegAlloc *ra = BoxVMCode_Get_RegAlloc(p);
    uint32_t num_temps[NUM_REGISTER_TYPES], num_vars[NUM_REGISTER_TYPES];
    BoxInt asm_code[NUM_REGISTER_TYPES] = {BOXOP_NEWC_II, BOXOP_NEWI_II,
                                           BOXOP_NEWR_II, BOXOP_NEWP_II,
                                           BOXOP_NEWO_II};
    int i;

    /* New procedure. */
    prev_proc_id = BoxVM_Proc_Target_Set(vm, proc_id);

    /* Write register allocation instructions. */
    RegAlloc_Get_Local_Nums(ra, num_temps, num_vars);
    for (i = 0; i < NUM_TYPES; i++) {
      BoxOpId op = (BoxOpId) asm_code[i];
      BoxInt nv = num_vars[i], nr = num_temps[i];
      if (nv || nr)
        BoxVM_Assemble(vm, op, BOXCONTCATEG_IMM, nv, BOXCONTCATEG_IMM, nr);
    }

    /* If this is a subprocedure then we need to get parent and child
     * from global registers and put them into local registers, so they can
     * be utilised later.
     */
    if (p->style == BOXVMCODESTYLE_SUB) {
      if (p->have.parent_reg)
        BoxVM_Assemble(vm, BOXOP_SHIFT_OO,
                       BOXOPARGFORM_LREG, p->reg_parent,
                       BOXOPARGFORM_GREG, (BoxInt) 1);
      if (p->have.child_reg)
        BoxVM_Assemble(vm, BOXOP_SHIFT_OO,
                       BOXOPARGFORM_LREG, p->reg_child,
                       BOXOPARGFORM_GREG, (BoxInt) 2);
    }

    /* First determine the instruction offsets. */
    for (op = proc->first_op, offset = 0; op; op = op->next) {
      BoxVMOp vm_op;
      vm_op.id = (BoxOpId) op->op_id;
      vm_op.desc = & vm->exec_table[op->op_id];
      vm_op.next = 0;
      vm_op.format = BOXVMOPFMT_UNDECIDED;
      vm_op.length = 0;
      vm_op.args_forms = 0;
      vm_op.data = NULL;

      op->offset = offset;

      switch (op->head.type) {
      case BOXLIRNODETYPE_OP:
        vm_op.num_args = 0;
        vm_op.has_data = BOXBOOL_FALSE;
        break;
      case BOXLIRNODETYPE_OP1:
        vm_op.num_args = 1;
        vm_op.args_forms = op->cats[0];
        vm_op.args[0] = (BoxInt) ((BoxLIRNodeOp1 *) op)->regs[0];
        vm_op.has_data = BOXBOOL_FALSE;
        break;
      case BOXLIRNODETYPE_OP2:
        vm_op.num_args = 2;
        vm_op.args_forms = op->cats[0] | (op->cats[1] << 2);
        vm_op.args[0] = (BoxInt) ((BoxLIRNodeOp2 *) op)->regs[0];
        vm_op.args[1] = (BoxInt) ((BoxLIRNodeOp2 *) op)->regs[1];
        vm_op.has_data = BOXBOOL_FALSE;
        break;
      case BOXLIRNODETYPE_OP_LD_CHAR:
        /* No real reason to use this. */
        abort();
        break;
      case BOXLIRNODETYPE_OP_LD_INT:
        vm_op.num_args = 1;
        vm_op.args_forms = op->cats[0];
        vm_op.args[0] = (BoxInt) ((BoxLIRNodeOpLdInt *) op)->regs[0];
        vm_op.has_data = BOXBOOL_TRUE;
        vm_op.data = (BoxVMWord *) & ((BoxLIRNodeOpLdInt *) op)->value;
        break;
      case BOXLIRNODETYPE_OP_LD_REAL:
        vm_op.num_args = 1;
        vm_op.args_forms = op->cats[0];
        vm_op.args[0] = (BoxInt) ((BoxLIRNodeOpLdChar *) op)->regs[0];
        vm_op.has_data = BOXBOOL_TRUE;
        vm_op.data = (BoxVMWord *) & ((BoxLIRNodeOpLdReal *) op)->value;
        break;
      case BOXLIRNODETYPE_OP_BRANCH:
        assert(((BoxLIRNodeOpBranch *) op)->target);
        vm_op.format = BOXVMOPFMT_LONG;
        vm_op.num_args = 1;
        vm_op.args_forms = BOXCONTCATEG_IMM;
        vm_op.has_data = BOXBOOL_FALSE;
        break;
      case BOXLIRNODETYPE_OP_LABEL:
        continue;
      default: printf("?\n"); break;
      }

      offset += BoxVMOp_Get_Length(& vm_op);
    }

    /* And now emit the byte code. */
    for (op = proc->first_op; op; op = op->next) {
      switch (op->head.type) {
      case BOXLIRNODETYPE_OP:
        BoxVM_Assemble(vm, (BoxOpId) op->op_id);
        break;
      case BOXLIRNODETYPE_OP1:
        BoxVM_Assemble(vm, (BoxOpId) op->op_id,
                       (BoxContCateg) op->cats[0],
                       (BoxInt) ((BoxLIRNodeOp1 *) op)->regs[0]);
        break;
      case BOXLIRNODETYPE_OP2:
        BoxVM_Assemble(vm, (BoxOpId) op->op_id,
                       (BoxContCateg) op->cats[0],
                       (BoxInt) ((BoxLIRNodeOp2 *) op)->regs[0],
                       (BoxContCateg) op->cats[1],
                       (BoxInt) ((BoxLIRNodeOp2 *) op)->regs[1]);
        break;
      case BOXLIRNODETYPE_OP_LD_CHAR:
        BoxVM_Assemble(vm, (BoxOpId) op->op_id,
                       (BoxContCateg) op->cats[0],
                       (BoxInt) ((BoxLIRNodeOpLdChar *) op)->regs[0],
                       BOXCONTCATEG_IMM,
                       ((BoxLIRNodeOpLdChar *) op)->value);
        break;
      case BOXLIRNODETYPE_OP_LD_INT:
        BoxVM_Assemble(vm, (BoxOpId) op->op_id,
                       (BoxContCateg) op->cats[0],
                       (BoxInt) ((BoxLIRNodeOpLdInt *) op)->regs[0],
                       BOXCONTCATEG_IMM,
                       ((BoxLIRNodeOpLdInt *) op)->value);
        break;
      case BOXLIRNODETYPE_OP_LD_REAL:
        BoxVM_Assemble(vm, (BoxOpId) op->op_id,
                       (BoxContCateg) op->cats[0],
                       (BoxInt) ((BoxLIRNodeOpLdReal *) op)->regs[0],
                       BOXCONTCATEG_IMM,
                       ((BoxLIRNodeOpLdReal *) op)->value);
        break;
      case BOXLIRNODETYPE_OP_BRANCH:
        {
          BoxLIRNodeOp *target = ((BoxLIRNodeOpBranch *) op)->target;
          BoxVM_Assemble_Long(vm, (BoxOpId) op->op_id,
                              BOXCONTCATEG_IMM,
                              (BoxInt) (target->offset - op->offset));
          break;
        }
      case BOXLIRNODETYPE_OP_LABEL:
        break;
      default:
        abort();
        break;
      }
    }

    BoxVM_Proc_Target_Set(vm, prev_proc_id);
  }

  if (!BoxVM_Install_Proc_Code(p->cmp->vm, p->call_num, proc_id)) {
    (void) BoxVM_Deallocate_Call_Num(p->cmp->vm, p->call_num);
    return BOXVMCALLNUM_NONE;
  }

  alter_name = BoxVMCode_Get_Alter_Name(p);
  (void) BoxVM_Set_Proc_Names(p->cmp->vm, p->call_num, proc_name, alter_name);
  Box_Mem_Free(alter_name);

  p->have.installed = 1;
  return p->call_num;
}
