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
#include "value.h"
#include "messages.h"
#include "new_compiler.h"
#include "virtmach.h"
#include "cmpproc.h"
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
  if (opr->first_operation != NULL)
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
    Operator_Init(opr, ASTUnOp_To_String(i));
    Operator_Attr_Set(opr, OPR_ATTR_BINARY, 0);
  }

  for(i = 0; i < ASTBINOP__NUM_OPS; i++) {
    Operator *opr = BoxCmp_BinOp_Get(c, i);
    Operator_Init(opr, ASTBinOp_To_String(i));
    Operator_Attr_Set(opr, OPR_ATTR_BINARY, OPR_ATTR_BINARY);
  }

  if (1) { /* Temporary code */
    Operator *opr;
    Operation *opn;

    opr = BoxCmp_BinOp_Get(c, ASTBINOP_ADD);

    opn = Operator_Add_Opn(opr, BOXTYPE_INT, BOXTYPE_INT, BOXTYPE_INT);
    opn->implem.opcode = BOXGOP_ADD;

    opn = Operator_Add_Opn(opr, BOXTYPE_REAL, BOXTYPE_REAL, BOXTYPE_REAL);
    opn->implem.opcode = BOXGOP_ADD;

    opr = BoxCmp_BinOp_Get(c, ASTBINOP_SUB);

    opn = Operator_Add_Opn(opr, BOXTYPE_INT, BOXTYPE_INT, BOXTYPE_INT);
    opn->implem.opcode = BOXGOP_SUB;

    opn = Operator_Add_Opn(opr, BOXTYPE_REAL, BOXTYPE_REAL, BOXTYPE_REAL);
    opn->implem.opcode = BOXGOP_SUB;

    opr = BoxCmp_BinOp_Get(c, ASTBINOP_MUL);

    opn = Operator_Add_Opn(opr, BOXTYPE_INT, BOXTYPE_INT, BOXTYPE_INT);
    opn->implem.opcode = BOXGOP_MUL;

    opn = Operator_Add_Opn(opr, BOXTYPE_REAL, BOXTYPE_REAL, BOXTYPE_REAL);
    opn->implem.opcode = BOXGOP_MUL;
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
}

#if 1

/*
  Functions required by the following code:

    Tym_Type_Resolve_All(opn->type_rs);

    Cmp_Complete_Ptr_1(& old_e2)
    Cmp_Complete_Ptr_2(e1, e2)

    Cmp_Expr_Force_To_LReg(e2)
    Cmp_Expr_To_LReg(e2)
    Cmp_Expr_To_X(e2, CAT_LREG, 0, 1)
    Cmp_Expr_Reg0_To_LReg(opn->type_rs)
    Cmp_Expr_Destroy_Tmp(e2)

    Cmp_Assemble(opn->asm_code, old_e2.categ, old_e2.value.i)
*/

/* DESCRIZIONE: Questa funzione gestisce la generazione del codice
 *  corrispondente ad un gran numero di operazioni intrinseche.
 * NOTA: Viene chiamata da Cmp_Operation_Exec.
 */

void Value_Make_Temp(BoxCmp *c, Value *v);

/** This is the function which actually emits the VM code for quite a number
 * of Operations.
 */
static Value *My_Opn_Emit(BoxCmp *c, Operation *opn,
                          Value *v_left, Value *v_right) {
  Value_Make_Temp(c, v_left);
  CmpProc_Assemble(c->cur_proc, opn->implem.opcode,
                   2, & v_left->cont, & v_right->cont);
  Value_Link(v_left);
  return v_left;

#if 0
  struct {
    unsigned int unary     :1,
                 right     :1,
                 monotype  :1,
                 immediate :1;} opn_is;

  /* Usually the operands and the result all have the same type. If this is
   * the case, then we set opn_is.monotype to 1. If this is not the case
   * then we set opn_is.monotype to 0.
   */
  switch((e1 == NULL) + ((e2 == NULL) << 1)) {
  case 0: /* Operazione a 2 argomenti */
    opn_is.monotype = (rs_resolved != e1->resolved)
                      || (rs_resolved != e2->resolved);
    opn_is.immediate = (e1->categ == CAT_IMM) && (e2->categ == CAT_IMM);
    opn_is.unary = 0;
    opn_is.right = 0;
    break;

  case 1: /* Operazione a 1 argomento (destro, cioe' e2) */
    opn_is.strange = (e2->resolved != rs_resolved);
    opn_is.immediate = (e2->categ == CAT_IMM);
    opn_is.unary = 1;
    opn_is.right = 1;
    e1 = e2;
    break;

  case 2: /* Operazione a 1 argomento (sinistro, cioe' e1) */
    opn_is.strange = (e1->resolved != rs_resolved);
    opn_is.immediate = (e1->categ == CAT_IMM);
    opn_is.unary = 1;
    opn_is.right = 0;
    e2 = e1;
    break;

  default:
    MSG_FATAL("Internal error: operation has no arguments!");
    return NULL;
  }

  return NULL;

  Int rs_resolved = Tym_Type_Resolve_All(opn->type_rs);


  if ( opn->is.assignment ) { /***********************************ASSIGNMENT*/
    if ( opn_is.strange ) {
      MSG_ERROR("Errore interno: operazione di assegnazione anomala!");
      return NULL;
    }

    /* Mi assicuro che il primo argomento possa fungere da target
     * dell'assegazione!
     */
    if ( ! e1->is.target ) {
      MSG_ERROR("This expression of type %~s cannot be modified!",
                TS_Name_Get(cmp->ts, e1->type));
      return NULL;
    }

    if ( opn_is.unary ) {
      /* Ora compilo l'operazione */
      if ( opn_is.right ) {
        Expr old_e2 = *e2;
        /*e2value = e2->value.i, e2categ = e2->categ;*/
        if IS_FAILED( Cmp_Expr_Force_To_LReg(e2) ) return NULL;
        if IS_FAILED( Cmp_Complete_Ptr_1(& old_e2) ) return NULL;
        Cmp_Assemble(opn->asm_code, old_e2.categ, old_e2.value.i);
        return e2;
      } else {
        if IS_FAILED( Cmp_Complete_Ptr_1(e1) ) return NULL;
        Cmp_Assemble(opn->asm_code, e1->categ, e1->value.i);
        return e1;
      }
    } else {
      /* Se necessario, converto e2 in registro locale. */
      if IS_FAILED( Cmp_Expr_To_LReg(e2) ) return NULL;

      /* Ora compilo l'operazione */
      if IS_FAILED( Cmp_Complete_Ptr_2(e1, e2) ) return NULL;
      Cmp_Assemble(opn->asm_code,
                   e1->categ, e1->value.i, e2->categ, e2->value.i);

      /* Ora libero il registro che non contiene il risultato! */
      Cmp_Expr_Destroy_Tmp(e2);
      return e1;
    }

  } else { /**************************************************NOT ASSIGNMENT*/
    struct {unsigned int e1 : 1, e2 : 1;} result_in;

    /* Determino se e1 puo' contenere il risultato dell'operazione
     * (il quale e' una espressione temporanea).
     * A tale scopo bisogna che e1 abbia lo stesso tipo del risultato,
     * che sia un registro locale e che non sia una variabile (cioe' un
     * target per l'operazione di assegnazione).
     */
    result_in.e1 = (e1->resolved == rs_resolved)
     && (e1->categ == CAT_LREG) && (! e1->is.target);

    if ( opn_is.strange ) { /**************************************STRANGE*/
      /* Le operazioni strane sono, ad esempio: il confronto fra numeri
       * (operatori <, >, ==, ...), moltiplicazione fra un punto e
       * un reale, etc.
       */
      if ( opn_is.unary ) {
        MSG_ERROR("Still not implemented!");
        return NULL;

      } else {
        /* Se l'operazione e' binaria ho 3 casi a seconda dei tipi
        * delle 3 quantita' coinvolte dall'operazione (2 argomenti +
        * 1 risultato):
        */
        struct {unsigned int er_e1 : 1, er_e2 : 1, e1_e2 : 1;} eq;
        eq.e1_e2 = (e1->resolved == e2->resolved);
        eq.er_e1 = (rs_resolved == e1->resolved);
        eq.er_e2 = (rs_resolved == e2->resolved);
        if ( eq.e1_e2 ) {
          /* caso 1: argomenti dello stesso tipo, ma risultato
           *  di tipo diverso (caso di <, <=, >, >=, ==, !=, ...)
           */
          if ( (Cmp_Expr_To_LReg(e1) == Failed)
            || (Cmp_Expr_To_LReg(e2) == Failed) ) return NULL;
          if IS_FAILED( Cmp_Complete_Ptr_2(e1, e2) ) return NULL;
          Cmp_Assemble(opn->asm_code,
           e1->categ, e1->value.i, e2->categ, e2->value.i);
          Cmp_Expr_Destroy_Tmp(e1);
          Cmp_Expr_Destroy_Tmp(e2);
          return Cmp_Expr_Reg0_To_LReg(opn->type_rs);

        } else {
          if ( eq.er_e2 ) {
            Expr *tmp; tmp = e1; e1 = e2; e2 = tmp;
            goto er_equal_e1;
          }

          if ( eq.er_e1 ) { /* caso 2: un argomento e' dello stesso tipo
                                del risultato */
er_equal_e1:
            if IS_FAILED( Cmp_Expr_Force_To_LReg(e1) ) return NULL;
            if IS_FAILED( Cmp_Expr_To_X(e2, CAT_LREG, 0, 1) ) return NULL;
            if IS_FAILED( Cmp_Complete_Ptr_1(e1) ) return NULL;
            Cmp_Assemble(opn->asm_code, e1->categ, e1->value.i);
            return e1;

          } else { /* caso 3: tipi tutti diversi */
            MSG_ERROR("Still not implemented!");
            return NULL;
          }
        }
      }
      return NULL;

    } else { /*************************************************NOT STRANGE*/
      if ( opn_is.unary ) {
        if ( opn_is.right ) e1 = e2;
        if IS_FAILED( Cmp_Expr_Force_To_LReg(e1) ) return NULL;
        if IS_FAILED( Cmp_Complete_Ptr_1(e1) ) return NULL;
        Cmp_Assemble(opn->asm_code, e1->categ, e1->value.i);
        return e1;

      } else {
        result_in.e2 = (e2->resolved == rs_resolved)
         && (e2->categ == CAT_LREG) && (! e2->is.target);

        /* Se l'operazione e' commutativa, posso usare e2 per contenere
         * il risultato dell'operazione!
         */
        if ( result_in.e2 && !result_in.e1 && opn->is.commutative ) {
          register Expr *tmp = e2;
          e2 = e1; e1 = tmp;
          result_in.e1 = 1;
          result_in.e2 = 0;
        }

        /* Se non posso mettere il risultato in e1, allora devo
         * convertire e1 in un registro locale (che puo' contenerlo!)
         */
        if ( ! result_in.e1 ) {
          if IS_FAILED( Cmp_Expr_Force_To_LReg(e1) ) return NULL;
        }

        /* Se necessario, converto e2 in registro locale. */
        if IS_FAILED( Cmp_Expr_To_LReg(e2) ) return NULL;

        /* Ora compilo l'operazione */
        if IS_FAILED( Cmp_Complete_Ptr_2(e1, e2) ) return NULL;
        Cmp_Assemble(opn->asm_code,
                     e1->categ, e1->value.i, e2->categ, e2->value.i);

        /* Ora libero il registro che non contiene il risultato! */
        Cmp_Expr_Destroy_Tmp(e2);
        return e1;
      }
    }
  }
#endif

}
#endif

/** Compiles an operation between the two expression e1 and e2, where
 * the operator is opr.
 */
Value *BoxCmp_Opr_Emit_BinOp(BoxCmp *c, ASTBinOp op,
                             Value *v_left, Value *v_right) {
  Operator *opr = BoxCmp_BinOp_Get(c, op);
  Operation *opn;
  OprMatch match;

#if 0
  /* Require operands have value. */
  if (!v_left->is.value || !v_right->is.value) {
    if (!v_left->is.value) {
      MSG_ERROR("The expression on the left of '%s' "
                "must have a definite value!", opr->name);
    } else {
      MSG_ERROR("The expression on the right of '%s' "
                "must have a definite value!", opr->name);
    }
    return NULL;
  }

  /* Subtypes cannot be used for operator overloading, so we expand
   * them anyway!
   */
  Expr_Resolve_Subtype(v_left);
  Expr_Resolve_Subtype(v_right);
#endif

  /* Now we search the operation */
  opn = BoxCmp_Operator_Find_Opn(c, opr, & match,
                                 v_left->type, v_right->type);
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

    return My_Opn_Emit(c, opn, v_left, v_right);

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
    MSG_ERROR("%~s %s %~s <-- Operation has not been defined!",
              TS_Name_Get(& c->ts, v_left->type), opr->name,
              TS_Name_Get(& c->ts, v_right->type));
    return NULL;
  }

}
