/****************************************************************************
 * Copyright (C) 2014 by Matteo Franchin                                    *
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

#include <box/mem.h>

#include <box/lir_priv.h>
#include <box/vm_priv.h>


/* Initialize a BoxLIR. */
void
BoxLIR_Init(BoxLIR *lir)
{
  BoxAllocPool_Init(& lir->pool, sizeof(BoxLIRNode)*16);
  lir->logger = NULL;
  lir->first_proc = NULL;
  lir->last_proc = NULL;
  lir->target = NULL;
  lir->last_op = NULL;
  lir->is_sane = BOXBOOL_TRUE;
}

/* Finalize a BoxLIR object initialized with BoxLIR_Init(). */
void
BoxLIR_Finish(BoxLIR *lir)
{
  BoxAllocPool_Finish(& lir->pool);
}

/* Create a new LIR tree. */
BoxLIR *
BoxLIR_Create(void)
{
  BoxLIR *lir = (BoxLIR *) Box_Mem_Alloc(sizeof(BoxLIR));
  if (lir)
    BoxLIR_Init(lir);
  return lir;
}

/* Destroy an abstract syntax tree created with BoxAST_Create(). */
void
BoxLIR_Destroy(BoxLIR *lir)
{
  if (lir) {
    BoxLIR_Finish(lir);
    Box_Mem_Free(lir);
  }
}

/* Get size and alignment for the given node type. */
static uint32_t
My_Get_Node_Size(BoxLIRNodeType type, uint32_t *alignment)
{
#define BOXLIRNODE_DEF(NODE, Node)               \
  case BOXLIRNODETYPE_##NODE:                    \
    *alignment = __alignof__(BoxLIRNode##Node);  \
    return (uint32_t) sizeof(BoxLIRNode##Node);

  switch (type) {
#include "lirnodes.h"
  default:
    return 0;
  }

#undef BOXLIRNODE_DEF
}

void *
BoxLIR_Create_Node(BoxLIR *lir, BoxLIRNodeType type)
{
  uint32_t node_alignment;
  uint32_t node_size = My_Get_Node_Size(type, & node_alignment);
  if (node_size) {
    BoxLIRNode *node =
      BoxAllocPool_Alloc_Aligned(& lir->pool, node_size, node_alignment);
    if (node) {
      node->head.type = type;
      return node;
    }
  }

  lir->is_sane = BOXBOOL_FALSE;
  return NULL;
}

BoxLIRNodeOp *
BoxLIR_Get_Last_Op(BoxLIR *lir)
{
  return lir->last_op;
}

BoxLIRNodeOp *
BoxLIR_Get_Next_Op(BoxLIR *lir, BoxLIRNodeOp *op)
{
  if (op)
    return op->next;
  assert(lir->target);
  return lir->target->first_op;
}

BoxLIRNodeOp *
My_Append_Op(BoxLIR *lir, BoxLIRNodeType type, uint8_t op_id)
{
  BoxLIRNodeOp *node = (BoxLIRNodeOp *) BoxLIR_Create_Node(lir, type);
  if (node) {
    node->op_id = op_id;
    node->offset = 0;
    node->next = NULL;
    if (lir->last_op)
      lir->last_op->next = node;
    else {
      if (lir->target)
        lir->target->first_op = node;
    }
    lir->last_op = node;
  }
  return node;
}

BoxLIRNodeOp *
BoxLIR_Append_Op(BoxLIR *lir, uint8_t op_id)
{
  return My_Append_Op(lir, BOXLIRNODETYPE_OP, op_id);
}

BoxLIRNodeOp *
BoxLIR_Append_Op1(BoxLIR *lir, uint8_t op_id, uint8_t cat0, uint32_t arg0)
{
  BoxLIRNodeOp *node = My_Append_Op(lir, BOXLIRNODETYPE_OP1, op_id);
  if (node) {
    ((BoxLIRNodeOp1 *) node)->regs[0] = arg0;
    ((BoxLIRNodeOp1 *) node)->op.cats[0] = cat0;
  }
  return node;
}

BoxLIRNodeOp *
BoxLIR_Append_Op2(BoxLIR *lir, uint8_t op_id,
                  uint8_t cat0, uint32_t arg0, uint8_t cat1, uint32_t arg1)
{
  BoxLIRNodeOp *node = My_Append_Op(lir, BOXLIRNODETYPE_OP2, op_id);
  if (node) {
    BoxLIRNodeOp2 *op2 = (BoxLIRNodeOp2 *) node;
    op2->op.cats[0] = cat0;
    op2->regs[0] = arg0;
    op2->op.cats[1] = cat1;
    op2->regs[1] = arg1;
  }
  return node;
}

/* Append a load-char-immediate instruction to the instruction chain. */
BoxLIRNodeOp *
BoxLIR_Append_Op_Ld_Char(BoxLIR *lir, uint8_t op_id,
                         uint8_t cat0, uint32_t reg0, char value)
{
  BoxLIRNodeOp *node =
    (BoxLIRNodeOp *) My_Append_Op(lir, BOXLIRNODETYPE_OP_LD_CHAR, op_id);
  if (node) {
    BoxLIRNodeOpLdChar *op = (BoxLIRNodeOpLdChar *) node;
    op->op.cats[0] = cat0;
    op->regs[0] = reg0;
    op->value = value;
  }
  return node;
}

/* Append a load-int-immediate instruction to the instruction chain. */
BoxLIRNodeOp *
BoxLIR_Append_Op_Ld_Int(BoxLIR *lir, uint8_t op_id,
                        uint8_t cat0, uint32_t reg0, BoxInt value)
{
  BoxLIRNodeOp *node =
    (BoxLIRNodeOp *) My_Append_Op(lir, BOXLIRNODETYPE_OP_LD_INT, op_id);
  if (node) {
    BoxLIRNodeOpLdInt *op = (BoxLIRNodeOpLdInt *) node;
    op->op.cats[0] = cat0;
    op->regs[0] = reg0;
    op->value = value;
  }
  return node;
}

/* Append a load-real-immediate instruction to the instruction chain. */
BoxLIRNodeOp *
BoxLIR_Append_Op_Ld_Real(BoxLIR *lir, uint8_t op_id,
                         uint8_t cat0, uint32_t reg0, BoxReal value)
{
  BoxLIRNodeOp *node =
    (BoxLIRNodeOp *) My_Append_Op(lir, BOXLIRNODETYPE_OP_LD_REAL, op_id);
  if (node) {
    BoxLIRNodeOpLdReal *op = (BoxLIRNodeOpLdReal *) node;
    op->op.cats[0] = cat0;
    op->regs[0] = reg0;
    op->value = value;
  }
  return node;
}

BoxLIRNodeOp *
BoxLIR_Append_Op_Branch(BoxLIR *lir, uint8_t op_id, BoxLIRNodeOp *target)
{
  BoxLIRNodeOp *node = My_Append_Op(lir, BOXLIRNODETYPE_OP_BRANCH, op_id);
  if (node) {
    BoxLIRNodeOpBranch *branch = (BoxLIRNodeOpBranch *) node;
    branch->target = target;
  }
  return node;
}

BoxLIRNodeOp *
BoxLIR_Append_Op_Label(BoxLIR *lir)
{
  BoxLIRNodeOp *prev = BoxLIR_Get_Last_Op(lir),
    *node = My_Append_Op(lir, BOXLIRNODETYPE_OP_LABEL, 0);
  if (node) {
    BoxLIRNodeOpLabel *label = (BoxLIRNodeOpLabel *) node;
    label->prev = prev;
  }
  return node;
}

BoxLIRNodeOp *
BoxLIR_Move_Label(BoxLIR *lir, BoxLIRNodeOp *label_op, BoxLIRNodeOp *dest)
{
  BoxLIRNodeOpLabel *label = (BoxLIRNodeOpLabel *) label_op;
  assert(label_op->head.type == BOXLIRNODETYPE_OP_LABEL);

  /* Unchain the label. */
  if (label->prev)
    label->prev->next = label_op->next;
  if (label_op->next && label_op->next->head.type == BOXLIRNODETYPE_OP_LABEL)
    ((BoxLIRNodeOpLabel *) label_op->next)->prev = label->prev;

  /* Insert the label. */
  label->prev = dest;
  if (dest) {
    label_op->next = dest->next;
    dest->next = label_op;
  } else {
    /* dest is the beginning of the chain (dest == NULL). */
    if (lir->target) {
      label_op->next = lir->target->first_op;
      lir->target->first_op = label_op;
    }
  }

  if (label_op->next) {
    if (label_op->next->head.type == BOXLIRNODETYPE_OP_LABEL)
      ((BoxLIRNodeOpLabel *) label_op->next)->prev = label_op;
  } else
    lir->last_op = label_op;
  return label_op;
}

BoxLIRNodeOp *
BoxLIR_Move_Label_Back(BoxLIR *lir, BoxLIRNodeOp *label_op)
{
  return BoxLIR_Move_Label(lir, label_op, BoxLIR_Get_Last_Op(lir));
}

BoxLIRNodeProc *
BoxLIR_Append_Proc(BoxLIR *lir)
{
  BoxLIRNodeProc *node =
    (BoxLIRNodeProc *) BoxLIR_Create_Node(lir, BOXLIRNODETYPE_PROC);
  if (node) {
    node->next = NULL;
    node->first_op = NULL;
    node->last_op = NULL;
    if (lir->last_proc)
      lir->last_proc->next = node;
    else
      lir->first_proc = node;
    lir->last_proc = node;
  }
  return node;
}

BoxLIRNodeProc *
BoxLIR_Set_Target_Proc(BoxLIR *lir, BoxLIRNodeProc *proc)
{
  BoxLIRNodeProc *previous = lir->target;
  if (previous)
    previous->last_op = lir->last_op;
  lir->target = proc;
  lir->last_op = proc->last_op;
  return previous;
}

static void
My_Log_Fatal(BoxLIR *lir, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  BoxLogger_Log_VA(lir->logger, NULL, NULL, BOXLOGLEVEL_FATAL, fmt, ap);
  va_end(ap);
}

/* Internal function used by My_Unsafe_Assemble to assemble operation
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

/* Internal function used by My_Unsafe_Assemble.
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

/* Internal function used by BoxLIR_Append_GOp and co.
 * Similar to BoxLIR_Append_GOp, but works only in certain particular cases
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
        My_Log_Fatal(lir, "My_Unsafe_Assemble: invalid type for immediate.");
        abort();
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
        My_Log_Fatal(lir, "My_Unsafe_Assemble: invalid type for immediate.");
        abort();
      }
    }
  }
}

static void
My_Gather_Implicit_Input_Regs(BoxLIR *lir, int num_regs, const BoxOpReg *regs,
                              const BoxCont **args)
{
  int i;
  for(i = 0; i < num_regs; i++) {
    const BoxOpReg *reg = & regs[i];
    if (reg->kind == 'r') {
      if (reg->io == 'i' || reg->io == 'b') {
        const BoxCont *src = args[i];
        BoxCont dst;

        dst.type = BoxContType_From_Char(reg->type);
        dst.categ = BOXCONTCATEG_LREG;
        dst.value.reg = reg->num;

        if (!(dst.categ == src->categ && dst.value.reg == src->value.reg))
          BoxLIR_Append_GOp(lir, BOXGOP_MOV, 2, & dst, src);
      }
    }
  }
}

static void
My_Scatter_Implicit_Input_Regs(BoxLIR *lir, int num_regs, const BoxOpReg *regs,
                               const BoxCont **args)
{
  int i;
  for(i = 0; i < num_regs; i++) {
    const BoxOpReg *reg = & regs[i];
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
          BoxLIR_Append_GOp(lir, gop, 2, dst, & src);
      }
    }
  }
}

typedef struct {
  const BoxOpInfo *oi;
  int             num_exp_args,
                  ro0_arg_conflict,
                  ro0_input_conflict;
  const BoxCont   *exp_args[2];
  BoxCont         aux_arg;
} MyFoundOp;

int
My_ContTypes_Match(BoxContType t1, BoxContType t2)
{
  return    ((t1 == BOXCONTTYPE_OBJ) ? BOXCONTTYPE_PTR : t1)
         == ((t2 == BOXCONTTYPE_OBJ) ? BOXCONTTYPE_PTR : t2);
}

static const BoxOpInfo *
My_Find_Op(MyFoundOp *info, BoxGOp g_op, int num_args,
           const BoxCont **args, int ignore_signature)
{
  const BoxOpInfo *oi;
  int num_exp_args, ro0_arg_conflict, ro0_input_conflict;

  /* Search for operations whose argument number and type match */
  for(oi = Box_Get_VM_Op_Info(g_op); oi; oi = oi->next) {
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
        const BoxOpReg *reg = & oi->regs[i];
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
My_Load_Immediates(BoxLIR *lir, int num_regs, const BoxOpReg *regs,
                   const BoxCont **args, MyFoundOp *op)
{
  int i, j;
  for(i = 0; i < num_regs; i++) {
    const BoxCont *src = args[i];
    const BoxOpReg *reg = & regs[i];
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
BoxLIR_Append_GOp_VA(BoxLIR *lir, BoxGOp g_op, int num_args, va_list ap)
{
  const BoxCont *args[BOXOP_MAX_NUM_ARGS];
  const BoxOpInfo *oi;
  MyFoundOp op;
  int i;

  if (num_args > BOXOP_MAX_NUM_ARGS) {
    My_Log_Fatal(lir, "BoxLIR_Append_GOp: the given number of arguments is "
                 "too high.");
    abort();
  }

  for(i = 0; i < num_args; i++)
    args[i] = va_arg(ap, BoxCont *);

  /* Search for operations whose argument number and type match */
  oi = My_Find_Op(& op, g_op, num_args, args, /*ignore_signature*/ 0);

  if (oi == NULL) {
    /* Try to search again ignoring the signature: it may be that an immediate
     * value is being passed and that the operation doesn't allow immediate
     * values. If this is the case we should move the immediate to a temporary
     * register and retry
     */
    oi = My_Find_Op(& op, g_op, num_args, args, /*ignore_signature*/ 1);
    if (!oi) {
      int i;
      char *sep = "";
      fprintf(stderr, "BoxLIR_Append_GOp: cannot find a matching operation.\n");
      fprintf(stderr, "Possible signatures are:\n");
      BoxOpInfo_Print(stderr, Box_Get_VM_Op_Info(g_op));
      fprintf(stderr, "Got the following %d arguments: ", num_args);
      for(i = 0; i < num_args; i++) {
        fprintf(stderr, "%s%s", sep, BoxCont_To_String(args[i]));
        sep = ", ";
      }
      fprintf(stderr, "\n");
      My_Log_Fatal(lir, "BoxLIR_Append_GOp: aborting!");
      abort();
    } else {
      /* This means that the first operation search failed because the last
       * argument is an immediate
       */
      BoxCont r0;
      r0.type = BoxContType_From_Char(oi->arg_type);
      r0.categ = BOXCONTCATEG_LREG;
      r0.value.reg = 0;
      if (op.num_exp_args == 2) {
        BoxLIR_Append_GOp(lir, BOXGOP_MOV, 2, & r0, op.exp_args[1]);
        BoxLIR_Append_GOp(lir, g_op, 2, op.exp_args[0], & r0);

      } else {
        assert(op.num_exp_args == 1);
        BoxLIR_Append_GOp(lir, BOXGOP_MOV, 2, & r0, op.exp_args[1]);
        BoxLIR_Append_GOp(lir, g_op, 1, & r0);
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
  My_Gather_Implicit_Input_Regs(lir, num_args, oi->regs, args);

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
        BoxLIR_Append_GOp(lir, BOXGOP_MOV, 2, & r0, op.exp_args[1]);
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
        BoxLIR_Append_GOp(lir, BOXGOP_MOV, 2, & ro1, op.exp_args[1]);
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
      My_Log_Fatal(lir, "ro0 is an implicit input register: not implemented "
                   "yet!");
      abort();
    } else {
      My_Log_Fatal(lir, "too many register conflicts: not implemented yet!");
      abort();
    }
  }

  /* Distributing all the implicit output registers to the corresponding
   * arguments
   */
  My_Scatter_Implicit_Input_Regs(lir, num_args, oi->regs, args);
}

void
BoxLIR_Append_GOp(BoxLIR *lir, BoxGOp g_op, int num_args, ...)
{
  va_list ap;
  va_start(ap, num_args);
  BoxLIR_Append_GOp_VA(lir, g_op, num_args, ap);
  va_end(ap);
}
