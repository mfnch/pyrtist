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

/** @file operator.h
 * @brief A module to manage compilation of operations.
 *
 * A nice description...
 */

#ifndef _OPERATOR_H_TYPES
#  define _OPERATOR_H_TYPES

#  include "value.h"
#  include "ast.h"
#  include "compiler.h"
#  include "vm_priv.h"

typedef enum {
  OPR_ATTR_INVALID     = 0,  /**< Invalid value. */
  OPR_ATTR_NATIVE      = 1,  /**< Is it a native operation: does the VM
                                  implement the operation with dedicated
                                  instruction or should we use a procedure? */
  OPR_ATTR_BINARY      = 2,  /**< Is it a binary or unary operator? */
  OPR_ATTR_MATCH_RESULT= 4,  /**< Match the result when searching? */
  OPR_ATTR_UN_RIGHT    = 8,  /**< Is it a right (or left) unary operation? */
  OPR_ATTR_COMMUTATIVE = 16, /**< Is it a commutative operator? */
  OPR_ATTR_ASSIGNMENT  = 32, /**< Is it an assignment operator? (is it
                                  supposed to change its operand?) */
  OPR_ATTR_IGNORE_RES  = 64, /**< Is the result ignorable? */
  OPR_ATTR_ALL         =127  /**< Used as the full mask */
} OprAttr;

/** Describes the scheme to follow when assemblying the operation */
typedef enum {
  OPASMSCHEME_STD_UN,
  OPASMSCHEME_RIGHT_UN,
  OPASMSCHEME_USR_UN,
  OPASMSCHEME_STD_BIN,
  OPASMSCHEME_R_LR_BIN, /**< Left and right operands with same type */
  OPASMSCHEME_RL_R_BIN, /**< Result and left operand with same type */
  OPASMSCHEME_UNKNOWN
} OpAsmScheme;

typedef struct Operation_struct Operation;
typedef struct Operator_struct Operator;

struct Operation_struct {
  Operator  *opr;          /**< The corresponding operator */

  OprAttr   attr;          /**< Attributes overridden from operation */

  BoxType   *type_left,    /**< Type of the left operand */
            *type_right,   /**< Type of the right operand */
            *type_result;  /**< Type of the result */

  OpAsmScheme
            asm_scheme;    /**< Scheme for assemblying the operation */

  union {
    BoxGOp  opcode;        /**< Bytecode instrucion associated with the op. */
    BoxVMCallNum
            call_num;      /**< Call number for the procedure which implements
                                the operation. */
  } implem;                /**< The implementation of the operation */

  Operation *next,         /**< Next operation of the current operator */
            *previous;     /**< Prevous operation */
};

struct Operator_struct {
  OprAttr     attr;             /**< Operator attributes */
  const char  *name;            /**< Name of the operator */
  Operation   *first_operation;
};

#endif /* _OPERATOR_H_TYPES */

#if !defined(_OPERATOR_H_FNS) && !defined(_BOX_NDECL_FNS)
#  define _OPERATOR_H_FNS

BOX_BEGIN_DECLS

/** Change attributes for operator. 'mask' tells what attributes to change,
 * 'value' tells how to change them.
 */
void Operator_Attr_Set(Operator *opr, OprAttr mask, OprAttr attr);

/** Provide the symbol for the procedure implementing the operation 'opn'. */
void Operation_Set_User_Implem(Operation *opn, BoxVMCallNum call_num);

/** Change attributes for operation. 'mask' tells what attributes to change,
 * 'value' tells how to change them.
 */
void Operation_Attr_Set(Operation *opn, OprAttr mask, OprAttr attr);

/** Add a new operation with the given type of operands and results to the
 * operator 'opr'.  NOTE: for a unary operator type_right is ignore
 * (BOXTYPEID_NONE can be used).
 */
Operation *Operator_Add_Opn(Operator *opr, BoxType *type_left,
                            BoxType *type_right, BoxType *type_result);

/** Compiles an operation between the two expression e1 and e2, where
 * the operator is opr.
 */
Value *Opr_Emit_UnOp(BoxASTUnOp op, Value *v);

/** Compiles an operation between the two expression e1 and e2, where
 * the operator is opr.
 */
Value *Opr_Emit_BinOp(BoxASTBinOp op,
                      Value *v_left, Value *v_right);

BOX_END_DECLS

#endif /* _OPERATOR_H_FNS */
