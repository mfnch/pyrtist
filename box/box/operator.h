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
#  include "cmpptrs.h"
#  include "vm_priv.h"

typedef enum {
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
  BoxCmp      *cmp;             /**< Compiler to which the operator refers */
  OprAttr     attr;             /**< Operator attributes */
  const char  *name;            /**< Name of the operator */
  Operation   *first_operation;
};

/**
 * Structure returned by BoxCmp_Operator_Find_Opn containing details about
 * the Operation which was found.
 */
typedef struct {
  Operator   *opr;               /**< Operator for which we got the match. */
  OprAttr    attr;               /**< Attributes of the matched operation */
  BoxTypeCmp match_left,         /**< How the left operand was matched. */
             match_right;        /**< How the right operand was matched. */
  BoxType    *expand_type_left,  /**< Type to which the left operand should be
                                    expanded */
             *expand_type_right; /**< Type to which the right operand should be
                                    expanded */
} OprMatch;

#endif /* _OPERATOR_H_TYPES */

#if !defined(_OPERATOR_H_FNS) && !defined(_BOX_NDECL_FNS)
#  define _OPERATOR_H_FNS

/** INTERNAL: Called by BoxCmp_Init to initialise the operator table. */
void BoxCmp_Init__Operators(BoxCmp *c);

/** INTERNAL: Called by BoxCmp_Finish to finalise the operator table. */
void BoxCmp_Finish__Operators(BoxCmp *c);

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

/** Get the Operator object corresponding to the given ASTBinOp constant. */
Operator *BoxCmp_BinOp_Get(BoxCmp *c, ASTBinOp bin_op);

/** Get the Operator object corresponding to the given ASTUnOp constant. */
Operator *BoxCmp_UnOp_Get(BoxCmp *c, ASTUnOp un_op);

/** Compiles an operation between the two expression e1 and e2, where
 * the operator is opr.
 * REFERENCES: return: new, v: -1;
 */
Value *BoxCmp_Opr_Emit_UnOp(BoxCmp *c, ASTUnOp op, Value *v);

/** Compiles an operation between the two expression e1 and e2, where
 * the operator is opr.
 * REFERENCES: return: new, v_left: -1, v_right: -1;
 */
Value *BoxCmp_Opr_Emit_BinOp(BoxCmp *c, ASTBinOp op,
                             Value *v_left, Value *v_right);

/** Map an object 'src' to an object 'dest' with (possibly) different type.
 * This function is similar to BoxCmp_Opr_Emit_Conversion, with the difference
 * that the final object is put in the given 'dest' object (and is not created
 * from scratch).
 * REFERENCES: return: new, dest: -1, src: -1;
 */
BoxTask BoxCmp_Opr_Try_Emit_Conversion(BoxCmp *c, Value *dest, Value *src);

/** Emits the conversion from the source expression 'v', to the given type 't'
 */
Value *BoxCmp_Opr_Emit_Conversion(BoxCmp *c, Value *src, BoxType *dest);

Operation *BoxCmp_Operator_Find_Opn(BoxCmp *c, Operator *opr, OprMatch *match,
                                    BoxType *type_left, BoxType *type_right,
                                    BoxType *type_result);

#endif /* _OPERATOR_H_FNS */
