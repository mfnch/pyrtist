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
#include "compiler.h"
#include "vm_priv.h"
#include "vmcode.h"
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
void Operator_Init(Operator *opr, BoxCmp *c, const char *name) {
  opr->cmp = c;
  opr->attr = 0;
  opr->name = name;
  opr->first_operation = NULL;
}

void Operator_Finish(Operator *opr) {
  Operation *opn = opr->first_operation;
  while (opn != NULL) { /* Destroy the chain of operations */
    Operation *this = opn;
    opn = opn->next;
    Box_Mem_Free(this);
  }
}

/** Guess the operation assembly scheme from the arguments and the other
 * attributes of the operation.
 */
static void My_Guess_AsmScheme(Operation *opn) {
  if (opn->attr & OPR_ATTR_NATIVE) {
    if (opn->attr & OPR_ATTR_BINARY) {
      TS *ts = & opn->opr->cmp->ts;
      BoxTypeId t_result = TS_Get_Core_Type(ts, opn->type_result),
              t_left = TS_Get_Core_Type(ts, opn->type_left),
              t_right = TS_Get_Core_Type(ts, opn->type_right);
      int res_eq_l = (t_result == t_left),
          res_eq_r = (t_result == t_right),
          l_eq_r = (t_left == t_right);
      if (l_eq_r && res_eq_l)
        opn->asm_scheme = OPASMSCHEME_STD_BIN;

      else if (l_eq_r)               /* and, implicitly, res_eq_l == 0 */
        opn->asm_scheme = OPASMSCHEME_R_LR_BIN;

      else if (res_eq_l || res_eq_r) /* and, implicitly, l_eq_r == 0 */
        opn->asm_scheme = OPASMSCHEME_RL_R_BIN;

      else {
        MSG_FATAL("Unknow operation scheme in My_Guess_AsmScheme");
        assert(0);
      }

    } else
      opn->asm_scheme = (opn->attr & OPR_ATTR_UN_RIGHT) != 0 ?
                        OPASMSCHEME_RIGHT_UN : OPASMSCHEME_STD_UN;

  } else {
    opn->asm_scheme = OPASMSCHEME_UNKNOWN;
  }
}

/** Change attributes for operator. 'mask' tells what attributes to change,
 * 'value' tells how to change them.
 */
void Operator_Attr_Set(Operator *opr, OprAttr mask, OprAttr attr) {
  opr->attr = (opr->attr & (~mask)) | (attr & mask);
}

/** Change attributes for operation. 'mask' tells what attributes to change,
 * 'value' tells how to change them.
 */
void Operation_Attr_Set(Operation *opn, OprAttr mask, OprAttr attr) {
  opn->attr = (opn->attr & (~mask)) | (attr & mask);
  My_Guess_AsmScheme(opn);
}

void Operation_Set_User_Implem(Operation *opn, BoxVMCallNum call_num) {
  opn->asm_scheme = OPASMSCHEME_USR_UN;
  opn->implem.call_num = call_num;
}

/* Aggiunge una nuova operazione di tipo type1 opr type2
 * all'operatore *opr. Se type1 o type2 sono uguali a TYPE_NONE si tratta
 * di un'operazione unaria (sinistra o destra rispettivamente).
 */
Operation *Operator_Add_Opn(Operator *opr, BoxTypeId type_left,
                            BoxTypeId type_right, BoxTypeId type_result) {
  Operation *opn;

  opn = (Operation *) Box_Mem_Safe_Alloc(sizeof(Operation));
  opn->opr = opr;
  opn->attr = opr->attr;
  opn->type_left = type_left;
  opn->type_right = type_right;
  opn->type_result = type_result;

  /** Link to chain */
  opn->next = opr->first_operation;
  opn->previous = NULL;
  if (opr->first_operation != NULL)
    opr->first_operation->previous = opn;
  opr->first_operation = opn;

  My_Guess_AsmScheme(opn);
  return opn;
}

void Operator_Del_Opn(Operator *opr, Operation *opn) {
  assert(opn->opr == opr);
  if (opn->next != NULL)
    opn->next->previous = opn->previous;
  if (opn->previous != NULL)
    opn->previous->next = opn->next;
  if (opr->first_operation == opn)
    opr->first_operation = opn->next;
  Box_Mem_Free(opn);
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
    OprAttr attr;
    Operator *opr = BoxCmp_UnOp_Get(c, i);
    Operator_Init(opr, c, ASTUnOp_To_String(i));
    attr = OPR_ATTR_NATIVE |
           (ASTUnOp_Is_Right(i) ? OPR_ATTR_UN_RIGHT : 0);
    Operator_Attr_Set(opr, OPR_ATTR_ALL, attr);
  }

  for(i = 0; i < ASTBINOP__NUM_OPS; i++) {
    Operator *opr = BoxCmp_BinOp_Get(c, i);
    Operator_Init(opr, c, ASTBinOp_To_String(i));
    Operator_Attr_Set(opr, OPR_ATTR_ALL, OPR_ATTR_BINARY | OPR_ATTR_NATIVE);
  }

  Operator_Init(& c->convert, c, "(->)");
  Operator_Attr_Set(& c->convert, OPR_ATTR_ALL,
                    OPR_ATTR_NATIVE | OPR_ATTR_MATCH_RESULT);
}

/** INTERNAL: Called by BoxCmp_Finish to finalise the operator table. */
void BoxCmp_Finish__Operators(BoxCmp *c) {
  int i;

  for(i = 0; i < ASTUNOP__NUM_OPS; i++)
    Operator_Finish(BoxCmp_UnOp_Get(c, i));

  for(i = 0; i < ASTBINOP__NUM_OPS; i++)
    Operator_Finish(BoxCmp_BinOp_Get(c, i));

  Operator_Finish(& c->convert);
}


/** Finds the unary or binary operation associated with the operator *opr.
 *  If type1 and type2 are both different from BOXTYPEID_NONE, then the function
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
                                    BoxTypeId type_left, BoxTypeId type_right,
                                    BoxTypeId type_result) {
  int opr_is_unary = ((opr->attr & OPR_ATTR_BINARY) == 0),
      do_match_res = ((opr->attr & OPR_ATTR_MATCH_RESULT) != 0);
  Operation *opn;
  for(opn = opr->first_operation; opn != NULL; opn = opn->next) {
    BoxTypeCmp match_left;

    /* Check for matching result, if required */
    if (do_match_res) {
      BoxTypeCmp match_result =
        BoxType_Compare(BoxType_From_Id(& c->ts, opn->type_result),
                        BoxType_From_Id(& c->ts, type_result));
      if (match_result == BOXTYPECMP_DIFFERENT)
        continue;
    }

    match_left = BoxType_Compare(BoxType_From_Id(& c->ts, opn->type_left),
                                 BoxType_From_Id(& c->ts, type_left));
    if (match_left != BOXTYPECMP_DIFFERENT) {
      if (opr_is_unary) {
          match->opr = opr;
          match->attr = opn->attr;
          match->match_left = match_left;
          match->match_right = 0;
          match->expand_type_left = opn->type_left;
          match->expand_type_right = BOXTYPEID_NONE;
          return opn;

      } else {
        BoxTypeCmp match_right =
          BoxType_Compare(BoxType_From_Id(& c->ts, opn->type_right),
                          BoxType_From_Id(& c->ts, type_right));
        if (match_right != BOXTYPECMP_DIFFERENT) {
          match->opr = opr;
          match->attr = opn->attr;
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

/** This is the function which actually emits the VM code for quite a number
 * of Operations.
 * NOTE: this function assumes that the two operands have both type and values
 *  and checks only if the left operand is a target value, if the operation
 *  requires that.
 * REFERENCES: return: new, v_left: ?, v_right: ?;
 */
static Value *My_Opn_Emit(BoxCmp *c, Operation *opn,
                          Value *v_left, Value *v_right) {
  Value *result = NULL;

  switch(opn->asm_scheme) {
  case OPASMSCHEME_STD_UN:
    if (opn->attr & OPR_ATTR_ASSIGNMENT) {
      if (Value_Is_Target(v_left))
        Value_Link(v_left);

      else {
        MSG_ERROR("Unary operator '%s' cannot modify its operand (%s)",
                  opn->opr->name, ValueKind_To_Str(v_left->kind));
        return NULL;
      }

    } else
      v_left = Value_To_Temp(v_left);

    BoxVMCode_Assemble(c->cur_proc, opn->implem.opcode,
                     1, & v_left->value.cont);
    result = v_left;
    break;

  case OPASMSCHEME_RIGHT_UN:
    assert(opn->attr & OPR_ATTR_ASSIGNMENT);

    if (Value_Is_Target(v_left)) {
      Value *v_temp;
      Value_Link(v_left); /* Make sure v_left is not destroyed
                             by Value_To_Temp */
      v_temp = Value_To_Temp(v_left);
      BoxVMCode_Assemble(c->cur_proc, opn->implem.opcode,
                       1, & v_left->value.cont);
      Value_Unlink(v_left); /* We don't need v_left anymore! */
      result = v_temp;
      break;

    } else {
      MSG_ERROR("Unary operator '%s' cannot modify its operand (%s)",
                opn->opr->name, ValueKind_To_Str(v_left->kind));
      return NULL;
    }
    break;

  case OPASMSCHEME_STD_BIN:
    assert((opn->attr & OPR_ATTR_NATIVE) && (opn->attr & OPR_ATTR_BINARY));
    if (opn->attr & OPR_ATTR_ASSIGNMENT) {
      if (Value_Is_Target(v_left))
        Value_Link(v_left);

      else {
        MSG_ERROR("Binary operator '%s' cannot modify its left operand (%s)",
                  opn->opr->name, ValueKind_To_Str(v_left->kind));
        return NULL;
      }

    } else {
      /* If the operation is commutative and v_left is not an intermediate
       * value, then we check if the v_right is a temporary value.
       * If it is, we exchange the operands, since the op is commutative,
       * and this will save us one VM operation.
       */
      if (opn->attr & OPR_ATTR_COMMUTATIVE && !Value_Is_Temp(v_left)) {
        if (Value_Is_Temp(v_right)) {
          Value *v = v_left;
          v_left = v_right;
          v_right = v;
        }
      }

      v_left = Value_To_Temp(v_left);
    }

    BoxVMCode_Assemble(c->cur_proc, opn->implem.opcode,
                     2, & v_left->value.cont, & v_right->value.cont);
    result = v_left;
    break;

  case OPASMSCHEME_R_LR_BIN:
    assert((opn->attr & OPR_ATTR_NATIVE) && (opn->attr & OPR_ATTR_BINARY));

    result = Value_New(c->cur_proc);
    Value_Setup_As_Temp_Old(result, opn->type_result);
    v_left = Value_To_Temp_Or_Target(v_left);
    v_right = Value_To_Temp_Or_Target(v_right);
    BoxVMCode_Assemble(c->cur_proc, opn->implem.opcode,
                     3, & result->value.cont,
                     & v_left->value.cont, & v_right->value.cont);
    Value_Unlink(v_left);
    Value_Unlink(v_right);
    return result;

  case OPASMSCHEME_RL_R_BIN:
    if (BoxType_Compare(BoxType_From_Id(& c->ts, opn->type_result),
                        v_right->type)
        != BOXTYPECMP_DIFFERENT) {
      Value *v_tmp = v_left;
      v_left = v_right;
      v_right = v_tmp;
    }

    v_left = Value_To_Temp(v_left);

    result = Value_New(c->cur_proc);
    Value_Setup_As_Temp_Old(result, opn->type_result);

    BoxVMCode_Assemble(c->cur_proc, opn->implem.opcode,
                     2, & v_left->value.cont, & v_right->value.cont);

    result = v_left;
    break;

  default:
    MSG_FATAL("Non-native operators are not supported, yet!");
    assert(0);
  }

  if (opn->attr & OPR_ATTR_IGNORE_RES) {
    Value_Set_Ignorable(result, 1);
  }
  return result;
}

/** Compiles an operation between the two expression e1 and e2, where
 * the operator is opr.
 * REFERENCES: return: new, v: -1;
 */
Value *BoxCmp_Opr_Emit_UnOp(BoxCmp *c, ASTUnOp op, Value *v) {
  Operator *opr = BoxCmp_UnOp_Get(c, op);
  Operation *opn;
  OprMatch match;
  Value *v_result = NULL;

  /* Subtypes cannot be used for operator overloading, so we expand
   * them anyway!
   */
  v = Value_Expand_Subtype(v);

  /* Now we search the operation */
  opn = BoxCmp_Operator_Find_Opn(c, opr, & match,
                                 BoxType_Get_Id(v->type),
                                 BOXTYPEID_NONE, BOXTYPEID_NONE);
  if (opn != NULL) {
    /* Now we expand the types, if necessary */
    if (match.match_left == TS_TYPES_EXPAND)
      v = Value_Expand(v, BoxType_From_Id(& c->ts, match.expand_type_left));

    v_result = My_Opn_Emit(c, opn, v, v); /* XXX */

  } else {
    if ((opr->attr & OPR_ATTR_UN_RIGHT) != 0) {
      MSG_ERROR("%~s%s <-- Operation has not been defined!",
                BoxType_Get_Repr(v->type), opr->name);
      return NULL;

    } else {
      MSG_ERROR("%s%~s <-- Operation has not been defined!",
                opr->name, BoxType_Get_Id(v->type));
    }
  }

  Value_Unlink(v);
  return v_result;
}

/** Compiles an operation between the two expression e1 and e2, where
 * the operator is opr.
 * REFERENCES: return: new, v_left: -1, v_right: -1;
 */
Value *BoxCmp_Opr_Emit_BinOp(BoxCmp *c, ASTBinOp op,
                             Value *v_left, Value *v_right) {
  Operator *opr = BoxCmp_BinOp_Get(c, op);
  Operation *opn;
  OprMatch match;
  Value *v_result = NULL;

  /* Subtypes cannot be used for operator overloading, so we expand
   * them anyway!
   */
  v_left = Value_Expand_Subtype(v_left);
  v_right = Value_Expand_Subtype(v_right);

  /* Now we search the operation */
  opn = BoxCmp_Operator_Find_Opn(c, opr, & match,
                                 BoxType_Get_Id(v_left->type),
                                 BoxType_Get_Id(v_right->type),
                                 BOXTYPEID_NONE);
  if (opn != NULL) {
    /* Now we expand the types, if necessary */
    if (match.match_left == TS_TYPES_EXPAND)
      v_left = Value_Expand(v_left, BoxType_From_Id(& c->ts, match.expand_type_left));

    if (match.match_right == TS_TYPES_EXPAND)
      v_right = Value_Expand(v_right, BoxType_From_Id(& c->ts, match.expand_type_right));

    /* XXX */
    v_result = My_Opn_Emit(c, opn, v_left, v_right);

  } else {
    MSG_ERROR("%~s %s %~s <-- Operation has not been defined!",
              BoxType_Get_Repr(v_left->type), opr->name,
              BoxType_Get_Repr(v_right->type));
  }

  Value_Unlink(v_left);
  Value_Unlink(v_right);
  return v_result;
}

/** Map an object 'src' to an object 'dest' with (possibly) different type.
 * REFERENCES: return: new, dest: -1, src: -1;
 */
BoxTask BoxCmp_Opr_Try_Emit_Conversion(BoxCmp *c, Value *dest, Value *src) {
  Operation *opn;
  OprMatch match;

#if 0
  /* Subtypes cannot be used for operator overloading, so we expand
   * them anyway!
   */
  Expr_Resolve_Subtype(v);
#endif

  /* Now we search the operation */
  opn = BoxCmp_Operator_Find_Opn(c, & c->convert, & match,
                                 BoxType_Get_Id(src->type),
                                 BOXTYPEID_NONE, BoxType_Get_Id(dest->type));

  if (opn != NULL) {
    /* Now we expand the types, if necessary */
    if (match.match_left == TS_TYPES_EXPAND)
      src = Value_Expand(src, BoxType_From_Id(& c->ts, match.expand_type_left));

    if (opn->asm_scheme == OPASMSCHEME_STD_UN) {
      BoxVMCode_Assemble(c->cur_proc, opn->implem.opcode,
                       2, & dest->value.cont, & src->value.cont);
      Value_Unlink(src);
      Value_Unlink(dest);
      return BOXTASK_OK;

    } else if (opn->asm_scheme == OPASMSCHEME_USR_UN) {
      Value_Emit_Call_From_Call_Num(opn->implem.call_num, dest, src);
      Value_Unlink(src);
      Value_Unlink(dest);
      return BOXTASK_OK;

    } else {
      MSG_FATAL("BoxCmp_Opr_Emit_Conversion: unexpected asm-scheme!");
      assert(0);
    }

  } else {
    Value_Unlink(src);
    Value_Unlink(dest);
    return BOXTASK_FAILURE;
  }
}

/** Emits the conversion from the source expression 'v', to the given type 't'
 * REFERENCES: return: new, src: -1;
 */
Value *BoxCmp_Opr_Emit_Conversion(BoxCmp *c, Value *src, BoxType *t_dest) {
  Value *v_dest = Value_New(c->cur_proc);
  Value_Setup_As_Temp(v_dest, t_dest);
  Value_Link(v_dest); /* We want to return a new reference! */
  if (BoxCmp_Opr_Try_Emit_Conversion(c, v_dest, src) == BOXTASK_OK)
    return v_dest;

  else {
    MSG_ERROR("Don't know how to convert objects of type %~s to %~s.",
              BoxType_Get_Repr(src->type), BoxType_Get_Repr(t_dest));
    Value_Unlink(v_dest); /* Unlink, since we are not returning it! */
    return NULL;
  }
}
