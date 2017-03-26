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


/* Function which does all the steps needed to get from a Box source file
 * to a VM with the corresponding compiled bytecode.
 */
BoxBool
Box_Compile_To_VM_From_File(BoxVMCallNum *main, BoxVM *target_vm,
                            FILE *file, const char *file_name,
                            const char *setup_file_name,
                            BoxPaths *paths, BoxLogger *logger)
{
  Box::Compiler compiler(target_vm);

  compiler.ast_ = Box_Parse_FILE(file, file_name, setup_file_name,
                                 paths, logger);
  if (!(BoxAST_Is_Sane(compiler.ast_) && compiler.Compile(compiler.ast_)))
    return BOXBOOL_FALSE;

  if (main)
    *main = compiler.Install();
  return BOXBOOL_TRUE;
}


namespace Box {

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

/** Return a new error value.
 */
Value *My_Value_New_Error(Box::Compiler *c)
{
  return c->Create_Value();
}

/* We may optimize this later, by just passing a reference to a Value
 * object which is created once for all at the beginning!
 */
static Value *My_Get_Void_Value(Box::Compiler *c)
{
  Value *v = c->Create_Value();
  c->Setup_Value_As_Void(v);
  return v;
}

BoxASTNode *
Compiler::Set_Cur_Node(BoxASTNode *cur_ast_node)
{
  BoxASTNode *prev_ast_node = ast_node_;
  ast_node_ = cur_ast_node;
  return prev_ast_node;
}

Compiler::Compiler(BoxVM *target_vm)
{
  BoxArr_Init(& active_values_, sizeof(Value *), 32);
  BoxArr_Init(& value_pos_, sizeof(int), 32);
  BoxAllocPool_Init(& value_pool_, sizeof(Value)*32);
  free_value_chain_ = NULL;
  Namespace_Init();

  BoxLIRNodeProc *proc;
  ast_ = NULL;
  ast_node_ = NULL;
  lir_ = BoxLIR_Create();
  assert(lir_);
  attr_.is_sane = 0;
  attr_.own_vm = (target_vm == NULL);
  vm_ = (target_vm) ? target_vm : BoxVM_Create();

  BoxArr_Init(& stack_, sizeof(StackItem), 32);

  BoxBool success = Box_Initialize_Type_System();
  assert(success);

  Init_Operators();
  proc = BoxLIR_Append_Proc(lir_);
  (void) BoxLIR_Set_Target_Proc(lir_, proc);

  BoxVMCode_Init(& main_proc_, BOXVMCODESTYLE_MAIN);
  BoxVMCode_Set_Alter_Name(& main_proc_, "main");
  cur_proc_ = & main_proc_;

  BoxCont_Set(& pass_child_, "go", 2);
  BoxCont_Set(& pass_parent_, "go", 1);
  Init_Builtins();
}

Compiler::~Compiler() {
  if (BoxArr_Num_Items(& stack_) != 0)
    LOG_WARN("~Compiler: stack is not empty at destruction");

  BoxLIR_Destroy(lir_);
  BoxAST_Destroy(ast_);
  Finish_Builtins();
  BoxVMCode_Finish(& main_proc_);
  BoxArr_Finish(& stack_);
  Finish_Operators();

  if (attr_.own_vm)
    BoxVM_Destroy(vm_);

  Namespace_Finish();
  BoxAllocPool_Finish(& value_pool_);
  BoxArr_Finish(& active_values_);
  BoxArr_Finish(& value_pos_);
}

void
Compiler::Log(BoxSrc *src, BoxLogLevel level, const char *fmt, ...)
{
  if (!src)
    src = & ast_node_->head.src;

  va_list ap;
  va_start(ap, fmt);
  const char *s = Box_Print_VA(fmt, ap);
  va_end(ap);

  if (level >= BOXLOGLEVEL_ERROR)
    attr_.is_sane = 0;

  BoxAST_Log(ast_, src, level, s);
}

Value *
Compiler::Track_Value(Value *v)
{
  if (BoxArr_Get_Num_Items(& value_pos_) == 0)
    return v;

  assert(v->attr.new_or_init);

  BoxArr_Push(& active_values_, & v);
  return v;
}

Value *
Compiler::Untrack_Value(Value *v)
{
  if (BoxArr_Get_Num_Items(& value_pos_) == 0)
    return v;

  assert(v->attr.new_or_init);

  int pos = *((int *) BoxArr_Get_Last_Item_Ptr(& value_pos_));
  Value **vs = (Value **) BoxArr_First_Item_Ptr(& active_values_);
  for (int i = pos; i < BoxArr_Get_Num_Items(& active_values_); i++) {
    if (v == vs[i]) {
      vs[i] = NULL;
      return v;
    }
  }

  abort();
  return v;
}

void
Compiler::Begin_Leak_Check()
{
  int pos = BoxArr_Get_Num_Items(& active_values_);
  BoxArr_Push(& value_pos_, & pos);
}

int
Compiler::End_Leak_Check()
{
  int pos;
  assert(BoxArr_Get_Num_Items(& value_pos_) > 0);
  BoxArr_Pop(& value_pos_, & pos);

  int num_leaked_values = 0;
  int num_items = BoxArr_Get_Num_Items(& active_values_);
  Value **vs = (Value **) BoxArr_First_Item_Ptr(& active_values_);
  for (int i = pos; i < num_items; i++) {
    if (vs[i])
      num_leaked_values++;
  }
  BoxArr_MPop(& active_values_, NULL, num_items - pos);
  assert(BoxArr_Get_Num_Items(& active_values_) == pos);
  return num_leaked_values;
}

void
Compiler::Remove_Any(int num_items_to_remove)
{
  int i;
  for(i = 0; i < num_items_to_remove; i++) {
    StackItem *si = (StackItem *) BoxArr_Last_Item_Ptr(& stack_);
    if (si->type == STACKITEM_VALUE)
      Destroy_Value(Track_Value((Value *) si->item));
    if (si->destructor)
      si->destructor(si->item);
    (void) BoxArr_Pop(& stack_, NULL);
  }
}

void
Compiler::Push_Error(int num_errors)
{
  int i;
  for(i = 0; i < num_errors; i++) {
    StackItem *si = (StackItem *) BoxArr_Push(& stack_, NULL);
    si->type = STACKITEM_ERROR;
    si->item = NULL;
    si->destructor = NULL;
  }
}

bool
Compiler::Pop_Errors(int items_to_pop, int errors_to_push)
{
  BoxInt n = BoxArr_Num_Items(& stack_), i;
  int ok = BOXBOOL_TRUE;

  for(i = 0; i < items_to_pop; i++) {
    StackItem *si = (StackItem *) BoxArr_Item_Ptr(& stack_, n - i);

    if (si->type == STACKITEM_VALUE) {
      Value *v = (Value *) si->item;
      if (Is_Err(v)) {
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

  Remove_Any(items_to_pop);
  Push_Error(errors_to_push);
  return BOXBOOL_TRUE;
}

void
Compiler::Push_Value(Value *v)
{
  if (v) {
    StackItem *si = (StackItem *) BoxArr_Push(& stack_, NULL);
    si->type = STACKITEM_VALUE;
    si->item = v;
    si->destructor = NULL;
    Untrack_Value(v);
  } else
    Push_Error(1);
}

Value *
Compiler::Pop_Value()
{
  StackItem *si = (StackItem *) BoxArr_Last_Item_Ptr(& stack_);
  Value *v;

  switch(si->type) {
  case STACKITEM_ERROR:
    (void) BoxArr_Pop(& stack_, NULL);
    return My_Value_New_Error(this);
  case STACKITEM_VALUE:
    v = (Value *) si->item; /* return the value and its reference */
    (void) BoxArr_Pop(& stack_, NULL);
    return Track_Value(v);
  default:
    LOG_FATAL("Pop_Value: want value, but top of stack "
              "contains incompatible item.");
    return NULL;
  }
}

Value *
Compiler::Get_Value(int pos)
{
  BoxInt n = BoxArr_Num_Items(& stack_);
  StackItem *si = (StackItem *) BoxArr_Item_Ptr(& stack_, n - pos);
  switch(si->type) {
  case STACKITEM_ERROR:
    return My_Value_New_Error(this);

  case STACKITEM_VALUE:
    return (Value *) si->item;

  default:
    LOG_FATAL("Get_Value: want value, but top of stack  contains "
              "incompatible item.");
    return NULL;
  }
}

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
   * message is reported via Log() or derived functions/macros.
   * Based on this attribute, we will decide whether it is safe to execute
   * the compiled code.
   */
  attr_.is_sane = 1;

  Compile_Any(root);
  Remove_Any(1);
  return (attr_.is_sane == 1);
}

void
Compiler::Compile_Any(BoxASTNode *node)
{
  BoxASTNodeType node_type = (BoxASTNodeType) BoxASTNode_Get_Type(node);
  BoxASTNode *prev_ast_node = Set_Cur_Node(node);

#if BOX_CHECK_VALUE_LEAKS
  Begin_Leak_Check();
#endif

#define BOXASTNODE_DEF(NODE, Node) \
  case BOXASTNODETYPE_##NODE: Compile_##Node(node); break;

  switch (node_type) {
#include "astnodes.h"
  default:
    LOG_ERR("Compilation of node %s (%d) is not implemented yet",
            BoxASTNodeType_To_Str(node_type), (int) node_type);
    break;
  }

#undef BOXASTNODE_DEF

#if BOX_CHECK_VALUE_LEAKS
  int num_leaks = End_Leak_Check();
  if (true && num_leaks)
    LOG_ERR("%d Value objects leaked in node %s",
            num_leaks, BoxASTNodeType_To_Str(node_type));
#endif

  Set_Cur_Node(prev_ast_node);
}

void
Compiler::Compile_CharImm(BoxASTNode *node)
{
  Value *v = Create_Value();
  assert(BoxASTNode_Get_Type(node) == BOXASTNODETYPE_CHAR_IMM);
  Setup_Value_As_Imm_Char(v, ((BoxASTNodeCharImm *) node)->value);
  Push_Value(v);
}

void
Compiler::Compile_IntImm(BoxASTNode *node)
{
  Value *v = Create_Value();
  assert(BoxASTNode_Get_Type(node) == BOXASTNODETYPE_INT_IMM);
  Setup_Value_As_Imm_Int(v, ((BoxASTNodeIntImm *) node)->value);
  Push_Value(v);
}

void
Compiler::Compile_RealImm(BoxASTNode *node)
{
  Value *v = Create_Value();
  assert(BoxASTNode_Get_Type(node) == BOXASTNODETYPE_REAL_IMM);
  Setup_Value_As_Imm_Real(v, ((BoxASTNodeRealImm *) node)->value);
  Push_Value(v);
}

void
Compiler::Compile_StrImm(BoxASTNode *node)
{
  Value *v = Create_Value();
  assert(BoxASTNode_Get_Type(node) == BOXASTNODETYPE_STR_IMM);
  Setup_Value_As_String(v, ((BoxASTNodeStrImm *) node)->str);
  Push_Value(v);
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
  v = Namespace_Get_Value(f, type_name);
  if (v) {
    Push_Value(v);
  } else {
    Value v;
    Init_Value(& v);
    Setup_Value_As_Type_Name(& v, type_name);
    Push_Value(Namespace_Add_Value(f, type_name, & v));
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
  v = Create_Value();
  Setup_Value_As_Type(v, Box_Get_Core_Type(type_id));
  Push_Value(v);
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
    if (Pop_Errors(/* pop */ 1, /* push err */ 1))
      return;
    v_parent = Pop_Value();
  } else {
    v_parent = Namespace_Get_Value(NMSPFLOOR_DEFAULT, "#");
    if (!v_parent)
      LOG_ERR("Cannot get implicit method `%s'. Default parent is "
              "not defined in current scope.", name);
  }

  if (v_parent) {
    if (Want_Instance(v_parent))
      v_result = Emit_Subtype_Build(v_parent, name);
    else
      Destroy_Value(v_parent);
  }

  Push_Value(v_result);
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
  if (Pop_Errors(/* pop */ 1, /* push err */ 1))
    return;

  parent_type = Pop_Value();
  if (Want_Type(parent_type)) {
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
        LOG_ERR("Cannot build subtype `%s' of undefined subtype `%T'.",
                name, pt);
    } else {
      new_subtype = BoxType_Find_Subtype(pt, name);
      if (!new_subtype)
        new_subtype = BoxType_Create_Subtype(pt, name, NULL);
    }
  }

  if (new_subtype) {
    Destroy_Value(parent_type);
    Value *v = Create_Value();
    Setup_Value_As_Type(v, new_subtype);
    (void) BoxType_Unlink(new_subtype);
    Push_Value(v);
  } else {
    Destroy_Value(parent_type);
    Push_Error(1);
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
  if (Pop_Errors(/* pop */ 1, /* push */ 1))
    return;
  else {
    Value *v = Pop_Value();
    Push_Value(Emit_Load_Into_Reg(v, v));
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
  Value *parent = NULL, *destroy_parent = NULL;
  BoxBool parent_is_err = 0, need_floor_down;
  BoxLIRNodeOp *begin_label, *else_label = NULL, *end_label = NULL;
  MyBoxState state;

  assert(BoxASTNode_Get_Type(box_node) == BOXASTNODETYPE_BOX);
  box = (BoxASTNodeBox *) box_node;

  if (!box->parent) {
    Value *v_void = My_Get_Void_Value(this);
    Push_Value(v_void);

    parent = Namespace_Get_Value(NMSPFLOOR_DEFAULT, "#");
    if (!parent)
      parent = v_void;
    else
      destroy_parent = parent;
  } else {
    Value *parent_type;
    Compile_Any(box->parent);
    parent_type = Pop_Value();
    assert(parent_type);

    if (BoxASTNode_Is_Type(box->parent)
        && BoxType_Is_Subtype(parent_type->type)) {
      LOG_ERR("Cannot instantiate unbound subtype %T", parent_type->type);
      parent = Finish_Value(parent_type);
      parent_is_err = BOXBOOL_TRUE;
    } else {
      parent = Emit_Load_Into_Reg(parent_type, parent_type);
      parent_is_err = Is_Err(parent);
    }
    Push_Value(parent);
  }

  Namespace_Floor_Up(); // Variables defined in this box will be
                        // destroyed when it gets closed!

  /* Add $ (the child) to namespace */
  if (t_child) {
    Value *v_tmp;
    Value v_child;
    Init_Value(& v_child);
    Setup_Value_As_Child(& v_child, t_child);
    v_tmp = Namespace_Add_Value(NMSPFLOOR_DEFAULT, "$", & v_child);
    Destroy_Value(v_tmp);
    Finish_Value(& v_child);
  }

  /* Add $$ (the parent) to namespace */
  if (t_parent) {
    Value v_parent;
    Init_Value(& v_parent);
    Setup_Value_As_Parent(& v_parent, t_parent);
    parent = Namespace_Add_Value(NMSPFLOOR_DEFAULT, "$$", & v_parent);
    Finish_Value(& v_parent);
    Destroy_Value(destroy_parent);
    destroy_parent = parent;
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
    Value *v_parent = Weak_Copy_Value(parent);
    v_parent = Temp_As_Target(v_parent);
    /* ^^^ Promote # (the Box object) to a target so that it can be
     * changed inside the Box
     */
    Destroy_Value(Namespace_Add_Value(NMSPFLOOR_DEFAULT, "#", v_parent));
    /* ^^^ adding # to the namespace removes all spurious error messages
     *     for parent == NULL.
     */
    Destroy_Value(v_parent);
  }

  /* Invoke the opening procedure */
  if (box->parent)
    Emit_Call(parent, BOXTYPEID_BEGIN);

  /* Create jump-labels for If and For */
  begin_label = BoxLIR_Get_Last_Op(lir_);

  /* Save previous source position */
  //BoxSrc *prev_src_of_err = Msg_Set_Src(& box->head.src);

  need_floor_down = BOXBOOL_FALSE;

  /* Loop over all the statements of the box */
  for(s = box->first_stmt, state = MYBOXSTATE_INITIAL; s; s = s->next) {
    Value *stmt_val;
    Set_Cur_Node((BoxASTNode *) s);

    if (s->sep == BOXASTSEP_PAUSE && !parent_is_err) {
      BoxTask emit_task;
      emit_task = Emit_Call(parent, BOXTYPEID_PAUSE);

      if (emit_task == BOXTASK_FAILURE) {
        BoxType *t_pause = Box_Get_Core_Type(BOXTYPEID_PAUSE);
        BoxSrc sep_src;
        sep_src.begin = s->sep_pos;
        sep_src.end = s->sep_pos + 1;
        Log(& sep_src, BOXLOGLEVEL_WARNING, "Don't know "
            "how to use `%T' expressions inside a `%T' box.",
            t_pause, parent->type);
      }
    }

    if (s->value) {
      Compile_Any(s->value);
      stmt_val = Pop_Value();
    } else
      stmt_val = My_Get_Void_Value(this);

    if (parent_is_err || Is_Ignorable(stmt_val)) {
      Destroy_Value(stmt_val);
    } else {
      if (!Want_Type(stmt_val)) {
        Destroy_Value(stmt_val);
      } else {
        BoxTask emit_task = Emit_Call(parent, stmt_val);
        if (emit_task == BOXTASK_FAILURE) {
          /* Handle the case where stmt_val is an If[] or For[] value */
          if (BoxType_Compare(stmt_val->type,
                              Box_Get_Core_Type(BOXTYPEID_IF))) {
            BoxLIRNodeOpBranch *branch;
            if (!else_label)
              else_label = BoxLIR_Append_Op_Label(lir_);
            branch = Emit_Conditional_Jump(stmt_val);
            branch->target = else_label;

            if (state != MYBOXSTATE_GOT_IF) {
              assert(!need_floor_down);
              Namespace_Floor_Up();
              need_floor_down = BOXBOOL_TRUE;
            }
            state = MYBOXSTATE_GOT_IF;

          } else if (BoxType_Compare(stmt_val->type,
                                     Box_Get_Core_Type(BOXTYPEID_ELSE))) {
            if (state == MYBOXSTATE_GOT_IF) {
              if (!end_label)
                end_label = BoxLIR_Append_Op_Label(lir_);
              BoxLIR_Append_Op_Branch(lir_, BOXOP_JMP_I, end_label);

              assert(else_label);
              BoxLIR_Move_Label_Back(lir_, else_label);
              else_label = NULL;

              assert(need_floor_down);
              Namespace_Floor_Down();
              need_floor_down = BOXBOOL_FALSE;

            } else {
              if (state == MYBOXSTATE_GOT_ELSE)
                LOG_ERR("Double 'Else'.");
              else
                LOG_ERR("'Else' without 'If'.");
            }
            state = MYBOXSTATE_GOT_ELSE;
          } else if (BoxType_Compare(stmt_val->type,
                                     Box_Get_Core_Type(BOXTYPEID_FOR))) {
            BoxLIRNodeOpBranch *branch;
            branch = Emit_Conditional_Jump(stmt_val);
            branch->target = BoxLIR_Get_Next_Op(lir_, begin_label);
          } else {
            LOG_WARN("Don't know how to use `%T' expressions inside "
                     "a `%T' box.", stmt_val->type, parent->type);
          }

          Destroy_Value(stmt_val);
        }
      }
    }
  }

  if (need_floor_down)
    Namespace_Floor_Down();

  /* Restore previous source position */
  //(void) Msg_Set_Src(prev_src_of_err);

  if (end_label)
    BoxLIR_Move_Label_Back(lir_, end_label);

  /* If without else: make If[] jump to the end of the [] block. */
  if (else_label)
    BoxLIR_Move_Label_Back(lir_, else_label);

  /* Invoke the closing procedure */
  if (box->parent)
    Emit_Call(parent, BOXTYPEID_END);

  Namespace_Floor_Down(); /* close the scope unit */

  if (destroy_parent)
    Destroy_Value(destroy_parent);
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
  v = Namespace_Get_Value(NMSPFLOOR_DEFAULT, item_name);
  if (v) {
    Push_Value(v);
  } else {
    Value *new_v = Create_Value();
    Setup_Value_As_Var_Name(new_v, item_name);
    Push_Value(new_v);
  }
}

void
Compiler::Compile_Ignore(BoxASTNode *node)
{
  assert(BoxASTNode_Get_Type(node) == BOXASTNODETYPE_IGNORE);

  /* Compile operand and get it from the stack */
  Compile_Any(((BoxASTNodeIgnore *) node)->value);

  if (Pop_Errors(/* pop */ 1, /* push err */ 1))
    return;

  Set_Ignorable(Get_Value(0));
}

void
Compiler::Compile_UnTypeOp(BoxASTNode *node)
{
  BoxASTNodeUnTypeOp *un_type_op = (BoxASTNodeUnTypeOp *) node;
  Value *v_type, *v_out_type = NULL;

  assert(BoxASTNode_Get_Type(node) == BOXASTNODETYPE_UN_TYPE_OP);

  Compile_Any(un_type_op->value);
  if (Pop_Errors(/* pop */ 1, /* push err */ 1))
    return;

  v_type = Pop_Value();
  if (Want_Type(v_type)) {
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
      LOG_ERR("Cannot apply unary operator %s to type.",
              BoxASTUnOp_To_String(un_type_op->op));
      break;
    }

    if (out_type) {
      v_out_type = Create_Value();
      Setup_Value_As_Type(v_out_type, out_type);
      (void) BoxType_Unlink(out_type);
    }
  }

  Destroy_Value(v_type);
  Push_Value(v_out_type);
}

void
Compiler::Compile_UnOp(BoxASTNode *node)
{
  BoxASTNodeUnOp *un_op = (BoxASTNodeUnOp *) node;
  Value *operand, *v_result = NULL;

  assert(BoxASTNode_Get_Type(node) == BOXASTNODETYPE_UN_OP);

  /* Compile operand and get it from the stack */
  Compile_Any(un_op->value);
  if (Pop_Errors(/* pop */ 1, /* push err */ 1))
    return;

  operand = Pop_Value();
  if (Want_Instance(operand)) {
    switch (un_op->op) {
    case BOXASTUNOP_RAISE:
      v_result = Emit_Raise_Instance(operand);
      break;
    case BOXASTUNOP_REF:
      v_result = Emit_Reference_Instance(operand);
      break;
    case BOXASTUNOP_DEREF:
      v_result = Emit_Dereference_Instance(operand);
      break;
    default:
      v_result = Emit_UnOp(un_op->op, operand);
      break;
    }
  } else
    Destroy_Value(operand);

  Push_Value(v_result);
}

/**
 * @brief Assignments between instances.
 * @note Destroy @p left and @p right. Return new Value.
 */
Value *
Compiler::Compile_Value_Assignment(Value *left, Value *right)
{
  if (Want_Instance(right)) {
    /* Subtypes are always expanded in assignments */
    left = Emit_Subtype_Expansion(left);
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
    right = Emit_Subtype_Expansion(right);
    /* NOTE: The line above will never allow one to have a variable with
     * type X.Y
     */

    /* If the value is an identifier (thing without type, nor value),
     * then we transform it to a proper target.
     */
    if (Is_Var_Name(left)) {
      return Set_Ignorable(Emit_Value_Assignment(left, right));
    } else if (Is_Target(left)) {
      return Set_Ignorable(Emit_Value_Move(left, right));
    } else {
      LOG_ERR("Invalid target for assignment (%s).",
              ValueKind_To_Str(left->kind));
    }
  }

  Destroy_Value(left);
  Destroy_Value(right);
  return NULL;
}

/**
 * @brief Assignments between types.
 * @note Destroy @p left and @p right. Return new Value.
 */
Value *
Compiler::Compile_Type_Assignment(Value *v_name, Value *v_type)
{
  Value *v_named_type = NULL;

  if (Want_Type(v_type)) {
    BoxType *t_type = BoxType_Link(v_type->type);

    if (Is_Type_Name(v_name)) {
      /* Create the new identity type. */
      Value v;
      BoxType *ident_type = BoxType_Create_Ident(t_type, v_name->name);

      /* Register the type in the proper namespace. */
      Init_Value(& v);
      Setup_Value_As_Type(& v, ident_type);
      v_named_type = Namespace_Add_Value(NMSPFLOOR_DEFAULT, v_name->name, & v);
      Finish_Value(& v);
      (void) BoxType_Unlink(ident_type);

      Destroy_Value(v_type);
      Destroy_Value(v_name);
      return v_named_type;

    } else if (Has_Type(v_name)) {
      BoxType *t = v_name->type;
      if (BoxType_Is_Subtype(t)) {
        if (BoxType_Is_Registered_Subtype(t)) {
          BoxType *t_child;
          BoxBool success = BoxType_Get_Subtype_Info(t, NULL, NULL, & t_child);
          assert(success);
          if (BoxType_Compare(t_child, v_type->type) == BOXTYPECMP_DIFFERENT)
            LOG_ERR("Inconsistent redefinition of type `%T': was `%T' and is "
                    "now `%T'", v_name->type, t_child, v_type->type);
        } else {
          (void) BoxType_Register_Subtype(t, v_type->type);
          /* ^^^ ignore state of success of operation. */
        }

      } else if (BoxType_Compare(v_name->type, v_type->type)
                 == BOXTYPECMP_DIFFERENT) {
        LOG_ERR("Inconsistent redefinition of type `%T'.", v_name->type);
      }

      Destroy_Value(v_name);
      return v_type;
    }
  }

  Destroy_Value(v_type);
  Destroy_Value(v_name);
  return v_named_type;
}

void
Compiler::Compile_BinOp(BoxASTNode *node)
{
  BoxASTNodeBinOp *bin_op_node = (BoxASTNodeBinOp *) node;

  assert(BoxASTNode_Get_Type(node) == BOXASTNODETYPE_BIN_OP);

  Compile_Any(bin_op_node->lhs);
  Compile_Any(bin_op_node->rhs);
  if (Pop_Errors(/* pop */ 2, /* push err */ 1)) {
    return;
  }

  {
    Value *left, *right, *result = NULL;
    BoxASTBinOp op;

    /* Get values from stack */
    right = Pop_Value();
    left  = Pop_Value();

    op = bin_op_node->op;
    if (op == BOXASTBINOP_ASSIGN) {
      if (BoxASTNode_Is_Type(bin_op_node->lhs)
          && BoxASTNode_Is_Type(bin_op_node->rhs)) {
        result = Compile_Type_Assignment(left, right);
      } else {
        result = Compile_Value_Assignment(left, right);
      }
    } else {
      if (Want_Instance(left) & Want_Instance(right)) {
         /* NOTE: ^^^ We use & rather than &&*/
        result = Emit_BinOp(op, left, right);
      } else {
        Destroy_Value(left);
        Destroy_Value(right);
      }
    }

    Push_Value(result);
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
    v_struc = Namespace_Get_Value(NMSPFLOOR_DEFAULT, "#");
    if (!v_struc) {
      LOG_ERR("Cannot get implicit member '%s'. Default "
              "parent is not defined in current scope.", member_name);
      Push_Error(1);
      return;
    }
  } else {
    Compile_Any(parent);
    v_struc = Pop_Value();
  }

  if (!Want_Instance(v_struc)) {
    Destroy_Value(v_struc);
    Push_Error(1);
    return;
  }

  t = BoxType_Link(v_struc->type);
  v_struc = Emit_Get_Struc_Member(v_struc, member_name);
  if (!v_struc)
    LOG_ERR("Cannot find the member `%s' of an object with type `%T'.",
            member_name, t);
  (void) BoxType_Unlink(t);
  Push_Value(v_struc);
}

void
Compiler::Compile_ArgGet(BoxASTNode *node)
{
  Value *v_self = NULL;
  const char *n_self = NULL;
  ASTSelfLevel self_level = ((BoxASTNodeArgGet *) node)->depth;
  bool promote_to_target = false;
  int i;

  assert(BoxASTNode_Get_Type(node) == BOXASTNODETYPE_ARG_GET);

  switch (self_level) {
  case 1:
    n_self = "$";
    v_self = Namespace_Get_Value(NMSPFLOOR_DEFAULT, "$");
    break;

  case 2:
    n_self = "$$";
    v_self = Namespace_Get_Value(NMSPFLOOR_DEFAULT, "$$");
    promote_to_target = true;
    break;

  default:
    n_self = "$$, $3, ...";
    v_self = Namespace_Get_Value(NMSPFLOOR_DEFAULT, "$$");
    for (i = 2; i < self_level && v_self; i++) {
      v_self = Emit_Get_Subtype_Parent(v_self);
    }
    promote_to_target = true;
  }

  if (!v_self) {
    LOG_ERR("%s not defined in the current scope.", n_self);
  } else {
    if (promote_to_target)
      v_self = Temp_As_Target(v_self);
  }

  Push_Value(v_self);
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
  v_child = Pop_Value();
  Compile_Any(comb_def_node->parent);
  v_parent = Pop_Value();
  comb_type = (BoxCombType) comb_def_node->comb_type;

  no_err = Want_Type(v_child) & Want_Type(v_parent);

  /* Get the types from the values, and destroy the latters. */
  t_child = BoxType_Link(v_child->type);
  t_parent = BoxType_Link(v_parent->type);
  Destroy_Value(v_child);
  Destroy_Value(v_parent);

  /* now get the C-name, if present */
  if (comb_def_node->c_name) {
    assert(BoxASTNode_Get_Type(comb_def_node->c_name)
           == BOXASTNODETYPE_STR_IMM);
    c_name = ((BoxASTNodeStrImm *) comb_def_node->c_name)->str;
    if (strlen(c_name) < 1) {
      /* NOTE: Should we test any other kind of "badness"? */
      Log(& comb_def_node->c_name->head.src, BOXLOGLEVEL_ERROR,
          "Empty string in C-name for procedure declaration.");
      no_err = 0;
    }
  }

  /* Now let's find whether a procedure of this kind is already registered. */
  if (!no_err) {
    /* For now we cowardly refuse to examine the body of the procedure
     * and immediately exit pushing an error.
     */
    Push_Value(v_ret);
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
    Namespace_Add_Procedure(NMSPFLOOR_DEFAULT, t_parent, comb);
  }

  /* Set the C-name of the procedure, if given. */
  if (c_name) {
    BoxCallable_Set_Uid(comb_callable, c_name);
    if (!n_implem)
      BoxVMSym_Reference_Proc(vm_, comb_callable);
  }

  /* If an implementation is also provided, then we define the procedure */
  if (n_implem) {
    /* we have the implementation */
    BoxVMCode *save_cur_proc = cur_proc_;
    BoxLIR *save_cur_lir = lir_;
    Value *v_implem;
    BoxVMCode proc_implem;
    BoxVMCallNum cn;

    /* We change target of the compilation to the new procedure */
    BoxVMCode_Init(& proc_implem, BOXVMCODESTYLE_SUB);
    cur_proc_ = & proc_implem;
    lir_ = BoxLIR_Create();
    assert(lir_);

    /* A BoxVMCode object is used to get the procedure symbol and to register
     * and assemble it.
     */
    proc = BoxLIR_Append_Proc(lir_);
    assert(proc);
    BoxLIR_Set_Target_Proc(lir_, proc);

    /* Set the call number. */
    if (!BoxType_Generate_Combination_Call_Num(comb, vm_, & cn))
      LOG_FATAL("Cannot generate call number for combination.");
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
    v_implem = Pop_Value();
    Destroy_Value(v_implem);

    cur_proc_ = save_cur_proc;

    Install(& proc_implem);

    BoxVMCode_Finish(& proc_implem);
    BoxLIR_Destroy(lir_);
    lir_ = save_cur_lir;
  }

  (void) BoxType_Unlink(t_child);
  (void) BoxType_Unlink(t_parent);

  /* NOTE: for now we return Void[]. In future extensions we'll return
   * a function object
   */
  v_ret = My_Get_Void_Value(this);

  /* for now we return v_ret = NULL. We'll return a function, when Box will
   * support functions.
   */
  Push_Value(v_ret);
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
    Set_Cur_Node((BoxASTNode *) memb);

    assert(BoxASTNode_Get_Type((BoxASTNode *) memb) == BOXASTNODETYPE_MEMBER);
    assert(memb->expr);

    Compile_Any(memb->expr);
    v_member = Get_Value(0);
    no_err &= Want_Instance(v_member);
    if (no_err && BoxType_Is_Empty(v_member->type)) {
      LOG_ERR("Invalid structure member of type `%T'", v_member->type);
      no_err = 0;
    }
  }

  /* Check for errors */
  if (!no_err) {
    Remove_Any(num_members);
    Push_Error(1);
    return;
  }

  /* built the type for the structure */
  i = num_members;
  t_struc = BoxType_Create_Structure();
  for(memb = compound->memb; memb; memb = memb->next) {
    Value *v_member = Get_Value(--i);
    BoxType_Add_Member_To_Structure(t_struc, v_member->type, /*name*/ NULL);
  }

  /* create and populate the structure */
  v_struc = Create_Value();
  Setup_Value_As_Temp(v_struc, t_struc);

  for(ValueStrucIter_Init(& vsi, v_struc);
      vsi.has_next; ValueStrucIter_Do_Next(& vsi)) {
    Value *v_member = Get_Value(num_members - vsi.index - 1);
    Destroy_Value(Emit_Value_Move(Weak_Copy_Value(& vsi.v_member),
                                  Weak_Copy_Value(v_member)));

  }

  ValueStrucIter_Finish(& vsi);

  Remove_Any(num_members);
  Push_Value(v_struc);
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
    Set_Cur_Node((BoxASTNode *) memb);

    assert(BoxASTNode_Get_Type((BoxASTNode *) memb) == BOXASTNODETYPE_MEMBER);

    if (memb->name) {
      assert(BoxASTNode_Get_Type(memb->name) == BOXASTNODETYPE_VAR_IDFR);
      memb_name = ((BoxASTNodeVarIdfr *) memb->name)->name;
    }

    if (memb->expr) {
      Value *v_type;
      Compile_Any(memb->expr);
      v_type = Pop_Value();

      err = !Want_Type(v_type);
      if (!err) {
        previous_type = v_type->type;
        if (BoxType_Is_Empty(previous_type)) {
          LOG_ERR("Zero-sized type `%T' not allowed as the member of a "
                  "structure", previous_type);
          err = 1;
        }
      }

      Destroy_Value(v_type);
    }

    if (previous_type && !err) {
      /* Check for duplicate structure members */
      if (memb_name) {
        if (BoxType_Find_Structure_Member(struc_type, memb_name))
          LOG_ERR("Duplicate member `%s' in structure type definition.",
                  memb_name);
      }

      BoxType_Add_Member_To_Structure(struc_type, previous_type, memb_name);
    }
  }

  /* Check for errors */
  if (err) {
    Push_Error(1);
    return;
  }

  v_struc_type = Create_Value();
  Setup_Value_As_Type(v_struc_type, struc_type);
  (void) BoxType_Unlink(struc_type);
  Push_Value(v_struc_type);
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
    v_type = Pop_Value();

    if (Want_Type(v_type)) {
      BoxType *memb_type = v_type->type;
      /* NOTE: should check for duplicate types in species */
      BoxType_Add_Member_To_Species(spec_type, memb_type);
    }

    Destroy_Value(v_type);
  }

  v_spec_type = Create_Value();
  Setup_Value_As_Type(v_spec_type, spec_type);
  (void) BoxType_Unlink(spec_type);

  Push_Value(v_spec_type);
}

void
Compiler::Compile_Paren(BoxASTNode *expr)
{
  Compile_Any(expr);
  Set_Ignorable(Get_Value(0), false);
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
    LOG_FATAL("Unexpected compound kind %d", (int) compound->kind);
    abort();
  }
}

}
