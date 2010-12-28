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
#include "autogen.h"

#include "vmsymstuff.h"


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
  Value_Init(& c->value.begin, & c->main_proc);
  Value_Setup_As_Type(& c->value.begin, BOXTYPE_BEGIN);
  Value_Init(& c->value.end, & c->main_proc);
  Value_Setup_As_Type(& c->value.end, BOXTYPE_END);
  Value_Init(& c->value.pause, & c->main_proc);
  Value_Setup_As_Temp(& c->value.pause, BOXTYPE_PAUSE);
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
  CmpProc_Set_Alter_Name(& c->main_proc, "main");
  c->cur_proc = & c->main_proc;

  My_Init_Const_Values(c);
  Namespace_Init(& c->ns);
  Bltin_Init(c);

  BoxSrcPos_Init(& c->src_pos, NULL);
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
                                   FILE *file, const char *file_name,
                                   const char *setup_file_name) {
  ASTNode *program_node;
  BoxCmp *compiler;
  BoxVM *vm;
  BoxVMCallNum dummy_cn;

  if (main == NULL)
    main = & dummy_cn;

  compiler = BoxCmp_New(target_vm);
  program_node = Parser_Parse(file, file_name, setup_file_name);
  BoxCmp_Compile(compiler, program_node);
  ASTNode_Destroy(program_node);
  *main = CmpProc_Install(& compiler->main_proc);
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

/** Check the last 'items_to_pop' stack entries for errors. If all these
 * entries have no errors, then do nothing and return 0. If one or more
 * of these entries have errors, then removes all of them from the stack,
 * push the given number of errors and return 1.
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
    (void) BoxArr_Pop(& c->stack, NULL);
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
static void My_Compile_TypeTag(BoxCmp *c, ASTNode *node);
static void My_Compile_Subtype(BoxCmp *c, ASTNode *node);
static void My_Compile_Box(BoxCmp *c, ASTNode *box,
                           BoxType child_t, BoxType parent_t);
static void My_Compile_String(BoxCmp *c, ASTNode *node);
static void My_Compile_Const(BoxCmp *c, ASTNode *n);
static void My_Compile_Var(BoxCmp *c, ASTNode *n);
static void My_Compile_Ignore(BoxCmp *c, ASTNode *n);
static void My_Compile_UnOp(BoxCmp *c, ASTNode *n);
static void My_Compile_BinOp(BoxCmp *c, ASTNode *n);
static void My_Compile_Struc(BoxCmp *c, ASTNode *n);
static void My_Compile_MemberGet(BoxCmp *c, ASTNode *n);
static void My_Compile_SubtypeBld(BoxCmp *c, ASTNode *n);
static void My_Compile_SelfGet(BoxCmp *c, ASTNode *n);
static void My_Compile_ProcDef(BoxCmp *c, ASTNode *n);
static void My_Compile_TypeDef(BoxCmp *c, ASTNode *n);
static void My_Compile_StrucType(BoxCmp *c, ASTNode *n);
static void My_Compile_SpecType(BoxCmp *c, ASTNode *n);
static void My_Compile_IncType(BoxCmp *c, ASTNode *n);

void BoxCmp_Compile(BoxCmp *c, ASTNode *program) {
  if (program == NULL)
    return;

  My_Compile_Any(c, program);
  BoxCmp_Remove_Any(c, 1);
}

static void My_Compile_Any(BoxCmp *c, ASTNode *node) {
  BoxSrc *prev_src_of_err = Msg_Set_Src(& node->src);
  BoxSrcPos *new_src_pos = & node->src.end;

  /* Output line information to current procedure */
  if (/*c->src_pos.file_name != NULL && */ new_src_pos->line != 0 &&
      (new_src_pos->file_name != c->src_pos.file_name
       || new_src_pos->line != c->src_pos.line)) {
    CmpProc_Associate_Source(c->cur_proc, new_src_pos);
    c->src_pos = *new_src_pos;
  }

  switch(node->type) {
  case ASTNODETYPE_ERROR:
    My_Compile_Error(c, node); break;
  case ASTNODETYPE_TYPENAME:
    My_Compile_TypeName(c, node); break;
  case ASTNODETYPE_TYPETAG:
    My_Compile_TypeTag(c, node); break;
  case ASTNODETYPE_SUBTYPE:
    My_Compile_Subtype(c, node); break;
  case ASTNODETYPE_BOX:
    My_Compile_Box(c, node, BOXTYPE_NONE, BOXTYPE_NONE); break;
  case ASTNODETYPE_STRING:
    My_Compile_String(c, node); break;
  case ASTNODETYPE_CONST:
    My_Compile_Const(c, node); break;
  case ASTNODETYPE_VAR:
    My_Compile_Var(c, node); break;
  case ASTNODETYPE_IGNORE:
    My_Compile_Ignore(c, node); break;
  case ASTNODETYPE_UNOP:
    My_Compile_UnOp(c, node); break;
  case ASTNODETYPE_BINOP:
    My_Compile_BinOp(c, node); break;
  case ASTNODETYPE_STRUC:
    My_Compile_Struc(c, node); break;
  case ASTNODETYPE_MEMBERGET:
    My_Compile_MemberGet(c, node); break;
  case ASTNODETYPE_SUBTYPEBLD:
    My_Compile_SubtypeBld(c, node); break;
  case ASTNODETYPE_SELFGET:
    My_Compile_SelfGet(c, node); break;
  case ASTNODETYPE_PROCDEF:
    My_Compile_ProcDef(c, node); break;
  case ASTNODETYPE_TYPEDEF:
    My_Compile_TypeDef(c, node); break;
  case ASTNODETYPE_STRUCTYPE:
    My_Compile_StrucType(c, node); break;
  case ASTNODETYPE_SPECTYPE:
    My_Compile_SpecType(c, node); break;
  case ASTNODETYPE_INCTYPE:
    My_Compile_IncType(c, node); break;
  default:
    printf("Compilation of node is not implemented, yet!\n");
    break;
  }

  (void) Msg_Set_Src(prev_src_of_err);
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
    /* We return a copy, not the original! */
    Value *v_copy = Value_New(c->cur_proc);
    Value_Setup_As_Weak_Copy(v_copy, v);
    Value_Unlink(v);
    BoxCmp_Push_Value(c, v_copy);

  } else {
    v = Value_New(c->cur_proc);
    Value_Setup_As_Type_Name(v, type_name);
    Namespace_Add_Value(& c->ns, f, type_name, v);
    BoxCmp_Push_Value(c, v);
  }
}

static void My_Compile_TypeTag(BoxCmp *c, ASTNode *n) {
  Value *v;

  assert(n->type == ASTNODETYPE_TYPETAG);
  assert(TS_Is_Special(n->attr.typetag.type) != BOXTYPE_NONE);

  /* Should we use c->value.create, etc. ? */
  v = Value_New(c->cur_proc);
  Value_Init(v, c->cur_proc);
  Value_Setup_As_Type(v, n->attr.typetag.type);
  BoxCmp_Push_Value(c, v);
}

static void My_Compile_Subtype(BoxCmp *c, ASTNode *p) {
  TS *ts = & c->ts;
  Value *parent_type;
  const char *name = p->attr.subtype.name;
  BoxType new_subtype = BOXTYPE_NONE;

  assert(p->type == ASTNODETYPE_SUBTYPE);
  assert(p->attr.subtype.parent != NULL);

  My_Compile_Any(c, p->attr.subtype.parent);
  if (BoxCmp_Pop_Errors(c, /* pop */ 1, /* push err */ 1))
    return;

  parent_type = BoxCmp_Pop_Value(c);
  if (Value_Want_Has_Type(parent_type)) {
    BoxType pt = parent_type->type;
    if (TS_Is_Subtype(ts, pt)) {
      /* Our parent is already a subtype (example X.Y) and we want X.Y.Z:
       * we then require X.Y to be a registered subtype
       */
      if (TS_Subtype_Is_Registered(ts, pt)) {
        new_subtype = TS_Subtype_Find(ts, pt, name);
        if (new_subtype == BOXTYPE_NONE)
          new_subtype = TS_Subtype_New(ts, pt, name);

      } else {
        MSG_ERROR("Cannot build subtype '%s' of undefined subtype '%~s'.",
                  name, TS_Name_Get(ts, pt));
      }

    } else {
      new_subtype = TS_Subtype_Find(ts, pt, name);
      if (new_subtype == BOXTYPE_NONE)
        new_subtype = TS_Subtype_New(ts, pt, name);
    }
  }

  Value_Unlink(parent_type);

  if (new_subtype != BOXTYPE_NONE) {
    Value *v = Value_New(c->cur_proc);
    Value_Setup_As_Type(v, new_subtype);
    BoxCmp_Push_Value(c, v);

  } else
    BoxCmp_Push_Error(c, 1);
}

static void My_Compile_Statement(BoxCmp *c, ASTNode *s) {
  assert(s->type == ASTNODETYPE_STATEMENT);

  if (s->attr.statement.target != NULL) {
    assert(s->attr.statement.sep == ASTSEP_VOID);
    My_Compile_Any(c, s->attr.statement.target);

  } else {
    assert(s->attr.statement.sep != ASTSEP_VOID);
    Value_Link(& c->value.pause);
    BoxCmp_Push_Value(c, & c->value.pause);
  }
}

static void My_Compile_Box(BoxCmp *c, ASTNode *box,
                           BoxType t_child, BoxType t_parent) {
  TS *ts = & c->ts;
  ASTNode *s;
  Value *parent = NULL;
  int parent_is_err = 0;
  BoxVMSymID jump_label_begin, jump_label_end;

  assert(box->type == ASTNODETYPE_BOX);

  if (box->attr.box.parent == NULL) {
    parent = My_Get_Void_Value(c);
    BoxCmp_Push_Value(c, parent);

  } else {
    Value *parent_type;
    My_Compile_Any(c, box->attr.box.parent);
    parent_type = BoxCmp_Pop_Value(c);
    parent = Value_To_Temp_Or_Target(parent_type);
    parent_is_err = Value_Is_Err(parent);
    Value_Unlink(parent_type); /* XXX */
    BoxCmp_Push_Value(c, parent);
  }

  Namespace_Floor_Up(& c->ns); /* variables defined in this box will be
                                  destroyed when it gets closed! */

  /* Add $ (the child) to namespace */
  if (t_child != BOXTYPE_NONE) {
    Value *v_child = Value_New(c->cur_proc);
    Value_Setup_As_Child(v_child, t_child);
    Namespace_Add_Value(& c->ns, NMSPFLOOR_DEFAULT, "$", v_child);
    Value_Unlink(v_child);
  }

  /* Add $$ (the parent) to namespace */
  if (t_parent != BOXTYPE_NONE) {
    Value *v_parent = Value_New(c->cur_proc);
    Value_Setup_As_Parent(v_parent, t_parent);
    Namespace_Add_Value(& c->ns, NMSPFLOOR_DEFAULT, "$$", v_parent);
    Value_Unlink(v_parent); /* has already a link from the namespace */
    parent = v_parent;      /* So that Int@X[] behaves somewhat like X[] */
  }

  {
    /* # represents internally the object the Box is constructing, so that
     * implicit members can access it. Like:
     *   X = (Real a, b)
     *   x = X[.a = 1.0]
     * It also works in procedure definitions:
     *   Int@X[.a = $]
     * The implicit member is .a and internally it is treated as #.a, even
     * if such syntax is not available to the user.
     *
     * NOTE: the following works also when parent = ERROR
     */
    Value *v_parent = Value_New(c->cur_proc);
    Value_Setup_As_Weak_Copy(v_parent, parent);
    v_parent = Value_Promote_Temp_To_Target(v_parent);
    /* ^^^ Promote # (the Box object) to a target so that it can be
     * changed inside the Box
     */
    Namespace_Add_Value(& c->ns, NMSPFLOOR_DEFAULT, "#", v_parent);
    /* ^^^ adding # to the namespace removes all spurious error messages
     *     for parent == NULL.
     */
    Value_Unlink(v_parent);
  }

  /* Invoke the opening procedure */
  if (box->attr.box.parent != NULL) {
    Value_Link(& c->value.begin);
    (void) Value_Emit_Call_Or_Blacklist(parent, & c->value.begin);
  }

  /* Create jump-labels for If and For */
  jump_label_begin = CmpProc_Jump_Label_Here(c->cur_proc);
  jump_label_end = CmpProc_Jump_Label_New(c->cur_proc);

  /* Loop over all the statements of the box */
  for(s = box->attr.box.first_statement;
      s != NULL;
      s = s->attr.statement.next_statement) {
    Value *stmt_val;

    My_Compile_Statement(c, s);
    stmt_val = BoxCmp_Pop_Value(c);

    if (!(parent_is_err || Value_Is_Ignorable(stmt_val))) {
      if (Value_Want_Has_Type(stmt_val)) {
        BoxTask status;
        stmt_val = Value_Emit_Call(parent, stmt_val, & status);

        if (stmt_val != NULL) {
          BoxType stmt_type = stmt_val->type;

          assert(status == BOXTASK_FAILURE);

          /* Treat the case where stmt_val is an If[] or For[] value */
          if (TS_Compare(ts, stmt_type, c->bltin.alias_if))
            Value_Emit_CJump(stmt_val, jump_label_end);

          else if (TS_Compare(ts, stmt_type, c->bltin.alias_for))
            Value_Emit_CJump(stmt_val, jump_label_begin);

          else {
            MSG_WARNING("Don't know how to use '%~s' expressions inside "
                        "a '%~s' box.",
                        TS_Name_Get(& c->ts, stmt_type),
                        TS_Name_Get(& c->ts, parent->type));
            Value_Unlink(stmt_val);
          }

          stmt_val = NULL; /* To prevent double unlink */
        }
      }
    }

    Value_Unlink(stmt_val);
  }

  /* Define the end label and release it together with the begin label */
  CmpProc_Jump_Label_Define(c->cur_proc, jump_label_end);
  CmpProc_Jump_Label_Release(c->cur_proc, jump_label_end);
  CmpProc_Jump_Label_Release(c->cur_proc, jump_label_begin);

  /* Invoke the closing procedure */
  if (box->attr.box.parent != NULL) {
    Value_Link(& c->value.end);
    (void) Value_Emit_Call_Or_Blacklist(parent, & c->value.end);
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
    Value_Unlink(v);

  } else {
    v = Value_New(c->cur_proc);
    Value_Setup_As_Var_Name(v, item_name);
    Namespace_Add_Value(& c->ns, NMSPFLOOR_DEFAULT, item_name, v);
    BoxCmp_Push_Value(c, v);
  }
}

static void My_Compile_Ignore(BoxCmp *c, ASTNode *n) {
  Value *operand;

  assert(n->type == ASTNODETYPE_IGNORE);

  /* Compile operand and get it from the stack */
  My_Compile_Any(c, n->attr.ignore.expr);
  operand = BoxCmp_Get_Value(c, 0);

  Value_Set_Ignorable(operand, n->attr.ignore.ignore);
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
  Value *operand, *v_result = NULL;

  assert(n->type == ASTNODETYPE_UNOP);

  /* Compile operand and get it from the stack */
  My_Compile_Any(c, n->attr.un_op.expr);
  if (BoxCmp_Pop_Errors(c, /* pop */ 1, /* push err */ 1))
    return;

  operand = BoxCmp_Pop_Value(c);
  if (Value_Want_Value(operand))
    v_result = BoxCmp_Opr_Emit_UnOp(c, n->attr.un_op.operation, operand);
  else
    Value_Unlink(operand);

  BoxCmp_Push_Value(c, v_result);
}

/** Deal with assignments.
 * REFERENCES: result: new, left: -1, right: -1;
 */
static Value *My_Compile_Assignment(BoxCmp *c, Value *left, Value *right) {
  if (Value_Want_Value(right)) {
    /* Subtypes are always expanded in assignments */
    left = Value_Expand_Subtype(left);
    /* ^^^ XXX NOTE: The species expansion above is used to allow
     * the following:
     *
     *   P = Point
     *   P.Y = Real
     *   P.Y[$$ = $.y]
     *
     * To expand $$ to the child and assign to it the value 1.
     * If we remove this expansion, then $$ = $.y fails, because $$ is seen as
     * an object of type P.Y, rather than an object of type Real.
     */
    right = Value_Expand_Subtype(right);
    /* XXX NOTE: The line above will never allow one to have a variable with
     * type X.Y
     * Can we live with that?
     * Maybe, we should expand types only when necessary...
     */

    /* If the value is an identifier (thing without type, nor value),
     * then we transform it to a proper target.
     */
    if (Value_Is_Var_Name(left))
      Value_Setup_As_Var(left, right->type);

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
      if (Value_Want_Value(left) & Value_Want_Value(right))
        /* NOTE: ^^^ We use & rather than &&*/
        result = BoxCmp_Opr_Emit_BinOp(c, op, left, right);
      else {
        Value_Unlink(left);
        Value_Unlink(right);
      }
    }

    BoxCmp_Push_Value(c, result);
  }
}

static void My_Compile_Struc(BoxCmp *c, ASTNode *n) {
  int i, num_members;
  ASTNode *member;
  BoxType t_struc;
  Value *v_struc, *v_struc_memb;
  BoxType t_struc_memb;
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
  t_struc = TS_Structure_Begin(& c->ts);
  for(member = n->attr.struc.first_member;
      member != NULL;
      member = member->attr.member.next) {
    Value *v_member = BoxCmp_Get_Value(c, --i);
    TS_Structure_Add(& c->ts, t_struc, v_member->type,
                     member->attr.member.name);
  }

  /* create and populate the structure */
  v_struc = Value_New(c->cur_proc);
  Value_Setup_As_Temp(v_struc, t_struc);
  v_struc_memb = Value_New(c->cur_proc);
  Value_Setup_As_Weak_Copy(v_struc_memb, v_struc);
  t_struc_memb = BOXTYPE_NONE;
  for(i = 1; i <= num_members; i++) {
    Value *v_member = BoxCmp_Get_Value(c, num_members - i);
    v_struc_memb = Value_Struc_Get_Next_Member(v_struc_memb, & t_struc_memb);
    assert(t_struc_memb != BOXTYPE_NONE);
    Value_Link(v_member);
    Value_Move_Content(v_struc_memb, v_member);
  }

  Value_Unlink(v_struc_memb);
  BoxCmp_Remove_Any(c, num_members);
  BoxCmp_Push_Value(c, v_struc);
}

static void My_Compile_MemberGet(BoxCmp *c, ASTNode *n) {
  Value *v_struc, *v_memb = NULL;
  ASTNode *n_struc;

  assert(n->type == ASTNODETYPE_MEMBERGET);

  n_struc = n->attr.member_get.struc;
  if (n_struc == NULL) {
    v_struc = Namespace_Get_Value(& c->ns, NMSPFLOOR_DEFAULT, "#");
    if (v_struc == NULL) {
      MSG_ERROR("Cannot get implicit member '%s'. Default parent is not "
                "defined in current scope.", n->attr.member_get.member);
      BoxCmp_Push_Value(c, NULL);
      return;
    }

  } else {
    My_Compile_Any(c, n_struc);
    v_struc = BoxCmp_Pop_Value(c);
  }

  if (Value_Want_Value(v_struc)) {
    BoxType t_struc = v_struc->type;
    v_memb = Value_Struc_Get_Member(v_struc, n->attr.member_get.member);
    /* No need to unlink v_struc here */
    if (v_memb == NULL)
      MSG_ERROR("Cannot find the member '%s' of an object with type '%~s'.",
                n->attr.member_get.member, TS_Name_Get(& c->ts, t_struc));

  } else
    Value_Unlink(v_struc);

  BoxCmp_Push_Value(c, v_memb);
}

static void My_Compile_SubtypeBld(BoxCmp *c, ASTNode *n) {
  Value *v_parent = NULL, *v_result = NULL;

  assert(n->type == ASTNODETYPE_SUBTYPEBLD);

  if (n->attr.subtype_bld.parent != NULL) {
    My_Compile_Any(c, n->attr.subtype_bld.parent);
    if (BoxCmp_Pop_Errors(c, /* pop */ 1, /* push err */ 1))
      return;
    v_parent = BoxCmp_Pop_Value(c);

  } else {
    v_parent = Namespace_Get_Value(& c->ns, NMSPFLOOR_DEFAULT, "#");
    if (v_parent == NULL) {
      MSG_ERROR("Cannot get implicit method '%s'. Default parent is not "
                "defined in current scope.", n->attr.subtype_bld.subtype);
    }
  }

  if (v_parent != NULL) {
    if (Value_Want_Value(v_parent))
        v_result = Value_Subtype_Build(v_parent, n->attr.subtype_bld.subtype);
    else
        Value_Unlink(v_parent);
  }

  BoxCmp_Push_Value(c, v_result);
}

static void My_Compile_SelfGet(BoxCmp *c, ASTNode *n) {
  Value *v_self = NULL;
  const char *n_self = NULL;
  ASTSelfLevel self_level = n->attr.self_get.level;
  int i, promote_to_target = 0;

  assert(n->type == ASTNODETYPE_SELFGET);

  switch (self_level) {
  case 1:
    n_self = "$";
    v_self = Namespace_Get_Value(& c->ns, NMSPFLOOR_DEFAULT, "$");
    break;

  case 2:
    n_self = "$$";
    v_self = Namespace_Get_Value(& c->ns, NMSPFLOOR_DEFAULT, "$$");
//     v_self = Value_Subtype_Get_Child(v_self); /* WARNING remove that later */
    promote_to_target = 1;
    break;

  default:
    n_self = "$$, $3, ...";
    v_self = Namespace_Get_Value(& c->ns, NMSPFLOOR_DEFAULT, "$$");
    for (i = 2; i < self_level && v_self != NULL; i++)
      v_self = Value_Subtype_Get_Parent(v_self);
    promote_to_target = 1;
  }

  if (v_self == NULL) {
    MSG_ERROR("%s not defined in the current scope.", n_self);

  } else {
    /* Return only a weak copy */
    Value *v_copy = Value_New(c->cur_proc);
    Value_Setup_As_Weak_Copy(v_copy, v_self);
    Value_Unlink(v_self);
    v_self = v_copy;

    if (promote_to_target)
      v_self = Value_Promote_Temp_To_Target(v_self);
  }

  BoxCmp_Push_Value(c, v_self);
}

static void My_Compile_ProcDef(BoxCmp *c, ASTNode *n) {
  Value *v_child, *v_parent, *v_ret = NULL;
  BoxType t_child, t_parent,
          t_proc = BOXTYPE_NONE;
  ASTNode *n_c_name = n->attr.proc_def.c_name,
          *n_implem = n->attr.proc_def.implem;
  char *c_name = NULL;
  CmpProc proc_implem;
  CmpProcStyle proc_style;
  int no_err, do_register = 1;

  assert(n->type == ASTNODETYPE_PROCDEF);

  /* first, get the type of child and parent */
  My_Compile_Any(c, n->attr.proc_def.child_type);
  v_child = BoxCmp_Pop_Value(c);
  My_Compile_Any(c, n->attr.proc_def.parent_type);
  v_parent = BoxCmp_Pop_Value(c);

  no_err = Value_Want_Has_Type(v_child) & Value_Want_Has_Type(v_parent);
  t_child = v_child->type;
  t_parent = v_parent->type;
  Value_Unlink(v_child);
  Value_Unlink(v_parent);

  /* now get the C-name, if present */
  if (n_c_name != NULL) {
    assert(n_c_name->type == ASTNODETYPE_STRING);
    c_name = n_c_name->attr.string.str;
    if (strlen(c_name) < 1) {
      /* NOTE: Should we test any other kind of "badness"? */
      MSG_ERROR("Empty string in C-name for procedure declaration.");
      no_err = 0;
    }
  }

  /* Now let's find whether a procedure of this kind is already registered */
  if (!no_err) {
    /* For now we cowardly refuse to examine the body of the procedure
     * and immediately exit pushing an error.
     */
    BoxCmp_Push_Value(c, v_ret);
    return;
  }

  /* A CmpProc object is used to get the procedure symbol and to register and
   * assemble it.
   */
  proc_style = (n_implem != NULL) ? CMPPROCSTYLE_SUB : CMPPROCSTYLE_EXTERN;
  CmpProc_Init(& proc_implem, c, proc_style);

  t_proc = TS_Procedure_Search(& c->ts, /*expansion_type*/ NULL,
                               t_child, t_parent, 0);

  if (t_proc != BOXTYPE_NONE) {
    /* A procedure of this kind is already registered: we need to know if
     * it is defined or not
     */
    BoxVMSymID sym_id = TS_Procedure_Get_Sym(& c->ts, t_proc);
    int not_defined = !BoxVMSym_Is_Defined(c->vm, sym_id),
        has_no_name = (BoxVMSym_Get_Name(c->vm, sym_id) == NULL);

    /* If the procedure is not defined and has no name then we re-use
     * the previous registered one, without covering the old definition.
     */
    if (not_defined && has_no_name) {
      CmpProc_Set_Sym(& proc_implem, sym_id);
      do_register = 0;
    }

    /* The following means that X@Y ? has no effect if X@Y is already
     * registered (no matter if defined or undefined)
     */
    if (c_name == NULL && n_implem == NULL)
      do_register = 0;
  }

  /* Set the C-name of the procedure, if given */
  if (c_name != NULL)
    CmpProc_Set_Name(& proc_implem, c_name);

  /* Register the procedure, covering old ones */
  if (do_register) {
    BoxVMSymID sym_id = CmpProc_Get_Sym(& proc_implem);
    t_proc = TS_Procedure_New(& c->ts, t_parent, t_child);
    TS_Procedure_Register(& c->ts, t_proc, sym_id);
    Namespace_Add_Procedure(& c->ns, NMSPFLOOR_DEFAULT, & c->ts, t_proc);
  }

  /* If an implementation is also provided, then we define the procedure */
  if (n_implem != NULL) {
    /* we have the implementation */
    CmpProc *save_cur_proc = c->cur_proc;
    Value *v_implem;
    BoxVMCallNum call_num;

    /* Set the alternative name to make the bytecode more readable */
    if (t_proc != BOXTYPE_NONE) {
      char *alter_name = TS_Name_Get(& c->ts, t_proc);
      CmpProc_Set_Alter_Name(& proc_implem, alter_name);
      BoxMem_Free(alter_name);
    }

    /* We change target of the compilation to the new procedure */
    c->cur_proc = & proc_implem;

    /* Specify the prototype for the procedure */
    CmpProc_Set_Prototype(& proc_implem,
                          !TS_Is_Empty(& c->ts, t_child),
                          !TS_Is_Empty(& c->ts, t_parent));

    /* If this is a creator then we should automatically insert some code at
     * the beginning to initialise the stuff the user is not responsible of.
     * For example, if X = (Int a, Str b, Real c) then (.[)@X should start
     * with creating X.b, since X.b is a Str object and has a creator.
     * We must do this in order to avoid segfaults. Box should never give
     * the chance to get to segfaults, at least for programs written
     * entirely in the Box language.
     */
    if (TS_Is_Special(t_child) == BOXTYPE_CREATE) {
      (void) Auto_Generate_Code(c, t_child, t_parent);
    }

    My_Compile_Box(c, n_implem, t_child, t_parent);
    v_implem = BoxCmp_Pop_Value(c);
    /* NOTE: we should double check that this is void! */
    Value_Unlink(v_implem);

    /* Similarly for creators, we need to automatically generate code to
     * destroy composite objects.
     */
    if (TS_Is_Special(t_child) == BOXTYPE_DESTROY) {
      (void) Auto_Generate_Code(c, t_child, t_parent);
    }

    c->cur_proc = save_cur_proc;

    {
      call_num = CmpProc_Install(& proc_implem);
      BoxVMSymID sym_id = CmpProc_Get_Sym(& proc_implem);
      BoxVMSym_Def_Call(c->vm, sym_id);
    }
  }

  CmpProc_Finish(& proc_implem);

  /* NOTE: for now we return Void[]. In future extensions we'll return
   * a function object
   */
  v_ret = My_Get_Void_Value(c);

  /* for now we return v_ret = NULL. We'll return a function, when Box will
   * support functions.
   */
  BoxCmp_Push_Value(c, v_ret);
}

static void My_Compile_TypeDef(BoxCmp *c, ASTNode *n) {
  Value *v_name, *v_type, *v_named_type = NULL;
  TS *ts = & c->ts;

  assert(n->type == ASTNODETYPE_TYPEDEF);

  My_Compile_Any(c, n->attr.type_def.name);
  My_Compile_Any(c, n->attr.type_def.src_type);

  v_type = BoxCmp_Pop_Value(c);
  v_name = BoxCmp_Pop_Value(c);

  if (Value_Want_Has_Type(v_type)) {
    if (Value_Is_Type_Name(v_name)) {
      Value *v = Value_New(c->cur_proc);
      BoxType new_type;

      /* First create the alias type */
      new_type = TS_Alias_New(ts, v_type->type);
      TS_Name_Set(ts, new_type, v_name->name);

      /* Register the type in the proper namespace */
      v = Value_New(c->cur_proc);
      Value_Setup_As_Type(v, new_type);
      Namespace_Add_Value(& c->ns, NMSPFLOOR_DEFAULT, v_name->name, v);

      /* Return a copy of the created type */
      v_named_type = Value_New(c->cur_proc);
      Value_Setup_As_Weak_Copy(v_named_type, v);
      Value_Unlink(v);

    } else if (Value_Has_Type(v_name)) {
      BoxType t = v_name->type;

      if (TS_Is_Subtype(ts, t)) {
        if (TS_Subtype_Is_Registered(ts, t)) {
          BoxType ot = TS_Subtype_Get_Child(ts, t);
          if (TS_Compare(ts, ot, v_type->type) == TS_TYPES_UNMATCH)
            MSG_ERROR("Inconsistent redefinition of type '%~s': was '%~s' "
                      "and is now '%~s'", TS_Name_Get(ts, v_name->type),
                      TS_Name_Get(ts, ot), TS_Name_Get(ts, v_type->type));

        } else {
          (void) TS_Subtype_Register(ts, t, v_type->type);
          /* ^^^ ignore state of success of operation */
        }

      } else if (TS_Compare(ts, t, v_type->type) == TS_TYPES_UNMATCH) {
        MSG_ERROR("Inconsistent redefinition of type '%~s.''",
                  TS_Name_Get(ts, v_name->type));
      }

      v_named_type = v_type;
      Value_Link(v_type);
    }
  }

  Value_Unlink(v_type);
  Value_Unlink(v_name);

  BoxCmp_Push_Value(c, v_named_type);
}

static void My_Compile_StrucType(BoxCmp *c, ASTNode *n) {
  int err;
  ASTNode *member;
  BoxType previous_type = BOXTYPE_NONE, struc_type;
  Value *v_struc_type;

  assert(n->type == ASTNODETYPE_STRUCTYPE);

  /* Create a new structure type */
  struc_type = TS_Structure_Begin(& c->ts);

  /* Compile the members, check their types and leave them on the stack */
  err = 0;
  for(member = n->attr.struc_type.first_member;
      member != NULL;
      member = member->attr.member_type.next) {
    char *member_name = member->attr.member_type.name;

    assert(member->type == ASTNODETYPE_MEMBERTYPE);

    if (member->attr.member_type.type != NULL) {
      Value *v_type;
      My_Compile_Any(c, member->attr.member_type.type);
      v_type = BoxCmp_Pop_Value(c);
      if (Value_Want_Has_Type(v_type))
        previous_type = v_type->type;
      else
        err = 1;
      Value_Unlink(v_type);

    } else {
      assert(member != NULL);
    }

    if (previous_type != BOXTYPE_NONE && !err) {
      /* Check for duplicate structure members */
      if (member_name != NULL)
        if (TS_Member_Find(& c->ts, struc_type, member_name) != BOXTYPE_NONE)
          MSG_ERROR("Duplicate member '%s' in structure type definition.",
                    member_name);
      TS_Structure_Add(& c->ts, struc_type, previous_type, member_name);
    }
  }

  /* Check for errors */
  if (err) {
    BoxCmp_Push_Error(c, 1);
    return;

  } else {
    v_struc_type = Value_New(c->cur_proc);
    Value_Setup_As_Type(v_struc_type, struc_type);
    BoxCmp_Push_Value(c, v_struc_type);
  }
}

static void My_Compile_SpecType(BoxCmp *c, ASTNode *n) {
  BoxType spec_type;
  Value *v_spec_type;
  ASTNode *member;

  assert(n->type == ASTNODETYPE_SPECTYPE);

  /* Create a new species type */
  TS_Species_Begin(& c->ts, & spec_type);

  for(member = n->attr.spec_type.first_member;
      member != NULL;
      member = member->attr.member_type.next) {
    Value *v_type;

    assert(member->type == ASTNODETYPE_MEMBERTYPE);
    assert(member->attr.member_type.name == NULL
           && member->attr.member_type.type != NULL);

    My_Compile_Any(c, member->attr.member_type.type);
    v_type = BoxCmp_Pop_Value(c);

    if (Value_Want_Has_Type(v_type)) {
      BoxType memb_type = v_type->type;
      /* NOTE: should check for duplicate types in species */
      TS_Species_Add(& c->ts, spec_type, memb_type);
    }

    Value_Unlink(v_type);
  }

  v_spec_type = Value_New(c->cur_proc);
  Value_Setup_As_Type(v_spec_type, spec_type);

  BoxCmp_Push_Value(c, v_spec_type);
}

static void My_Compile_IncType(BoxCmp *c, ASTNode *n) {
  Value *v_type, *v_inc_type = NULL;
  BoxType t_inc_type;

  assert(n->type == ASTNODETYPE_INCTYPE);

  My_Compile_Any(c, n->attr.inc_type.type);
  if (BoxCmp_Pop_Errors(c, /* pop */ 1, /* push err */ 1))
    return;

  v_type = BoxCmp_Pop_Value(c);
  if (Value_Want_Has_Type(v_type)) {
    t_inc_type = TS_Detached_New(& c->ts, v_type->type);
    v_inc_type = Value_New(c->cur_proc);
    Value_Setup_As_Type(v_inc_type, t_inc_type);
  }

  BoxCmp_Push_Value(c, v_inc_type);
}

