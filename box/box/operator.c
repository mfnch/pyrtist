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

#include <assert.h>
#include <stdlib.h>

#include "types.h"
#include "mem.h"
#include "ast.h"
#include "expr.h"
#include "messages.h"
#include "new_compiler.h"
#include "operator.h"

/* We have two concepts in this module. One is the concept of Operator
 * (+ or - are two examples), the other is the concept of Operation,
 * which is just an operator applied to definite types.
 * We need here to be able to define an Operator, to associate new operations
 * with it, to search for them and all this should be Scope dependent:
 * an Operation is defined inside a given scope and can override previous
 * Operations.
 */

/* Create a new operator */
void Operator_Init(Operator *opr, const char *name) {
  opr->attr = 0;
  opr->name = name;
  opr->can_define = 0;
  opr->first_operation = NULL;
}

void Operator_Finish(Operator *opr) {
  Operation *opn = opr->first_operation;
  while (opn != NULL) { /* Destroy the chain of operations */
    Operation *this = opn;
    opn = opn->next;
    BoxMem_Free(this);
  }
}

/** Change attributes for operator. 'mask' tells what attributes to change,
 * 'value' tells how to change them.
 */
void Operator_Attr_Set(Operator *opr, OprAttr mask, OprAttr attr) {
  opr->attr = (opr->attr & (~mask)) | (attr & mask);
}

/* Aggiunge una nuova operazione di tipo type1 opr type2
 * all'operatore *opr. Se type1 o type2 sono uguali a TYPE_NONE si tratta
 * di un'operazione unaria (sinistra o destra rispettivamente).
 */
Operation *Operator_Add_Opn(Operator *opr, BoxType type_left,
                            BoxType type_right, BoxType type_result) {
  Operation *opn;

  opn = (Operation *) BoxMem_Safe_Alloc(sizeof(Operation));
  opn->attr_mask = 0;
  opn->attr = 0;
  opn->type_left = type_left;
  opn->type_right = type_right;
  opn->type_result = type_result;

  /** Link to chain */
  opn->next = opr->first_operation;
  opn->previous = NULL;
  opr->first_operation->previous = opn;
  opr->first_operation = opn;

  return opn;
}

void Operator_Del_Opn(Operator *opr, Operation *opn) {
  if (opn->next != NULL)
    opn->next->previous = opn->previous;
  if (opn->previous != NULL)
    opn->previous->next = opn->next;
  if (opr->first_operation == opn)
    opr->first_operation = opn->next;
  BoxMem_Free(opn);
}

/** Get the Operator object corresponding to the given ASTBinOp constant. */
Operator *BoxCmp_BinOp_Get(BoxCmp *c, ASTBinOp bin_op) {
  assert(bin_op >= 0 && bin_op < ASTBINOP__NUM_OPS);
  return & c->bin_ops[bin_op];
}

/** Get the Operator object corresponding to the given ASTUnOp constant. */
Operator *BoxCmp_UnOp_Get(BoxCmp *c, ASTUnOp un_op) {
  assert(un_op >= 0 && un_op < ASTUNOP__NUM_OPS);
  return & c->un_ops[un_op];
}

/** INTERNAL: Called by BoxCmp_Init to initialise the operator table. */
void BoxCmp_Init__Operators(BoxCmp *c) {
  int i;

  for(i = 0; i < ASTUNOP__NUM_OPS; i++) {
    Operator *opr = BoxCmp_UnOp_Get(c, i);
    Operator_Init(opr, ASTBinOp_To_String(i));
    Operator_Attr_Set(opr, OPR_ATTR_BINARY, 0);
  }

  for(i = 0; i < ASTBINOP__NUM_OPS; i++) {
    Operator *opr = BoxCmp_BinOp_Get(c, i);
    Operator_Init(opr, ASTUnOp_To_String(i));
    Operator_Attr_Set(opr, OPR_ATTR_BINARY, OPR_ATTR_BINARY);
  }
}

/** INTERNAL: Called by BoxCmp_Finish to finalise the operator table. */
void BoxCmp_Finish__Operators(BoxCmp *c) {
  int i;

  for(i = 0; i < ASTUNOP__NUM_OPS; i++)
    Operator_Finish(BoxCmp_UnOp_Get(c, i));

  for(i = 0; i < ASTBINOP__NUM_OPS; i++)
    Operator_Finish(BoxCmp_BinOp_Get(c, i));
}


/** Finds the unary or binary operation associated with the operator *opr.
 *  If type1 and type2 are both different from BOXTYPE_NONE, then the function
 *  will search for a binary operation of the following kind:
 *                               type1 opr type2
 *  If type1 = TYPE_NONE, then a left-unary operation will be searched:
 *                                  opr type2
 *  If type2 = TYPE_NONE, then a right-unary operation will be searched:
 *                                  type1 opr
 *  If typer != TYPE_NONE also the type of the result will be checked
 *  during the search.
 * NOTE: it should not happen that type1 = type2 = TYPE_NONE.
 */
Operation *BoxCmp_Operator_Find_Opn(BoxCmp *c, Operator *opr, OprMatch *match,
                                    BoxType type_left, BoxType type_right) {
  int opr_is_unary = ((opr->attr & OPR_ATTR_BINARY) == 0);
  Operation *opn;
  for(opn = opr->first_operation; opn != NULL; opn = opn->next) {
    printf("Operation at %p\n", opn);
    TSCmp match_left, match_right;
    match_left = TS_Compare(& c->ts, opn->type_left, type_left);
    if (match_left != TS_TYPES_UNMATCH) {
      if (opr_is_unary) {
          match->opr = opr;
          match->attr = (opr->attr & (~opn->attr_mask))
                        | (opn->attr_mask & opn->attr);
          match->match_left = match_left;
          match->match_right = 0;
          match->expand_type_left = opn->type_left;
          match->expand_type_right = BOXTYPE_NONE;
          return opn;

      } else {
        match_right = TS_Compare(& c->ts, opn->type_right, type_right);
        if (match_right != TS_TYPES_UNMATCH) {
          match->opr = opr;
          match->attr = (opr->attr & (~opn->attr_mask))
                        | (opn->attr_mask & opn->attr);
          match->match_left = match_left;
          match->match_right = match_right;
          match->expand_type_left = opn->type_left;
          match->expand_type_right = opn->type_right;
          return opn;
        }
      }
    }
  }
  return NULL;

#if 0
  Int type;
  int no_check_arg1, no_check_arg2, check_rs, unary;
  int ne1, ne2;
  Operation *opn;

#if 0
  printf("Cmp_Operation_Find: Cerco %s OP %s\n",
   Tym_Type_Names(type1), Tym_Type_Names(type2));
#endif

  no_check_arg1 = (type1 == TYPE_NONE);
  no_check_arg2 = (type2 == TYPE_NONE);
  check_rs      = (typer != TYPE_NONE);
  unary = no_check_arg1 || no_check_arg2;

  for (opn = opr->opn_chain; opn != NULL; opn = opn->next ) {
    register Int t1 = opn->type1, t2 = opn->type2;
    int ok_1, ok_2, ok_rs = 1;

    ok_1 = (t1 == type1);
    ok_2 = (t2 == type2);
    ne1 = ne2 = 0;
    if ( ! (ok_1 || no_check_arg1) ) ok_1 = Tym_Compare_Types(t1, type1, & ne1);
    if ( ! (ok_2 || no_check_arg2) ) ok_2 = Tym_Compare_Types(t2, type2, & ne2);

    if ( check_rs ) {
      ok_rs = Tym_Compare_Types(typer, opn->type_rs, NULL);
    }

    if (ok_rs) {
      if (ok_1 && ok_2) {
        if (oi == NULL) return opn;
        oi->commute = 0;
        oi->expand1 = ne1;
        oi->expand2 = ne2;
        oi->exp_type1 = t1;
        oi->exp_type2 = t2;
        return opn;
      }

      if ( !unary && opn->is.commutative ) {
        ok_1 = (t1 == type2);
        ok_2 = (t2 == type1);
        if ( ! ok_1 ) ok_1 = Tym_Compare_Types(t1, type2, & ne2);
        if ( ! ok_2 ) ok_2 = Tym_Compare_Types(t2, type1, & ne1);
        if ( ok_1 && ok_2 ) {
          if ( oi == NULL ) return opn;
          oi->commute = 1;
          oi->expand1 = ne1;
          oi->expand2 = ne2;
          oi->exp_type1 = t2;
          oi->exp_type2 = t1;
          return opn;
        }
      }
    }
  }
  return NULL;
#endif

}

/** Compiles an operation between the two expression e1 and e2, where
 * the operator is opr.
 */
Expr *BoxCmp_Opr_Emit_BinOp(BoxCmp *c, ASTBinOp op,
                            Expr *expr_left, Expr *expr_right) {
  Operator *opr = BoxCmp_BinOp_Get(c, op);
  Operation *opn;
  OprMatch match;

  /* Subtypes cannot be used for operator overloading, so we expand
   * them anyway!
   */
  Expr_Resolve_Subtype(expr_left);
  Expr_Resolve_Subtype(expr_right);

  /* Require operands have value. */
  if (!expr_left->is.value || !expr_right->is.value) {
    if (!expr_left->is.value) {
      MSG_ERROR("The expression on the left of '%s' "
                "must have a definite value!", opr->name);
    } else {
      MSG_ERROR("The expression on the right of '%s' "
                "must have a definite value!", opr->name);
    }
    return NULL;
  }

  /* Now we search the operation */
  opn = BoxCmp_Operator_Find_Opn(c, opr, & match,
                                 expr_left->type, expr_right->type);
  if (opn != NULL) {
    /* Ora eseguo le espansioni, se necessario */
    if (match.match_left == TS_TYPES_EXPAND) {
      printf("BoxCmp_Opr_Emit_BinOp: Expansion not implemented, yet!");
      return NULL;
      /*if (Cmp_Expr_Expand(oi.exp_type1, e1) == BoxFailure) return NULL;*/
    }
    if (match.match_right == TS_TYPES_EXPAND) {
      printf("BoxCmp_Opr_Emit_BinOp: Expansion not implemented, yet!");
      return NULL;
      /*if (Cmp_Expr_Expand(oi.exp_type2, e2) == BoxFailure) return NULL;*/
    }

    return NULL;

#if 0
    /* manca la valutazione della commutativita'! */

    return Cmp_Operation_Exec(opn, e1, e2);
#endif

  } else if (op == ASTBINOP_EQ) {
    /*if (op != 0)
      goto Exec_Opn_Error;

    if IS_FAILED( Cmp_Expr_Expand(e1->type, e2) )
      return NULL;

    if IS_FAILED(Expr_Move(e1, e2))
      return NULL;

    return e1;*/
    return NULL;

  } else {
    MSG_ERROR("%s %s %s <-- Operation has not been defined!",
              TS_Name_Get(& c->ts, expr_left->type), opr->name,
              TS_Name_Get(& c->ts, expr_right->type));
    return NULL;
  }

}
