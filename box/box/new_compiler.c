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
#include "array.h"
#include "ast.h"
#include "expr.h"
#include "new_compiler.h"
#include "operator.h"

/** Type of items which may be inserted inside the compiler stack.
 * @see StackItem
 */
typedef enum {
  STACKITEM_ERROR,
  STACKITEM_EXPR
} StackItemType;

/** Called when removing the item from the stack.
 * @see StackItem
 */
typedef void (*StackItemDestructor)(void *item);

/** The compiler has a stack of expression which are currently being
 * processed. This structure describes one of such expressions.
 * We actually store a pointer to the item and an integer number, which
 * identifies the type of such item. The type introduce some redundancy
 * which may help to track down bugs...
 */
typedef struct {
  StackItemType       type;
  void                *item;
  StackItemDestructor destructor;
} StackItem;

void BoxCmp_Init(BoxCmp *c) {
  BoxArr_Init(& c->stack, sizeof(StackItem), 32);
  BoxCmp_Operator_Init(c);
}

void BoxCmp_Finish(BoxCmp *c) {
  BoxArr_Finish(& c->stack);
  BoxCmp_Operator_Finish(c);
}

BoxCmp *BoxCmp_New(void) {
  BoxCmp *c = BoxMem_Alloc(sizeof(BoxCmp));
  if (c == NULL) return NULL;
  BoxCmp_Init(c);
  return c;
}

void BoxCmp_Destroy(BoxCmp *c) {
  BoxCmp_Finish(c);
  BoxMem_Free(c);
}

void BoxCmp_Pop_Any(BoxCmp *c, int num_items_to_pop) {
  int i;
  for(i = 0; i < num_items_to_pop; i++) {
    StackItem *si = (StackItem *) BoxArr_Last_Item_Ptr(& c->stack);
    if (si->destructor != NULL)
      si->destructor(si->item);
    (void) BoxArr_Pop(& c->stack, NULL);
  }
}

/** Used to push an error into the stack. Errors are propagated. For example:
 * a binary operation where one of the two arguments is an error returns
 * silently an error into the stack.
 */
void BoxCmp_Push_Error(BoxCmp *c, int num_errors) {
  int i;
  for(i = 0; i < num_errors; i++) {
    StackItem *si = (StackItem *) BoxArr_Push(& c->stack, NULL);
    si->type = STACKITEM_ERROR;
    si->item = NULL;
    si->destructor = NULL;
  }
}

/** Check the last num_errors stack entries for errors. If all these entries
 * have no errors, then do nothing and return 0. If one or more of these
 * entries have errors, then removes all of them from the stack, push the
 * given number of errors and return 1.
 */
int BoxCmp_Pop_Errors(BoxCmp *c, int items_to_pop, int errors_to_push) {
  BoxInt n = BoxArr_Num_Items(& c->stack), i;
  for(i = 0; i < items_to_pop; i++) {
    StackItem *si = (StackItem *) BoxArr_Item_Ptr(& c->stack, n - i);
    if (si->type == STACKITEM_ERROR) {
      BoxCmp_Pop_Any(c, items_to_pop);
      BoxCmp_Push_Error(c, errors_to_push);
      return 1;
    }
  }
  return 0;
}

void BoxCmp_Push_Expr(BoxCmp *c, Expr *expr) {
  if (expr != NULL) {
    StackItem *si = (StackItem *) BoxArr_Push(& c->stack, NULL);
    si->type = STACKITEM_EXPR;
    si->item = expr;
    si->destructor = (StackItemDestructor) Expr_Unlink;

  } else
    BoxCmp_Push_Error(c, 1);
}

Expr *BoxCmp_Get_Expr(BoxCmp *c, BoxInt pos) {
  BoxInt n = BoxArr_Num_Items(& c->stack);
  StackItem *si = BoxArr_Item_Ptr(& c->stack, n - pos);
  assert(si->type == STACKITEM_EXPR);
  return (Expr *) si->item;
}

static void My_Compile_Any(BoxCmp *c, ASTNode *node);
static void My_Compile_Const(BoxCmp *c, ASTNode *n);
static void My_Compile_BinOp(BoxCmp *c, ASTNode *n);

void BoxCmp_Compile(BoxCmp *c, ASTNode *program) {
  ASTNode *s;

  if (program == NULL)
    return;

  assert(program->type == ASTNODETYPE_BOX);

  for(s = program->attr.box.first_statement;
      s != NULL;
      s = s->attr.statement.next_statement) {
    assert(s->type == ASTNODETYPE_STATEMENT);
    My_Compile_Any(c, s->attr.statement.target);
  }
}

static void My_Compile_Any(BoxCmp *c, ASTNode *node) {
  switch(node->type) {
  case ASTNODETYPE_CONST:
    My_Compile_Const(c, node); break;
  case ASTNODETYPE_BINOP:
    My_Compile_BinOp(c, node); break;
  default:
    printf("Compilation of node is not implemented, yet!\n");
    break;
  }
}

static void My_Compile_Const(BoxCmp *c, ASTNode *n) {
  Expr *expr;
  assert(n->type = ASTNODETYPE_CONST);
  expr = Expr_New();
  switch(n->attr.constant.type) {
  case ASTCONSTTYPE_CHAR:
    Expr_New_Imm_Char(expr, n->attr.constant.value.c);
    break;
  case ASTCONSTTYPE_INT:
    Expr_New_Imm_Int(expr, n->attr.constant.value.i);
    break;
  case ASTCONSTTYPE_REAL:
    Expr_New_Imm_Real(expr, n->attr.constant.value.r);
    break;
  }
  BoxCmp_Push_Expr(c, expr);
  printf("Pushing constant into stack!\n");
}

static void My_Compile_BinOp(BoxCmp *c, ASTNode *n) {
  Expr *left, *right, *result;

  assert(n->type == ASTNODETYPE_BINOP);

  My_Compile_Any(c, n->attr.bin_op.left);
  My_Compile_Any(c, n->attr.bin_op.right);
  if (BoxCmp_Pop_Errors(c, /* pop */ 2, /* push err */ 1)) return;

  printf("Compiling binary operation!\n");
  right = BoxCmp_Get_Expr(c, 0);
  left  = BoxCmp_Get_Expr(c, 1);

  result = NULL;
#if 0
  if (left->is.typed && right->is.typed) {
    Expr *result = Cmp_Operator_Exec(opr, left, right);
    if (result == NULL) return Failed;
    *rs = *result;
    return Success;

  } else {
    if ( opr->can_define ) {
      return Prs_Def_Operator(opr, rs, a, b);
    } else {
      MSG_ERROR("The expression should have type!");
      return Failed;
    }
  }
#endif

  BoxCmp_Pop_Any(c, 2);
  BoxCmp_Push_Expr(c, result);
}


