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


BoxLIRNodeProc *
My_Get_Proc(BoxVMCode *p)
{
  if (p->proc)
    return p->proc;
  p->proc = BoxLIR_Append_Proc(& p->cmp->lir);
  return p->proc;
}

/* Function called to end for BOXVMCODETYPE_MAIN or BOXVMCODETYPE_SUB */
static void
My_Proc_End(BoxVMCode *p)
{
  RegAlloc *ra = BoxVMCode_Get_RegAlloc(p);
  BoxInt num_reg[NUM_TYPES], num_var[NUM_TYPES];

  /* Global registers are allocated only for main */
  if (p->style == BOXVMCODESTYLE_MAIN) {
    Reg_Get_Global_Nums(ra, num_reg, num_var);
    ASSERT_TASK( BoxVM_Alloc_Global_Regs(p->cmp->vm, num_var, num_reg) );
  }

  (void) BoxLIR_Set_Target_Proc(& p->cmp->lir, My_Get_Proc(p));
  BoxLIR_Append_Op(& p->cmp->lir, BOXOP_RET);
}

void
BoxVMCode_Init(BoxVMCode *p, BoxCmp *c, BoxVMCodeStyle style)
{
  p->style = style;
  p->cmp = c;
  p->proc = NULL;
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
    assert(0);
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

  else {
    Reg_Init(& p->reg_alloc);
    p->have.reg_alloc = 1;
    return & p->reg_alloc;
  }
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

  if (p->proc) {
    BoxLIRNodeOp *op;
    int32_t offset;
    RegAlloc *ra = BoxVMCode_Get_RegAlloc(p);
    BoxInt num_regs[NUM_TYPES], num_vars[NUM_TYPES];
    BoxInt asm_code[NUM_TYPES] = {BOXOP_NEWC_II, BOXOP_NEWI_II,
                                    BOXOP_NEWR_II, BOXOP_NEWP_II,
                                    BOXOP_NEWO_II};
    int i;

    /* New procedure. */
    prev_proc_id = BoxVM_Proc_Target_Set(vm, proc_id);

    /* Write register allocation instructions. */
    Reg_Get_Local_Nums(ra, num_regs, num_vars);
    for (i = 0; i < NUM_TYPES; i++) {
      BoxOpId op = asm_code[i];
      BoxInt nv = num_vars[i], nr = num_regs[i];
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
    for (op = p->proc->first_op, offset = 0; op; op = op->next) {
      BoxVMOp vm_op;
      vm_op.id = op->op_id;
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

    for (op = p->proc->first_op; op; op = op->next) {
      switch (op->head.type) {
      case BOXLIRNODETYPE_OP:
        BoxVM_Assemble(vm, op->op_id);
        break;
      case BOXLIRNODETYPE_OP1:
        BoxVM_Assemble(vm, op->op_id,
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
  } else {
    MSG_FATAL("Procedure not created");
    abort();
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

void
BoxVMCode_Assemble_Call(BoxVMCode *code, BoxVMCallNum call_num)
{
  (void) BoxLIR_Set_Target_Proc(& code->cmp->lir, My_Get_Proc(code));
  BoxLIR_Append_Op1(& code->cmp->lir,
                    BOXOP_CALL_I, BOXCONTCATEG_IMM, call_num);
}

/** Internal function used by My_Unsafe_Assemble to assemble operation
 * involving pointer access (i.e. involving args like i[ro5+16]).
 * This function is used when assembling things like:
 *
 *   mov ro0, ro3  <-- this line is what we want to emit
 *   mov i[ro0+8], 45
 */
static void
My_Prepare_Ptr_Access(BoxLIR *lir, const BoxCont *c)
{
  BoxInt ptr_reg = c->value.ptr.reg;
  if (c->categ == BOXCONTCATEG_PTR && (c->value.ptr.is_greg || ptr_reg != 0)) {
    BoxInt categ = c->value.ptr.is_greg ? BOXCONTCATEG_GREG : BOXCONTCATEG_LREG;
    BoxLIR_Append_Op2(lir, BOXOP_MOV_OO, BOXOPARGFORM_LREG, 0, categ, ptr_reg);
  }
}

/** Internal function used by My_Unsafe_Assemble.
 * Used to get the integer value of a container whose value is an integer.
 * Containers having this property are: registers (both local and global)
 * pointers and immediate integers/chars.
 */
static BoxInt
My_Int_Val_From_Cont(const BoxCont *c)
{
  if (c->categ == BOXCONTCATEG_LREG || c->categ == BOXCONTCATEG_GREG)
    return c->value.reg;

  if (c->categ == BOXCONTCATEG_PTR)
    return c->value.ptr.offset;

  if (c->type == BOXCONTTYPE_CHAR)
    return (BoxInt) c->value.imm.box_char;

  if (c->type == BOXCONTTYPE_INT)
    return c->value.imm.box_int;
  abort();
}

/** Internal function used by BoxVMCode_Assemble and co.
 * Similar to BoxVMCode_Assemble, but works only in certain particular cases
 * and does not check these are actually satisfied. In particular this
 * function assumes that:
 *  - the operands pointed by cs[0] (and cs[1], if num_args == 2)
 *    are not both pointers, such as:
 *
 *      cs[0] -> i[ro5+8]       cs[1] -> i[ro6+16]
 *
 *  - there are no immediates in the operands (such as cs[1] -> 1.2345).
 * Moreover this function does not deal with implicit input/output registers.
 */
static void
My_Unsafe_Assemble(BoxLIR *lir, BoxOpId op, int num_args, const BoxCont **cs)
{
  assert(num_args >= 0 && num_args <= 2);
  if (num_args == 0) {
    BoxLIR_Append_Op(lir, op);
    return;
  } else if (num_args == 1) {
    const BoxCont *c1 = cs[0];
    My_Prepare_Ptr_Access(lir, c1);
    if (c1->categ != BOXCONTCATEG_IMM) {
      BoxInt c1_int_val = My_Int_Val_From_Cont(c1);
      BoxLIR_Append_Op1(lir, op, c1->categ, c1_int_val);
      return;

    } else {
      switch(c1->type) {
      case BOXCONTTYPE_CHAR:
        BoxLIR_Append_Op1(lir, op, c1->categ, c1->value.imm.box_char);
        return;
      case BOXCONTTYPE_INT:
        BoxLIR_Append_Op1(lir, op, c1->categ, c1->value.imm.box_int);
        return;
      default:
        MSG_FATAL("My_Unsafe_Assemble: invalid type for immediate.");
        assert(0);
      }
    }

  } else {
    const BoxCont *c1 = cs[0], *c2 = cs[1];

    My_Prepare_Ptr_Access(lir, c1);
    My_Prepare_Ptr_Access(lir, c2);

    if (c2->categ != BOXCONTCATEG_IMM) {
      BoxInt c1_int_val = My_Int_Val_From_Cont(c1),
             c2_int_val = My_Int_Val_From_Cont(c2);
      BoxLIR_Append_Op2(lir, op, c1->categ, c1_int_val, c2->categ, c2_int_val);
      return;

    } else {
      BoxInt c1_int_val = (c1->categ == BOXCONTCATEG_PTR ?
                           c1->value.ptr.offset : c1->value.reg);
      switch(c1->type) {
      case BOXCONTTYPE_CHAR:
        BoxLIR_Append_Op2(lir, op, c1->categ, c1_int_val,
                          c2->categ, c2->value.imm.box_char);
        return;
      case BOXCONTTYPE_INT:
        BoxLIR_Append_Op2(lir, op, c1->categ, c1_int_val,
                          c2->categ, c2->value.imm.box_int);
        return;
      case BOXCONTTYPE_REAL:
        BoxLIR_Append_Op_Ld_Real(lir, op, c1->categ, c1_int_val,
                                 c2->value.imm.box_real);
        return;
      default:
        MSG_FATAL("My_Unsafe_Assemble: invalid type for immediate.");
        assert(0);
      }
    }
  }
}

static void
My_Gather_Implicit_Input_Regs(BoxVMCode *p, int num_regs, BoxOpReg *regs,
                              const BoxCont **args)
{
  int i;
  for(i = 0; i < num_regs; i++) {
    BoxOpReg *reg = & regs[i];
    if (reg->kind == 'r') {
      if (reg->io == 'i' || reg->io == 'b') {
        const BoxCont *src = args[i];
        BoxCont dst;

        dst.type = BoxContType_From_Char(reg->type);
        dst.categ = BOXCONTCATEG_LREG;
        dst.value.reg = reg->num;

        if (!(dst.categ == src->categ && dst.value.reg == src->value.reg))
          BoxVMCode_Assemble(p, BOXGOP_MOV, 2, & dst, src);
      }
    }
  }
}

static void
My_Scatter_Implicit_Input_Regs(BoxVMCode *p, int num_regs, BoxOpReg *regs,
                               const BoxCont **args)
{
  int i;
  for(i = 0; i < num_regs; i++) {
    BoxOpReg *reg = & regs[i];
    if (reg->kind == 'r') {
      if (reg->io == 'o' || reg->io == 'b') {
        BoxTypeId t = BoxContType_From_Char(reg->type);
        BoxGOp gop = (t == BOXTYPEID_PTR) ? BOXGOP_SHIFT : BOXGOP_MOV;
        const BoxCont *dst = args[i];
        BoxCont src;

        src.type = t;
        src.categ = BOXCONTCATEG_LREG;
        src.value.reg = reg->num;

        if (!(dst->categ == src.categ && dst->value.reg == src.value.reg))
          BoxVMCode_Assemble(p, gop, 2, dst, & src);
      }
    }
  }
}

typedef struct {
  BoxOpInfo     *oi;
  int           num_exp_args,
                ro0_arg_conflict,
                ro0_input_conflict;
  const BoxCont *exp_args[2];
  BoxCont       aux_arg;
} MyFoundOp;

int
My_ContTypes_Match(BoxContType t1, BoxContType t2)
{
  return    ((t1 == BOXCONTTYPE_OBJ) ? BOXCONTTYPE_PTR : t1)
         == ((t2 == BOXCONTTYPE_OBJ) ? BOXCONTTYPE_PTR : t2);
}

static BoxOpInfo *
My_Find_Op(BoxVMCode *p, MyFoundOp *info, BoxGOp g_op,
           int num_args, const BoxCont **args, int ignore_signature)
{
  BoxOpInfo *oi;
  int num_exp_args, ro0_arg_conflict, ro0_input_conflict;

  /* Search for operations whose argument number and type match */
  for(oi = BoxVM_Get_Op_Info(p->cmp->vm, g_op); oi; oi = oi->next) {
    if (oi->num_regs == num_args) {
      /* Consider only operations with matching arg number */
      BoxOpSignature signature;
      int i;

      /* Now let's check for matching arg type and signature */
      num_exp_args = 0;
      ro0_arg_conflict = 0;
      ro0_input_conflict = 0;
      signature = BOXOPSIGNATURE_NONE;
      for(i = 0; i < num_args; i++) {
        BoxOpReg *reg = & oi->regs[i];
        BoxContType t = BoxContType_From_Char(reg->type);
        if (!My_ContTypes_Match(t, args[i]->type))
          break; /* Exit if type for this arg does not match */

        /* In the meanwhile we compute the signature: if types of args match
         * then we'll have to check also for matching signature!
         */
        if (reg->kind == 'a') {
          /* Explicit register */
          int is_immediate;

          assert(num_exp_args < 2);

          info->exp_args[num_exp_args] = args[i];

          /* We also start to take note about possible register conflicts */
          ro0_arg_conflict += (args[i]->categ == BOXCONTCATEG_PTR);

          is_immediate = (args[i]->categ == BOXCONTCATEG_IMM &&
                          args[i]->type != BOXCONTTYPE_INT &&
                          args[i]->type != BOXCONTTYPE_CHAR);
          if (num_exp_args == 0)
            signature = (is_immediate) ?
                        BOXOPSIGNATURE_IMM : BOXOPSIGNATURE_ANY;
          else
            signature = (is_immediate) ?
                        BOXOPSIGNATURE_ANY_IMM : BOXOPSIGNATURE_ANY_ANY;
          ++num_exp_args;

        } else {
          /* Implicit register */
          assert(reg->kind == 'r');
          if (reg->type == 'o' && reg->num == 0)
            ro0_input_conflict |= (reg->io == 'i' || reg->io == 'b');
        }
      }

      if (i >= num_args && (signature == oi->signature || ignore_signature)) {
        info->oi = oi;
        info->num_exp_args = num_exp_args;
        info->ro0_arg_conflict = ro0_arg_conflict;
        info->ro0_input_conflict = ro0_input_conflict;
        return oi;
      }
    }
  }

  info->oi = NULL;
  return NULL;
}

static void
My_Load_Immediates(BoxLIR *lir, int num_regs, BoxOpReg *regs,
                   const BoxCont **args, MyFoundOp *op)
{
  int i, j;
  for(i = 0; i < num_regs; i++) {
    const BoxCont *src = args[i];
    BoxOpReg *reg = & regs[i];
    if (src->categ == BOXCONTCATEG_IMM && (reg->io == 'i' || reg->io == 'b')) {
      if (src->type == BOXCONTTYPE_INT && sizeof(BoxInt) > sizeof(int32_t)) {
        BoxInt value = src->value.imm.box_int;
        if (value != (BoxInt) ((int32_t) value)) {
          BoxCont *ri0 = & op->aux_arg;

          ri0->categ = BOXCONTCATEG_LREG;
          ri0->type = BOXCONTTYPE_INT;
          ri0->value.reg = 0;
          BoxLIR_Append_Op_Ld_Int(lir, BOXOP_MOV_Iimm,
                                  ri0->categ, ri0->value.reg, value);

          /* Update references to the container. */
          args[i] = ri0;
          for (j = 0; j < op->num_exp_args; j++)
            if (op->exp_args[j] == src)
              op->exp_args[j] = ri0;
        }
      }
    }
  }
}

void
BoxVMCode_VA_Assemble(BoxVMCode *p, BoxGOp g_op, int num_args, va_list ap)
{
  BoxLIR *lir = & p->cmp->lir;
  const BoxCont *args[BOXOP_MAX_NUM_ARGS];
  BoxOpInfo *oi;
  MyFoundOp op;
  int i;

  if (num_args > BOXOP_MAX_NUM_ARGS) {
    MSG_FATAL("BoxVMCode_Assemble: the given number of arguments is too high.");
    assert(0);
  }

  (void) BoxLIR_Set_Target_Proc(lir, My_Get_Proc(p));

  for(i = 0; i < num_args; i++)
    args[i] = va_arg(ap, BoxCont *);

  /* Search for operations whose argument number and type match */
  oi = My_Find_Op(p, & op, g_op, num_args, args, /*ignore_signature*/ 0);

  if (oi == NULL) {
    /* Try to search again ignoring the signature: it may be that an immediate
     * value is being passed and that the operation doesn't allow immediate
     * values. If this is the case we should move the immediate to a temporary
     * register and retry
     */
    oi = My_Find_Op(p, & op, g_op, num_args, args, /*ignore_signature*/ 1);
    if (oi == NULL) {
      int i;
      char *sep = "";
      fprintf(stderr, "BoxVMCode_Assemble: cannot find a matching operation.\n");
      fprintf(stderr, "Possible signatures are:\n");
      BoxOpInfo_Print(stderr, BoxVM_Get_Op_Info(p->cmp->vm, g_op));
      fprintf(stderr, "Got the following %d arguments: ", num_args);
      for(i = 0; i < num_args; i++) {
        fprintf(stderr, "%s%s", sep, BoxCont_To_String(args[i]));
        sep = ", ";
      }
      fprintf(stderr, "\n");
      MSG_FATAL("BoxVMCode_Assemble: aborting!");
      assert(0);

    } else {
      /* This means that the first operation search failed because the last
       * argument is an immediate
       */
      BoxCont r0;
      r0.type = BoxContType_From_Char(oi->arg_type);
      r0.categ = BOXCONTCATEG_LREG;
      r0.value.reg = 0;
      if (op.num_exp_args == 2) {
        BoxVMCode_Assemble(p, BOXGOP_MOV, 2, & r0, op.exp_args[1]);
        BoxVMCode_Assemble(p, g_op, 2, op.exp_args[0], & r0);

      } else {
        assert(op.num_exp_args == 1);
        BoxVMCode_Assemble(p, BOXGOP_MOV, 2, & r0, op.exp_args[1]);
        BoxVMCode_Assemble(p, g_op, 1, & r0);
      }
      return;
    }
  }

  /* Make sure there are no conflicts between implicit and explicit
   * registers. A conflict happens when an implicit input register appears
   * also as explicit register or when an implicit output register appears
   * also as explicit register (this case is practically excluded for now,
   * as there is no operation which has more than one output register).
   *   Example: jc   ri0  (which doesn't make sense anyway)
   * This seems to be the only operation presenting such conflicts.
   * It may then make sense not to check for it.
   * NOTE: we assume the user does not fiddle with 0 registers: we do not
   *  catch things like "mov o[ro0+8], ro0"
   * NOT DOING THIS FOR NOW!
   */

  /* Use separate load for immediates, if they are out of the admissible
   * range.
   */
  My_Load_Immediates(lir, num_args, oi->regs, args, & op);

  /* Setting all the implicit input registers from the given arguments */
  My_Gather_Implicit_Input_Regs(p, num_args, oi->regs, args);

  if (!op.ro0_input_conflict) {
    /* ro0 is not an implicit input register */
    if (op.ro0_arg_conflict < 2) {
      /* The arguments of the operation are not both pointers.
       * A direct assembly is then possible:
       *
       *   add ri5, ri6
       *
       * or:
       *
       *   mov ro0, ro5
       *   add ri5, i[ro0+32]
       */
      My_Unsafe_Assemble(lir, oi->opcode, op.num_exp_args, op.exp_args);

    } else {
      /* The argument of the operation are both pointers: we then have to
       * be careful on how the ro0 register is used!
       */
      assert(op.num_exp_args == 2);
      if (oi->arg_type != 'o') {
        /* we can do something like:
         *
         *   mov ro0, ro5
         *   mov ri0, i[ro0+8]
         *   mov ro0, ro6
         *   add i[ro0+16], ri0
         */
        BoxCont r0;
        r0.type = BoxContType_From_Char(oi->arg_type);
        r0.categ = BOXCONTCATEG_LREG;
        r0.value.reg = 0;
        /* Note exp_arg[1] is never an output register, so we can put it
         * into a temporary register (the Box VM operations never have more
         * than one output argument and when they have just one, then it is
         * always exp_arg[0] and not exp_arg[1]).
         */
        BoxVMCode_Assemble(p, BOXGOP_MOV, 2, & r0, op.exp_args[1]);
        op.exp_args[1] = & r0;
        My_Unsafe_Assemble(lir, oi->opcode, 2, op.exp_args);

      } else {
        /* we can do something like:
         *
         *   push ro1
         *   mov ro0, ro5
         *   mov ro1, o[ro0+8]
         *   mov ro0, ro6
         *   pop ro1
         */
        BoxCont ro1;
        const BoxCont *cs[1];
        ro1.type = BOXCONTTYPE_PTR;
        ro1.categ = BOXCONTCATEG_LREG;
        ro1.value.reg = 1;
        cs[0] = & ro1;
        My_Unsafe_Assemble(lir, BOXOP_PUSH_O, 1, cs);
        BoxVMCode_Assemble(p, BOXGOP_MOV, 2, & ro1, op.exp_args[1]);
        op.exp_args[1] = & ro1;
        My_Unsafe_Assemble(lir, oi->opcode, 2, op.exp_args);
        My_Unsafe_Assemble(lir, BOXOP_POP_O, 1, cs);
      }
    }

  } else {
    if (op.ro0_arg_conflict == 0)
      My_Unsafe_Assemble(lir, oi->opcode, op.num_exp_args, op.exp_args);

    else if (op.ro0_arg_conflict == 1) {
      /* ro0 is an implicit input register */
      MSG_FATAL("ro0 is an implicit input register: not implemented yet!");
      assert(0);

    } else {
      MSG_FATAL("too many register conflicts: not implemented yet!");
      assert(0);
    }
  }

  /* Distributing all the implicit output registers to the corresponding
   * arguments
   */
  My_Scatter_Implicit_Input_Regs(p, num_args, oi->regs, args);
}

void
BoxVMCode_Assemble(BoxVMCode *p, BoxGOp g_op, int num_args, ...)
{
  va_list ap;
  va_start(ap, num_args);
  BoxVMCode_VA_Assemble(p, g_op, num_args, ap);
  va_end(ap);
}
