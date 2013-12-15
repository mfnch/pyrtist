/****************************************************************************
 * Copyright (C) 2009-2013 by Matteo Franchin                               *
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
#include "vmcode.h"
#include "operator.h"
#include "namespace.h"
#include "messages.h"
#include "parser.h"
#include "combs.h"
#include "vmsymstuff.h"

#include "compiler_priv.h"

/* For now it is safer not to cache const values!
 * (will make easier to detect reference count problems)
 */
#define DONT_CACHE_CONST_VALUES 1

/**
 * @brief Type of items which may be inserted inside the compiler stack.
 * @see StackItem
 */
typedef enum {
  STACKITEM_ERROR,
  STACKITEM_VALUE
} StackItemType;

/**
 * @brief Called when removing the item from the stack.
 * @see StackItem
 */
typedef void (*StackItemDestructor)(void *item);

/**
 * The compiler has a stack of values which are currently being
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

/**
 * Here we define, once for all, a number of useful Value instances which
 * are used massively in the compiler. We could just allocate and create
 * them whenever needed, but we do it here and just return new references
 * to them whenever it is needed, with the hope to improve performance.
 */
static void My_Init_Const_Values(BoxCmp *c)
{
  Value_Init(& c->value.error, & c->main_proc);
  Value_Init(& c->value.void_val, & c->main_proc);
  Value_Setup_As_Void(& c->value.void_val);
  BoxCont_Set(& c->cont.pass_child, "go", 2);
  BoxCont_Set(& c->cont.pass_parent, "go", 1);

  Value_Init(& c->value.create, & c->main_proc);
  Value_Setup_As_Type(& c->value.create, Box_Get_Core_Type(BOXTYPEID_INIT));
  Value_Init(& c->value.destroy, & c->main_proc);
  Value_Setup_As_Type(& c->value.destroy, Box_Get_Core_Type(BOXTYPEID_FINISH));
  Value_Init(& c->value.begin, & c->main_proc);
  Value_Setup_As_Type(& c->value.begin, Box_Get_Core_Type(BOXTYPEID_BEGIN));
  Value_Init(& c->value.end, & c->main_proc);
  Value_Setup_As_Type(& c->value.end, Box_Get_Core_Type(BOXTYPEID_END));
  Value_Init(& c->value.pause, & c->main_proc);
  Value_Setup_As_Temp(& c->value.pause, Box_Get_Core_Type(BOXTYPEID_PAUSE));
}

void My_Finish_Const_Values(BoxCmp *c)
{
  Value_Unlink(& c->value.error);
  Value_Unlink(& c->value.void_val);
  Value_Unlink(& c->value.create);
  Value_Unlink(& c->value.destroy);
  Value_Unlink(& c->value.pause);
}

/** Return a new error value (actually a new reference to it,
 * see My_Init_Const_Values)
 */
Value *My_Value_New_Error(BoxCmp *c)
{
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
static Value *My_Get_Void_Value(BoxCmp *c)
{
#if DONT_CACHE_CONST_VALUES == 1
  Value *v = Value_New(c->cur_proc);
  Value_Setup_As_Void(v);
  return v;
#else
  Value_Link(& c->value.void_val);
  return & c->value.void_val;
#endif
}

void BoxCmp_Init(BoxCmp *c, BoxVM *target_vm)
{
  c->attr.own_vm = (target_vm == NULL);
  c->vm = (target_vm != NULL) ? target_vm : BoxVM_Create();

  BoxArr_Init(& c->stack, sizeof(StackItem), 32);

  BoxBool success = Box_Initialize_Type_System();
  assert(success);

  BoxCmp_Init__Operators(c);

  BoxVMCode_Init(& c->main_proc, c, BOXVMCODESTYLE_MAIN);
  BoxVMCode_Set_Alter_Name(& c->main_proc, "main");
  c->cur_proc = & c->main_proc;

  My_Init_Const_Values(c);
  Namespace_Init(& c->ns);
  Bltin_Init(c);

  BoxSrcPos_Init(& c->src_pos, NULL);
}

void BoxCmp_Finish(BoxCmp *c)
{
  Bltin_Finish(c);
  Namespace_Finish(& c->ns);
  My_Finish_Const_Values(c);
  BoxVMCode_Finish(& c->main_proc);

  if (BoxArr_Num_Items(& c->stack) != 0)
    MSG_WARNING("BoxCmp_Finish: stack is not empty at compiler destruction.");
  BoxArr_Finish(& c->stack);

  BoxCmp_Finish__Operators(c);

  if (c->attr.own_vm)
    BoxVM_Destroy(c->vm);
}

BoxCmp *BoxCmp_Create(BoxVM *target_vm)
{
  BoxCmp *c = Box_Mem_Alloc(sizeof(BoxCmp));
  if (c)
    BoxCmp_Init(c, target_vm);
  return c;
}

void BoxCmp_Destroy(BoxCmp *c)
{
  BoxCmp_Finish(c);
  Box_Mem_Free(c);
}

BoxVM *BoxCmp_Steal_VM(BoxCmp *c)
{
  BoxVM *vm = c->vm;
  c->attr.own_vm = 0;
  c->vm = 0;
  return vm;
}

void BoxCmp_Log_Err(BoxCmp *c, BoxASTNode *node, const char *fmt, ...)
{
  va_list ap;
  const char *msg;
  va_start(ap, fmt);
  msg = Box_VA_Print(fmt, ap);
  MSG_ERROR("%s\n", msg);
  va_end(ap);
}

/* Function which does all the steps needed to get from a Box source file
 * to a VM with the corresponding compiled bytecode.
 */
BoxVM *Box_Compile_To_VM_From_File(BoxVMCallNum *main, BoxVM *target_vm,
                                   FILE *file, const char *file_name,
                                   const char *setup_file_name,
                                   BoxPaths *paths)
{
  BoxAST *ast;
  BoxCmp *compiler;
  BoxVM *vm;
  BoxVMCallNum dummy_cn;

  if (main == NULL)
    main = & dummy_cn;

  compiler = BoxCmp_Create(target_vm);
  ast = Box_Parse_FILE(file, file_name, setup_file_name, paths);
  BoxCmp_Compile(compiler, ast);
  BoxAST_Destroy(ast);
  *main = BoxVMCode_Install(& compiler->main_proc);
  vm = BoxCmp_Steal_VM(compiler);
  BoxCmp_Destroy(compiler);
  return vm;
}

void BoxCmp_Remove_Any(BoxCmp *c, int num_items_to_remove)
{
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
void BoxCmp_Push_Error(BoxCmp *c, int num_errors)
{
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
int BoxCmp_Pop_Errors(BoxCmp *c, int items_to_pop, int errors_to_push)
{
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
void BoxCmp_Push_Value(BoxCmp *c, Value *v)
{
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
Value *BoxCmp_Pop_Value(BoxCmp *c)
{
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

Value *BoxCmp_Get_Value(BoxCmp *c, BoxInt pos)
{
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

/* Forward declarations. */
#define BOXASTNODE_DEF(NODE, Node) \
  static void My_Compile_##Node(BoxCmp *, BoxASTNode *);
static void My_Compile_Any(BoxCmp *c, BoxASTNode *node);
#include "astnodes.h"
#undef BOXASTNODE_DEF

void BoxCmp_Compile(BoxCmp *c, BoxAST *ast)
{
  BoxASTNode *root;

  if (!ast)
    return;

  root = BoxAST_Get_Root(ast);
  if (!root)
    return;

  My_Compile_Any(c, root);
  BoxCmp_Remove_Any(c, 1);
}

static void My_Compile_Any(BoxCmp *c, BoxASTNode *node)
{
  BoxASTNodeType node_type = BoxASTNode_Get_Type(node);

  printf("Compiling node %s\n", BoxASTNodeType_To_Str(node_type));

#define BOXASTNODE_DEF(NODE, Node) \
  case BOXASTNODETYPE_##NODE: My_Compile_##Node(c, node); break;

  switch (node_type) {
#include "astnodes.h"
  default:
    BoxCmp_Log_Err(c, node, "Compilation of node %s (%d) is not implemented, "
                   " yet!\n", BoxASTNodeType_To_Str(node_type),
                   (int) node_type);
    break;
  }

#undef BOXASTNODE_DEF
}

static void My_Compile_CharImm(BoxCmp *c, BoxASTNode *node)
{
  Value *v = Value_Create(c->cur_proc);
  assert(BoxASTNode_Get_Type(node) == BOXASTNODETYPE_CHAR_IMM);
  Value_Setup_As_Imm_Char(v, ((BoxASTNodeCharImm *) node)->value);
  BoxCmp_Push_Value(c, v);
}

static void My_Compile_IntImm(BoxCmp *c, BoxASTNode *node)
{
  Value *v = Value_Create(c->cur_proc);
  assert(BoxASTNode_Get_Type(node) == BOXASTNODETYPE_INT_IMM);
  Value_Setup_As_Imm_Int(v, ((BoxASTNodeIntImm *) node)->value);
  BoxCmp_Push_Value(c, v);
}

static void My_Compile_RealImm(BoxCmp *c, BoxASTNode *node)
{
  Value *v = Value_Create(c->cur_proc);
  assert(BoxASTNode_Get_Type(node) == BOXASTNODETYPE_REAL_IMM);
  Value_Setup_As_Imm_Real(v, ((BoxASTNodeRealImm *) node)->value);
  BoxCmp_Push_Value(c, v);
}

static void My_Compile_StrImm(BoxCmp *c, BoxASTNode *node)
{
  Value *v = Value_Create(c->cur_proc);
  assert(BoxASTNode_Get_Type(node) == BOXASTNODETYPE_STR_IMM);
  Value_Setup_As_String(v, ((BoxASTNodeStrImm *) node)->str);
  BoxCmp_Push_Value(c, v);
}

#if 0
static void My_Compile_Error(BoxCmp *c, ASTNode *node)
{
  BoxCmp_Push_Value(c, Value_New(c->cur_proc));
}
#endif

static void My_Compile_Idfr(BoxCmp *c, BoxASTNode *node)
{
  Value *v;
  char *type_name = & ((BoxASTNodeIdfr *) node)->name[0];
  NmspFloor f;

  assert(BoxASTNode_Get_Type(node) == BOXASTNODETYPE_IDFR);

  f = NMSPFLOOR_DEFAULT;
  v = Namespace_Get_Value(& c->ns, f, type_name);
  if (v) {
    /* We return a copy, not the original! */
    Value *v_copy = Value_Create(c->cur_proc);
    Value_Setup_As_Weak_Copy(v_copy, v);
    Value_Unlink(v);
    BoxCmp_Push_Value(c, v_copy);
  } else {
    v = Value_Create(c->cur_proc);
    Value_Setup_As_Type_Name(v, type_name);
    Namespace_Add_Value(& c->ns, f, type_name, v);
    BoxCmp_Push_Value(c, v);
  }
}

#if 0
static void My_Compile_TypeTag(BoxCmp *c, ASTNode *n)
{
  Value *v;

  assert(n->type == ASTNODETYPE_TYPETAG);

  /* Should we use c->value.create, etc. ? */
  v = Value_Create(c->cur_proc);
  Value_Setup_As_Type(v, Box_Get_Core_Type(n->attr.typetag.type));
  BoxCmp_Push_Value(c, v);
}

static void My_Compile_Subtype(BoxCmp *c, ASTNode *p) {
  Value *parent_type;
  const char *name = p->attr.subtype.name;
  BoxType *new_subtype = NULL;

  assert(p->type == ASTNODETYPE_SUBTYPE);
  assert(p->attr.subtype.parent);

  My_Compile_Any(c, p->attr.subtype.parent);
  if (BoxCmp_Pop_Errors(c, /* pop */ 1, /* push err */ 1))
    return;

  parent_type = BoxCmp_Pop_Value(c);
  if (Value_Want_Has_Type(parent_type)) {
    BoxType *pt = parent_type->type;
    if (BoxType_Is_Subtype(pt)) {
      /* Our parent is already a subtype (example X.Y) and we want X.Y.Z:
       * we then require X.Y to be a registered subtype
       */
      if (BoxType_Is_Registered_Subtype(pt)) {
        new_subtype = BoxType_Find_Subtype(pt, name);
        if (!new_subtype)
          new_subtype = BoxType_Create_Subtype(pt, name, NULL);
      } else {
        MSG_ERROR("Cannot build subtype '%s' of undefined subtype '%T'.",
                  name, parent_type->type);
      }

    } else {
      new_subtype = BoxType_Find_Subtype(pt, name);
      if (!new_subtype)
        new_subtype = BoxType_Create_Subtype(pt, name, NULL);
    }
  }

  Value_Unlink(parent_type);

  if (new_subtype) {
    Value *v = Value_Create(c->cur_proc);
    Value_Setup_As_Type(v, new_subtype);
    (void) BoxType_Unlink(new_subtype);
    BoxCmp_Push_Value(c, v);
  } else
    BoxCmp_Push_Error(c, 1);
}
#endif

static void My_Compile_Statement(BoxCmp *c, BoxASTNode *stmt_node)
{
  BoxASTNodeStatement *stmt;

  assert(BoxASTNode_Get_Type(stmt_node) == BOXASTNODETYPE_STATEMENT);
  stmt = (BoxASTNodeStatement *) stmt_node;

  if (stmt->sep == BOXASTSEP_PAUSE) {
    Value_Link(& c->value.pause);
    BoxCmp_Push_Value(c, & c->value.pause);
  }

  if (stmt->value)
    My_Compile_Any(c, stmt->value);
  else {
    Value *v_void = My_Get_Void_Value(c);
    BoxCmp_Push_Value(c, v_void);
  }
}

#if 0
static void My_Compile_Instance(BoxCmp *c, ASTNode *instance) {
  assert(instance->type == ASTNODETYPE_INSTANCE);

  My_Compile_Any(c, instance->attr.instance.type);
  if (BoxCmp_Pop_Errors(c, /* pop */ 1, /* push */ 1))
    return;

  else {
    Value *instance = Value_To_Temp_Or_Target(BoxCmp_Pop_Value(c));
    BoxCmp_Push_Value(c, instance);
  }
}
#endif

typedef enum {
  MYBOXSTATE_INITIAL,
  MYBOXSTATE_GOT_IF,
  MYBOXSTATE_GOT_ELSE
} MyBoxState;

static void My_Compile_Box_Generic(BoxCmp *c, BoxASTNode *box_node,
                                   BoxType *t_child, BoxType *t_parent)
{
  BoxASTNodeBox *box;
  BoxASTNodeStatement *s;
  Value *parent = NULL, *outer_parent = NULL;
  BoxBool parent_is_err = 0, need_floor_down;
  BoxVMSymID jump_label_begin, jump_label_end, jump_label_next;
  MyBoxState state;

  assert(BoxASTNode_Get_Type(box_node) == BOXASTNODETYPE_BOX);
  box = (BoxASTNodeBox *) box_node;

  if (box->parent == NULL) {
    Value *v_void = My_Get_Void_Value(c);
    BoxCmp_Push_Value(c, v_void);

    parent = Namespace_Get_Value(& c->ns, NMSPFLOOR_DEFAULT, "#");
    if (parent == NULL)
      parent = v_void;
    else
      outer_parent = parent;

  } else {
    Value *parent_type;
    My_Compile_Any(c, box->parent);
    parent_type = BoxCmp_Pop_Value(c);
    parent = Value_To_Temp_Or_Target(parent_type);
    parent_is_err = Value_Is_Err(parent);
    Value_Unlink(parent_type); /* XXX */
    BoxCmp_Push_Value(c, parent);
  }

  Namespace_Floor_Up(& c->ns); /* variables defined in this box will be
                                  destroyed when it gets closed! */

  /* Add $ (the child) to namespace */
  if (t_child) {
    Value *v_child = Value_New(c->cur_proc);
    Value_Setup_As_Child(v_child, t_child);
    Namespace_Add_Value(& c->ns, NMSPFLOOR_DEFAULT, "$", v_child);
    Value_Unlink(v_child);
  }

  /* Add $$ (the parent) to namespace */
  if (t_parent) {
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
  if (box->parent) {
    Value_Link(& c->value.begin);
    (void) Value_Emit_Call_Or_Blacklist(parent, & c->value.begin);
  }

  /* Create jump-labels for If and For */
  jump_label_begin = BoxVMCode_Jump_Label_Here(c->cur_proc);
  jump_label_next = BoxVMCode_Jump_Label_New(c->cur_proc);
  jump_label_end = BOXVMSYMID_NONE;

  /* Save previous source position */
  //BoxSrc *prev_src_of_err = Msg_Set_Src(& box->head.src);

  need_floor_down = BOXBOOL_FALSE;

  /* Loop over all the statements of the box */
  for(s = box->first_stmt, state = MYBOXSTATE_INITIAL; s; s = s->next) {
    Value *stmt_val;

    /* Set the source position to the current statement */
    //Msg_Set_Src(& s->head.src);

    My_Compile_Statement(c, (BoxASTNode *) s);
    stmt_val = BoxCmp_Pop_Value(c);

    if (!(parent_is_err || Value_Is_Ignorable(stmt_val))) {
      if (Value_Want_Has_Type(stmt_val)) {
        BoxTask emit_task;
        stmt_val = Value_Emit_Call(parent, stmt_val, & emit_task);

        if (stmt_val) {
          assert(emit_task == BOXTASK_FAILURE);

          /* Handle the case where stmt_val is an If[] or For[] value */
          if (BoxType_Compare(stmt_val->type,
                              Box_Get_Core_Type(BOXTYPEID_IF))) {
            Value_Emit_CJump(stmt_val, jump_label_next);

            if (state != MYBOXSTATE_GOT_IF) {
              assert(!need_floor_down);
              Namespace_Floor_Up(& c->ns);
              need_floor_down = BOXBOOL_TRUE;
            }
            state = MYBOXSTATE_GOT_IF;

          } else if (BoxType_Compare(stmt_val->type,
                                     Box_Get_Core_Type(BOXTYPEID_ELSE))) {
            if (state == MYBOXSTATE_GOT_IF) {
              if (jump_label_end == BOXVMSYMID_NONE)
                jump_label_end = BoxVMCode_Jump_Label_New(c->cur_proc);
              BoxVMCode_Assemble_Jump(c->cur_proc, jump_label_end);
              BoxVMCode_Jump_Label_Define(c->cur_proc, jump_label_next);
              BoxVMCode_Jump_Label_Release(c->cur_proc, jump_label_next);
              jump_label_next = BoxVMCode_Jump_Label_New(c->cur_proc);

              assert(need_floor_down);
              Namespace_Floor_Down(& c->ns);
              need_floor_down = BOXBOOL_FALSE;

            } else {
              if (state == MYBOXSTATE_GOT_ELSE)
                MSG_ERROR("Double 'Else'.");
              else
                MSG_ERROR("'Else' without 'If'.");
            }
            Value_Unlink(stmt_val);
            state = MYBOXSTATE_GOT_ELSE;

          } else if (BoxType_Compare(stmt_val->type,
                                     Box_Get_Core_Type(BOXTYPEID_FOR)))
            Value_Emit_CJump(stmt_val, jump_label_begin);

          else {
            MSG_WARNING("Don't know how to use '%T' expressions inside "
                        "a '%T' box.", stmt_val->type, parent->type);
            Value_Unlink(stmt_val);
          }

          stmt_val = NULL; /* To prevent double unlink */
        }
      }
    }

    Value_Unlink(stmt_val);
  }

  if (need_floor_down)
    Namespace_Floor_Down(& c->ns);

  /* Restore previous source position */
  //(void) Msg_Set_Src(prev_src_of_err);

  /* Define the end label and release it together with the begin label */
  BoxVMCode_Jump_Label_Define(c->cur_proc, jump_label_next);
  BoxVMCode_Jump_Label_Release(c->cur_proc, jump_label_begin);
  BoxVMCode_Jump_Label_Release(c->cur_proc, jump_label_next);

  /* Define the end label, if used at all! */
  if (jump_label_end != BOXVMSYMID_NONE) {
    BoxVMCode_Jump_Label_Define(c->cur_proc, jump_label_end);
    BoxVMCode_Jump_Label_Release(c->cur_proc, jump_label_end);
  }

  /* Invoke the closing procedure */
  if (box->parent) {
    Value_Link(& c->value.end);
    (void) Value_Emit_Call_Or_Blacklist(parent, & c->value.end);
  }

  Namespace_Floor_Down(& c->ns); /* close the scope unit */

  if (outer_parent)
    Value_Unlink(outer_parent);
}

static void My_Compile_Box(BoxCmp *c, BoxASTNode *node)
{
  My_Compile_Box_Generic(c, node, NULL, NULL);
}

#if 0

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
    if (BoxValue_Is_Var_Name(left)) {
      BoxValue_Assign(left, right);
      Value_Set_Ignorable(left, 1);
      return left;

    } else if (Value_Is_Target(left)) {
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
  BoxType *t_struc;
  Value *v_struc;
  ValueStrucIter vsi;
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
  t_struc = BoxType_Create_Structure();
  for(member = n->attr.struc.first_member; member;
      member = member->attr.member.next) {
    Value *v_member = BoxCmp_Get_Value(c, --i);
    BoxType_Add_Member_To_Structure(t_struc, v_member->type,
                                    member->attr.member.name);
  }

  /* create and populate the structure */
  v_struc = Value_New(c->cur_proc);
  Value_Setup_As_Temp(v_struc, t_struc);

  for(ValueStrucIter_Init(& vsi, v_struc, c->cur_proc);
      vsi.has_next; ValueStrucIter_Do_Next(& vsi)) {
    Value *v_member = BoxCmp_Get_Value(c, num_members - vsi.index - 1);
    Value_Link(v_member);
    Value_Move_Content(& vsi.v_member, v_member);
  }

  ValueStrucIter_Finish(& vsi);

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
    v_memb = Value_Struc_Get_Member(v_struc, n->attr.member_get.member);
    /* No need to unlink v_struc here */
    if (v_memb == NULL)
      MSG_ERROR("Cannot find the member '%s' of an object with type '%T'.",
                n->attr.member_get.member, v_struc->type);

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
  int i, promote_to_target = 0, return_weak_copy = 0;

  assert(n->type == ASTNODETYPE_SELFGET);

  switch (self_level) {
  case 1:
    n_self = "$";
    v_self = Namespace_Get_Value(& c->ns, NMSPFLOOR_DEFAULT, "$");
    return_weak_copy = 1;
    break;

  case 2:
    n_self = "$$";
    v_self = Namespace_Get_Value(& c->ns, NMSPFLOOR_DEFAULT, "$$");
//     v_self = Value_Subtype_Get_Child(v_self); /* WARNING remove that later */
    promote_to_target = 1;
    return_weak_copy = 1;
    break;

  default:
    n_self = "$$, $3, ...";
    v_self = Namespace_Get_Value(& c->ns, NMSPFLOOR_DEFAULT, "$$");
    for (i = 2; i < self_level && v_self != NULL; i++)
      v_self = Value_Subtype_Get_Parent(v_self);
      /* FIXME: see Value_Init */
    promote_to_target = 1;
    return_weak_copy = 0;
  }

  if (v_self == NULL) {
    MSG_ERROR("%s not defined in the current scope.", n_self);

  } else {
    /* Return only a weak copy? */
    if (return_weak_copy) {
      Value *v_copy = Value_New(c->cur_proc);
      Value_Setup_As_Weak_Copy(v_copy, v_self);
      Value_Unlink(v_self);
      v_self = v_copy;
    }

    if (promote_to_target)
      v_self = Value_Promote_Temp_To_Target(v_self);
  }

  BoxCmp_Push_Value(c, v_self);
}

static void My_Compile_ProcDef(BoxCmp *c, ASTNode *n) {
  Value *v_child, *v_parent, *v_ret = NULL;
  BoxCombType comb_type;
  ASTNode *n_c_name = n->attr.proc_def.c_name,
          *n_implem = n->attr.proc_def.implem;
  char *c_name = NULL;
  BoxType *t_child, *t_parent, *comb;
  BoxCallable *comb_callable;
  int no_err;

  assert(n->type == ASTNODETYPE_PROCDEF);

  /* first, get the type of child and parent */
  My_Compile_Any(c, n->attr.proc_def.child_type);
  v_child = BoxCmp_Pop_Value(c);
  My_Compile_Any(c, n->attr.proc_def.parent_type);
  v_parent = BoxCmp_Pop_Value(c);
  comb_type = n->attr.proc_def.combine;

  no_err = Value_Want_Has_Type(v_child) & Value_Want_Has_Type(v_parent);

  /* Get the types from the values, and destroy the latters. */
  t_child = BoxType_Link(v_child->type);
  t_parent = BoxType_Link(v_parent->type);
  Value_Unlink(v_child);
  Value_Unlink(v_parent);

  /* now get the C-name, if present */
  if (n_c_name) {
    assert(n_c_name->type == ASTNODETYPE_STRING);
    c_name = n_c_name->attr.string.str;
    if (strlen(c_name) < 1) {
      /* NOTE: Should we test any other kind of "badness"? */
      MSG_ERROR("Empty string in C-name for procedure declaration.");
      no_err = 0;
    }
  }

  /* Now let's find whether a procedure of this kind is already registered. */
  if (!no_err) {
    /* For now we cowardly refuse to examine the body of the procedure
     * and immediately exit pushing an error.
     */
    BoxCmp_Push_Value(c, v_ret);
    (void) BoxType_Unlink(t_child);
    (void) BoxType_Unlink(t_parent);
    return;
  }

  /* Try to find the combination, if already defined. */
  comb_callable = NULL;
  comb = BoxType_Find_Own_Combination(t_parent, comb_type, t_child, NULL);

  if (comb) {
    /* A combination of this kind is already defined: we need to know if
     * it is implemented or not.
     */
    BoxCallable *cb;
    BoxBool success, is_already_implemented, has_name;

    success = BoxType_Get_Combination_Info(comb, NULL, & cb);
    assert(success);

    is_already_implemented = BoxCallable_Is_Implemented(cb);
    has_name = (BoxCallable_Get_Uid(cb) != NULL);

    /* If the procedure is not implemented and has no name then we re-use
     * the previous one, without covering the old definition.
     */
    if (!is_already_implemented && !has_name)
      comb_callable = cb;

    /* The following means that X@Y ? has no effect if X@Y is already
     * registered (no matter if defined or undefined)
     */
    if (!c_name && !n_implem)
      comb_callable = cb;
  }

  /* Define the combination, if necessary. */
  if (!comb_callable) {
    comb_callable = BoxCallable_Create_Undefined(t_parent, t_child);
    comb = BoxType_Define_Combination(t_parent, comb_type, t_child,
                                      comb_callable);
    Namespace_Add_Procedure(& c->ns, NMSPFLOOR_DEFAULT, t_parent, comb);
  }

  /* Set the C-name of the procedure, if given. */
  if (c_name) {
    BoxCallable_Set_Uid(comb_callable, c_name);
    if (!n_implem)
      BoxVMSym_Reference_Proc(c->vm, comb_callable);
  }

  /* If an implementation is also provided, then we define the procedure */
  if (n_implem) {
    /* we have the implementation */
    BoxVMCode *save_cur_proc = c->cur_proc;
    Value *v_implem;
    BoxVMCode proc_implem;
    BoxVMCallNum cn;

    /* A BoxVMCode object is used to get the procedure symbol and to register
     * and assemble it.
     */
    BoxVMCode_Init(& proc_implem, c, BOXVMCODESTYLE_SUB);

    /* Set the call number. */
    if (!BoxType_Generate_Combination_Call_Num(comb, c->vm, & cn))
      MSG_FATAL("Cannot generate call number for combination.");
    proc_implem.have.call_num = 1;
    proc_implem.call_num = cn;

    /* Set the alternative name to make the bytecode more readable */
    {
      char *alter_name = BoxType_Get_Repr(comb);
      assert(alter_name);

      BoxVMCode_Set_Alter_Name(& proc_implem, alter_name);
      Box_Mem_Free(alter_name);
    }

    /* We change target of the compilation to the new procedure */
    c->cur_proc = & proc_implem;

    /* Specify the prototype for the procedure */
    BoxVMCode_Set_Prototype(& proc_implem,
                            !BoxType_Is_Empty(t_child),
                            !BoxType_Is_Empty(t_parent));

    My_Compile_Box(c, n_implem, t_child, t_parent);
    v_implem = BoxCmp_Pop_Value(c);
    /* NOTE: we should double check that this is void! */
    Value_Unlink(v_implem);

    c->cur_proc = save_cur_proc;

    (void) BoxVMCode_Install(& proc_implem);

    BoxVMCode_Finish(& proc_implem);
  }

  (void) BoxType_Unlink(t_child);
  (void) BoxType_Unlink(t_parent);

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

  assert(n->type == ASTNODETYPE_TYPEDEF);

  My_Compile_Any(c, n->attr.type_def.name);
  My_Compile_Any(c, n->attr.type_def.src_type);

  v_type = BoxCmp_Pop_Value(c);
  v_name = BoxCmp_Pop_Value(c);

  if (Value_Want_Has_Type(v_type)) {
    if (Value_Is_Type_Name(v_name)) {
      Value *v;
      /* Create the new identity type. */
      BoxType *ident_type =
        BoxType_Create_Ident(BoxType_Link(v_type->type), v_name->name);

      /* Register the type in the proper namespace */
      v = Value_Create(c->cur_proc);
      Value_Setup_As_Type(v, ident_type);
      (void) BoxType_Unlink(ident_type);
      Namespace_Add_Value(& c->ns, NMSPFLOOR_DEFAULT, v_name->name, v);

      /* Return a copy of the created type */
      v_named_type = Value_Create(c->cur_proc);
      Value_Setup_As_Weak_Copy(v_named_type, v);
      Value_Unlink(v);

    } else if (Value_Has_Type(v_name)) {
      BoxType *t = v_name->type;
      if (BoxType_Is_Subtype(t)) {
        if (BoxType_Is_Registered_Subtype(t)) {
          BoxType *t_child;
          BoxBool success = BoxType_Get_Subtype_Info(t, NULL, NULL, & t_child);
          assert(success);
          if (BoxType_Compare(t_child, v_type->type) == BOXTYPECMP_DIFFERENT)
            MSG_ERROR("Inconsistent redefinition of type '%T': was '%T' "
                      "and is now '%T'", v_name->type, t_child, v_type->type);

        } else {
          (void) BoxType_Register_Subtype(t, v_type->type);
          /* ^^^ ignore state of success of operation */
        }

      } else if (BoxType_Compare(v_name->type, v_type->type)
                 == BOXTYPECMP_DIFFERENT) {
        MSG_ERROR("Inconsistent redefinition of type '%T.'", v_name->type);
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
  BoxType *previous_type = NULL, *struc_type;
  Value *v_struc_type;

  assert(n->type == ASTNODETYPE_STRUCTYPE);

  /* Create a new structure type */
  struc_type = BoxType_Create_Structure();

  /* Compile the members, check their types and leave them on the stack */
  err = 0;
  for(member = n->attr.struc_type.first_member; member;
      member = member->attr.member_type.next) {
    char *member_name = member->attr.member_type.name;

    assert(member->type == ASTNODETYPE_MEMBERTYPE);

    if (member->attr.member_type.type) {
      Value *v_type;
      My_Compile_Any(c, member->attr.member_type.type);
      v_type = BoxCmp_Pop_Value(c);
      if (Value_Want_Has_Type(v_type))
        previous_type = v_type->type;
      else
        err = 1;
      Value_Unlink(v_type);
    } else {
      assert(member);
    }

    if (previous_type && !err) {
      /* Check for duplicate structure members */
      if (member_name) {
        if (BoxType_Find_Structure_Member(struc_type, member_name))
          MSG_ERROR("Duplicate member '%s' in structure type definition.",
                    member_name);
      }

      BoxType_Add_Member_To_Structure(struc_type, previous_type, member_name);
    }
  }

  /* Check for errors */
  if (err) {
    BoxCmp_Push_Error(c, 1);
    return;

  } else {
    v_struc_type = Value_Create(c->cur_proc);
    Value_Setup_As_Type(v_struc_type, struc_type);
    (void) BoxType_Unlink(struc_type);
    BoxCmp_Push_Value(c, v_struc_type);
  }
}

static void My_Compile_SpecType(BoxCmp *c, ASTNode *n) {
  BoxType *spec_type;
  Value *v_spec_type;
  ASTNode *member;

  assert(n->type == ASTNODETYPE_SPECTYPE);

  /* Create a new species type */
  spec_type = BoxType_Create_Species();

  for(member = n->attr.spec_type.first_member; member;
      member = member->attr.member_type.next) {
    Value *v_type;

    assert(member->type == ASTNODETYPE_MEMBERTYPE);
    assert(member->attr.member_type.name == NULL
           && member->attr.member_type.type != NULL);

    My_Compile_Any(c, member->attr.member_type.type);
    v_type = BoxCmp_Pop_Value(c);

    if (Value_Want_Has_Type(v_type)) {
      BoxType *memb_type = v_type->type;
      /* NOTE: should check for duplicate types in species */
      BoxType_Add_Member_To_Species(spec_type, memb_type);
    }

    Value_Unlink(v_type);
  }

  v_spec_type = Value_Create(c->cur_proc);
  Value_Setup_As_Type(v_spec_type, spec_type);
  (void) BoxType_Unlink(spec_type);

  BoxCmp_Push_Value(c, v_spec_type);
}

static void My_Compile_RaiseType(BoxCmp *c, ASTNode *n) {
  Value *v_type, *v_inc_type = NULL;

  assert(n->type == ASTNODETYPE_RAISETYPE);

  My_Compile_Any(c, n->attr.raise_type.type);
  if (BoxCmp_Pop_Errors(c, /* pop */ 1, /* push err */ 1))
    return;

  v_type = BoxCmp_Pop_Value(c);
  if (Value_Want_Has_Type(v_type)) {
    BoxType *inc_type = BoxType_Create_Raised(BoxType_Link(v_type->type));
    v_inc_type = Value_Create(c->cur_proc);
    Value_Setup_As_Type(v_inc_type, inc_type);
    (void) BoxType_Unlink(inc_type);
  }

  BoxCmp_Push_Value(c, v_inc_type);
}

static void My_Compile_Raise(BoxCmp *c, ASTNode *n) {
  Value *v_operand = NULL;

  assert(n->type == ASTNODETYPE_RAISE);

  My_Compile_Any(c, n->attr.raise.expr);
  if (BoxCmp_Pop_Errors(c, /* pop */ 1, /* push err */ 1))
    return;

  v_operand = BoxCmp_Pop_Value(c);
  if (Value_Want_Value(v_operand)) {
    v_operand = Value_Raise(v_operand);

  } else {
    Value_Unlink(v_operand);
    v_operand = NULL;
  }

  BoxCmp_Push_Value(c, v_operand);

}
#endif
