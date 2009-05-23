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
#include "virtmach.h"
#include "vmsym.h"
#include "vmproc.h"
#include "vmsymstuff.h"
#include "container.h"
#include "typesys.h"

#include "cmpproc.h"
#include "compiler.h"

void CmpProc_Init(CmpProc *p, BoxCmp *c) {
  p->cmp = c;
  p->have.sym = 0;
  p->have.proc_id = 0;
  p->have.proc_name = 0;
  p->have.call_num = 0;
  p->have.type = 0;
}

void CmpProc_Finish(CmpProc *p) {
  if (p->have.proc_name)
    BoxMem_Free(p->proc_name);
}

CmpProc *CmpProc_New(BoxCmp *c) {
  CmpProc *p = BoxMem_Alloc(sizeof(CmpProc));
  if (p == NULL) return NULL;
  CmpProc_Init(p, c);
  return p;
}

void CmpProc_Destroy(CmpProc *p) {
  CmpProc_Finish(p);
  BoxMem_Free(p);
}

BoxVMSymID CmpProc_Get_Sym(CmpProc *p) {
  if (p->have.sym)
    return p->sym;

  else {
    p->have.sym = 1;
    return VM_Sym_New_Call(& p->cmp->vm);
  }
}

BoxVMProcID CmpProc_Get_ProcID(CmpProc *p) {
  if (p->have.proc_id)
    return p->proc_id;

  else {
    p->have.proc_id = 1;
    ASSERT_TASK( VM_Proc_Code_New(& p->cmp->vm, & p->proc_id) );
    return p->proc_id;
  }
}

char *CmpProc_Get_Proc_Desc(CmpProc *p) {
  /* The description is just a help string to make the ASM more readable
   * (may disappear in the future). For now it is:
   *  - the type of the procedure, if this is known;
   *  - the name of the procedure, if this is known;
   *  - "|unknown|"
   */
  if (p->have.type)
    return TS_Name_Get(& p->cmp->ts, p->type);

  else
    return BoxMem_Strdup((p->have.proc_name) ? p->proc_name : "|unknown|");
}

BoxVMCallNum CmpProc_Get_Call_Num(CmpProc *p) {
  if (p->have.call_num)
    return p->sym;

  else {
    BoxVMProcID pn = CmpProc_Get_ProcID(p);
    char *proc_desc = CmpProc_Get_Proc_Desc(p),
         *proc_name = (p->have.proc_name) ? p->proc_name : "(noname)";
    VM_Proc_Install_Code(& p->cmp->vm, & p->call_num, pn,
                         proc_name, proc_desc);
    BoxMem_Free(proc_desc);
    p->have.call_num = 1;
    return p->call_num;
  }
}

void CmpProc_Raw_VA_Assemble(CmpProc *p, BoxOpcode op, va_list ap) {
  BoxVMProcID proc_id, previous_target;
  proc_id = CmpProc_Get_ProcID(p);
  previous_target = BoxVM_Proc_Target_Set(& p->cmp->vm, proc_id);
  VM_VA_Assemble(& p->cmp->vm, op, ap);
  (void) BoxVM_Proc_Target_Set(& p->cmp->vm, previous_target);
}

void CmpProc_Raw_Assemble(CmpProc *p, BoxOpcode op, ...) {
  va_list ap;
  va_start(ap, op);
  CmpProc_Raw_VA_Assemble(p, op, ap);
  va_end(ap);
}

/** Internal function used by My_Unsafe_Assemble to assemble operation
 * involving pointer access (i.e. involving args like i[ro5+16]).
 * This function is used when assembling things like:
 *
 *   mov ro0, ro3  <-- this line is what we want to emit
 *   mov i[ro0+8], 45
 */
static void My_Prepare_Ptr_Access(CmpProc *p, const BoxCont *c) {
  Int ptr_reg = c->value.ptr.reg;
  if (c->categ == CAT_PTR && ptr_reg != 0) {
    Int addr_categ = c->value.ptr.is_greg ? CAT_GREG : CAT_LREG;
    CmpProc_Raw_Assemble(p, BOXOP_MOV_OO, BOXOPCAT_LREG, (Int) 0,
                         addr_categ, ptr_reg);
  }
}

/** Internal function used by My_Unsafe_Assemble.
 * Used to get the integer value of a container whose value is an integer.
 * Containers having this property are: registers (both local and global)
 * pointers and immediate integers/chars.
 */
static Int My_Int_Val_From_Cont(const BoxCont *c) {
  if (c->categ == BOXCONTCATEG_LREG || c->categ == BOXCONTCATEG_GREG)
    return c->value.reg;

  else if (c->categ == BOXCONTCATEG_PTR)
    return c->value.ptr.offset;

  else {
    if (c->type == BOXCONTTYPE_CHAR)
      return (Int) c->value.imm.boxchar;
    else if (c->type == BOXCONTTYPE_INT)
      return c->value.imm.boxint;
    assert(0);
  }
}

/** Internal function used by CmpProc_Assemble and co.
 * Similar to CmpProc_Assemble, but works only in certain particular cases
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
static void My_Unsafe_Assemble(CmpProc *p, BoxOp op,
                               int num_args, const BoxCont **cs) {
  assert(num_args == 1 || num_args == 2);
  if (num_args == 1) {
    const BoxCont *c1 = cs[0];
    My_Prepare_Ptr_Access(p, c1);
    if (c1->categ != BOXCONTCATEG_IMM) {
      Int c1_int_val = My_Int_Val_From_Cont(c1);
      CmpProc_Raw_Assemble(p, op, c1->categ, c1_int_val);
      return;

    } else {
      switch(c1->type) {
      case BOXCONTTYPE_CHAR:
      case BOXCONTTYPE_INT:
        CmpProc_Raw_Assemble(p, op, c1->categ, c1->value.imm.boxint);
        return;
      case BOXCONTTYPE_REAL:
        CmpProc_Raw_Assemble(p, op, c1->categ, c1->value.imm.boxreal);
        return;
      case BOXCONTTYPE_POINT:
        CmpProc_Raw_Assemble(p, op, c1->categ, c1->value.imm.boxpoint);
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
      Int c1_int_val = My_Int_Val_From_Cont(c1),
          c2_int_val = My_Int_Val_From_Cont(c2);

      CmpProc_Raw_Assemble(p, op, c1->categ, c1_int_val,
                           c2->categ, c2_int_val);
      return;

    } else {
      Int c1_int_val = (c1->categ == BOXCONTCATEG_PTR) ?
                       c1->value.ptr.offset : c1->value.reg;
      switch(c1->type) {
      case BOXCONTTYPE_CHAR:
      case BOXCONTTYPE_INT:
        CmpProc_Raw_Assemble(p, op, c1->categ, c1_int_val,
                             c2->categ, c2->value.imm.boxint);
        return;
      case BOXCONTTYPE_REAL:
        CmpProc_Raw_Assemble(p, op, c1->categ, c1_int_val,
                             c2->categ, c2->value.imm.boxreal);
        return;
      case BOXCONTTYPE_POINT:
        CmpProc_Raw_Assemble(p, op, c1->categ, c1_int_val,
                             c2->categ, c2->value.imm.boxpoint);
        return;
      default:
        MSG_FATAL("My_Unsafe_Assemble: invalid type for immediate.");
        assert(0);
      }
    }

  }
}

static void My_Gather_Implicit_Input_Regs(CmpProc *p,
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
        CmpProc_Assemble(p, BOXGOP_MOV, 2, & dest, args[i]);
      }
    }
  }
}

static void My_Scatter_Implicit_Input_Regs(CmpProc *p,
                                           int num_regs, BoxOpReg *regs,
                                           const BoxCont **args) {
  int i;
  for(i = 0; i < num_regs; i++) {
    BoxOpReg *reg = & regs[i];
    if (reg->kind == 'r') {
      if (reg->io == 'o' || reg->io == 'b') {
        BoxCont src;
        src.type = BoxContType_From_Char(reg->type);
        src.categ = BOXCONTCATEG_LREG;
        src.value.reg = reg->num;
        CmpProc_Assemble(p, BOXGOP_MOV, 2, args[i], & src);
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

static BoxOpInfo *My_Find_Op(CmpProc *p, FoundOP *info, BoxGOp g_op,
                             int num_args, const BoxCont **args,
                             int ignore_signature) {
  BoxOpInfo *oi;
  int num_exp_args, ro0_arg_conflict, ro0_input_conflict;

  /* Search for operations whose argument number and type match */
  oi = BoxVM_Get_Op_Info(& p->cmp->vm, g_op);
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
        if (t != args[i]->type) break; /* Exit if type for this arg
                                          does not match */

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

void CmpProc_VA_Assemble(CmpProc *p, BoxGOp g_op, int num_args, va_list ap) {
  const BoxCont *args[BOXOP_MAX_NUM_ARGS];
  BoxOpInfo *oi;
  FoundOP op;
  int i;

  if (num_args > BOXOP_MAX_NUM_ARGS) {
    MSG_FATAL("CmpProc_Assemble: the given number of arguments is too high.");
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
      fprintf(stderr, "CmpProc_Assemble: cannot find a matching operation.\n");
      fprintf(stderr, "Possible signatures are:\n");
      BoxOpInfo_Print(stderr, BoxVM_Get_Op_Info(& p->cmp->vm, g_op));
      MSG_FATAL("CmpProc_Assemble: aborting!");
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
        CmpProc_Assemble(p, BOXGOP_MOV, 2, & r0, op.exp_args[1]);
        CmpProc_Assemble(p, g_op, 2, op.exp_args[0], & r0);

      } else {
        assert(op.num_exp_args == 1);
        CmpProc_Assemble(p, BOXGOP_MOV, 2, & r0, op.exp_args[1]);
        CmpProc_Assemble(p, g_op, 1, & r0);
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
        CmpProc_Assemble(p, BOXGOP_MOV, 2, & r0, op.exp_args[1]);
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
        ro1.type = BOXCONTTYPE_OBJ;
        ro1.categ = BOXCONTCATEG_LREG;
        ro1.value.reg = 1;
        cs[0] = & ro1;
        My_Unsafe_Assemble(p, BOXOP_PUSH_O, 1, cs);
        CmpProc_Assemble(p, BOXGOP_MOV, 2, & ro1, op.exp_args[1]);
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

void CmpProc_Assemble(CmpProc *p, BoxGOp g_op, int num_args, ...) {
  va_list ap;
  va_start(ap, num_args);
  CmpProc_VA_Assemble(p, g_op, num_args, ap);
  va_end(ap);
}
