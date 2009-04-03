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
void Operator_Init(Operator *opr) {
  opr->name = "?";
  opr->can_define = 0;
  opr->first_operation = NULL;
}

void Operator_Finish(Operator *opr) {

}


Operator *BoxCmp_BinOp_Get(BoxCmp *c, ASTBinOp bin_op) {
  assert(bin_op >= 0 && bin_op < ASTBINOP__NUM_OPS);
  return & c->bin_ops[bin_op];
}

Operator *BoxCmp_UnOp_Get(BoxCmp *c, ASTUnOp un_op) {
  assert(un_op >= 0 && un_op < ASTUNOP__NUM_OPS);
  return & c->un_ops[un_op];
}

/** INTERNAL: Called by BoxCmp_Init to initialise the operator table. */
void BoxCmp_Init__Operators(BoxCmp *c) {
  int i;

  for(i = 0; i < ASTUNOP__NUM_OPS; i++)
    Operator_Init(BoxCmp_UnOp_Get(c, i));

  for(i = 0; i < ASTBINOP__NUM_OPS; i++)
    Operator_Init(BoxCmp_BinOp_Get(c, i));
}

/** INTERNAL: Called by BoxCmp_Finish to finalise the operator table. */
void BoxCmp_Finish__Operators(BoxCmp *c) {
  int i;

  for(i = 0; i < ASTUNOP__NUM_OPS; i++)
    Operator_Finish(BoxCmp_UnOp_Get(c, i));

  for(i = 0; i < ASTBINOP__NUM_OPS; i++)
    Operator_Finish(BoxCmp_BinOp_Get(c, i));
}


#if 0
void Cmp_Operator_Destroy(Operator *opr) {
  Operation *opn = opr->opn_chain;
  for(; opn != (Operation *) NULL;) {
    Operation *next = opn->next;
    BoxMem_Free(opn);
    opn = next;
  }

  BoxMem_Free(opr);
}

/* Aggiunge una nuova operazione di tipo type1 opr type2
 * all'operatore *opr. Se type1 o type2 sono uguali a TYPE_NONE si tratta
 * di un'operazione unaria (sinistra o destra rispettivamente).
 */
Operation *Cmp_Operation_Add(Operator *opr, Int type1, Int type2, Int typer) {
  Operation *opn;
  Int aa, t, t1, t2;
  int is_privileged;

#if 0
  printf("Adding operation (%s, %s) to operator '%s'\n",
   Tym_Type_Names(type1), Tym_Type_Names(type2), opr->name);
#endif
  opn = (Operation *) BoxMem_Alloc(sizeof(Operation));
  if ( opn == NULL ) {
    MSG_ERROR("Memory request failed.");
    return NULL;
  }

  /* Is it a privileged operation or not? */
  aa = (type1 == TYPE_NONE) + ((type2 == TYPE_NONE) << 1);
  switch (aa) {
  case 0:
    t = t1 = Tym_Type_Resolve_All(type1);
    t2     = Tym_Type_Resolve_All(type2);
    is_privileged = ( (t1 == t2) && (t <= CMP_PRIVILEGED) );
    break;
  case 1:
    t = Tym_Type_Resolve_All(type2);
    is_privileged = (t <= CMP_PRIVILEGED);
    break;
  case 2:
    t = Tym_Type_Resolve_All(type1);
    is_privileged = (t <= CMP_PRIVILEGED);
    break;
  default:
    MSG_ERROR("Operazione fra tipi nulli!");
    return NULL;
    break;
  }

#ifdef USE_PRIVILEGED
  /* Aggiungo l'operazione all'operatore: un'operazione privilegiata
   * viene inserita non solo nella catena delle operazioni, ma pure
   * in un'array che ne permette il ritrovamento in maniera immediata.
   */
  if ( is_privileged ) {
    /* In tal caso l'operazione e' privilegiata: i collegamenti
     * all'operatore sono diretti e quindi piu' veloci!
     */
    opr->opn[aa][t] = opn;
  }
#endif

  /* Creo un collegamento di tipo "a catena"
   */
  opn->next = opr->opn_chain;
  opr->opn_chain = opn;

  opn->type1 = type1;
  opn->type2 = type2;
  opn->type_rs = typer;
  return opn;
}

/* DESCRIPTION: Finds the unary or binary operation associated with
 *  the operator *opr.
 *  If type1 and type2 are both different from TYPE_NONE, then this function
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
Operation *Cmp_Operation_Find(Operator *opr,
                              Type type1, Type type2, Type typer,
                              OpnInfo *oi)
{

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

  /* Is it a privileged operation or not? */
  if ( ! check_rs ) {
    Int aa;
    int is_privileged;

    aa = no_check_arg1 | (no_check_arg2 << 1);
    switch (aa) {
    case 0:
      type = type1 = Tym_Type_Resolve_All(type1);
      type2 = Tym_Type_Resolve_All(type2);
      is_privileged = ( (type1 == type2) && (type <= CMP_PRIVILEGED) );
      break;
    case 1:
      type = Tym_Type_Resolve_All(type2);
      is_privileged = (type <= CMP_PRIVILEGED);
      break;
    case 2:
      type = Tym_Type_Resolve_All(type1);
      is_privileged = (type <= CMP_PRIVILEGED);
      break;
    default:
      MSG_ERROR("Operazione fra tipi nulli!");
      return NULL;
      break;
    }

#ifdef USE_PRIVILEGED
    if ( is_privileged ) {
      opn = opr->opn[aa][type];
      if (opn != NULL) {
        if (oi == NULL) return opn;
        oi->commute = 0;
        oi->expand1 = 0;
        oi->expand2 = 0;
        return opn;
      }
    }
#endif
  }

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
}



























/* DESCRIPTION: Finds the unary or binary operation associated with
 *  the operator *opr.
 *  If type1 and type2 are both different from TYPE_NONE, then this function
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
Operation *Cmp_Operation_Find(Operator *opr,
                              Type type1, Type type2, Type typer,
                              OpnInfo *oi)
{

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

  /* Is it a privileged operation or not? */
  if ( ! check_rs ) {
    Int aa;
    int is_privileged;

    aa = no_check_arg1 | (no_check_arg2 << 1);
    switch (aa) {
    case 0:
      type = type1 = Tym_Type_Resolve_All(type1);
      type2 = Tym_Type_Resolve_All(type2);
      is_privileged = ( (type1 == type2) && (type <= CMP_PRIVILEGED) );
      break;
    case 1:
      type = Tym_Type_Resolve_All(type2);
      is_privileged = (type <= CMP_PRIVILEGED);
      break;
    case 2:
      type = Tym_Type_Resolve_All(type1);
      is_privileged = (type <= CMP_PRIVILEGED);
      break;
    default:
      MSG_ERROR("Operazione fra tipi nulli!");
      return NULL;
      break;
    }

#ifdef USE_PRIVILEGED
    if ( is_privileged ) {
      opn = opr->opn[aa][type];
      if (opn != NULL) {
        if (oi == NULL) return opn;
        oi->commute = 0;
        oi->expand1 = 0;
        oi->expand2 = 0;
        return opn;
      }
    }
#endif
  }

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
}

/** Compiles an operation between the two expression e1 and e2, where
 * the operator is opr.
 */
Expr *Opr_Emit_BinOp(ASTBinOp op, Expr *e1, Expr *e2) {
  Operation *opn;
  OpnInfo oi;

  Expr_Resolve_Subtype(e1);
  Expr_Resolve_Subtype(e2);

  if (!e1->is.value || !e2->is.value) {
    if (!e1->is.value) {
      MSG_ERROR("The expression on the left of '%s' "
                "must have a definite value!", ASTBinOp_To_String(op));
    } else {
      MSG_ERROR("The expression on the right of '%s' "
                "must have a definite value!", ASTBinOp_To_String(op));
    }
    return NULL;
  }

  /* Prima cerco se esiste un'operazione fra i tipi di e1 e e2 */
  opn = Cmp_Operation_Find(opr, e1type, e2type, TYPE_NONE, & oi);
  if (opn != NULL) {
    /* Ora eseguo le espansioni, se necessario */
    if (oi.expand1) {
      if IS_FAILED(Cmp_Expr_Expand(oi.exp_type1, e1)) return NULL;
    }
    if (oi.expand2) {
      if IS_FAILED(Cmp_Expr_Expand(oi.exp_type2, e2)) return NULL;
    }

    /* manca la valutazione della commutativita'! */

    return Cmp_Operation_Exec(opn, e1, e2);

  } else {
    if (strcmp(opr->name, "=") != 0)
      goto Exec_Opn_Error;

    if IS_FAILED( Cmp_Expr_Expand(e1->type, e2) )
      return NULL;

    if IS_FAILED(Expr_Move(e1, e2))
      return NULL;

    return e1;
  }

Exec_Opn_Error:
  if ( num_arg == 1 ) {
    if ( e1type != TYPE_NONE ) {
        MSG_ERROR("%s %s <-- Operation has not been defined!",
         opr->name, Tym_Type_Name(e1->type) );
    } else {
        MSG_ERROR("%s %s <-- Operation has not been defined!",
         Tym_Type_Name(e2->type), opr->name );
    }
    return NULL;

  } else {
    MSG_ERROR("%s %s %s <-- Operation has not been defined!",
     Tym_Type_Names(e1->type), opr->name, Tym_Type_Names(e2->type) );
    return NULL;
  }
}
#endif
