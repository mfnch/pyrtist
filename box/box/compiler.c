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
#include <string.h>

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

/* For now it is safer not to cache const values!
 * (will make easier to detect reference count problems)
 */
#define DONT_CACHE_CONST_VALUES 1

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

/** Here we define, once for all, a number of useful Value instances which
 * are used massively in the compiler. We could just allocate and create
 * them whenever needed, but we do it here and just return new references
 * to them whenever it is needed, with the hope to improve performance.
 */
void My_Init_Const_Values(BoxCmp *c) {
  Value_Init(& c->value.error, & c->main_proc);
  Value_Init(& c->value.void_val, & c->main_proc);
  Value_Setup_As_Void(& c->value.void_val);
  BoxCont_Set(& c->cont.pass_child, "go", 2);
  BoxCont_Set(& c->cont.pass_parent, "go", 1);

  Value_Init(& c->value.create, & c->main_proc);
  Value_Setup_As_Type(& c->value.create, BOXTYPE_CREATE);
  Value_Init(& c->value.destroy, & c->main_proc);
  Value_Setup_As_Type(& c->value.destroy, BOXTYPE_DESTROY);
  Value_Init(& c->value.pause, & c->main_proc);
  Value_Setup_As_Type(& c->value.pause, BOXTYPE_PAUSE);
}

void My_Finish_Const_Values(BoxCmp *c) {
  Value_Unlink(& c->value.error);
  Value_Unlink(& c->value.void_val);
  Value_Unlink(& c->value.create);
  Value_Unlink(& c->value.destroy);
  Value_Unlink(& c->value.pause);
}

/** Return a new error value (actually a new reference to it,
 * see My_Init_Const_Values)
 */
Value *My_Value_New_Error(BoxCmp *c) {
#if DONT_CACHE_CONST_VALUES == 1
  return Value_New(c->cur_proc);
#else
  Value_Link(c->value.error);
  return & c->value.error; /* return an error value */
#endif
}

/* We may optimize this later, by just passing a reference to a Value
 * object which is created once for all at the beginning!
 */
static Value *My_Get_Void_Value(BoxCmp *c) {
#if DONT_CACHE_CONST_VALUES == 1
  Value *v = Value_New(c->cur_proc);
  Value_Setup_As_Void(v);
  return v;
#else
  Value_Link(& c->value.void_val);
  return & c->value.void_val;
#endif
}

void BoxCmp_Init(BoxCmp *c, BoxVM *target_vm) {
  c->attr.own_vm = (target_vm == NULL);
  c->vm = (target_vm != NULL) ? target_vm : BoxVM_New();

  BoxArr_Init(& c->stack, sizeof(StackItem), 32);

  TS_Init(& c->ts);
  TS_Init_Builtin_Types(& c->ts);

  BoxCmp_Init__Operators(c);

  CmpProc_Init(& c->main_proc, c, CMPPROCSTYLE_MAIN);
  c->cur_proc = & c->main_proc;

  My_Init_Const_Values(c);
  Namespace_Init(& c->ns);
  Bltin_Init(c);
}

void BoxCmp_Finish(BoxCmp *c) {
  Bltin_Finish(c);
  Namespace_Finish(& c->ns);
  My_Finish_Const_Values(c);
  CmpProc_Finish(& c->main_proc);

  if (BoxArr_Num_Items(& c->stack) != 0)
    MSG_WARNING("BoxCmp_Finish: stack is not empty at compiler destruction.");
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

void BoxCmp_Remove_Any(BoxCmp *c, int num_items_to_remove) {
  int i;
  for(i = 0; i < num_items_to_remove; i++) {
    StackItem *si = (StackItem *) BoxArr_Last_Item_Ptr(& c->stack);
    if (si->type == STACKITEM_VALUE)
      Value_Unlink((Value *) si->item);
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
    BoxCmp_Remove_Any(c, items_to_pop);
    BoxCmp_Push_Error(c, errors_to_push);
    return 1;
  }
}

/** Pushes the given value 'v' into the compiler stack, stealing a reference
 * to it.
 * REFERENCES: v: -1;
 */
void BoxCmp_Push_Value(BoxCmp *c, Value *v) {
  if (v != NULL) {
    StackItem *si = (StackItem *) BoxArr_Push(& c->stack, NULL);
    si->type = STACKITEM_VALUE;
    si->item = v;
    si->destructor = NULL;

  } else
    BoxCmp_Push_Error(c, 1);
}

/** Pops the last value in the compiler stack and returns it, together with
 * the corresponding reference.
 * REFERENCES: return: new;
 */
Value *BoxCmp_Pop_Value(BoxCmp *c) {
  StackItem *si = BoxArr_Last_Item_Ptr(& c->stack);
  Value *v;

  switch(si->type) {
  case STACKITEM_ERROR:
    return My_Value_New_Error(c); /* return an error value */

  case STACKITEM_VALUE:
    v = (Value *) si->item; /* return the value and its reference */
    (void) BoxArr_Pop(& c->stack, NULL);
    return v;

  default:
    MSG_FATAL("BoxCmp_Pop_Value: want value, but top of stack contains "
              "incompatible item.");
    assert(0);
  }
}

Value *BoxCmp_Get_Value(BoxCmp *c, BoxInt pos) {
  BoxInt n = BoxArr_Num_Items(& c->stack);
  StackItem *si = BoxArr_Item_Ptr(& c->stack, n - pos);
  switch(si->type) {
  case STACKITEM_ERROR:
    return My_Value_New_Error(c); /* return an error value */

  case STACKITEM_VALUE:
    return (Value *) si->item;

  default:
    MSG_FATAL("BoxCmp_Get_Value: want value, but top of stack contains "
              "incompatible item.");
    assert(0);
  }
}

static void My_Compile_Any(BoxCmp *c, ASTNode *node);
static void My_Compile_Error(BoxCmp *c, ASTNode *node);
static void My_Compile_TypeName(BoxCmp *c, ASTNode *node);
static void My_Compile_Box(BoxCmp *c, ASTNode *node);
static void My_Compile_String(BoxCmp *c, ASTNode *node);
static void My_Compile_Const(BoxCmp *c, ASTNode *n);
static void My_Compile_Var(BoxCmp *c, ASTNode *n);
static void My_Compile_DontIgnore(BoxCmp *c, ASTNode *n);
static void My_Compile_UnOp(BoxCmp *c, ASTNode *n);
static void My_Compile_BinOp(BoxCmp *c, ASTNode *n);
static void My_Compile_Struc(BoxCmp *c, ASTNode *n);

void BoxCmp_Compile(BoxCmp *c, ASTNode *program) {
  if (program == NULL)
    return;

  My_Compile_Any(c, program);
  BoxCmp_Remove_Any(c, 1);
}


static void My_Compile_Any(BoxCmp *c, ASTNode *node) {
  switch(node->type) {
  case ASTNODETYPE_ERROR:
    My_Compile_Error(c, node); break;
  case ASTNODETYPE_TYPENAME:
    My_Compile_TypeName(c, node); break;
  case ASTNODETYPE_BOX:
    My_Compile_Box(c, node); break;
  case ASTNODETYPE_STRING:
    My_Compile_String(c, node); break;
    break;
  case ASTNODETYPE_CONST:
    My_Compile_Const(c, node); break;
  case ASTNODETYPE_VAR:
    My_Compile_Var(c, node); break;
  case ASTNODETYPE_DONTIGNORE:
    My_Compile_DontIgnore(c, node); break;
  case ASTNODETYPE_UNOP:
    My_Compile_UnOp(c, node); break;
  case ASTNODETYPE_BINOP:
    My_Compile_BinOp(c, node); break;
  case ASTNODETYPE_STRUC:
    My_Compile_Struc(c, node); break;
  default:
    printf("Compilation of node is not implemented, yet!\n");
    break;
  }
}

static void My_Compile_Error(BoxCmp *c, ASTNode *node) {
  BoxCmp_Push_Value(c, Value_New(c->cur_proc));
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
    v = Value_New(c->cur_proc);
    Value_Setup_As_Type_Name(v, type_name);
    Namespace_Add_Value(& c->ns, f, type_name, v);
    BoxCmp_Push_Value(c, v);
    return;
  }
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
    parent_type = BoxCmp_Pop_Value(c);
    parent = Value_To_Temp_Or_Target(parent_type);
    Value_Unlink(parent_type); /* XXX */
    parent_is_err = Value_Is_Err(parent);
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
    stmt_val = BoxCmp_Pop_Value(c);
    if (parent_is_err || Value_Is_Err(stmt_val)
        || Value_Is_Ignorable(stmt_val))
      Value_Unlink(stmt_val);

    else {
      BoxTask status = Value_Emit_Call(parent, stmt_val);
      Value_Unlink(stmt_val); /* XXX */
      if (status == BOXTASK_FAILURE) {
        MSG_WARNING("Don't know how to use '%~s' expressions inside "
                    "a '%~s' box.",
                    TS_Name_Get(& c->ts, stmt_val->type),
                    TS_Name_Get(& c->ts, parent->type));
      }
    }
  }

  Namespace_Floor_Down(& c->ns); /* close the scope unit */
}

static void My_Compile_String(BoxCmp *c, ASTNode *node) {
  Value *v_str;

  assert(node->type == ASTNODETYPE_STRING);

  v_str = Value_New(c->cur_proc);
  Value_Setup_As_String(v_str, node->attr.string.str);
  BoxCmp_Push_Value(c, v_str);
}

static void My_Compile_Var(BoxCmp *c, ASTNode *n) {
  Value *v;
  char *item_name = n->attr.var.name;

  assert(n->type == ASTNODETYPE_VAR);

  v = Namespace_Get_Value(& c->ns, NMSPFLOOR_DEFAULT, item_name);
  if (v != NULL) {
    /* We just return a copy of the Value object corresponding to the
     * variable
     */
    Value *v_copy = Value_New(c->cur_proc);
    Value_Setup_As_Weak_Copy(v_copy, v);
    BoxCmp_Push_Value(c, v_copy);

  } else {
    v = Value_New(c->cur_proc);
    Value_Setup_As_Var_Name(v, item_name);
    Namespace_Add_Value(& c->ns, NMSPFLOOR_DEFAULT, item_name, v);
    BoxCmp_Push_Value(c, v);
  }
}

static void My_Compile_DontIgnore(BoxCmp *c, ASTNode *n) {
  Value *operand;

  assert(n->type == ASTNODETYPE_DONTIGNORE);

  /* Compile operand and get it from the stack */
  My_Compile_Any(c, n->attr.dont_ignore.expr);
  operand = BoxCmp_Get_Value(c, 0);

  Value_Set_Ignorable(operand, 0);
}

static void My_Compile_Const(BoxCmp *c, ASTNode *n) {
  Value *v;
  assert(n->type == ASTNODETYPE_CONST);
  v = Value_New(c->cur_proc);
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
  operand = BoxCmp_Pop_Value(c);

  if (Value_Is_Err(operand))
    BoxCmp_Push_Value(c, operand); /* Re-put the error in the stack */

  else if (Value_Is_Value(operand))
    result = BoxCmp_Opr_Emit_UnOp(c, n->attr.un_op.operation, operand);

  else
    Value_Want_Value(operand);

  Value_Unlink(operand); /* XXX */
  BoxCmp_Push_Value(c, result);
}

/** Deal with assignments.
 * REFERENCES: result: new, left: -1, right: -1;
 */
static Value *My_Compile_Assignment(BoxCmp *c, Value *left, Value *right) {
  if (Value_Want_Value(right)) {
    /* If the value is an identifier (thing without type, nor value),
     * then we transform it to a proper target.
     */
    if (Value_Is_Var_Name(left)) {
      ValContainer vc = {VALCONTTYPE_LVAR, -1, 0};
      Value_Setup_Container(left, right->type, & vc);
      Value_Emit_Allocate(left);
    }

    if (Value_Is_Target(left)) {
      Value_Move_Content(left, right);
      Value_Set_Ignorable(left, 1);
      /*return BoxCmp_Opr_Emit_BinOp(c, ASTBINOP_ASSIGN, left, right);*/
      return left;

    } else {
      MSG_ERROR("Invalid target for assignment (%s).",
                ValueKind_To_Str(left->kind));
      Value_Unlink(left);
      Value_Unlink(right);
      return NULL;
    }

  } else {
    Value_Unlink(left);
    Value_Unlink(right);
    return NULL;
  }
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
    right = BoxCmp_Pop_Value(c);
    left  = BoxCmp_Pop_Value(c);

    op = n->attr.bin_op.operation;
    if (op == ASTBINOP_ASSIGN) {
      result = My_Compile_Assignment(c, left, right);

    } else {
      if (Value_Want_Value(left) && Value_Want_Value(right))
        result = BoxCmp_Opr_Emit_BinOp(c, op, left, right);
      Value_Unlink(left); /* XXX */
      Value_Unlink(right); /* XXX */
    }

    BoxCmp_Push_Value(c, result);
  }
}

static void My_Compile_Struc(BoxCmp *c, ASTNode *n) {
  int i, num_members;
  ASTNode *member;
  BoxType t_struc;
  Value *v_struc;
  int no_err;

  assert(n->type == ASTNODETYPE_STRUC);

  /* Compile the members, check their types and leave them on the stack */
  num_members = 0;
  no_err = 1;
  for(member = n->attr.struc.first_member;
      member != NULL;
      member = member->attr.member.next) {
    Value *v_member;

    assert(member->type == ASTNODETYPE_MEMBER);

    My_Compile_Any(c, member->attr.member.expr);
    v_member = BoxCmp_Get_Value(c, 0);

    no_err &= Value_Want_Value(v_member);

    ++num_members;
  }

  /* Check for errors */
  if (!no_err) {
    BoxCmp_Remove_Any(c, num_members);
    BoxCmp_Push_Error(c, 1);
    return;
  }

  /* built the type for the structure */
  i = num_members;
  TS_Structure_Begin(& c->ts, & t_struc);
  for(member = n->attr.struc.first_member;
      member != NULL;
      member = member->attr.member.next) {
    Value *v_member = BoxCmp_Get_Value(c, --i);
    TS_Structure_Add(& c->ts, t_struc, v_member->type,
                     member->attr.member.name);
  }

  v_struc = Value_New(c->cur_proc);
  Value_Setup_As_Temp(v_struc, t_struc);


  BoxCmp_Remove_Any(c, num_members);
  BoxCmp_Push_Value(c, v_struc);
}
