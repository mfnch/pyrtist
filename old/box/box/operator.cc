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
#include "vm_priv.h"
#include "vmcode.h"
#include "operator.h"

#include "compiler_priv.h"


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
  opr->attr = OPR_ATTR_INVALID;
  opr->name = name;
  opr->first_operation = NULL;
}

void Operator_Finish(Operator *opr) {
  Operation *opn = opr->first_operation;
  while (opn != NULL) { /* Destroy the chain of operations */
    Operation *cur = opn;
    opn = opn->next;
    Box_Mem_Free(cur);
  }
}

/** Guess the operation assembly scheme from the arguments and the other
 * attributes of the operation.
 */
static void My_Guess_AsmScheme(Operation *opn) {
  if (opn->attr & OPR_ATTR_NATIVE) {
    if (opn->attr & OPR_ATTR_BINARY) {
      BoxType *t_result = opn->type_result,
              *t_left = opn->type_left,
              *t_right = opn->type_right;
      BoxBool
        res_eq_l = BoxType_Compare(t_result, t_left) >= BOXTYPECMP_EQUAL,
        res_eq_r = BoxType_Compare(t_result, t_right) >= BOXTYPECMP_EQUAL,
        l_eq_r = BoxType_Compare(t_left, t_right) >= BOXTYPECMP_EQUAL;
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
      opn->asm_scheme = ((opn->attr & OPR_ATTR_UN_RIGHT) != 0 ?
                         OPASMSCHEME_RIGHT_UN : OPASMSCHEME_STD_UN);

  } else {
    opn->asm_scheme = OPASMSCHEME_UNKNOWN;
  }
}

/** Change attributes for operator. 'mask' tells what attributes to change,
 * 'value' tells how to change them.
 */
void Operator_Attr_Set(Operator *opr, OprAttr mask, OprAttr attr) {
  opr->attr = (OprAttr) ((opr->attr & (~mask)) | (attr & mask));
}

/** Change attributes for operation. 'mask' tells what attributes to change,
 * 'value' tells how to change them.
 */
void Operation_Attr_Set(Operation *opn, OprAttr mask, OprAttr attr) {
  opn->attr = (OprAttr) ((opn->attr & (~mask)) | (attr & mask));
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
Operation *Operator_Add_Opn(Operator *opr, BoxType *type_left,
                            BoxType *type_right, BoxType *type_result) {
  Operation *opn;

  opn = (Operation *) Box_Mem_Safe_Alloc(sizeof(Operation));
  opn->opr = opr;
  opn->attr = opr->attr;
  opn->type_left = BoxType_Link(type_left);
  opn->type_right = BoxType_Link(type_right);
  opn->type_result = BoxType_Link(type_result);

  /* Link to chain */
  opn->next = opr->first_operation;
  opn->previous = NULL;
  if (opr->first_operation)
    opr->first_operation->previous = opn;
  opr->first_operation = opn;

  My_Guess_AsmScheme(opn);
  return opn;
}

void Operator_Del_Opn(Operator *opr, Operation *opn) {
  assert(opn->opr == opr);
  if (opn->next)
    opn->next->previous = opn->previous;
  if (opn->previous)
    opn->previous->next = opn->next;
  if (opr->first_operation == opn)
    opr->first_operation = opn->next;
  (void) BoxType_Unlink(opn->type_left);
  (void) BoxType_Unlink(opn->type_right);
  (void) BoxType_Unlink(opn->type_result);
  Box_Mem_Free(opn);
}

namespace Box {

void
Compiler::Init_Operators()
{
  int i;

  for(i = 0; i < BOXASTUNOP_NUM_OPS; i++) {
    BoxASTUnOp un_op = (BoxASTUnOp) i;
    Operator *opr = (Operator *) Get_Un_Op(un_op);
    Operator_Init(opr, BoxASTUnOp_To_String(un_op));
    OprAttr attr = (OprAttr) (OPR_ATTR_NATIVE |
                              (BoxASTUnOp_Is_Right(un_op) ?
                               OPR_ATTR_UN_RIGHT : 0));
    Operator_Attr_Set(opr, OPR_ATTR_ALL, attr);
  }

  for(i = 0; i < BOXASTBINOP_NUM_OPS; i++) {
    BoxASTBinOp bin_op = (BoxASTBinOp) i;
    Operator *opr = Get_Bin_Op(bin_op);
    Operator_Init(opr, BoxASTBinOp_To_String(bin_op));
    Operator_Attr_Set(opr, OPR_ATTR_ALL,
                      (OprAttr) (OPR_ATTR_BINARY | OPR_ATTR_NATIVE));
  }

  Operator_Init(& convert_, "(->)");
  Operator_Attr_Set(& convert_, OPR_ATTR_ALL,
                    (OprAttr) (OPR_ATTR_NATIVE | OPR_ATTR_MATCH_RESULT));
}

void
Compiler::Finish_Operators()
{
  int i;

  for(i = 0; i < BOXASTUNOP_NUM_OPS; i++)
    Operator_Finish(Get_Un_Op((BoxASTUnOp) i));

  for(i = 0; i < BOXASTBINOP_NUM_OPS; i++)
    Operator_Finish(Get_Bin_Op((BoxASTBinOp) i));

  Operator_Finish(& convert_);
}

}

/// @brief Details about a operation found by My_Operator_Find_Opn().
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
static Operation *
My_Operator_Find_Opn(Operator *opr, OprMatch *match,
                     BoxType *type_left, BoxType *type_right,
                     BoxType *type_result)
{
  int opr_is_unary = ((opr->attr & OPR_ATTR_BINARY) == 0),
      do_match_res = ((opr->attr & OPR_ATTR_MATCH_RESULT) != 0);
  Operation *opn;
  for(opn = opr->first_operation; opn != NULL; opn = opn->next) {
    BoxTypeCmp match_left;

    /* Check for matching result, if required */
    if (do_match_res) {
      BoxTypeCmp match_result = BoxType_Compare(opn->type_result, type_result);
      if (match_result == BOXTYPECMP_DIFFERENT)
        continue;
    }

    match_left = BoxType_Compare(opn->type_left, type_left);
    if (match_left != BOXTYPECMP_DIFFERENT) {
      if (opr_is_unary) {
          match->opr = opr;
          match->attr = opn->attr;
          match->match_left = match_left;
          match->match_right = BOXTYPECMP_DIFFERENT;
          match->expand_type_left = opn->type_left;
          match->expand_type_right = NULL;
          return opn;

      } else {
        BoxTypeCmp match_right = BoxType_Compare(opn->type_right, type_right);
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

namespace Box {

/** This is the function which actually emits the VM code for quite a number
 * of Operations.
 * NOTE: this function assumes that the two operands have both type and values
 *  and checks only if the left operand is a target value, if the operation
 *  requires that.
 * REFERENCES: return: new, v_left: ?, v_right: ?;
 */
Value *
Compiler::Emit_Operation(Operation *opn, Value *v_left, Value *v_right)
{
  Value *result = NULL;

  switch(opn->asm_scheme) {
  case OPASMSCHEME_STD_UN:
    if (!(opn->attr & OPR_ATTR_ASSIGNMENT))
      Emit_Make_Temp(v_left, v_left);
    else {
      if (!Is_Target(v_left)) {
        LOG_ERR("Unary operator '%s' cannot modify its operand (%s)",
                opn->opr->name, ValueKind_To_Str(v_left->kind));
        return NULL;
      }
    }

    Append_LIR1(opn->implem.opcode, & v_left->cont);
    result = v_left;
    break;

  case OPASMSCHEME_RIGHT_UN:
    assert(opn->attr & OPR_ATTR_ASSIGNMENT);

    if (Is_Target(v_left)) {
      BoxCont cont = v_left->cont;
      Emit_Make_Temp(v_left, v_left);
      Append_LIR1(opn->implem.opcode, & cont);
      result = v_left;
      break;
    } else {
      LOG_ERR("Unary operator '%s' cannot modify its operand (%s)",
              opn->opr->name, ValueKind_To_Str(v_left->kind));
      return NULL;
    }
    break;

  case OPASMSCHEME_STD_BIN:
    assert((opn->attr & OPR_ATTR_NATIVE) && (opn->attr & OPR_ATTR_BINARY));
    if (opn->attr & OPR_ATTR_ASSIGNMENT) {
      if (!Is_Target(v_left)) {
        Destroy_Value(v_left);
        Destroy_Value(v_right);
        LOG_ERR("Binary operator '%s' cannot modify its left operand (%s)",
                opn->opr->name, ValueKind_To_Str(v_left->kind));
        return NULL;
      }
    } else {
      /* If the operation is commutative and v_left is not an intermediate
       * value, then we check if the v_right is a temporary value.
       * If it is, we exchange the operands, since the op is commutative,
       * and this will save us one VM operation.
       */
      if (opn->attr & OPR_ATTR_COMMUTATIVE && !Is_Temp(v_left)) {
        if (Is_Temp(v_right)) {
          Value *v = v_left;
          v_left = v_right;
          v_right = v;
        }
      }

      Emit_Make_Temp(v_left, v_left);
    }

    Append_LIR2(opn->implem.opcode, & v_left->cont, & v_right->cont);
    result = v_left;
    Destroy_Value(v_right);
    break;

  case OPASMSCHEME_R_LR_BIN:
    {
      assert((opn->attr & OPR_ATTR_NATIVE) && (opn->attr & OPR_ATTR_BINARY));

      result = Create_Value();
      Setup_Value_As_Temp(result, opn->type_result);
      Emit_Load_Into_Reg(v_left, v_left);
      Emit_Load_Into_Reg(v_right, v_right);
      Append_LIR3(opn->implem.opcode, & result->cont, & v_left->cont,
                  & v_right->cont);
      Destroy_Value(v_left);
      Destroy_Value(v_right);
      return result;
    }

  case OPASMSCHEME_RL_R_BIN:
    if (BoxType_Compare(opn->type_result, v_right->type)
        != BOXTYPECMP_DIFFERENT) {
      Value *v_tmp = v_left;
      v_left = v_right;
      v_right = v_tmp;
    }

    Emit_Make_Temp(v_left, v_left);
    Append_LIR2(opn->implem.opcode, & v_left->cont, & v_right->cont);
    Destroy_Value(v_right);
    result = v_left;
    break;

  default:
    LOG_FATAL("Non-native operators are not supported, yet!");
    abort();
  }

  return Set_Ignorable(result, (opn->attr & OPR_ATTR_IGNORE_RES) != 0);
}

/** Compiles an operation between the two expression e1 and e2, where
 * the operator is opr.
 * REFERENCES: return: new, v: -1;
 */
Value *
Compiler::Emit_UnOp(BoxASTUnOp op, Value *v)
{
  Operator *opr = Get_Un_Op(op);
  Operation *opn;
  OprMatch match;
  Value *v_result = NULL;

  /* Subtypes cannot be used for operator overloading, so we expand
   * them anyway!
   */
  v = Emit_Subtype_Expansion(v);

  /* Now we search the operation */
  opn = My_Operator_Find_Opn(opr, & match, v->type, NULL, NULL);
  if (opn) {
    /* Now we expand the types, if necessary */
    if (match.match_left == BOXTYPECMP_MATCHING)
      v = Emit_Value_Expansion(v, match.expand_type_left);

    v_result = Emit_Operation(opn, v, v);

  } else {
    if ((opr->attr & OPR_ATTR_UN_RIGHT) != 0) {
      LOG_ERR("%~s%s <-- Operation has not been defined!",
              BoxType_Get_Repr(v->type), opr->name);
      return NULL;

    } else {
      LOG_ERR("%s%~s <-- Operation has not been defined!",
              opr->name, BoxType_Get_Repr(v->type));
    }
  }

  return v_result;
}

/**
 * Compiles an operation between the two expression e1 and e2, where
 * the operator is opr.
 * @note Destroy @p v_left, @p v_right and return a new Value.
 */
Value *
Compiler::Emit_BinOp(BoxASTBinOp op, Value *v_left, Value *v_right)
{
  Operator *opr = Get_Bin_Op(op);
  Operation *opn;
  OprMatch match;
  Value *v_result = NULL;

  /* Subtypes cannot be used for operator overloading, so we expand
   * them anyway!
   */
  v_left = Emit_Subtype_Expansion(v_left);
  v_right = Emit_Subtype_Expansion(v_right);

  /* Now we search the operation */
  opn = My_Operator_Find_Opn(opr, & match, v_left->type,
                                 v_right->type, NULL);
  if (opn) {
    /* Now we expand the types, if necessary */
    if (match.match_left == BOXTYPECMP_MATCHING)
      v_left = Emit_Value_Expansion(v_left, match.expand_type_left);

    if (match.match_right == BOXTYPECMP_MATCHING)
      v_right = Emit_Value_Expansion(v_right, match.expand_type_right);

    v_result = Emit_Operation(opn, v_left, v_right);

  } else {
    LOG_ERR("%~s %s %~s <-- Operation has not been defined!",
            BoxType_Get_Repr(v_left->type), opr->name,
            BoxType_Get_Repr(v_right->type));
    Destroy_Value(v_left);
    Destroy_Value(v_right);
 }

  return v_result;
}

/**
 * @brief Convert a given object to an object with (possibly) different type.
 * @note Destroy @p v_src, but only when @c true is returned.
 */
bool
Compiler::Try_Emit_Conversion(Value *v_dst, Value *v_src)
{
  Operation *opn;
  OprMatch match;

  /* Now we search the operation */
  opn = My_Operator_Find_Opn(& convert_, & match,
                             v_src->type, NULL, v_dst->type);

  if (!opn)
    return false;

  /* Now we expand the types, if necessary */
  if (match.match_left == BOXTYPECMP_MATCHING)
    v_src = Emit_Value_Expansion(v_src, match.expand_type_left);

  if (opn->asm_scheme == OPASMSCHEME_STD_UN)
    Append_LIR2(opn->implem.opcode, & v_dst->cont, & v_src->cont);
  else if (opn->asm_scheme == OPASMSCHEME_USR_UN)
    Emit_Call(opn->implem.call_num, v_dst, v_src);
  else {
    LOG_FATAL("Try_Emit_Conversion: unexpected asm-scheme!");
    abort();
  }

  Destroy_Value(v_src);
  return true;
}

}