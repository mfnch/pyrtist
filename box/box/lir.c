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


/* Initialize a BoxLIR. */
void
BoxLIR_Init(BoxLIR *lir)
{
  BoxAllocPool_Init(& lir->pool, sizeof(BoxLIRNode)*16);
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
