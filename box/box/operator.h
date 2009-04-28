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

#ifndef _OPERATOR_H
#  define _OPERATOR_H

#  include "typesys.h"
#  include "new_compiler.h"
#  include "ast.h"
#  include "value.h"

typedef enum {
  OPR_ATTR_NATIVE      = 1, /**< Is it a native operation (does the VM
                                 implement the operation with dedicated
                                 instruction or should we use a procedure)? */
  OPR_ATTR_BINARY      = 2, /**< Is it a binary or unary operator? */
  OPR_ATTR_COMMUTATIVE = 4, /**< Is it a commutative operator? */
  OPR_ATTR_ASSIGNMENT  = 8, /**< Is it an assignment operator? */
  OPR_ATTR_ALL         = 15 /**< Used as the full mask */
} OprAttr;

typedef struct _operation_struc Operation;
typedef struct _operator_struc Operator;

struct _operation_struc {
  OprAttr attr;            /**< Attributes overridden from operation */

  BoxType type_left,       /**< Type of the left operand */
          type_right,      /**< Type of the right operand */
          type_result;     /**< Type of the result */

  union {
    BoxGOp  opcode;        /**< Bytecode instrucion associated with the op. */
    Int     module;        /**<  */
  } implem;                /**< The implementation of the operation */

  struct _operation_struc
          *next,           /**< Next operation of the current operator */
          *previous;       /**< Prevous operation */
};

struct _operator_struc {
  OprAttr    attr;             /**< Operator attributes */
  const char *name;            /**< Name of the operator */
  Operation  *first_operation;
};

/** Structure returned by BoxCmp_Operator_Find_Opn containing details about
 * the Operation which was found.
 */
typedef struct {
  Operator *opr;              /**< Operator for which we got the match. */
  OprAttr  attr;              /**< Attributes of the matched operation */
  TSCmp    match_left,        /**< How the left operand was matched. */
           match_right;       /**< How the right operand was matched. */
  BoxType  expand_type_left,  /**< Type to which the left operand should be
                                   expanded */
           expand_type_right; /**< Type to which the right operand should be
                                   expanded */
} OprMatch;

/** INTERNAL: Called by BoxCmp_Init to initialise the operator table. */
void BoxCmp_Init__Operators(BoxCmp *c);

/** INTERNAL: Called by BoxCmp_Finish to finalise the operator table. */
void BoxCmp_Finish__Operators(BoxCmp *c);

/** Change attributes for operator. 'mask' tells what attributes to change,
 * 'value' tells how to change them.
 */
void Operator_Attr_Set(Operator *opr, OprAttr mask, OprAttr attr);

/** Change attributes for operation. 'mask' tells what attributes to change,
 * 'value' tells how to change them.
 */
void Operation_Attr_Set(Operation *opn, OprAttr mask, OprAttr attr);


Value *BoxCmp_Opr_Emit_BinOp(BoxCmp *c, ASTBinOp op,
                             Value *v_left, Value *v_right);

#endif /* _OPERATOR_H */
