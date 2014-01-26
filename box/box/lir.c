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
My_Append_Op(BoxLIR *lir, BoxLIRNodeType type, uint8_t op_id)
{
  BoxLIRNodeOp *node = (BoxLIRNodeOp *) BoxLIR_Create_Node(lir, type);
  if (node) {
    node->op_id = op_id;
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
BoxLIR_Append_Op1(BoxLIR *lir, uint8_t op_id, uint32_t arg1)
{
  BoxLIRNodeOp *node = My_Append_Op(lir, BOXLIRNODETYPE_OP1, op_id);
  if (node)
    ((BoxLIRNodeOp1 *) node)->args[0] = arg1;
  return node;
}

BoxLIRNodeOp *
BoxLIR_Append_Op2(BoxLIR *lir, uint8_t op_id, uint32_t arg1, uint32_t arg2)
{
  BoxLIRNodeOp *node = My_Append_Op(lir, BOXLIRNODETYPE_OP2, op_id);
  if (node) {
    ((BoxLIRNodeOp2 *) node)->args[0] = arg1;
    ((BoxLIRNodeOp2 *) node)->args[1] = arg2;
  }
  return node;
}

BoxLIRNodeProc *
BoxLIR_Append_Proc(BoxLIR *lir)
{
  BoxLIRNodeProc *node =
    (BoxLIRNodeProc *) BoxLIR_Create_Node(lir, BOXLIRNODETYPE_PROC);
  if (node) {
    node->next = NULL;
    node->first_op = NULL;
    if (lir->last_proc)
      lir->last_proc->next = node;
    else
      lir->first_proc = node;
    lir->last_proc = node;
  }
  return node;
}

void
BoxLIR_Set_Target_Proc(BoxLIR *lir, BoxLIRNodeProc *proc)
{
  lir->target = proc;
  lir->last_op = NULL;
}
