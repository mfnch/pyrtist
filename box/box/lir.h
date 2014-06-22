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

/**
 * @file lir.h
 * @brief Low-level Intermediate Representation for Box.
 */

#ifndef _BOX_LIR_H
#  define _BOX_LIR_H

#  include <stdint.h>

#  include <box/core.h>
#  include <box/vm.h>


/**
 * @brief Possible types of nodes in the LIR.
 */
typedef enum {
#  define BOXLIRNODE_DEF(NODE, Node) BOXLIRNODETYPE_##NODE,
#  include <box/lirnodes.h>
#  undef BOXLIRNODE_DEF
} BoxLIRNodeType;

/**
 * @brief Box Compiler's LIR object.
 */
typedef struct BoxLIR_struct BoxLIR;

/**
 * @brief Generic LIR node.
 *
 * LIR nodes have type names starting with BoxLIRNode and can all be safely
 * cast to the generic LIR node here defined.
 *
 * @see BOXLIRNODEHEAD
 */
typedef struct BoxLIRNode_struct BoxLIRNode;

/**
 * @brief First element of a #BoxLIRNode.
 *
 * @see #BOXLIRNODEHEAD
 */
struct BoxLIRNodeHead_struct {
  uint8_t type;
};

/**
 * @brief Beginning of every LIR node.
 *
 * The C standard guarantees that the pointer to a structure is always the
 * pointer to its first member. We therefore demand that every LIR node is
 * declared like this:
 * @code
 * typedef struct BoxLIRNodeXXX {
 *   BOXLIRNODEHEAD
 *   // More stuff here ...
 * }
 * @endcode
 * This guarantees that every LIR node can be safely cast to a generic
 * #BoxLIRNode node.
 */
#define BOXLIRNODEHEAD \
  struct BoxLIRNodeHead_struct head;

/**
 * @brief Implementation of a generic LIR node.
 *
 * @see BoxLIRNode
 */
struct BoxLIRNode_struct {
  BOXLIRNODEHEAD
};

/* Define all nodes types. */
#  include <box/lirnodes.h>

BOX_BEGIN_DECLS

/**
 * @brief Create a new LIR tree.
 */
BoxLIR *
BoxLIR_Create(void);

/**
 * @brief Destroy an LIR tree created with BoxAST_Create().
 */
void
BoxLIR_Destroy(BoxLIR *lir);

/**
 * @brief Create a new target procedure.
 */
BOXEXPORT BoxLIRNodeProc *
BoxLIR_Append_Proc(BoxLIR *lir);

/**
 * @brief Change the target procedure.
 *
 * @param lir The LIR tree.
 * @param proc The procedure to be set as the new target for instruction
 *   append.
 * @return The previous active procedure.
 */
BOXEXPORT BoxLIRNodeProc *
BoxLIR_Set_Target_Proc(BoxLIR *lir, BoxLIRNodeProc *proc);

/**
 * @brief Get the last instruction in the current instruction chain.
 * @return The last instruction, or @c NULL if the chain is empty.
 */
BOXEXPORT BoxLIRNodeOp *
BoxLIR_Get_Last_Op(BoxLIR *lir);

/**
 * @brief Get the instruction following the given one.
 */
BOXEXPORT BoxLIRNodeOp *
BoxLIR_Get_Next_Op(BoxLIR *lir, BoxLIRNodeOp *op);

/**
 * @brief Append a zero argument instruction to the current instruction chain.
 */
BOXEXPORT BoxLIRNodeOp *
BoxLIR_Append_Op(BoxLIR *lir, uint8_t op_id);

/**
 * @brief Append a one argument instruction to the current instruction chain.
 */
BOXEXPORT BoxLIRNodeOp *
BoxLIR_Append_Op1(BoxLIR *lir, uint8_t op_id, uint8_t cat0, uint32_t arg0);

/**
 * @brief Append a two arguments instruction to the current instruction chain.
 */
BOXEXPORT BoxLIRNodeOp *
BoxLIR_Append_Op2(BoxLIR *lir, uint8_t op_id,
                  uint8_t cat0, uint32_t arg0, uint8_t cat1, uint32_t arg1);

/**
 * @brief Append a load-char-immediate instruction to the instruction chain.
 */
BOXEXPORT BoxLIRNodeOp *
BoxLIR_Append_Op_Ld_Char(BoxLIR *lir, uint8_t op_id,
                         uint8_t cat0, uint32_t reg0, char value);

/**
 * @brief Append a load-int-immediate instruction to the instruction chain.
 */
BOXEXPORT BoxLIRNodeOp *
BoxLIR_Append_Op_Ld_Int(BoxLIR *lir, uint8_t op_id,
                        uint8_t cat0, uint32_t reg0, BoxInt value);

/**
 * @brief Append a load-real-immediate instruction to the instruction chain.
 */
BOXEXPORT BoxLIRNodeOp *
BoxLIR_Append_Op_Ld_Real(BoxLIR *lir, uint8_t op_id,
                         uint8_t cat0, uint32_t reg0, BoxReal value);

/**
 * @brief Append a branch instruction to the instruction chain.
 */
BOXEXPORT BoxLIRNodeOp *
BoxLIR_Append_Op_Branch(BoxLIR *lir, uint8_t op_id, BoxLIRNodeOp *target);

/**
 * @brief Append a branch target (label) to the instruction chain.
 */
BOXEXPORT BoxLIRNodeOp *
BoxLIR_Append_Op_Label(BoxLIR *lir);

/**
 * @brief Move a label in another position in the chain.
 */
BOXEXPORT BoxLIRNodeOp *
BoxLIR_Move_Label(BoxLIR *lir, BoxLIRNodeOp *label, BoxLIRNodeOp *dest);

/**
 * @brief Move a label to the end of the instruction chain.
 */
BOXEXPORT BoxLIRNodeOp *
BoxLIR_Move_Label_Back(BoxLIR *lir, BoxLIRNodeOp *label_op);

/**
 * @brief Append a LIR node for a Generic OPcode.
 */
BOXEXPORT void
BoxLIR_Append_GOp(BoxLIR *lir, BoxGOp g_op, int num_args, ...);

BOX_END_DECLS

#endif /* _BOX_LIR_H */
