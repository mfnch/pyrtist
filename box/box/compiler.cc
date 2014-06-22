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

#include "compiler_priv.h"


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
  int i;
  struct {
    BoxTypeId type_id;
    Value *value;
  } ids[] = {
    {BOXTYPEID_INIT, & c->value.create},
    {BOXTYPEID_FINISH, & c->value.destroy},
    {BOXTYPEID_BEGIN, & c->value.begin},
    {BOXTYPEID_END, & c->value.end},
    {BOXTYPEID_PAUSE, & c->value.pause}
  };

  Value_Init(& c->value.error, c);
  Value_Mark_RO(& c->value.error);

  for (i = 0; i < BOX_SIZEOF_ARRAY(ids); i++) {
    Value_Init(ids[i].value, c);
    Value_Setup_As_Type(ids[i].value, Box_Get_Core_Type(ids[i].type_id));
    Value_Mark_RO(ids[i].value);
  }

  Value_Init(& c->value.void_val, c);
  Value_Setup_As_Void(& c->value.void_val);
  Value_Mark_RO(& c->value.void_val);

  BoxCont_Set(& c->cont.pass_child, "go", 2);
  BoxCont_Set(& c->cont.pass_parent, "go", 1);
}

void My_Finish_Const_Values(BoxCmp *c)
{
  Value_Force_Finish(& c->value.error);
  Value_Force_Finish(& c->value.void_val);
  Value_Force_Finish(& c->value.create);
  Value_Force_Finish(& c->value.destroy);
  Value_Force_Finish(& c->value.pause);
}

/** Return a new error value (actually a new reference to it,
 * see My_Init_Const_Values)
 */
Value *My_Value_New_Error(BoxCmp *c)
{
  return & c->value.error; /* return an error value */
}

/* We may optimize this later, by just passing a reference to a Value
 * object which is created once for all at the beginning!
 */
static Value *My_Get_Void_Value(BoxCmp *c)
{
  return & c->value.void_val;
}

void
BoxCmp_Init(BoxCmp *c, BoxVM *target_vm)
{
  BoxLIRNodeProc *proc;

  c->compiler = new Box::Compiler(c);

  c->ast = NULL;
  c->ast_node = NULL;

  c->lir = BoxLIR_Create();
  assert(c->lir);
  c->attr.is_sane = 0;
  c->attr.own_vm = (target_vm == NULL);
  c->vm = (target_vm) ? target_vm : BoxVM_Create();

  BoxArr_Init(& c->stack, sizeof(StackItem), 32);

  BoxBool success = Box_Initialize_Type_System();
  assert(success);

  BoxCmp_Init__Operators(c);
  proc = BoxLIR_Append_Proc(c->lir);
  (void) BoxLIR_Set_Target_Proc(c->lir, proc);
  BoxVMCode_Init(& c->main_proc, c, BOXVMCODESTYLE_MAIN);
  BoxVMCode_Set_Alter_Name(& c->main_proc, "main");
  c->cur_proc = & c->main_proc;

  My_Init_Const_Values(c);
  Namespace_Init(& c->ns);
  Bltin_Init(c);
}

void BoxCmp_Finish(BoxCmp *c)
{
  if (!c)
    return;

  delete c->compiler;

  if (BoxArr_Num_Items(& c->stack) != 0)
    BoxCmp_Log_Warn(c, "BoxCmp_Finish: stack is not empty at destruction");

  BoxLIR_Destroy(c->lir);
  BoxAST_Destroy(c->ast);
  Bltin_Finish(c);
  Namespace_Finish(& c->ns);
  My_Finish_Const_Values(c);
  BoxVMCode_Finish(& c->main_proc);
  BoxArr_Finish(& c->stack);
  BoxCmp_Finish__Operators(c);

  if (c->attr.own_vm)
    BoxVM_Destroy(c->vm);
}

BoxCmp *BoxCmp_Create(BoxVM *target_vm)
{
  BoxCmp *c = (BoxCmp *) Box_Mem_Alloc(sizeof(BoxCmp));
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

BoxASTNode *
BoxCmp_Set_Cur_Node(BoxCmp *c, BoxASTNode *cur_ast_node)
{
  BoxASTNode *prev_ast_node = c->ast_node;
  c->ast_node = cur_ast_node;
  return prev_ast_node;
}

void
BoxCmp_Log(BoxCmp *c, BoxSrc *src, BoxLogLevel level, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  const char *s = Box_Print_VA(fmt, ap);
  va_end(ap);

  if (level >= BOXLOGLEVEL_ERROR)
    c->attr.is_sane = 0;

  BoxAST_Log(c->ast, src, level, s);
}

/* Function which does all the steps needed to get from a Box source file
 * to a VM with the corresponding compiled bytecode.
 */
BoxBool
Box_Compile_To_VM_From_File(BoxVMCallNum *main, BoxVM *target_vm,
                            FILE *file, const char *file_name,
                            const char *setup_file_name,
                            BoxPaths *paths, BoxLogger *logger)
{
  BoxAST *ast;
  BoxCmp *compiler = BoxCmp_Create(target_vm);

  ast = Box_Parse_FILE(file, file_name, setup_file_name, paths, logger);
  compiler->ast = ast;
  if (!(BoxAST_Is_Sane(ast) && compiler->compiler->Compile(ast))) {
    BoxCmp_Destroy(compiler);
    return BOXBOOL_FALSE;
  }

  if (main)
    *main = BoxVMCode_Install(& compiler->main_proc);
  BoxCmp_Destroy(compiler);
  return BOXBOOL_TRUE;
}

void BoxCmp_Remove_Any(BoxCmp *c, int num_items_to_remove)
{
  int i;
  for(i = 0; i < num_items_to_remove; i++) {
    StackItem *si = (StackItem *) BoxArr_Last_Item_Ptr(& c->stack);
    if (si->type == STACKITEM_VALUE)
      Value_Destroy((Value *) si->item);
    if (si->destructor)
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

/**
 * @brief Manage the compiler stack in case of errors.
 *
 * Check the last @p items_to_pop stack entries for errors. If all these
 * entries have no errors, then do nothing and return true. If one or more
 * of these entries have errors, then removes all of them from the stack,
 * push the given number of errors and return true.
 */
BoxBool
BoxCmp_Pop_Errors(BoxCmp *c, int items_to_pop, int errors_to_push)
{
  BoxInt n = BoxArr_Num_Items(& c->stack), i;
  int ok = BOXBOOL_TRUE;

  for(i = 0; i < items_to_pop; i++) {
    StackItem *si = (StackItem *) BoxArr_Item_Ptr(& c->stack, n - i);

    if (si->type == STACKITEM_VALUE) {
      Value *v = (Value *) si->item;
      if (Value_Is_Err(v)) {
        ok = BOXBOOL_FALSE;
        break;
      }
    } else if (si->type == STACKITEM_ERROR) {
      ok = BOXBOOL_FALSE;
      break;
    }
  }

  if (ok)
    return BOXBOOL_FALSE;

  BoxCmp_Remove_Any(c, items_to_pop);
  BoxCmp_Push_Error(c, errors_to_push);
  return BOXBOOL_TRUE;
}

/** Pushes the given value 'v' into the compiler stack, stealing a reference
 * to it.
 * REFERENCES: v: -1;
 */
void BoxCmp_Push_Value(BoxCmp *c, Value *v)
{
  if (v) {
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
  StackItem *si = (StackItem *) BoxArr_Last_Item_Ptr(& c->stack);
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
    BoxCmp_Log_Fatal(c, "BoxCmp_Pop_Value: want value, but top of stack "
                     "contains incompatible item.");
    return NULL;
  }
}

Value *BoxCmp_Get_Value(BoxCmp *c, BoxInt pos)
{
  BoxInt n = BoxArr_Num_Items(& c->stack);
  StackItem *si = (StackItem *) BoxArr_Item_Ptr(& c->stack, n - pos);
  switch(si->type) {
  case STACKITEM_ERROR:
    return My_Value_New_Error(c); /* return an error value */

  case STACKITEM_VALUE:
    return (Value *) si->item;

  default:
    BoxCmp_Log_Fatal(c, "BoxCmp_Get_Value: want value, but top of stack "
                     "contains incompatible item.");
    return NULL;
  }
}

namespace Box {

bool
Compiler::Compile(BoxAST *ast)
{
  BoxASTNode *root;

  if (!ast)
    return BOXBOOL_FALSE;

  root = BoxAST_Get_Root(ast);
  if (!root)
    return BOXBOOL_FALSE;

  /*
   * The "sane" attribute is set here to 1 and is reset whenever an error
   * message is reported via BoxCmp_Log() or derived functions/macros.
   * Based on this attribute, we will decide whether it is safe to execute
   * the compiled code.
   */
  c->attr.is_sane = 1;

  Compile_Any(root);
  BoxCmp_Remove_Any(c, 1);
  return (c->attr.is_sane == 1);
}

void
Compiler::Compile_Any(BoxASTNode *node)
{
  BoxASTNodeType node_type = (BoxASTNodeType) BoxASTNode_Get_Type(node);
  BoxASTNode *prev_ast_node = BoxCmp_Set_Cur_Node(c, node);

#define BOXASTNODE_DEF(NODE, Node) \
  case BOXASTNODETYPE_##NODE: Compile_##Node(node); break;

  switch (node_type) {
#include "astnodes.h"
  default:
    BoxCmp_Log_Err(c, "Compilation of node %s (%d) is not implemented "
                   " yet", BoxASTNodeType_To_Str(node_type), (int) node_type);
    break;
  }

#undef BOXASTNODE_DEF

  BoxCmp_Set_Cur_Node(c, prev_ast_node);
}

void
Compiler::Compile_CharImm(BoxASTNode *node)
{
  Value *v = Value_Create(c);
  assert(BoxASTNode_Get_Type(node) == BOXASTNODETYPE_CHAR_IMM);
  Value_Setup_As_Imm_Char(v, ((BoxASTNodeCharImm *) node)->value);
  BoxCmp_Push_Value(c, v);
}

void
Compiler::Compile_IntImm(BoxASTNode *node)
{
  Value *v = Value_Create(c);
  assert(BoxASTNode_Get_Type(node) == BOXASTNODETYPE_INT_IMM);
  Value_Setup_As_Imm_Int(v, ((BoxASTNodeIntImm *) node)->value);
  BoxCmp_Push_Value(c, v);
}

void
Compiler::Compile_RealImm(BoxASTNode *node)
{
  Value *v = Value_Create(c);
  assert(BoxASTNode_Get_Type(node) == BOXASTNODETYPE_REAL_IMM);
  Value_Setup_As_Imm_Real(v, ((BoxASTNodeRealImm *) node)->value);
  BoxCmp_Push_Value(c, v);
}

void
Compiler::Compile_StrImm(BoxASTNode *node)
{
  Value *v = Value_Create(c);
  assert(BoxASTNode_Get_Type(node) == BOXASTNODETYPE_STR_IMM);
  Value_Setup_As_String(v, ((BoxASTNodeStrImm *) node)->str);
  BoxCmp_Push_Value(c, v);
}

void
Compiler::Compile_TypeIdfr(BoxASTNode *node)
{
  Value *v;
  char *type_name;
  NmspFloor f;

  assert(BoxASTNode_Get_Type(node) == BOXASTNODETYPE_TYPE_IDFR);

  type_name = & ((BoxASTNodeTypeIdfr *) node)->name[0];
  f = NMSPFLOOR_DEFAULT;
  v = Namespace_Get_Value(& c->ns, f, type_name);
  if (v) {
    BoxCmp_Push_Value(c, v);
  } else {
    Value v;
    Value_Init(& v, c);
    Value_Setup_As_Type_Name(& v, type_name);
    BoxCmp_Push_Value(c, Namespace_Add_Value(& c->ns, f, type_name, & v));
  }
}

void
Compiler::Compile_TypeTag(BoxASTNode *node)
{
  Value *v;
  BoxTypeId type_id;

  assert(BoxASTNode_Get_Type(node) == BOXASTNODETYPE_TYPE_TAG);

  /* Should we use c->value.create, etc. ? */
  type_id = (BoxTypeId) ((BoxASTNodeTypeTag *) node)->type_id;
  v = Value_Create(c);
  Value_Setup_As_Type(v, Box_Get_Core_Type(type_id));
  BoxCmp_Push_Value(c, v);
}

void
Compiler::Compile_Subtype_Value(BoxASTNodeSubtype *node)
{
  const char *name;
  Value *v_parent = NULL, *v_result = NULL;

  assert(BoxASTNode_Get_Type((BoxASTNode *) node->name)
         == BOXASTNODETYPE_TYPE_IDFR);
  name = node->name->name;

  if (node->parent) {
    Compile_Any(node->parent);
    if (BoxCmp_Pop_Errors(c, /* pop */ 1, /* push err */ 1))
      return;
    v_parent = BoxCmp_Pop_Value(c);
  } else {
    v_parent = Namespace_Get_Value(& c->ns, NMSPFLOOR_DEFAULT, "#");
    if (!v_parent)
      BoxCmp_Log_Err(c, "Cannot get implicit method `%s'. Default parent is "
                     "not defined in current scope.", name);
  }

  if (v_parent) {
    if (Value_Want_Value(v_parent))
      v_result = Value_Subtype_Build(v_parent, name);
    else
      Value_Destroy(v_parent);
  }

  BoxCmp_Push_Value(c, v_result);
}

void
Compiler::Compile_Subtype_Type(BoxASTNodeSubtype *node)
{
  Value *parent_type;
  BoxType *new_subtype = NULL;
  const char *name;

  assert(BoxASTNode_Get_Type((BoxASTNode *) node->name)
         == BOXASTNODETYPE_TYPE_IDFR);
  name = node->name->name;

  Compile_Any(node->parent);
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
      } else
        BoxCmp_Log_Err(c, "Cannot build subtype `%s' of undefined subtype "
                       "`%T'.", name, pt);
    } else {
      new_subtype = BoxType_Find_Subtype(pt, name);
      if (!new_subtype)
        new_subtype = BoxType_Create_Subtype(pt, name, NULL);
    }
  }

  if (new_subtype) {
    Value *v = Value_Recycle(parent_type);
    Value_Setup_As_Type(v, new_subtype);
    (void) BoxType_Unlink(new_subtype);
    BoxCmp_Push_Value(c, v);
  } else {
    Value_Destroy(parent_type);
    BoxCmp_Push_Error(c, 1);
  }
}

void
Compiler::Compile_Subtype(BoxASTNode *node)
{
  BoxASTNodeSubtype *subtype_node;

  assert(BoxASTNode_Get_Type(node) == BOXASTNODETYPE_SUBTYPE);
  subtype_node = (BoxASTNodeSubtype *) node;
  if (subtype_node->parent && BoxASTNode_Is_Type(subtype_node->parent))
    Compile_Subtype_Type(subtype_node);
  else
    Compile_Subtype_Value(subtype_node);
}

void
Compiler::Compile_Keyword(BoxASTNode *node)
{
  assert(BoxASTNode_Get_Type(node) == BOXASTNODETYPE_KEYWORD);

  Compile_Any((BoxASTNode *) ((BoxASTNodeKeyword *) node)->type);
  if (BoxCmp_Pop_Errors(c, /* pop */ 1, /* push */ 1))
    return;
  else {
    Value *v = BoxCmp_Pop_Value(c);
    BoxCmp_Push_Value(c, Value_To_Temp_Or_Target(c, v, v));
  }
}

void
Compiler::Compile_Statement(BoxASTNode *s)
{
  abort();
}

void
Compiler::Compile_Member(BoxASTNode *s)
{
  abort();
}

typedef enum {
  MYBOXSTATE_INITIAL,
  MYBOXSTATE_GOT_IF,
  MYBOXSTATE_GOT_ELSE
} MyBoxState;

void
Compiler::Compile_Box_Generic(BoxASTNode *box_node,
                              BoxType *t_child, BoxType *t_parent)
{
  BoxASTNodeBox *box;
  BoxASTNodeStatement *s;
  Value *parent = NULL, *outer_parent = NULL;
  BoxBool parent_is_err = 0, need_floor_down;
  BoxLIRNodeOp *begin_label, *else_label = NULL, *end_label = NULL;
  MyBoxState state;

  assert(BoxASTNode_Get_Type(box_node) == BOXASTNODETYPE_BOX);
  box = (BoxASTNodeBox *) box_node;

  if (!box->parent) {
    Value *v_void = My_Get_Void_Value(c);
    BoxCmp_Push_Value(c, v_void);

    parent = Namespace_Get_Value(& c->ns, NMSPFLOOR_DEFAULT, "#");
    if (!parent)
      parent = v_void;
    else
      outer_parent = parent;

  } else {
    Value *parent_type;
    Compile_Any(box->parent);
    parent_type = BoxCmp_Pop_Value(c);
    assert(parent_type);

    if (BoxASTNode_Is_Type(box->parent)
        && BoxType_Is_Subtype(parent_type->type)) {
      BoxCmp_Log_Err(c, "Cannot instantiate unbound subtype %T",
                     parent_type->type);
      parent = Value_Finish(parent_type);
      parent_is_err = BOXBOOL_TRUE;
    } else {
      parent = Value_To_Temp_Or_Target(c, parent_type, parent_type);
      parent_is_err = Value_Is_Err(parent);
    }
    BoxCmp_Push_Value(c, parent);
  }

  Namespace_Floor_Up(& c->ns); /* variables defined in this box will be
                                  destroyed when it gets closed! */

  /* Add $ (the child) to namespace */
  if (t_child) {
    Value *v_tmp;
    Value v_child;
    Value_Init(& v_child, c);
    Value_Setup_As_Child(& v_child, t_child);
    v_tmp = Namespace_Add_Value(& c->ns, NMSPFLOOR_DEFAULT, "$", & v_child);
    Value_Destroy(v_tmp);
    Value_Finish(& v_child);
  }

  /* Add $$ (the parent) to namespace */
  if (t_parent) {
    Value v_parent;
    Value_Init(& v_parent, c);
    Value_Setup_As_Parent(& v_parent, t_parent);
    parent = Namespace_Add_Value(& c->ns, NMSPFLOOR_DEFAULT, "$$", & v_parent);
    Value_Finish(& v_parent);
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
    Value *v_parent = Value_Create(c);
    Value_Setup_As_Weak_Copy(v_parent, parent);
    v_parent = Value_Promote_Temp_To_Target(v_parent);
    /* ^^^ Promote # (the Box object) to a target so that it can be
     * changed inside the Box
     */
    Namespace_Add_Value(& c->ns, NMSPFLOOR_DEFAULT, "#", v_parent);
    /* ^^^ adding # to the namespace removes all spurious error messages
     *     for parent == NULL.
     */
    Value_Destroy(v_parent);
  }

  /* Invoke the opening procedure */
  if (box->parent)
    Value_Emit_Call(parent, & c->value.begin, NULL);

  /* Create jump-labels for If and For */
  begin_label = BoxLIR_Get_Last_Op(c->lir);

  /* Save previous source position */
  //BoxSrc *prev_src_of_err = Msg_Set_Src(& box->head.src);

  need_floor_down = BOXBOOL_FALSE;

  /* Loop over all the statements of the box */
  for(s = box->first_stmt, state = MYBOXSTATE_INITIAL; s; s = s->next) {
    Value *stmt_val;
    BoxCmp_Set_Cur_Node(c, (BoxASTNode *) s);

    if (s->sep == BOXASTSEP_PAUSE && !parent_is_err) {
      BoxTask emit_task;
      Value *v_pause = & c->value.pause;
      v_pause = Value_Emit_Call(parent, v_pause, & emit_task);

      if (v_pause) {
        BoxSrc sep_src;
        assert(emit_task == BOXTASK_FAILURE);
        sep_src.begin = s->sep_pos;
        sep_src.end = s->sep_pos + 1;
        BoxCmp_Log(c, & sep_src, BOXLOGLEVEL_WARNING, "Don't know "
                   "how to use `%T' expressions inside a `%T' box.",
                   v_pause->type, parent->type);
      }
    }

    if (s->value) {
      Compile_Any(s->value);
      stmt_val = BoxCmp_Pop_Value(c);
    } else
      stmt_val = My_Get_Void_Value(c);

    if (!(parent_is_err || Value_Is_Ignorable(stmt_val))) {
      if (Value_Want_Has_Type(stmt_val)) {
        BoxTask emit_task;
        stmt_val = Value_Emit_Call(parent, stmt_val, & emit_task);

        if (stmt_val) {
          assert(emit_task == BOXTASK_FAILURE);

          /* Handle the case where stmt_val is an If[] or For[] value */
          if (BoxType_Compare(stmt_val->type,
                              Box_Get_Core_Type(BOXTYPEID_IF))) {
            BoxLIRNodeOpBranch *branch;
            if (!else_label)
              else_label = BoxLIR_Append_Op_Label(c->lir);
            branch = Value_Emit_CJump(stmt_val);
            branch->target = else_label;

            if (state != MYBOXSTATE_GOT_IF) {
              assert(!need_floor_down);
              Namespace_Floor_Up(& c->ns);
              need_floor_down = BOXBOOL_TRUE;
            }
            state = MYBOXSTATE_GOT_IF;

          } else if (BoxType_Compare(stmt_val->type,
                                     Box_Get_Core_Type(BOXTYPEID_ELSE))) {
            if (state == MYBOXSTATE_GOT_IF) {
              if (!end_label)
                end_label = BoxLIR_Append_Op_Label(c->lir);
              BoxLIR_Append_Op_Branch(c->lir, BOXOP_JMP_I, end_label);

              assert(else_label);
              BoxLIR_Move_Label_Back(c->lir, else_label);
              else_label = NULL;

              assert(need_floor_down);
              Namespace_Floor_Down(& c->ns);
              need_floor_down = BOXBOOL_FALSE;

            } else {
              if (state == MYBOXSTATE_GOT_ELSE)
                BoxCmp_Log_Err(c, "Double 'Else'.");
              else
                BoxCmp_Log_Err(c, "'Else' without 'If'.");
            }
            state = MYBOXSTATE_GOT_ELSE;
          } else if (BoxType_Compare(stmt_val->type,
                                     Box_Get_Core_Type(BOXTYPEID_FOR))) {
            BoxLIRNodeOpBranch *branch;
            branch = Value_Emit_CJump(stmt_val);
            branch->target = BoxLIR_Get_Next_Op(c->lir, begin_label);
          } else {
            BoxCmp_Log_Warn(c, "Don't know how to use `%T' expressions inside "
                            "a `%T' box.", stmt_val->type, parent->type);
          }
        }
      }
    }

    Value_Destroy(stmt_val);
  }

  if (need_floor_down)
    Namespace_Floor_Down(& c->ns);

  /* Restore previous source position */
  //(void) Msg_Set_Src(prev_src_of_err);

  if (end_label)
    BoxLIR_Move_Label_Back(c->lir, end_label);

  /* If without else: make If[] jump to the end of the [] block. */
  if (else_label)
    BoxLIR_Move_Label_Back(c->lir, else_label);

  /* Invoke the closing procedure */
  if (box->parent)
    Value_Emit_Call(parent, & c->value.end, NULL);

  Namespace_Floor_Down(& c->ns); /* close the scope unit */

  if (outer_parent)
    Value_Unlink(outer_parent);
}

void
Compiler::Compile_Box(BoxASTNode *node)
{
  Compile_Box_Generic(node, NULL, NULL);
}

void
Compiler::Compile_VarIdfr(BoxASTNode *node)
{
  Value *v;
  char *item_name;

  assert(BoxASTNode_Get_Type(node) == BOXASTNODETYPE_VAR_IDFR);

  item_name = & ((BoxASTNodeVarIdfr *) node)->name[0];
  v = Namespace_Get_Value(& c->ns, NMSPFLOOR_DEFAULT, item_name);
  if (v) {
    BoxCmp_Push_Value(c, v);
  } else {
    Value *new_v = Value_Create(c);
    Value_Setup_As_Var_Name(new_v, item_name);
    BoxCmp_Push_Value(c, new_v);
  }
}

void
Compiler::Compile_Ignore(BoxASTNode *node)
{
  Value *operand;

  assert(BoxASTNode_Get_Type(node) == BOXASTNODETYPE_IGNORE);

  /* Compile operand and get it from the stack */
  Compile_Any(((BoxASTNodeIgnore *) node)->value);
  operand = BoxCmp_Get_Value(c, 0);
  Value_Set_Ignorable(operand, BOXBOOL_TRUE);
}

void Compiler::Compile_UnTypeOp(BoxASTNode *node)
{
  BoxASTNodeUnTypeOp *un_type_op = (BoxASTNodeUnTypeOp *) node;
  Value *v_type, *v_out_type = NULL;

  assert(BoxASTNode_Get_Type(node) == BOXASTNODETYPE_UN_TYPE_OP);

  Compile_Any(un_type_op->value);
  if (BoxCmp_Pop_Errors(c, /* pop */ 1, /* push err */ 1))
    return;

  v_type = BoxCmp_Pop_Value(c);
  if (Value_Want_Has_Type(v_type)) {
    BoxType *out_type = NULL;

    switch (un_type_op->op) {
    case BOXASTUNOP_LINC:
    case BOXASTUNOP_RAISE:
      out_type = BoxType_Create_Raised(BoxType_Link(v_type->type));
      break;
    case BOXASTUNOP_REF:
      out_type = BoxType_Create_Pointer(BoxType_Link(v_type->type));
      break;
    case BOXASTUNOP_DEREF:
      out_type = BoxType_Dereference_Pointer(BoxType_Link(v_type->type));
      break;
    default:
      BoxCmp_Log_Err(c, "Cannot apply unary operator %s to type.",
                     BoxASTUnOp_To_String(un_type_op->op));
      break;
    }

    if (out_type) {
      v_out_type = Value_Create(c);
      Value_Setup_As_Type(v_out_type, out_type);
      (void) BoxType_Unlink(out_type);
    }
  }

  Value_Destroy(v_type);
  BoxCmp_Push_Value(c, v_out_type);
}

void
Compiler::Compile_UnOp(BoxASTNode *node)
{
  BoxASTNodeUnOp *un_op = (BoxASTNodeUnOp *) node;
  Value *operand, *v_result = NULL;

  assert(BoxASTNode_Get_Type(node) == BOXASTNODETYPE_UN_OP);

  /* Compile operand and get it from the stack */
  Compile_Any(un_op->value);
  if (BoxCmp_Pop_Errors(c, /* pop */ 1, /* push err */ 1))
    return;

  operand = BoxCmp_Pop_Value(c);
  if (Value_Want_Value(operand)) {
    switch (un_op->op) {
    case BOXASTUNOP_RAISE:
      v_result = Value_Raise(operand);
      break;
    case BOXASTUNOP_REF:
      v_result = Value_Reference(operand);
      break;
    case BOXASTUNOP_DEREF:
      v_result = Value_Dereference(operand);
      break;
    default:
      v_result = BoxCmp_Opr_Emit_UnOp(c, un_op->op, operand);
      break;
    }
  } else
    Value_Destroy(operand);

  BoxCmp_Push_Value(c, v_result);
}

/** Deal with assignments.
 * REFERENCES: result: new, left: -1, right: -1;
 */
Value *
Compiler::Compile_Value_Assignment(Value *left, Value *right)
{
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
      Value_Assign(c, left, right);
      return Value_Set_Ignorable(left, BOXBOOL_TRUE);

    } else if (Value_Is_Target(left)) {
      Value_Move_Content(left, right);
      return Value_Set_Ignorable(left, BOXBOOL_TRUE);

    } else {
      BoxCmp_Log_Err(c, "Invalid target for assignment (%s).",
                     ValueKind_To_Str(left->kind));
      Value_Destroy(left);
      Value_Destroy(right);
      return NULL;
    }

  } else {
    Value_Unlink(left);
    Value_Unlink(right);
    return NULL;
  }
}

Value *
Compiler::Compile_Type_Assignment(Value *v_name, Value *v_type)
{
  Value *v_named_type = NULL;

  if (Value_Want_Has_Type(v_type)) {
    if (Value_Is_Type_Name(v_name)) {
      Value v;
      /* Create the new identity type. */
      BoxType *ident_type =
        BoxType_Create_Ident(BoxType_Link(v_type->type), v_name->name);

      /* Register the type in the proper namespace. */
      Value_Init(& v, c);
      Value_Setup_As_Type(& v, ident_type);
      v_named_type = Namespace_Add_Value(& c->ns, NMSPFLOOR_DEFAULT,
                                         v_name->name, & v);
      Value_Finish(& v);
      (void) BoxType_Unlink(ident_type);

      /* Return a copy of the created type. */
      v_named_type = Value_Create(c);
      Value_Setup_As_Weak_Copy(v_named_type, & v);

    } else if (Value_Has_Type(v_name)) {
      BoxType *t = v_name->type;
      if (BoxType_Is_Subtype(t)) {
        if (BoxType_Is_Registered_Subtype(t)) {
          BoxType *t_child;
          BoxBool success = BoxType_Get_Subtype_Info(t, NULL, NULL, & t_child);
          assert(success);
          if (BoxType_Compare(t_child, v_type->type) == BOXTYPECMP_DIFFERENT)
            BoxCmp_Log_Err(c, "Inconsistent redefinition of type `%T': was "
                           "`%T' and is now `%T'", v_name->type, t_child,
                           v_type->type);
        } else {
          (void) BoxType_Register_Subtype(t, v_type->type);
          /* ^^^ ignore state of success of operation. */
        }

      } else if (BoxType_Compare(v_name->type, v_type->type)
                 == BOXTYPECMP_DIFFERENT) {
        BoxCmp_Log_Err(c, "Inconsistent redefinition of type `%T'.",
                       v_name->type);
      }

      v_named_type = v_type;
      Value_Link(v_type);
    }
  }

  Value_Unlink(v_type);
  Value_Unlink(v_name);

  return v_named_type;
}

void
Compiler::Compile_BinOp(BoxASTNode *node)
{
  BoxASTNodeBinOp *bin_op_node = (BoxASTNodeBinOp *) node;

  assert(BoxASTNode_Get_Type(node) == BOXASTNODETYPE_BIN_OP);

  Compile_Any(bin_op_node->lhs);
  Compile_Any(bin_op_node->rhs);
  if (BoxCmp_Pop_Errors(c, /* pop */ 2, /* push err */ 1))
    return;

  {
    Value *left, *right, *result = NULL;
    BoxASTBinOp op;

    /* Get values from stack */
    right = BoxCmp_Pop_Value(c);
    left  = BoxCmp_Pop_Value(c);

    op = bin_op_node->op;
    if (op == BOXASTBINOP_ASSIGN) {
      if (BoxASTNode_Is_Type(bin_op_node->lhs)
          && BoxASTNode_Is_Type(bin_op_node->rhs))
        result = Compile_Type_Assignment(left, right);
      else
        result = Compile_Value_Assignment(left, right);
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

void
Compiler::Compile_Get(BoxASTNode *node)
{
  BoxType *t;
  Value *v_struc;
  BoxASTNode *parent;
  const char *member_name;

  assert(BoxASTNode_Get_Type(node) == BOXASTNODETYPE_GET);

  parent = ((BoxASTNodeGet *) node)->parent;
  member_name = ((BoxASTNodeGet *) node)->name->name;
  if (!parent) {
    v_struc = Namespace_Get_Value(& c->ns, NMSPFLOOR_DEFAULT, "#");
    if (!v_struc) {
      BoxCmp_Log_Err(c, "Cannot get implicit member '%s'. Default "
                     "parent is not defined in current scope.", member_name);
      BoxCmp_Push_Value(c, NULL);
      return;
    }
  } else {
    Compile_Any(parent);
    v_struc = BoxCmp_Pop_Value(c);
  }

  if (!Value_Want_Value(v_struc)) {
    Value_Destroy(v_struc);
    BoxCmp_Push_Error(c, 1);
  }

  t = BoxType_Link(v_struc->type);
  v_struc = Value_Struc_Get_Member(c, v_struc, member_name);
  if (!v_struc)
    BoxCmp_Log_Err(c, "Cannot find the member `%s' of an object "
                   "with type `%T'.", member_name, t);
  (void) BoxType_Unlink(t);
  BoxCmp_Push_Value(c, v_struc);
}

void
Compiler::Compile_ArgGet(BoxASTNode *node)
{
  Value *v_self = NULL;
  const char *n_self = NULL;
  ASTSelfLevel self_level = ((BoxASTNodeArgGet *) node)->depth;
  int i, promote_to_target = 0, return_weak_copy = 0;

  assert(BoxASTNode_Get_Type(node) == BOXASTNODETYPE_ARG_GET);

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
    for (i = 2; i < self_level && v_self; i++)
      v_self = Value_Subtype_Get_Parent(v_self);
      /* FIXME: see Value_Init */
    promote_to_target = 1;
    return_weak_copy = 0;
  }

  if (!v_self) {
    BoxCmp_Log_Err(c, "%s not defined in the current scope.", n_self);
  } else {
    /* Return only a weak copy? */
    if (return_weak_copy) {
      Value *v_copy = Value_Create(c);
      Value_Setup_As_Weak_Copy(v_copy, v_self);
      Value_Unlink(v_self);
      v_self = v_copy;
    }

    if (promote_to_target)
      v_self = Value_Promote_Temp_To_Target(v_self);
  }

  BoxCmp_Push_Value(c, v_self);
}

void
Compiler::Compile_CombDef(BoxASTNode *node)
{
  BoxLIRNodeProc *proc;
  BoxASTNodeCombDef *comb_def_node;
  BoxASTNode *n_implem;
  Value *v_child, *v_parent, *v_ret = NULL;
  BoxCombType comb_type;
  char *c_name = NULL;
  BoxType *t_child, *t_parent, *comb;
  BoxCallable *comb_callable;
  int no_err;

  assert(BoxASTNode_Get_Type(node) == BOXASTNODETYPE_COMB_DEF);
  comb_def_node = (BoxASTNodeCombDef *) node;
  n_implem = comb_def_node->implem;

  /* first, get the type of child and parent */
  Compile_Any(comb_def_node->child);
  v_child = BoxCmp_Pop_Value(c);
  Compile_Any(comb_def_node->parent);
  v_parent = BoxCmp_Pop_Value(c);
  comb_type = (BoxCombType) comb_def_node->comb_type;

  no_err = Value_Want_Has_Type(v_child) & Value_Want_Has_Type(v_parent);

  /* Get the types from the values, and destroy the latters. */
  t_child = BoxType_Link(v_child->type);
  t_parent = BoxType_Link(v_parent->type);
  Value_Destroy(v_child);
  Value_Destroy(v_parent);

  /* now get the C-name, if present */
  if (comb_def_node->c_name) {
    assert(BoxASTNode_Get_Type(comb_def_node->c_name)
           == BOXASTNODETYPE_STR_IMM);
    c_name = ((BoxASTNodeStrImm *) comb_def_node->c_name)->str;
    if (strlen(c_name) < 1) {
      /* NOTE: Should we test any other kind of "badness"? */
      BoxCmp_Log(c, & comb_def_node->c_name->head.src, BOXLOGLEVEL_ERROR,
                 "Empty string in C-name for procedure declaration.");
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
    BoxLIR *save_cur_lir = c->lir;
    Value *v_implem;
    BoxVMCode proc_implem;
    BoxVMCallNum cn;

    /* We change target of the compilation to the new procedure */
    BoxVMCode_Init(& proc_implem, c, BOXVMCODESTYLE_SUB);
    c->cur_proc = & proc_implem;
    c->lir = BoxLIR_Create();
    assert(c->lir);

    /* A BoxVMCode object is used to get the procedure symbol and to register
     * and assemble it.
     */
    proc = BoxLIR_Append_Proc(c->lir);
    assert(proc);
    BoxLIR_Set_Target_Proc(c->lir, proc);

    /* Set the call number. */
    if (!BoxType_Generate_Combination_Call_Num(comb, c->vm, & cn))
      BoxCmp_Log_Fatal(c, "Cannot generate call number for combination.");
    proc_implem.have.call_num = 1;
    proc_implem.call_num = cn;

    /* Set the alternative name to make the bytecode more readable */
    {
      char *alter_name = BoxType_Get_Repr(comb);
      assert(alter_name);

      BoxVMCode_Set_Alter_Name(& proc_implem, alter_name);
      Box_Mem_Free(alter_name);
    }

    /* Specify the prototype for the procedure */
    BoxVMCode_Set_Prototype(& proc_implem,
                            !BoxType_Is_Empty(t_child),
                            !BoxType_Is_Empty(t_parent));

    Compile_Box_Generic(n_implem, t_child, t_parent);
    v_implem = BoxCmp_Pop_Value(c);
    /* NOTE: we should double check that this is void! */
    Value_Unlink(v_implem);

    c->cur_proc = save_cur_proc;

    (void) BoxVMCode_Install(& proc_implem);

    BoxVMCode_Finish(& proc_implem);
    BoxLIR_Destroy(c->lir);
    c->lir = save_cur_lir;
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

void
Compiler::Compile_Struct_Value(BoxASTNodeCompound *compound)
{
  int i, num_members;
  BoxASTNodeMember *memb;
  BoxType *t_struc;
  Value *v_struc;
  ValueStrucIter vsi;
  int no_err;

  /* Compile the members, check their types and leave them on the stack */
  num_members = 0;
  no_err = 1;
  for(memb = compound->memb; memb; memb = memb->next, num_members++) {
    Value *v_member;
    BoxCmp_Set_Cur_Node(c, (BoxASTNode *) memb);

    assert(BoxASTNode_Get_Type((BoxASTNode *) memb) == BOXASTNODETYPE_MEMBER);
    assert(memb->expr);

    Compile_Any(memb->expr);
    v_member = BoxCmp_Get_Value(c, 0);
    no_err &= Value_Want_Value(v_member);
    if (no_err && BoxType_Is_Empty(v_member->type)) {
      BoxCmp_Log_Err(c, "Invalid structure member of type `%T'",
                     v_member->type);
      no_err = 0;
    }
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
  for(memb = compound->memb; memb; memb = memb->next) {
    Value *v_member = BoxCmp_Get_Value(c, --i);
    BoxType_Add_Member_To_Structure(t_struc, v_member->type, /*name*/ NULL);
  }

  /* create and populate the structure */
  v_struc = Value_Create(c);
  Value_Setup_As_Temp(v_struc, t_struc);

  for(ValueStrucIter_Init(& vsi, v_struc, c);
      vsi.has_next; ValueStrucIter_Do_Next(& vsi)) {
    Value *v_member = BoxCmp_Get_Value(c, num_members - vsi.index - 1);
    Value_Link(v_member);
    Value_Move_Content(& vsi.v_member, v_member);
  }

  ValueStrucIter_Finish(& vsi);

  BoxCmp_Remove_Any(c, num_members);
  BoxCmp_Push_Value(c, v_struc);
}

void
Compiler::Compile_Struct_Type(BoxASTNodeCompound *compound)
{
  int err;
  BoxASTNodeMember *memb;
  BoxType *previous_type = NULL, *struc_type;
  Value *v_struc_type;

  /* Create a new structure type */
  struc_type = BoxType_Create_Structure();

  /* Compile the members, check their types and leave them on the stack */
  err = 0;
  for (memb = compound->memb; memb; memb = memb->next) {
    char *memb_name = NULL;
    BoxCmp_Set_Cur_Node(c, (BoxASTNode *) memb);

    assert(BoxASTNode_Get_Type((BoxASTNode *) memb) == BOXASTNODETYPE_MEMBER);

    if (memb->name) {
      assert(BoxASTNode_Get_Type(memb->name) == BOXASTNODETYPE_VAR_IDFR);
      memb_name = ((BoxASTNodeVarIdfr *) memb->name)->name;
    }

    if (memb->expr) {
      Value *v_type;
      Compile_Any(memb->expr);
      v_type = BoxCmp_Pop_Value(c);

      err = !Value_Want_Has_Type(v_type);
      if (!err) {
        previous_type = v_type->type;
        if (BoxType_Is_Empty(previous_type)) {
          BoxCmp_Log_Err(c, "Zero-sized type `%T' not allowed as the member "
                         "of a structure", previous_type);
          err = 1;
        }
      }

      Value_Unlink(v_type);
    }

    if (previous_type && !err) {
      /* Check for duplicate structure members */
      if (memb_name) {
        if (BoxType_Find_Structure_Member(struc_type, memb_name))
          BoxCmp_Log_Err(c, "Duplicate member `%s' in structure type "
                         "definition.", memb_name);
      }

      BoxType_Add_Member_To_Structure(struc_type, previous_type, memb_name);
    }
  }

  /* Check for errors */
  if (err) {
    BoxCmp_Push_Error(c, 1);
    return;
  }

  v_struc_type = Value_Create(c);
  Value_Setup_As_Type(v_struc_type, struc_type);
  (void) BoxType_Unlink(struc_type);
  BoxCmp_Push_Value(c, v_struc_type);
}

void
Compiler::Compile_Species_Type(BoxASTNodeCompound *compound)
{
  BoxType *spec_type;
  Value *v_spec_type;
  BoxASTNodeMember *memb;

  /* Create a new species type */
  spec_type = BoxType_Create_Species();

  for(memb = compound->memb; memb; memb = memb->next) {
    Value *v_type;

    assert(BoxASTNode_Get_Type((BoxASTNode *) memb) == BOXASTNODETYPE_MEMBER
           && !memb->name);

    Compile_Any(memb->expr);
    v_type = BoxCmp_Pop_Value(c);

    if (Value_Want_Has_Type(v_type)) {
      BoxType *memb_type = v_type->type;
      /* NOTE: should check for duplicate types in species */
      BoxType_Add_Member_To_Species(spec_type, memb_type);
    }

    Value_Unlink(v_type);
  }

  v_spec_type = Value_Create(c);
  Value_Setup_As_Type(v_spec_type, spec_type);
  (void) BoxType_Unlink(spec_type);

  BoxCmp_Push_Value(c, v_spec_type);
}

void
Compiler::Compile_Paren(BoxASTNode *expr)
{
  Value *v;
  Compile_Any(expr);
  v = BoxCmp_Get_Value(c, 0);
  Value_Set_Ignorable(v, BOXBOOL_FALSE);
}

void
Compiler::Compile_Compound(BoxASTNode *compound_node)
{
  BoxASTNodeCompound *compound;

  assert(BoxASTNode_Get_Type(compound_node) == BOXASTNODETYPE_COMPOUND);
  compound = (BoxASTNodeCompound *) compound_node;

  switch (compound->kind) {
  case BOXASTCOMPOUNDKIND_IDENTITY:
    Compile_Paren(compound->memb->expr);
    return;
  case BOXASTCOMPOUNDKIND_SPECIES:
    Compile_Species_Type(compound);
    return;
  case BOXASTCOMPOUNDKIND_STRUCT:
    if (BoxASTNode_Is_Type(compound_node))
      Compile_Struct_Type(compound);
    else
      Compile_Struct_Value(compound);
    return;
  default:
    printf("Unexpected compound kind %d", (int) compound->kind);
    abort();
  }
}

}
