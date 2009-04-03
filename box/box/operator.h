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

#  include "new_compiler.h"

typedef struct _operation_struc Operation;
typedef struct _operator_struc Operator;

struct _operation_struc {
  struct {
    unsigned int
          intrinsic   : 1, /**< has a corresponding VM instruction */
          commutative : 1, /**< is commutative */
          immediate   : 1, /**< allow immediate operands (constants) */
          assignment  : 1, /**< is an assignment operation (=, +=, ...) */
          link        : 1; /**< ???? */
  } is;

  BoxType type_left,       /**< Type of the left operand */
          type_right,      /**< Type of the right operand */
          type_result;     /**< Type of the result */

  union {
    UInt  asm_code;        /**< Bytecode instrucion associated with the op. */
    Int   module;          /**<  */
  } implem;                /**< The implementation of the operation */

  struct _operation_struc
          *next;           /**< Next operation of the current operator */
};

struct _operator_struc {
  unsigned int
             can_define : 1;   /* E' un operatore di definizione? */
  const char *name;            /* Token che rappresenta l'operatore */
  Operation  *first_operation;
};

/** INTERNAL: Called by BoxCmp_Init to initialise the operator table. */
void BoxCmp_Init__Operators(BoxCmp *c);

/** INTERNAL: Called by BoxCmp_Finish to finalise the operator table. */
void BoxCmp_Finish__Operators(BoxCmp *c);

#if 0

typedef struct Operation Operation;

typedef struct Operator Operator;

/* "Collezione" di tutti gli operatori */
struct cmp_opr_struct {
  /* Operatore usato per la conversione fra tipi diversi */
  Operator *converter;

  /* Operatori di assegnazione */
  Operator *assign;
  Operator *aplus;
  Operator *aminus;
  Operator *atimes;
  Operator *adiv;
  Operator *arem;
  Operator *aband;
  Operator *abxor;
  Operator *abor;
  Operator *ashl;
  Operator *ashr;

  /* Operatori di incremento/decremento */
  Operator *inc;
  Operator *dec;

  /* Operatori aritmetici convenzionali */
  Operator *pow;
  Operator *plus;
  Operator *minus;
  Operator *times;
  Operator *div;
  Operator *rem;

  /* Operatori bit-bit */
  Operator *bor;
  Operator *bxor;
  Operator *band;
  Operator *bnot;

  /* Operatori di shift */
  Operator *shl;
  Operator *shr;

  /* Operatori di confronto */
  Operator *eq;
  Operator *ne;
  Operator *gt;
  Operator *ge;
  Operator *lt;
  Operator *le;

  /* Operatori logici */
  Operator *lor;
  Operator *land;
  Operator *lnot;
};
#endif

#endif /* _OPERATOR_H */
