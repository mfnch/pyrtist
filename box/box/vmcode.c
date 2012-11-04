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
#include "vmsym.h"
#include "vmproc.h"
#include "vmsymstuff.h"
#include "container.h"
#include "registers.h"

#include "vmcode.h"
#include "compiler.h"

/* Function called to begin for BOXVMCODETYPE_MAIN or BOXVMCODETYPE_SUB */
static void My_Proc_Begin(BoxVMCode *p) {
  BoxVMProcID proc_id, previous_target;
  BoxVMSymID sym_id;

  /* Assemble the newc, newi, ... instructions */
  proc_id = BoxVMCode_Get_ProcID(p);
  previous_target = BoxVM_Proc_Target_Set(p->cmp->vm, proc_id);
  ASSERT_TASK(BoxVMSym_Assemble_Proc_Head(p->cmp->vm, & sym_id));

  if (p->style == BOXVMCODESTYLE_SUB) {
    /* If this is a subprocedure then we need to get parent and child
     * from global registers and put them into local registers, so they can
     * be utilised later.
     */
    if (p->have.parent) {
      p->reg_parent = Reg_Occupy(& p->reg_alloc, BOXTYPEID_PTR);
      BoxVM_Assemble(p->cmp->vm, BOXOP_SHIFT_OO,
                     BOXOPCAT_LREG, p->reg_parent,
                     BOXOPCAT_GREG, (BoxInt) 1);
    }

    if (p->have.child) {
      p->reg_child = Reg_Occupy(& p->reg_alloc, BOXTYPEID_PTR);
      BoxVM_Assemble(p->cmp->vm, BOXOP_SHIFT_OO,
                     BOXOPCAT_LREG, p->reg_child,
                     BOXOPCAT_GREG, (BoxInt) 2);
    }
  }

  (void) BoxVM_Proc_Target_Set(p->cmp->vm, previous_target);

  p->have.head = 1;
  p->head_sym_id = sym_id;
}

/* Function called to end for BOXVMCODETYPE_MAIN or BOXVMCODETYPE_SUB */
static void My_Proc_End(BoxVMCode *p) {
  BoxVMProcID proc_id, previous_target;

  if (p->have.head) {
    RegAlloc *ra = BoxVMCode_Get_RegAlloc(p);
    BoxInt num_reg[NUM_TYPES], num_var[NUM_TYPES];

    /* Global registers are allocated only for main */
    if (p->style == BOXVMCODESTYLE_MAIN) {
      Reg_Get_Global_Nums(ra, num_reg, num_var);
      ASSERT_TASK( BoxVM_Alloc_Global_Regs(p->cmp->vm, num_var, num_reg) );
    }

    /* Local register should be allocated anyway */
    Reg_Get_Local_Nums(ra, num_reg, num_var);
    ASSERT_TASK(BoxVMSym_Def_Proc_Head(p->cmp->vm, p->head_sym_id,
                                       num_var, num_reg));

  }

  proc_id = BoxVMCode_Get_ProcID(p);
  previous_target = BoxVM_Proc_Target_Set(p->cmp->vm, proc_id);
  BoxVM_Assemble(p->cmp->vm, BOXOP_RET);
  (void) BoxVM_Proc_Target_Set(p->cmp->vm, previous_target);
}

void BoxVMCode_Init(BoxVMCode *p, BoxCmp *c, BoxVMCodeStyle style) {
  p->style = style;
  p->cmp = c;
  p->have.parent = 0;
  p->have.child = 0;
  p->have.reg_alloc = 0;
  p->have.proc_id = 0;
  p->have.proc_name = 0;
  p->have.alter_name = 0;
  p->have.call_num = 0;
  p->have.wrote_beg = 0;
  p->have.wrote_end = 0;
  p->have.installed = 0;
  p->have.head = 0;
  p->perm.proc_id = 1;
  p->beginning = NULL;
  p->ending = NULL;

  switch(style) {
  case BOXVMCODESTYLE_PLAIN:
    break;
  case BOXVMCODESTYLE_SUB:
  case BOXVMCODESTYLE_MAIN:
    p->beginning = My_Proc_Begin;
    p->ending = My_Proc_End;
    (void) BoxVMCode_Get_RegAlloc(p); /* Force initialisation
                                       of register allocator */
    break;
  case BOXVMCODESTYLE_EXTERN:
    p->perm.proc_id = 0; /* Deny permission to create proc_id */
    break;
  default:
    MSG_FATAL("BoxVMCode_Init: Invalid value for style (BoxVMCodeStyle).");
    assert(0);
  }

  p->have.callable = 0;
  p->callable = NULL;
}

void BoxVMCode_Finish(BoxVMCode *p) {
  if (p->have.callable)
    (void) BoxCallable_Unlink(p->callable);
  if (p->have.proc_name)
    BoxMem_Free(p->proc_name);
  if (p->have.alter_name)
    BoxMem_Free(p->alter_name);
  if (p->have.reg_alloc)
    Reg_Finish(& p->reg_alloc);
}

void BoxVMCode_Set_Callable(BoxVMCode *p, BoxCallable *cb) {
  if (p->have.callable)
    (void) BoxCallable_Unlink(cb);

  p->callable = BoxCallable_Link(cb);
  p->have.callable = 1;
}

BoxVMCode *BoxVMCode_Create(BoxCmp *c, BoxVMCodeStyle style) {
  BoxVMCode *p = BoxMem_Alloc(sizeof(BoxVMCode));
  if (p == NULL) return NULL;
  BoxVMCode_Init(p, c, style);
  return p;
}

void BoxVMCode_Destroy(BoxVMCode *p) {
  BoxVMCode_Finish(p);
  BoxMem_Free(p);
}

void BoxVMCode_Begin(BoxVMCode *p) {
  if (p->beginning != NULL && p->have.wrote_beg == 0) {
    p->beginning(p);
    p->have.wrote_beg = 1;
  }
}

void BoxVMCode_End(BoxVMCode *p) {
  if (p->ending != NULL && p->have.wrote_end == 0) {
    p->ending(p);
    p->have.wrote_end = 1;
  }
}

void BoxVMCode_Set_Prototype(BoxVMCode *p, int have_child, int have_parent) {
  if (p->have.wrote_beg) {
    MSG_WARNING("BoxVMCode_Set_Prototype: cannot change the prototype for "
                "the procedure: the procedure has been already generated!");

  } else if (p->style != BOXVMCODESTYLE_SUB) {
    MSG_WARNING("BoxVMCode_Set_Prototype: the prototype can be set only for "
                "BOXVMCODESTYLE_SUB.");
  }

  p->have.parent = have_parent;
  p->have.child = have_child;
}

BoxVMRegNum BoxVMCode_Get_Parent_Reg(BoxVMCode *p) {
  if (!p->have.wrote_beg)
    BoxVMCode_Begin(p);

  if (p->have.parent)
    return p->reg_parent;

  MSG_FATAL("BoxVMCode_Get_Parent_Reg: procedure does not have the parent.");
  assert(0);
  return 0;
}

BoxVMRegNum BoxVMCode_Get_Child_Reg(BoxVMCode *p) {
  if (!p->have.wrote_beg)
    BoxVMCode_Begin(p);

  if (p->have.child)
    return p->reg_child;

  MSG_FATAL("BoxVMCode_Get_Child_Reg: procedure does not have the child.");
  assert(0);
  return 0;
}

BoxVMCodeStyle BoxVMCode_Get_Style(BoxVMCode *p) {
  return p->style;
}

RegAlloc *BoxVMCode_Get_RegAlloc(BoxVMCode *p) {
  if (p->have.reg_alloc)
    return & p->reg_alloc;

  else {
    Reg_Init(& p->reg_alloc);
    p->have.reg_alloc = 1;
    return & p->reg_alloc;
  }
}

BoxVMProcID BoxVMCode_Get_ProcID(BoxVMCode *p) {
  if (!p->perm.proc_id) {
    MSG_FATAL("BoxVMCode_Get_ProcID: operation not permitted.");
    assert(0);
  }

  if (p->have.proc_id)
    return p->proc_id;

  else {
    p->have.proc_id = 1;
    p->proc_id = BoxVM_Proc_Code_New(p->cmp->vm);
    return p->proc_id;
  }
}

void BoxVMCode_Set_Alter_Name(BoxVMCode *p, const char *alter_name) {
  if (p->have.installed) {
    MSG_FATAL("Too late to set the alternative name \"%s\"! "
              "The procedure has already been installed using \"%s\".",
              alter_name, p->alter_name);
    assert(0);
  }

  if (p->have.alter_name)
    BoxMem_Free(p->alter_name);

  p->alter_name = BoxMem_Strdup(alter_name);
  p->have.alter_name = 1;
}

char *BoxVMCode_Get_Alter_Name(BoxVMCode *p) {
  /* The description is just a help string to make the ASM more readable
   * (may disappear in the future). For now it is:
   *  - the type of the procedure, if this is known;
   *  - the name of the procedure, if this is known;
   *  - "|unknown|"
   */
  if (p->have.alter_name)
    return BoxMem_Strdup(p->alter_name);

  else
    return BoxMem_Strdup((p->have.proc_name) ? p->proc_name : "|unknown|");
}

size_t BoxVMCode_Get_Code_Size(BoxVMCode *p) {
  if (p->have.proc_id)
    return BoxVM_Proc_Get_Size(p->cmp->vm, p->proc_id);

  else
    return 0;
}

BoxVMCallNum BoxVMCode_Get_Call_Num(BoxVMCode *p) {
  if (p->have.call_num)
    return p->call_num;

  else {
    p->call_num = BoxVM_Allocate_Call_Num(p->cmp->vm);
    p->have.call_num = 1;
    return p->call_num;
  }
}

BoxVMCallNum BoxVMCode_Install(BoxVMCode *p) {
  if (p->style == BOXVMCODESTYLE_EXTERN) {
    MSG_FATAL("BoxVMCode_Install: Case BOXVMCODESTYLE_EXTERN "
              "not implemented!");
    return BOXVMCALLNUM_NONE;

  } else if (!p->have.installed) {
    BoxVMProcID pn = BoxVMCode_Get_ProcID(p);
    char *alter_name,
         *proc_name  = (p->have.proc_name) ? p->proc_name : NULL;
    BoxVMCode_End(p); /* End the procedure, if not done explicitly */

    if (!p->have.call_num) {
      p->call_num = BoxVM_Allocate_Call_Num(p->cmp->vm);
      p->have.call_num = 1;
    }

    if (p->call_num == BOXVMCALLNUM_NONE)
      return BOXVMCALLNUM_NONE;

    if (!BoxVM_Install_Proc_Code(p->cmp->vm, p->call_num, pn)) {
      (void) BoxVM_Deallocate_Call_Num(p->cmp->vm, p->call_num);
      return BOXVMCALLNUM_NONE;
    }

    alter_name = BoxVMCode_Get_Alter_Name(p);
    (void) BoxVM_Set_Proc_Names(p->cmp->vm, p->call_num,
                                proc_name, alter_name);
    BoxMem_Free(alter_name);

    p->have.installed = 1;
    return p->call_num;

  } else {
    assert(p->have.call_num);
    return p->call_num;
  }
}

void BoxVMCode_Raw_VA_Assemble(BoxVMCode *p, BoxOp op, va_list ap) {
  BoxVMProcID proc_id, previous_target;
  BoxVMCode_Begin(p); /* Begin the procedure, if not done explicitly */
  proc_id = BoxVMCode_Get_ProcID(p);
  previous_target = BoxVM_Proc_Target_Set(p->cmp->vm, proc_id);
  BoxVM_VA_Assemble(p->cmp->vm, op, ap);
  (void) BoxVM_Proc_Target_Set(p->cmp->vm, previous_target);
}

void BoxVMCode_Raw_Assemble(BoxVMCode *p, BoxOp op, ...) {
  va_list ap;
  va_start(ap, op);
  BoxVMCode_Raw_VA_Assemble(p, op, ap);
  va_end(ap);
}

void BoxVMCode_Assemble_Call(BoxVMCode *code, BoxVMCallNum call_num) {
  BoxVMProcID proc_id, previous_target;
  BoxVMCode_Begin(code); /* Begin the procedure, if not done explicitly */
  proc_id = BoxVMCode_Get_ProcID(code);
  previous_target = BoxVM_Proc_Target_Set(code->cmp->vm, proc_id);
  BoxVM_Assemble(code->cmp->vm, BOXOP_CALL_I, BOXCONTCATEG_IMM, call_num);
  (void) BoxVM_Proc_Target_Set(code->cmp->vm, previous_target);
}

/** Internal function used by My_Unsafe_Assemble to assemble operation
 * involving pointer access (i.e. involving args like i[ro5+16]).
 * This function is used when assembling things like:
 *
 *   mov ro0, ro3  <-- this line is what we want to emit
 *   mov i[ro0+8], 45
 */
static void My_Prepare_Ptr_Access(BoxVMCode *p, const BoxCont *c) {
  BoxInt ptr_reg = c->value.ptr.reg;
  if (c->categ == BOXCONTCATEG_PTR && (c->value.ptr.is_greg || ptr_reg != 0)) {
    BoxInt categ = c->value.ptr.is_greg ? BOXCONTCATEG_GREG : BOXCONTCATEG_LREG;
    BoxVMCode_Raw_Assemble(p, BOXOP_MOV_OO, BOXOPCAT_LREG, (BoxInt) 0,
                         categ, ptr_reg);
  }
}

/** Internal function used by My_Unsafe_Assemble.
 * Used to get the integer value of a container whose value is an integer.
 * Containers having this property are: registers (both local and global)
 * pointers and immediate integers/chars.
 */
static BoxInt My_Int_Val_From_Cont(const BoxCont *c) {
  if (c->categ == BOXCONTCATEG_LREG || c->categ == BOXCONTCATEG_GREG)
    return c->value.reg;

  else if (c->categ == BOXCONTCATEG_PTR)
    return c->value.ptr.offset;

  else {
    if (c->type == BOXCONTTYPE_CHAR)
      return (BoxInt) c->value.imm.box_char;
    else if (c->type == BOXCONTTYPE_INT)
      return c->value.imm.box_int;
    assert(0);
  }
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
static void My_Unsafe_Assemble(BoxVMCode *p, BoxOp op,
                               int num_args, const BoxCont **cs) {
  assert(num_args >= 0 && num_args <= 2);
  if (num_args == 0) {
     BoxVMCode_Raw_Assemble(p, op);
     return;

  } else if (num_args == 1) {
    const BoxCont *c1 = cs[0];
    My_Prepare_Ptr_Access(p, c1);
    if (c1->categ != BOXCONTCATEG_IMM) {
      BoxInt c1_int_val = My_Int_Val_From_Cont(c1);
      BoxVMCode_Raw_Assemble(p, op, c1->categ, c1_int_val);
      return;

    } else {
      switch(c1->type) {
      case BOXCONTTYPE_CHAR:
        BoxVMCode_Raw_Assemble(p, op, c1->categ, c1->value.imm.box_char);
        return;
      case BOXCONTTYPE_INT:
        BoxVMCode_Raw_Assemble(p, op, c1->categ, c1->value.imm.box_int);
        return;
      case BOXCONTTYPE_REAL:
        BoxVMCode_Raw_Assemble(p, op, c1->categ, c1->value.imm.box_real);
        return;
      case BOXCONTTYPE_POINT:
        BoxVMCode_Raw_Assemble(p, op, c1->categ, c1->value.imm.box_point);
        return;
      default:
        MSG_FATAL("My_Unsafe_Assemble: invalid type for immediate.");
        assert(0);
      }
    }

  } else {
    const BoxCont *c1 = cs[0], *c2 = cs[1];

    My_Prepare_Ptr_Access(p, c1);
    My_Prepare_Ptr_Access(p, c2);

    if (c2->categ != BOXCONTCATEG_IMM) {
      BoxInt c1_int_val = My_Int_Val_From_Cont(c1),
             c2_int_val = My_Int_Val_From_Cont(c2);

      BoxVMCode_Raw_Assemble(p, op, c1->categ, c1_int_val,
                           c2->categ, c2_int_val);
      return;

    } else {
      BoxInt c1_int_val = (c1->categ == BOXCONTCATEG_PTR) ?
                          c1->value.ptr.offset : c1->value.reg;
      switch(c1->type) {
      case BOXCONTTYPE_CHAR:
        BoxVMCode_Raw_Assemble(p, op, c1->categ, c1_int_val,
                             c2->categ, c2->value.imm.box_char);
        return;
      case BOXCONTTYPE_INT:
        BoxVMCode_Raw_Assemble(p, op, c1->categ, c1_int_val,
                             c2->categ, c2->value.imm.box_int);
        return;
      case BOXCONTTYPE_REAL:
        BoxVMCode_Raw_Assemble(p, op, c1->categ, c1_int_val,
                             c2->categ, c2->value.imm.box_real);
        return;
      case BOXCONTTYPE_POINT:
        BoxVMCode_Raw_Assemble(p, op, c1->categ, c1_int_val,
                             c2->categ, c2->value.imm.box_point);
        return;
      default:
        MSG_FATAL("My_Unsafe_Assemble: invalid type for immediate.");
        assert(0);
      }
    }

  }
}

static void My_Gather_Implicit_Input_Regs(BoxVMCode *p,
                                          int num_regs, BoxOpReg *regs,
                                          const BoxCont **args) {
  int i;
  for(i = 0; i < num_regs; i++) {
    BoxOpReg *reg = & regs[i];
    if (reg->kind == 'r') {
      if (reg->io == 'i' || reg->io == 'b') {
        BoxCont dest;
        dest.type = BoxContType_From_Char(reg->type);
        dest.categ = BOXCONTCATEG_LREG;
        dest.value.reg = reg->num;
        BoxVMCode_Assemble(p, BOXGOP_MOV, 2, & dest, args[i]);
      }
    }
  }
}

static void My_Scatter_Implicit_Input_Regs(BoxVMCode *p,
                                           int num_regs, BoxOpReg *regs,
                                           const BoxCont **args) {
  int i;
  for(i = 0; i < num_regs; i++) {
    BoxOpReg *reg = & regs[i];
    if (reg->kind == 'r') {
      if (reg->io == 'o' || reg->io == 'b') {
        BoxTypeId t = BoxContType_From_Char(reg->type);
        BoxGOp gop = (t == BOXTYPEID_PTR) ? BOXGOP_SHIFT : BOXGOP_MOV;
        BoxCont src;
        src.type = t;
        src.categ = BOXCONTCATEG_LREG;
        src.value.reg = reg->num;
        BoxVMCode_Assemble(p, gop, 2, args[i], & src);
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
} FoundOP;

int My_ContTypes_Match(BoxContType t1, BoxContType t2) {
  return    ((t1 == BOXCONTTYPE_OBJ) ? BOXCONTTYPE_PTR : t1)
         == ((t2 == BOXCONTTYPE_OBJ) ? BOXCONTTYPE_PTR : t2);
}

static BoxOpInfo *My_Find_Op(BoxVMCode *p, FoundOP *info, BoxGOp g_op,
                             int num_args, const BoxCont **args,
                             int ignore_signature) {
  BoxOpInfo *oi;
  int num_exp_args, ro0_arg_conflict, ro0_input_conflict;

  /* Search for operations whose argument number and type match */
  oi = BoxVM_Get_Op_Info(p->cmp->vm, g_op);
  for(; oi != NULL; oi = oi->next) {
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

void BoxVMCode_VA_Assemble(BoxVMCode *p, BoxGOp g_op, int num_args, va_list ap) {
  const BoxCont *args[BOXOP_MAX_NUM_ARGS];
  BoxOpInfo *oi;
  FoundOP op;
  int i;

  if (num_args > BOXOP_MAX_NUM_ARGS) {
    MSG_FATAL("BoxVMCode_Assemble: the given number of arguments is too high.");
    assert(0);
  }

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
   * as it happens to be there is no operation which has more than one
   * output register).
   *   Example: jc   ri0  (which doesn't make sense anyway)
   *            araddr 0, ri0
   * These seem to be the only operations presenting such conflicts.
   * It may then make sense not to check for it.
   * NOTE: we assume the user does not fiddle with 0 registers: we do not
   *  catch things like "mov o[ro0+8], ro0"
   * NOT DOING THIS FOR NOW!
   */

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
      My_Unsafe_Assemble(p, oi->opcode, op.num_exp_args, op.exp_args);

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
        My_Unsafe_Assemble(p, oi->opcode, 2, op.exp_args);

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
        My_Unsafe_Assemble(p, BOXOP_PUSH_O, 1, cs);
        BoxVMCode_Assemble(p, BOXGOP_MOV, 2, & ro1, op.exp_args[1]);
        op.exp_args[1] = & ro1;
        My_Unsafe_Assemble(p, oi->opcode, 2, op.exp_args);
        My_Unsafe_Assemble(p, BOXOP_POP_O, 1, cs);
      }
    }

  } else {
    if (op.ro0_arg_conflict == 0)
      My_Unsafe_Assemble(p, oi->opcode, op.num_exp_args, op.exp_args);

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

void BoxVMCode_Assemble(BoxVMCode *p, BoxGOp g_op, int num_args, ...) {
  va_list ap;
  va_start(ap, num_args);
  BoxVMCode_VA_Assemble(p, g_op, num_args, ap);
  va_end(ap);
}

BoxVMSymID BoxVMCode_Jump_Label_New(BoxVMCode *p) {
  return BoxVMSym_New_Label(p->cmp->vm);
}

BoxVMSymID BoxVMCode_Jump_Label_Here(BoxVMCode *p) {
 BoxVMSymID jl = BoxVMCode_Jump_Label_New(p);
 BoxVMCode_Jump_Label_Define(p, jl);
 return jl;
}

/* Abbreviations for a few procedures sharing the same structure */
#define MY_ASSEMBLE_BEGIN() \
  BoxVMProcID proc_id, previous_target; \
  BoxVMCode_Begin(p); \
  proc_id = BoxVMCode_Get_ProcID(p); \
  previous_target = BoxVM_Proc_Target_Set(p->cmp->vm, proc_id)

#define MY_ASSEMBLE_END() \
  (void) BoxVM_Proc_Target_Set(p->cmp->vm, previous_target)

void BoxVMCode_Jump_Label_Define(BoxVMCode *p, BoxVMSymID jl) {
  MY_ASSEMBLE_BEGIN();
  ASSERT_TASK( BoxVMSym_Def_Label_Here(p->cmp->vm, jl) );
  MY_ASSEMBLE_END();
}

void BoxVMCode_Jump_Label_Release(BoxVMCode *p, BoxVMSymID jl) {
  ASSERT_TASK( BoxVMSym_Release_Label(p->cmp->vm, jl) );
}

void BoxVMCode_Assemble_Jump(BoxVMCode *p, BoxVMSymID jl) {
  MY_ASSEMBLE_BEGIN();
  ASSERT_TASK( BoxVMSym_Jmp(p->cmp->vm, jl) );
  MY_ASSEMBLE_END();
}

void BoxVMCode_Assemble_CJump(BoxVMCode *p, BoxVMSymID jl, BoxCont *cont) {
  BoxCont ri0_cont;
  MY_ASSEMBLE_BEGIN();
  BoxCont_Set(& ri0_cont, "ri", 0);
  BoxVMCode_Assemble(p, BOXGOP_MOV, 2, & ri0_cont, cont);
  ASSERT_TASK( BoxVMSym_Jc(p->cmp->vm, jl) );
  MY_ASSEMBLE_END();
}

void BoxVMCode_Associate_Source(BoxVMCode *p, BoxSrcPos *src_pos) {
  BoxVM_Proc_Associate_Source(p->cmp->vm, BoxVMCode_Get_ProcID(p), src_pos);
}
