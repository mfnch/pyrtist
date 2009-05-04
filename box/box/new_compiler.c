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
#include "value.h"
#include "cmpproc.h"
#include "operator.h"
#include "namespace.h"
#include "new_compiler.h"

/** Type of items which may be inserted inside the compiler stack.
 * @see StackItem
 */
typedef enum {
  STACKITEM_ERROR,
  STACKITEM_VALUE
} StackItemType;

/** Called when removing the item from the stack.
 * @see StackItem
 */
typedef void (*StackItemDestructor)(void *item);

/** The compiler has a stack of values which are currently being
 * processed. This structure describes one of such values.
 * We actually store a pointer to the item and an integer number, which
 * identifies the type of such item. The type introduces some redundancy
 * which may help to track down bugs...
 */
typedef struct {
  StackItemType       type;
  void                *item;
  StackItemDestructor destructor;
} StackItem;

void BoxCmp_Init(BoxCmp *c) {
  BoxVM_Init(& c->vm);
  BoxArr_Init(& c->stack, sizeof(StackItem), 32);

  Reg_Init(& c->regs);

  {
    TS *this_ts = last_ts;
    TS_Init(& c->ts);
    TS_Init_Builtin_Types(& c->ts);
    last_ts = this_ts;
  }
  BoxCmp_Init__Operators(c);

  CmpProc_Init(& c->main_proc, c);
  c->cur_proc = & c->main_proc;
  Namespace_Init(& c->ns);
}

void BoxCmp_Finish(BoxCmp *c) {
  Namespace_Finish(& c->ns);
  CmpProc_Get_Call_Num(& c->main_proc);
  VM_Proc_Disassemble_All(& c->vm, stdout);
  CmpProc_Finish(& c->main_proc);
  BoxArr_Finish(& c->stack);
  BoxCmp_Finish__Operators(c);
  TS_Finish(& c->ts);
  BoxVM_Finish(& c->vm);
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

void BoxCmp_Push_Value(BoxCmp *c, Value *v) {
  if (v != NULL) {
    StackItem *si = (StackItem *) BoxArr_Push(& c->stack, NULL);
    si->type = STACKITEM_VALUE;
    si->item = v;
    si->destructor = (StackItemDestructor) Value_Unlink;

  } else
    BoxCmp_Push_Error(c, 1);
}

Value *BoxCmp_Get_Value(BoxCmp *c, BoxInt pos) {
  BoxInt n = BoxArr_Num_Items(& c->stack);
  StackItem *si = BoxArr_Item_Ptr(& c->stack, n - pos);
  assert(si->type == STACKITEM_VALUE);
  return (Value *) si->item;
}

static void My_Compile_Any(BoxCmp *c, ASTNode *node);
static void My_Compile_Const(BoxCmp *c, ASTNode *n);
static void My_Compile_Var(BoxCmp *c, ASTNode *n);
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
  case ASTNODETYPE_VAR:
    My_Compile_Var(c, node); break;
  case ASTNODETYPE_BINOP:
    My_Compile_BinOp(c, node); break;
  default:
    printf("Compilation of node is not implemented, yet!\n");
    break;
  }
}

/** Use 'Namespace_Add_Item' to add the value 'v' to the namespace. */
void Namespace_Add_Value(Namespace *ns, NmspFloor floor,
                         const char *item_name, Value *v);

/** Get an item 'item_name' from the namespace, assuming this must be a
 * Value.
 */
Value *Namespace_Get_Value(Namespace *ns, NmspFloor floor,
                           const char *item_name);

static void My_Compile_Var(BoxCmp *c, ASTNode *n) {
  Value *v;
  char *item_name = n->attr.var.name;

  assert(n->type = ASTNODETYPE_VAR);

  v = Namespace_Get_Value(& c->ns, 0, item_name);
  if (v != NULL) {
    BoxCmp_Push_Value(c, v);
    return;

  } else {
    printf("Unknown identifier '%s'\n", item_name);
    v = Value_New(c);
    Namespace_Add_Value(& c->ns, 0, item_name, v);
    BoxCmp_Push_Error(c, 1);
    return;
  }

  assert(0);
  return;
}

static void My_Compile_Const(BoxCmp *c, ASTNode *n) {
  Value *v;
  assert(n->type = ASTNODETYPE_CONST);
  v = Value_New(c);
  switch(n->attr.constant.type) {
  case ASTCONSTTYPE_CHAR:
    Value_Set_Imm_Char(v, n->attr.constant.value.c);
    break;
  case ASTCONSTTYPE_INT:
    Value_Set_Imm_Int(v, n->attr.constant.value.i);
    break;
  case ASTCONSTTYPE_REAL:
    Value_Set_Imm_Real(v, n->attr.constant.value.r);
    break;
  }
  BoxCmp_Push_Value(c, v);
}

static void My_Compile_BinOp(BoxCmp *c, ASTNode *n) {
  Value *left, *right, *result;

  assert(n->type == ASTNODETYPE_BINOP);

  My_Compile_Any(c, n->attr.bin_op.left);
  My_Compile_Any(c, n->attr.bin_op.right);
  if (BoxCmp_Pop_Errors(c, /* pop */ 2, /* push err */ 1)) return;

  /* Get values from stack */
  right = BoxCmp_Get_Value(c, 0);
  left  = BoxCmp_Get_Value(c, 1);

  if (1 /*left->is.typed && right->is.typed*/) {
    result = BoxCmp_Opr_Emit_BinOp(c, n->attr.bin_op.operation, left, right);

  } else {
    result = NULL;
/*    if ( opr->can_define ) {
      return Prs_Def_Operator(opr, rs, a, b);
    } else {
      MSG_ERROR("The expression should have type!");
      return Failed; */
  }

  BoxCmp_Pop_Any(c, 2);
  BoxCmp_Push_Value(c, result);
}


