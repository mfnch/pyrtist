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
#include "messages.h"
#include "compiler.h"
#include "parserh.h"

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

void BoxCmp_Init(BoxCmp *c, BoxVM *target_vm) {
  c->attr.own_vm = (target_vm == NULL);
  c->vm = (target_vm != NULL) ? target_vm : BoxVM_New();

  BoxArr_Init(& c->stack, sizeof(StackItem), 32);

  Reg_Init(& c->regs);

  TS_Init(& c->ts);
  TS_Init_Builtin_Types(& c->ts);

  BoxCmp_Init__Operators(c);

  CmpProc_Init(& c->main_proc, c);
  c->cur_proc = & c->main_proc;
  Namespace_Init(& c->ns);
  Bltin_Init(c);
}

void BoxCmp_Finish(BoxCmp *c) {
  Bltin_Finish(c);
  Namespace_Finish(& c->ns);
  Reg_Finish(& c->regs);
  CmpProc_Finish(& c->main_proc);
  BoxArr_Finish(& c->stack);
  BoxCmp_Finish__Operators(c);
  TS_Finish(& c->ts);

  if (c->attr.own_vm)
    BoxVM_Destroy(c->vm);
}

BoxCmp *BoxCmp_New(BoxVM *target_vm) {
  BoxCmp *c = BoxMem_Alloc(sizeof(BoxCmp));
  if (c == NULL) return NULL;
  BoxCmp_Init(c, target_vm);
  return c;
}

void BoxCmp_Destroy(BoxCmp *c) {
  BoxCmp_Finish(c);
  BoxMem_Free(c);
}

BoxVM *BoxCmp_Steal_VM(BoxCmp *c) {
  BoxVM *vm = c->vm;
  c->attr.own_vm = 0;
  c->vm = 0;
  return vm;
}

/* Function which does all the steps needed to get from a Box source file
 * to a VM with the corresponding compiled bytecode.
 */
BoxVM *Box_Compile_To_VM_From_File(BoxVMCallNum *main, BoxVM *target_vm,
                                   FILE *file, const char *setup_file_name) {
  ASTNode *program_node;
  BoxCmp *compiler;
  BoxVM *vm;
  BoxVMCallNum dummy_cn;

  if (main == NULL)
    main = & dummy_cn;

  compiler = BoxCmp_New(target_vm);
  program_node = Parser_Parse(file, setup_file_name);
  BoxCmp_Compile(compiler, program_node);
  ASTNode_Destroy(program_node);
  *main = CmpProc_Get_Call_Num(& compiler->main_proc);
  vm = BoxCmp_Steal_VM(compiler);
  BoxCmp_Destroy(compiler);
  return vm;
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
  int no_err = 1;

  for(i = 0; i < items_to_pop; i++) {
    StackItem *si = (StackItem *) BoxArr_Item_Ptr(& c->stack, n - i);

    if (si->type == STACKITEM_VALUE) {
      Value *v = (Value *) si->item;
      if (Value_Is_Err(v)) {
        no_err = 0;
        break;
      }

    } else if (si->type == STACKITEM_ERROR) {
      no_err = 0;
      break;
    }
  }

  if (no_err)
    return 0;

  else {
    BoxCmp_Pop_Any(c, items_to_pop);
    BoxCmp_Push_Error(c, errors_to_push);
    return 1;
  }
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
  switch(si->type) {
  case STACKITEM_ERROR:
    return Value_New(c); /* return an error value */

  case STACKITEM_VALUE:
    return (Value *) si->item;

  default:
    MSG_FATAL("BoxCmp_Get_Value: want value, but top of stack contains "
              "incompatible item.");
    assert(0);
  }
}

static void My_Compile_Any(BoxCmp *c, ASTNode *node);
static void My_Compile_TypeName(BoxCmp *c, ASTNode *node);
static void My_Compile_Box(BoxCmp *c, ASTNode *node);
static void My_Compile_Const(BoxCmp *c, ASTNode *n);
static void My_Compile_Var(BoxCmp *c, ASTNode *n);
static void My_Compile_UnOp(BoxCmp *c, ASTNode *n);
static void My_Compile_BinOp(BoxCmp *c, ASTNode *n);

void BoxCmp_Compile(BoxCmp *c, ASTNode *program) {
  if (program == NULL)
    return;

  My_Compile_Any(c, program);
  BoxCmp_Pop_Any(c, 1);
}

static void My_Compile_Any(BoxCmp *c, ASTNode *node) {
  switch(node->type) {
  case ASTNODETYPE_TYPENAME:
    My_Compile_TypeName(c, node); break;
  case ASTNODETYPE_BOX:
    My_Compile_Box(c, node); break;
  case ASTNODETYPE_CONST:
    My_Compile_Const(c, node); break;
  case ASTNODETYPE_VAR:
    My_Compile_Var(c, node); break;
  case ASTNODETYPE_UNOP:
    My_Compile_UnOp(c, node); break;
  case ASTNODETYPE_BINOP:
    My_Compile_BinOp(c, node); break;
  default:
    printf("Compilation of node is not implemented, yet!\n");
    break;
  }
}

static void My_Compile_TypeName(BoxCmp *c, ASTNode *n) {
  Value *v;
  char *type_name = n->attr.var.name;
  NmspFloor f;

  assert(n->type == ASTNODETYPE_TYPENAME);

  f = NMSPFLOOR_DEFAULT;
  v = Namespace_Get_Value(& c->ns, f, type_name);
  if (v != NULL) {
    BoxCmp_Push_Value(c, v);
    return;

  } else {
    v = Value_New(c);
    Value_Setup_As_Type_Name(v, type_name);
    Namespace_Add_Value(& c->ns, f, type_name, v);
    BoxCmp_Push_Value(c, v);
    return;
  }
}

/* We may optimize this later, by just passing a reference to a Value
 * object which is created once for all at the beginning!
 */
static Value *My_Get_Void_Value(BoxCmp *c) {
  Value *v = Value_New(c);
  Value_Setup_As_Void(v);
  return v;
}

int XXX_Emit_Call(Value *parent, Value *child) {
  BoxCmp *c = parent->cmp;
  BoxType found_procedure, expansion_for_child;
  BoxVMSymID sym_id;

  assert(c == child->cmp);

  /* Now we search for the procedure associated with *child */
  TS_Procedure_Inherited_Search(& c->ts, & found_procedure,
                                & expansion_for_child,
                                parent->type, child->type, 1);

  if (found_procedure == BOXTYPE_NONE) return 0;

  if (expansion_for_child != BOXTYPE_NONE)
    child = Value_Expand(child, expansion_for_child);

  TS_Procedure_Sym_Num_Get(& c->ts, & sym_id, found_procedure);
  printf("Found procedure with sym_id="SUInt"\n", sym_id);
  CmpProc_Assemble_Call(c->cur_proc, sym_id);
  return 1;
}

static void My_Compile_Box(BoxCmp *c, ASTNode *box) {
  ASTNode *s;
  Value *parent = NULL;
  int parent_is_err = 0;

  assert(box->type == ASTNODETYPE_BOX);

  if (box->attr.box.parent == NULL) {
    parent = My_Get_Void_Value(c);
    BoxCmp_Push_Value(c, parent);

  } else {
    Value *parent_type;
    My_Compile_Any(c, box->attr.box.parent);
    parent_type = BoxCmp_Get_Value(c, 0);
    parent = Value_To_Temp_Or_Target(parent_type);
    parent_is_err = Value_Is_Err(parent);
    BoxCmp_Pop_Any(c, 1);
    BoxCmp_Push_Value(c, parent);
  }

  Namespace_Floor_Up(& c->ns); /* variables defined in this box will be
                                  destroyed when it gets closed! */

  /* Loop over all the statements of the box */
  for(s = box->attr.box.first_statement;
      s != NULL;
      s = s->attr.statement.next_statement) {
    Value *stmt_val;

    assert(s->type == ASTNODETYPE_STATEMENT);

    My_Compile_Any(c, s->attr.statement.target);
    stmt_val = BoxCmp_Get_Value(c, 0);
    if (parent_is_err || Value_Is_Err(stmt_val)
        || Value_Is_Ignorable(stmt_val))
      BoxCmp_Pop_Any(c, 1);

    else {
      Value *child = BoxCmp_Get_Value(c, 0);
      int have_found_it = XXX_Emit_Call(parent, child);
      BoxCmp_Pop_Any(c, 1);
      if (!have_found_it) {
        MSG_WARNING("Don't know how to use '%~s' expressions inside "
                    "a '%~s' box.",
                    TS_Name_Get(& c->ts, child->type),
                    TS_Name_Get(& c->ts, parent->type));
      }
    }
  }

  Namespace_Floor_Down(& c->ns); /* close the scope unit */
}

static void My_Compile_Var(BoxCmp *c, ASTNode *n) {
  Value *v;
  char *item_name = n->attr.var.name;

  assert(n->type = ASTNODETYPE_VAR);

  v = Namespace_Get_Value(& c->ns, NMSPFLOOR_DEFAULT, item_name);
  if (v != NULL) {
    BoxCmp_Push_Value(c, v);
    return;

  } else {
    v = Value_New(c);
    Value_Setup_As_Var_Name(v, item_name);
    Namespace_Add_Value(& c->ns, NMSPFLOOR_DEFAULT, item_name, v);
    BoxCmp_Push_Value(c, v);
    return;
  }
}

static void My_Compile_Const(BoxCmp *c, ASTNode *n) {
  Value *v;
  assert(n->type = ASTNODETYPE_CONST);
  v = Value_New(c);
  switch(n->attr.constant.type) {
  case ASTCONSTTYPE_CHAR:
    Value_Setup_As_Imm_Char(v, n->attr.constant.value.c);
    break;
  case ASTCONSTTYPE_INT:
    Value_Setup_As_Imm_Int(v, n->attr.constant.value.i);
    break;
  case ASTCONSTTYPE_REAL:
    Value_Setup_As_Imm_Real(v, n->attr.constant.value.r);
    break;
  }
  BoxCmp_Push_Value(c, v);
}

static void My_Compile_UnOp(BoxCmp *c, ASTNode *n) {
  Value *operand, *result = NULL;

  assert(n->type == ASTNODETYPE_UNOP);

  /* Compile operand and get it from the stack */
  My_Compile_Any(c, n->attr.un_op.expr);
  operand = BoxCmp_Get_Value(c, 0);

  if (Value_Is_Err(operand))
    return; /* Leave the error in the stack */

  else if (Value_Is_Value(operand))
    result = BoxCmp_Opr_Emit_UnOp(c, n->attr.un_op.operation, operand);

  else
    Value_Want_Value(operand);

  BoxCmp_Pop_Any(c, 1);
  BoxCmp_Push_Value(c, result);
}

static void My_Compile_BinOp(BoxCmp *c, ASTNode *n) {
  assert(n->type == ASTNODETYPE_BINOP);

  My_Compile_Any(c, n->attr.bin_op.left);
  My_Compile_Any(c, n->attr.bin_op.right);
  if (BoxCmp_Pop_Errors(c, /* pop */ 2, /* push err */ 1))
    return;

  else {
    Value *left, *right, *result = NULL;
    ASTBinOp op;

    /* Get values from stack */
    right = BoxCmp_Get_Value(c, 0);
    left  = BoxCmp_Get_Value(c, 1);

    op = n->attr.bin_op.operation;
    if (op == ASTBINOP_ASSIGN) {
      if (Value_Want_Value(right)) {
        /* If the value is an identifier (thing without type, nor value),
         * then we transform it to a proper target.
         */
        if (Value_Is_Var_Name(left)) {
          ValContainer vc = {VALCONTTYPE_LVAR, -1, 0};
          Value_Setup_Container(left, right->type, & vc);
        }

        if (Value_Is_Target(left)) {
          result = BoxCmp_Opr_Emit_BinOp(c, op, left, right);

        } else
          MSG_ERROR("Invalid target for assignment (%s).",
                    ValueKind_To_Str(left->kind));
      }

    } else {
      if (Value_Want_Value(left) && Value_Want_Value(right))
        result = BoxCmp_Opr_Emit_BinOp(c, op, left, right);
    }

    BoxCmp_Pop_Any(c, 2);
    BoxCmp_Push_Value(c, result);
  }
}


