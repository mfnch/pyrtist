/****************************************************************************
 * Copyright (C) 2009-2011 by Matteo Franchin                               *
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

#include <stdlib.h>
#include <assert.h>

#include "types.h"
#include "mem.h"
#include "print.h"
#include "messages.h"
#include "container.h"
#include "value.h"
#include "combs.h"

#include "compiler_priv.h"


namespace Box {

Value *
Compiler::Alloc_Value()
{
  if (free_value_chain_) {
    // Chain not empty. Get a value from there.
    Value *v = & free_value_chain_->value;
    free_value_chain_ = free_value_chain_->next_in_chain;
    return v;
  }

  // Chain is empty, we need to get genuine new memory.
  Value *v = (Value *) BoxAllocPool_Alloc(& value_pool_, sizeof(ValueOrChain));
  if (v)
    return v;

  LOG_FATAL("Cannot allocate new Value");
  return NULL;
}

void
Compiler::Free_Value(Value *v)
{
  ValueOrChain *vc = (ValueOrChain *) v;
  assert(& vc->value == v);
  vc->next_in_chain = free_value_chain_;
  free_value_chain_ = vc;
}

Value *
Compiler::Create_Value()
{
#if BOX_USE_VALUE_CACHE
  Value *v = Alloc_Value();
#else
  Value *v = (Value *) Box_Mem_Safe_Alloc(sizeof(Value));
#endif

  Init_Value(v);
  v->attr.new_or_init = 1;
  Track_Value(v);
  return v;
}

Value *
Compiler::Destroy_Value(Value *v)
{
  if (!v || v->attr.read_only)
    return NULL;

  Finish_Value(v);
  if (v->attr.new_or_init) {
    Untrack_Value(v);
#if BOX_USE_VALUE_CACHE
    Free_Value(v);
#else
    Box_Mem_Free(v);
#endif
    return NULL;
  }

  return v;
}

Value *
Compiler::Weak_Copy_Value(Value *v_src)
{
  Value *v_cpy = Create_Value();
  return Setup_Value_As_Weak_Copy(v_cpy, v_src);
}

Value *
Compiler::Init_Value(Value *v)
{
  v->kind = VALUEKIND_ERR;
  v->type = NULL;
  v->name = NULL;
  v->attr.read_only = 0;
  v->attr.new_or_init = 0;
  v->attr.own_register = 0;
  v->attr.ignore = 0;
  return v;
}

Value *
Compiler::Finish_Value(Value *v)
{
  RegAlloc *ra = & c->cur_proc->reg_alloc;

  assert(!v->attr.read_only);

  if (v->name) {
    Box_Mem_Free(v->name);
    v->name = NULL;
  }

  switch(v->kind) {
  case VALUEKIND_TYPE_NAME:
  case VALUEKIND_VAR_NAME:
    break;
  case VALUEKIND_TARGET:
  case VALUEKIND_TEMP:
    switch (v->cont.categ) {
    case BOXCONTCATEG_LREG:
      if (v->attr.own_register) {
        if (v->cont.value.reg >= 0)
          Reg_Release(ra, v->cont.type, v->cont.value.reg);
      }
      break;
    case BOXCONTCATEG_GREG:
      break;
    case BOXCONTCATEG_PTR:
      if (v->attr.own_register) {
        assert(!v->cont.value.ptr.is_greg);
        Reg_Release(ra, BOXTYPEID_OBJ, v->cont.value.ptr.reg);
      }
      break;
    default:
      abort();
    }
    break;
  default:
    break;
  }

  v->kind = VALUEKIND_ERR;
  return v;
}

}

Value *
Value_Move(BoxCmp *c, Value *v_dst, Value *v_src)
{
  int new_or_init;

  if (v_dst == v_src)
    return v_dst;

  if (v_src->attr.read_only)
    return c->compiler->Setup_Value_As_Weak_Copy(v_dst, v_src);

  new_or_init = v_dst->attr.new_or_init;
  *v_dst = *v_src;
  v_dst->attr.new_or_init = new_or_init;
  v_src->name = NULL;
  v_src->type = NULL;
  v_src->kind = VALUEKIND_ERR;
  return v_dst;
}

char *
Value_Steal_Name(Value *src)
{
  char *name = src->name;
  if (src->attr.read_only)
    return Box_Mem_Strdup(name);
  src->name = NULL;
  return name;
}

Value *
Value_Mark_RO(Value *v)
{
  v->attr.read_only = 1;
  return v;
}

Value *
Value_Unmark_RO(Value *v)
{
  v->attr.read_only = 0;
  return v;
}

const char *
ValueKind_To_Str(ValueKind vk)
{
  switch(vk) {
  case VALUEKIND_ERR: return "an error expression";
  case VALUEKIND_VAR_NAME: return "an undefined variable";
  case VALUEKIND_TYPE_NAME: return "an undefined type";
  case VALUEKIND_TYPE: return "a type expression";
  case VALUEKIND_IMM: return "a constant expression";
  case VALUEKIND_TEMP: return "an intermediate expression";
  case VALUEKIND_TARGET: return "a target expression";
  default: return "??? (unknown value kind)";
  }
}

namespace Box {

bool
Compiler::Want_Instance(Value *v)
{
  if (Value_Is_Value(v))
    return true;

  if (Value_Is_Err(v))
    return false;

  if (v->name)
    LOG_ERR("`%s' is undefined: an expression with both "
            "value and type is expected here.", v->name);
  else
    LOG_ERR("Got `%s', but an expression with both value "
            "and type is expected here.", ValueKind_To_Str(v->kind));
  return false;
}

bool
Compiler::Want_Type(Value *v)
{
  if (Value_Has_Type(v))
    return true;

  if (Value_Is_Err(v))
    return false;

  if (v->name)
    LOG_ERR("`%s' is undefined: an expression with "
            "defined type is expected here.", v->name);
  else
    LOG_ERR("Got `%s', but an expression with defined "
            "type is expected here.", ValueKind_To_Str(v->kind));
  return false;
}

Value *
Compiler::Setup_Value_As_Weak_Copy(Value *v_copy, Value *v)
{
  v_copy->kind = v->kind;
  v_copy->type = BoxType_Link(v->type);
  v_copy->cont = v->cont;
  v_copy->name = (v->name == NULL) ? NULL : Box_Mem_Strdup(v->name);
  v_copy->attr.own_register = 0;
  v_copy->attr.ignore = 0;
  return v_copy;
}

void
Compiler::Setup_Value_As_Var_Name(Value *v, const char *name)
{
  v->kind = VALUEKIND_VAR_NAME;
  v->name = Box_Mem_Strdup(name);
}

Value *
Compiler::Setup_Value_As_Type_Name(Value *v, const char *name)
{
  v->kind = VALUEKIND_TYPE_NAME;
  v->name = Box_Mem_Strdup(name);
  return v;
}

Value *
Compiler::Setup_Value_As_Type(Value *v, BoxType *t)
{
  v->kind = VALUEKIND_TYPE;
  v->type = BoxType_Link(t);
  v->cont.type = BoxType_Get_Cont_Type(v->type);
  return v;
}

void
Compiler::Setup_Value_As_Imm_Char(Value *v, BoxChar c)
{
  v->kind = VALUEKIND_IMM;
  v->type = BoxType_Link(Box_Get_Core_Type(BOXTYPEID_CHAR));
  BoxCont_Set(& v->cont, "ic", c);
}

void
Compiler::Setup_Value_As_Imm_Int(Value *v, BoxInt i)
{
  v->kind = VALUEKIND_IMM;
  v->type = BoxType_Link(Box_Get_Core_Type(BOXTYPEID_SINT));
  BoxCont_Set(& v->cont, "ii", i);
}

void
Compiler::Setup_Value_As_Imm_Real(Value *v, BoxReal r)
{
  v->kind = VALUEKIND_IMM;
  v->type = BoxType_Link(Box_Get_Core_Type(BOXTYPEID_SREAL));
  BoxCont_Set(& v->cont, "ir", r);
}

void
Compiler::Setup_Value_As_Void(Value *v)
{
  v->kind = VALUEKIND_IMM;
  v->type = BoxType_Link(Box_Get_Core_Type(BOXTYPEID_VOID));
  v->cont.type = BOXTYPEID_VOID;
}

void
Compiler::Setup_Value_As_Temp(Value *v, BoxType *t)
{
  ValContainer vc = {VALCONTTYPE_LREG, -1, 0};
  Setup_Value_Container(v, t, & vc);
  Emit_Value_Alloc(v);
}

void
Compiler::Setup_Value_As_Var(Value *v, BoxType *t)
{
  BoxVMCode *p = c->cur_proc;
  if (BoxVMCode_Get_Style(p) == BOXVMCODESTYLE_MAIN) {
    /* We are creating the main procedure so variables should get into global
     * registers
     */
    ValContainer vc = {VALCONTTYPE_GVAR, -1, 0};
    Setup_Value_Container(v, t, & vc);
  } else {
    /* We are not in the main: variables should go into local registers */
    ValContainer vc = {VALCONTTYPE_LVAR, -1, 0};
    Setup_Value_Container(v, t, & vc);
  }
}

void
Compiler::Setup_Value_As_String(Value *v_str, const char *str)
{
  BoxTask success;
  size_t len, addr;
  Value v_str_data;
  ValContainer vc = {VALCONTTYPE_GPTR, 0, 0};

  /* Setup string data. */
  len = strlen(str) + 1;
  addr = BoxVM_Add_Data(c->vm, str, len, BOXTYPEID_OBJ);
  assert(addr >= 0);
  vc.addr = addr;
  Init_Value(& v_str_data);
  Setup_Value_Container(& v_str_data, Box_Get_Core_Type(BOXTYPEID_OBJ), & vc);

  /* Setup string object. */
  Setup_Value_As_Temp(v_str, Box_Get_Core_Type(BOXTYPEID_STR));

  /* Populate string object from string data. */
  success = c->compiler->Emit_Call(v_str, & v_str_data);

  if (success != BOXTASK_OK)
    BoxCmp_Log_Fatal(c, "Failure while emitting string.");
}

/* Create a new empty container. */
void
Compiler::Setup_Value_Container(Value *v, BoxType *type, ValContainer *vc)
{
  RegAlloc *ra = & c->cur_proc->reg_alloc;
  int use_greg;

  v->type = BoxType_Link(type);
  v->cont.type = BoxType_Get_Cont_Type(v->type);

  switch(vc->type_of_container) {
  case VALCONTTYPE_IMM:
    v->kind = VALUEKIND_IMM;
    v->cont.categ = BOXCONTCATEG_IMM;
    return;

  case VALCONTTYPE_LREG:
    v->kind = VALUEKIND_TEMP;
    v->cont.categ = BOXCONTCATEG_LREG;
    if (vc->which_one < 0) {
      /* Automatically chooses the local register.
         NOTE: if cont.type == BOXTYPEID_VOID this function returns 0, meaning
           that for this type there is no need for registers. */
      BoxInt reg = Reg_Occupy(ra, v->cont.type);
      assert(reg >= 0);
      v->cont.value.reg = reg;
      v->attr.own_register = (reg > 0);
      return;
    } else {
      /* The user wants a particular register to be chosen */
      v->cont.value.reg = vc->which_one;
      return;
    }
    break;

  case VALCONTTYPE_GVAR:
    v->kind = VALUEKIND_TARGET;
    v->cont.categ = BOXCONTCATEG_GREG;
    if (vc->which_one < 0) {
      BoxInt reg = -GVar_Occupy(ra, v->cont.type);
      /* Automatically choses the local variables */
      assert(reg <= 0);
      v->cont.value.reg = reg;
      return;
    } else {
      /* The user wants a particolar variable to be chosen */
      v->cont.value.reg = vc->which_one;
      return;
    }
    break;

  case VALCONTTYPE_LVAR:
    v->kind = VALUEKIND_TARGET;
    v->cont.categ = BOXCONTCATEG_LREG;
    if (vc->which_one < 0) {
      /* Automatically choses the local variables */
      BoxInt reg = -Var_Occupy(ra, v->cont.type, 0);
      assert(reg <= 0);
      v->cont.value.reg = reg;
      return;
    } else {
      /* The user wants a particolar variable to be chosen */
      v->cont.value.reg = vc->which_one;
      return;
    }
    break;

  case VALCONTTYPE_GREG:
    v->cont.categ = BOXCONTCATEG_GREG;
    v->cont.value.reg = vc->which_one;
    return;

  case VALCONTTYPE_GPTR:
  case VALCONTTYPE_LRPTR:
  case VALCONTTYPE_LVPTR:
    use_greg = (vc->type_of_container == VALCONTTYPE_GPTR);
    v->kind = VALUEKIND_TARGET;
    v->cont.categ = BOXCONTCATEG_PTR;
    v->cont.value.ptr.is_greg = use_greg;
    v->cont.value.ptr.reg = vc->which_one;
    v->cont.value.ptr.offset = vc->addr;
    if (use_greg || vc->addr >= 0) return;

    if (vc->type_of_container == VALCONTTYPE_LRPTR) {
      BoxInt reg = Reg_Occupy(ra, BOXTYPEID_OBJ);
      v->cont.value.ptr.reg = reg;
      assert(reg >= 1);
      return;
    } else {
      BoxInt reg = -Var_Occupy(ra, BOXTYPEID_OBJ, 0);
      v->cont.value.ptr.reg = reg;
      assert(reg < 0);
    }
    return;

  default:
    LOG_FATAL("Setup_Value_Container: wrong type of container!");
    abort();
  }
}

/* Create a new local register value. */
void
Compiler::Setup_Value_As_LReg(Value *v, BoxType *type)
{
  ValContainer vc = {VALCONTTYPE_LREG, -1, 0};
  Setup_Value_Container(v, type, & vc);
}

void
Compiler::Emit_Value_Alloc(Value *v)
{
  switch (v->kind) {
  case VALUEKIND_ERR:
    return;
  case VALUEKIND_TEMP:
  case VALUEKIND_TARGET:
    if (v->cont.type == BOXTYPEID_OBJ) {
      BoxTypeId type_id = BoxVM_Install_Type(c->vm, v->type);

      /* The 'create' instruction automatically invokes the creator
       * when necessary
       */
      Value v_type_id;
      Init_Value(& v_type_id);
      Setup_Value_As_Imm_Int(& v_type_id, (BoxInt) type_id);
      BoxLIR_Append_GOp(c->lir, BOXGOP_CREATE,
                        2, & v->cont, & v_type_id.cont);
      Finish_Value(& v_type_id);
    }
    return;
  default:
    LOG_FATAL("Value_Emit_Allocate: invalid argument (%s).",
              ValueKind_To_Str(v->kind));
    abort();
  }
}

/* Doesn't unlink v, for coherence with Value_Emit_Unlink. */
void
Compiler::Emit_Link(Value *v)
{
  BoxTypeId cont_type = v->cont.type;
  if (cont_type == BOXTYPEID_OBJ || cont_type == BOXTYPEID_PTR) {
    assert(v->cont.categ == BOXCONTCATEG_LREG
           || v->cont.categ == BOXCONTCATEG_GREG);
    /* ^^^ for now we don't support the general case... */
    BoxLIR_Append_GOp(c->lir, BOXGOP_MLN, 1, & v->cont);
  }
}

void
Compiler::Emit_Unlink(Value *v)
{
  BoxTypeId cont_type = v->cont.type;
  if (cont_type == BOXTYPEID_OBJ || cont_type == BOXTYPEID_PTR) {
    assert(v->cont.categ == BOXCONTCATEG_LREG
           || v->cont.categ == BOXCONTCATEG_GREG);
    /* ^^^ for now we don't support the general case... */
    BoxLIR_Append_GOp(c->lir, BOXGOP_MUNLN, 1, & v->cont);
  }
}

BoxLIRNodeOpBranch *
Compiler::Emit_Conditional_Jump(Value *v)
{
  BoxLIRNodeOp *ret;
  BoxCont ri0_cont;
  BoxCont_Set(& ri0_cont, "ri", 0);
  BoxLIR_Append_GOp(c->lir, BOXGOP_MOV, 2, & ri0_cont, & v->cont);
  ret = BoxLIR_Append_Op_Branch(c->lir, BOXOP_JC_I, NULL);
  return (BoxLIRNodeOpBranch *) ret;
}

/* This will call Finish_Value() on v_dst, if necessary. */
Value *
Compiler::Emit_Make_Temp(Value *v_dst, Value *v_src)
{
  ValContainer vc = {VALCONTTYPE_LREG, -1, 0};

  switch (v_src->kind) {
  case VALUEKIND_VAR_NAME:
  case VALUEKIND_TYPE_NAME:
    LOG_ERR("Got %s ('%s'), but a defined type or value is expected "
            "here!", ValueKind_To_Str(v_src->kind), v_src->name);
  case VALUEKIND_ERR:
    Finish_Value(v_dst);
    v_dst->kind = VALUEKIND_ERR;
    break;
  case VALUEKIND_TEMP:
    return Value_Move(c, v_dst, v_src);
  case VALUEKIND_TYPE:
    {
      BoxType *t = BoxType_Link(v_src->type);
      Setup_Value_Container(Finish_Value(v_dst), t, & vc);
      (void) BoxType_Unlink(t);
      Emit_Value_Alloc(v_dst);
      return v_dst;
    }
  case VALUEKIND_IMM:
  case VALUEKIND_TARGET:
    {
      BoxType *t = BoxType_Link(v_src->type);
      BoxCont cont = v_src->cont;
      Setup_Value_Container(Finish_Value(v_dst), t, & vc);
      (void) BoxType_Unlink(t);
      BoxLIR_Append_GOp(c->lir, BOXGOP_MOV, 2, & v_dst->cont, & cont);
      return v_dst;
    }
  }

  abort();
  return NULL;
}

}

Value *
Value_To_Temp_Or_Target(BoxCmp *c, Value *v_dst, Value *v_src)
{
  if (v_src->kind == VALUEKIND_TARGET)
    return Value_Move(c, v_dst, v_src);
  return c->compiler->Emit_Make_Temp(v_dst, v_src);
}

Value *
Value_Promote_Temp_To_Target(Value *v)
{
  if (v->kind == VALUEKIND_TEMP)
    v->kind = VALUEKIND_TARGET;
  return v;
}

Value *
Value_Set_Ignorable(Value *v, BoxBool ignorable)
{
  v->attr.ignore = ignorable;
  return v;
}

int Value_Is_Err(Value *v) {
  return (v->kind == VALUEKIND_ERR);
}

int Value_Is_Temp(Value *v)
{
  return (v->kind == VALUEKIND_TEMP);
}

BoxBool BoxValue_Is_Var_Name(BoxValue *v)
{
  return (v->kind == VALUEKIND_VAR_NAME);
}

int Value_Is_Type_Name(Value *v)
{
  return (v->kind == VALUEKIND_TYPE_NAME);
}

int Value_Is_Target(Value *v)
{
  return (v->kind == VALUEKIND_TARGET);
}

int Value_Is_Value(Value *v)
{
  switch(v->kind) {
  case VALUEKIND_IMM: case VALUEKIND_TEMP: case VALUEKIND_TARGET:
    return 1;
  default:
    return 0;
  }
}

int Value_Is_Ignorable(Value *v) {
  int ignore =    (v->kind == VALUEKIND_ERR)
               || (v->kind == VALUEKIND_TYPE)
               || v->attr.ignore;
  if (ignore)
    return 1;

  else if (Value_Is_Value(v))
    return (BoxType_Compare(Box_Get_Core_Type(BOXTYPEID_VOID), v->type)
            != BOXTYPECMP_DIFFERENT);

  else
    return 0;
}

int Value_Has_Type(Value *v)
{
  switch(v->kind) {
  case VALUEKIND_TYPE_NAME:
  case VALUEKIND_VAR_NAME:
  case VALUEKIND_ERR:
    return 0;
  default:
    return 1;
  }
}

namespace Box {

/**
 * @note Destroy @p v and return a new Value.
 */
Value *
Compiler::Value_Cast_To_Ptr_2(Value *v)
{
  BoxContCateg v_categ = v->cont.categ;

  switch (v->cont.type) {
  case BOXTYPEID_OBJ:
    if (v_categ != BOXCONTCATEG_PTR) {
      assert(v_categ == BOXCONTCATEG_LREG || v_categ == BOXCONTCATEG_GREG);
      return v;
    } else {
      /* v_categ == BOXCONTCATEG_PTR */
      bool is_greg = v->cont.value.ptr.is_greg;
      BoxInt reg = v->cont.value.ptr.reg,
             offset = v->cont.value.ptr.offset;
      BoxCont cont, *cont_src;
      Value *v_unlink = NULL;

      if (offset == 0) {
        /* When offset == 0, we do not need to allocate a new register. We just
         * need to convert the ``o[reg + 0]'' value into a simple ``reg''
         * value.
         */
        cont_src = & v->cont;
      } else {
        /* When offset != 0, we need to increment the pointer in v. */
        if (v->attr.own_register) {
          assert(!is_greg);
          cont_src = & v->cont;
        } else {
          v_unlink = v;
          v = Create_Value();
          Setup_Value_As_LReg(v, v_unlink->type);
          cont_src = & cont;
        }
      }

      /* Set cont_src to the register containing the base address. */
      cont_src->categ = (is_greg) ? BOXCONTCATEG_GREG : BOXCONTCATEG_LREG;
      cont_src->type = BOXTYPEID_OBJ;
      cont_src->value.reg = reg;

      /* Obtain the destination register as base address plus offset. */
      if (offset != 0) {
        Value *v_offs = Create_Value();
        Setup_Value_As_Imm_Int(v_offs, offset);
        BoxLIR_Append_GOp(c->lir, BOXGOP_ADD,
                          3, & v->cont, & v_offs->cont, cont_src);
        Destroy_Value(v_offs);
      }

      if (v_unlink)
        Destroy_Value(v_unlink);
      return v;
    }
    break;

  case BOXTYPEID_PTR:
    return v;

  default:
    /* Deal with the fast types by creating a NULL-block pointer. */
    {
      Value *v_new = Create_Value();
      Setup_Value_As_Temp(v_new, Box_Get_Core_Type(BOXTYPEID_PTR));
      BoxLIR_Append_GOp(c->lir, BOXGOP_LEA,
                        2, & v_new->cont, & v->cont);
      Destroy_Value(v);
      return v_new;
    }
  }

  abort();
  return NULL;
}

/**
 * @brief Weak expansion to a weak boxed type (BoxAny).
 */
static Value *
My_Value_Weak_Box(BoxCmp *c, Value *src)
{
  BoxType *t_src = src->type;
  BoxType *t_dst = Box_Get_Core_Type(BOXTYPEID_ANY);
  BoxCont ri0, src_type_id_cont;
  BoxTypeId src_type_id = BoxVM_Install_Type(c->vm, src->type);

  if (t_src == t_dst)
    return src;

  t_src = BoxType_Resolve(t_src,
     static_cast<BoxTypeResolve>(BOXTYPERESOLVE_IDENT
                                 | BOXTYPERESOLVE_SPECIES), 0);

  if (t_src == t_dst)
    return src;

  assert(BoxType_Get_Class(t_dst) == BOXTYPECLASS_ANY);

  /* Set up a container representing the register ri0 and one representing
   * the type integer.
   */
  BoxCont_Set(& ri0, "ri", 0);
  BoxCont_Set(& src_type_id_cont, "ii", (BoxInt) src_type_id);

  /* Create a new ANY type. */
  Value *v_dst = c->compiler->Create_Value();
  c->compiler->Setup_Value_As_Temp(v_dst, Box_Get_Core_Type(BOXTYPEID_ANY));

  /* Generate the boxing instructions. */
  if (!BoxType_Is_Empty(src->type)) {
    /* The object has associated data: get data pointer in v_src_ptr. */
    Value *v_src_ptr = c->compiler->Weak_Copy_Value(src);

    /* If src is an immediate, we move it into a register, so we can use
     * the lea instruction to build a pointer to it. We then keep the
     * register allocated until we have finished with the boxing operation.
     * Note that the box instruction copies objects passed with NULL-block
     * pointers. This means that fast types are always copied.
     */
    if (v_src_ptr->kind == VALUEKIND_IMM)
      c->compiler->Emit_Make_Temp(v_src_ptr, v_src_ptr);

    /* Get a pointer to the object and use it in the boxing operation. */
    v_src_ptr = c->compiler->Value_Cast_To_Ptr_2(v_src_ptr);
    BoxLIR_Append_GOp(c->lir, BOXGOP_TYPEOF, 2, & ri0, & src_type_id_cont);
    BoxLIR_Append_GOp(c->lir, BOXGOP_WBOX,
                      3, & v_dst->cont, & v_src_ptr->cont, & ri0);
    c->compiler->Destroy_Value(v_src_ptr);
  } else {
    BoxLIR_Append_GOp(c->lir, BOXGOP_TYPEOF,
                      2, & ri0, & src_type_id_cont);
    BoxLIR_Append_GOp(c->lir, BOXGOP_BOX,
                      2, & v_dst->cont, & ri0);
  }

  return v_dst;
}

void
Compiler::Emit_Call(BoxVMCallNum call_num, Value *parent, Value *child)
{
  assert(parent && child);

  if (parent->cont.type != BOXTYPEID_VOID) {
    BoxGOp op = ((parent->cont.type == BOXTYPEID_OBJ
                  && parent->cont.categ != BOXCONTCATEG_PTR) ?
                 BOXGOP_MOV : BOXGOP_LEA);
    BoxLIR_Append_GOp(c->lir, op,
                      2, & c->cont.pass_parent, & parent->cont);
  }

  if (child->cont.type != BOXTYPEID_VOID) {
    Value v_to_pass;
    Init_Value(& v_to_pass);
    Value_To_Temp_Or_Target(c, & v_to_pass, child);
    BoxGOp op = ((child->cont.type == BOXTYPEID_OBJ
                  && child->cont.categ != BOXCONTCATEG_PTR) ?
                 BOXGOP_REF : BOXGOP_LEA);
    BoxLIR_Append_GOp(c->lir, op, 2, & c->cont.pass_child, & v_to_pass.cont);
    Finish_Value(& v_to_pass);
  }

  BoxLIR_Append_Op1(c->lir, BOXOP_CALL_I, BOXCONTCATEG_IMM, call_num);
}

BoxTask
Compiler::Emit_Call(Value *parent, BoxTypeId tid_child)
{
  Value v_child;
  Init_Value(& v_child);
  Setup_Value_As_Type(& v_child, Box_Get_Core_Type(tid_child));
  Emit_Call(parent, & v_child);
}

/**
 * @note Destroy @p v_child, except when @c false is returned.
 */
static bool
My_Emit_Dynamic_Call(BoxCmp *c, Value *v_parent, Value *v_child)
{
  assert(BoxType_Is_Any(v_parent->type) && BoxType_Is_Any(v_child->type));
  v_child = c->compiler->Value_Cast_To_Ptr_2(v_child);
  BoxLIR_Append_GOp(c->lir, BOXGOP_DYCALL,
                    2, & v_parent->cont, & v_child->cont);
  c->compiler->Destroy_Value(v_child);
  return true;
}

/**
 * @brief Input Value-s are not destroyed.
 * @note Destroy @p child, except when BOXTASK_FAILURE is returned.
 */
BoxTask
Compiler::Emit_Call(Value *parent, Value *child)
{
  BoxCallable *cb;
  BoxTask dummy;
  BoxType *expand_type;

  assert(parent && child);

  if (Value_Is_Err(parent) || Value_Is_Err(child)) {
    /* In case of error silently exits. */
    Destroy_Value(child);
    return BOXTASK_OK;
  }

  /* We expand the child, since things like X.Y@Z are not allowed: in other
   * words, subtypes can never be children of any type.
   */
  child = Emit_Subtype_Expansion(child);

  /* Types derived from Void are always ignored */
  if (BoxType_Compare(child->type, Box_Get_Core_Type(BOXTYPEID_VOID))) {
    Destroy_Value(child);
    return BOXTASK_OK;
  }

  /* Now we search for the procedure associated with *child */
  BoxTypeCmp expand;
  BoxType *found_combination =
    BoxType_Find_Combination(parent->type, BOXCOMBTYPE_AT,
                             child->type, & expand);

  if (!found_combination) {
    if (!BoxType_Is_Any(child->type))
      return BOXTASK_FAILURE;

    /* Dynamic call. */
    Value *dyn_parent = My_Value_Weak_Box(c, parent);
    if (dyn_parent && My_Emit_Dynamic_Call(c, dyn_parent, child)) {
      Destroy_Value(dyn_parent);
      return BOXTASK_OK;
    }

    Destroy_Value(dyn_parent);
    return BOXTASK_FAILURE;
  }

  if (!BoxType_Get_Combination_Info(found_combination, & expand_type, & cb)) {
    BoxCmp_Log_Fatal(c, "Failed getting combination info");
    abort();
  }

  if (expand == BOXTYPECMP_MATCHING) {
    child = Emit_Value_Expansion(child, expand_type);
    if (!child) {
      Destroy_Value(child);
      return BOXTASK_ERROR;
    }
  }

  BoxVMCallNum cn;
  if (BoxType_Generate_Combination_Call_Num(found_combination, c->vm, & cn)) {
    Emit_Call(cn, parent, child);
    Destroy_Value(child);
    return BOXTASK_OK;
  }

  Destroy_Value(child);
  return BOXTASK_ERROR;
}

/*
 * @note Destroy @p v_ptr and return a new Value.
 */
Value *
Compiler::Emit_Value_Cast(Value *v_ptr, BoxType *t)
{
  assert(v_ptr->cont.type == BOXTYPEID_PTR);

  BoxCont *cont = & v_ptr->cont;
  BoxType *old_type = v_ptr->type;
  BoxTypeId new_cont_type = BoxType_Get_Cont_Type(t);

  switch(cont->categ) {
  case BOXCONTCATEG_GREG:
  case BOXCONTCATEG_LREG:
    v_ptr->type = BoxType_Link(t);
    (void) BoxType_Unlink(old_type);
    cont->type = new_cont_type;
    if (new_cont_type != BOXTYPEID_OBJ && new_cont_type != BOXTYPEID_PTR) {
      int is_greg = (cont->categ == BOXCONTCATEG_GREG);
      BoxInt reg = cont->value.reg;
      cont->categ = BOXCONTCATEG_PTR;
      cont->value.ptr.reg = reg;
      cont->value.ptr.is_greg = is_greg;
      cont->value.ptr.offset = 0;
    }
    return v_ptr;

  case BOXCONTCATEG_PTR:
    {
      BoxCont v_ptr_cont = v_ptr->cont;
      Destroy_Value(v_ptr);
      v_ptr = Create_Value();
      Setup_Value_As_Temp(v_ptr, Box_Get_Core_Type(BOXTYPEID_PTR));
      BoxLIR_Append_GOp(c->lir, BOXGOP_REF, 2,
                        & v_ptr->cont, & v_ptr_cont);
      assert(v_ptr->cont.categ == BOXCONTCATEG_LREG);
      return Emit_Value_Cast(v_ptr, t);
    }

  default:
    BoxCmp_Log_Fatal(c, "Unexpected container category!");
    abort();
  }

  abort();
  return NULL;
}

/**
 * @note Destroy @p v and return a new Value.
 */
Value *
Compiler::Emit_Cast_To_Ptr(Value *v)
{
  BoxCont *v_cont = & v->cont;

  if (v_cont->type == BOXTYPEID_OBJ && v_cont->categ != BOXCONTCATEG_PTR) {
    /* This is the case where we already have the pointer to the object
     * stored inside a register. We then have two cases:
     *  - such register is already in use. we cannot just change the
     *    value into a Ptr value. We rather have to move the pointer
     *    to a new register.
     *  - the register is fully owned by us, we can recycle it!
     */
    assert(v_cont->categ == BOXCONTCATEG_LREG
           || v_cont->categ == BOXCONTCATEG_GREG);
    /* We own the sole reference to v, which is a temporary quantity:
     * in other words we can do whathever we want with it!
     */
    v->type = BoxType_Link(Box_Get_Core_Type(BOXTYPEID_PTR));
    v_cont->type = BOXTYPEID_PTR;
    return v;
  } else {
    /* We have to get the pointer with a lea instruction. */
    BoxCont v_cont_val = *v_cont;
    Destroy_Value(v);
    v = Create_Value();
    Setup_Value_As_Temp(v, Box_Get_Core_Type(BOXTYPEID_PTR));
    BoxLIR_Append_GOp(c->lir, BOXGOP_LEA,
                      2, & v->cont, & v_cont_val);
    return v;
  }
}

/**
 * @brief Transform an offsetted pointer to a register pointer.
 * @detail Given an offsetted pointer like [roN + offset], carry out the sum
 *   (e.g. lea roM, [roN + offset]) to obtain a register pointer expression
 *   (e.g. roM).
 */
Value *
Compiler::Emit_Reduce_Ptr_Offset(Value *v_src)
{
  assert(v_src->cont.type == BOXTYPEID_OBJ);

  if (v_src->cont.categ != BOXCONTCATEG_PTR)
    return v_src;

  ValContainer vc = {VALCONTTYPE_LREG, -1, 0};
  BoxCont cont = v_src->cont;
  BoxType *t = BoxType_Link(v_src->type);

  Destroy_Value(v_src);
  Value *v_ret = Create_Value();
  Setup_Value_Container(v_ret, t, & vc);
  (void) BoxType_Unlink(t);

  assert(v_ret->cont.type == BOXTYPEID_OBJ);
  BoxLIR_Append_GOp(c->lir, BOXGOP_LEA,
                    2, & v_ret->cont, & cont);
  return v_ret;
}

}

/**
 * @brief Return a sub-field of an object type.
 * @param c Compiler object.
 * @param v_src_dst The parent object (and where the field is stored).
 * @param offset Offset of the subfield inside @p v_src_dst.
 * @param subf_type Type of object in subfield.
 * @return Return @p v_src_dst.
 * @note This function calls Finish_Value(v_src_dst) and stores the result
 *   in @p v_src_dst.
 */
Value *
Value_Get_Subfield(BoxCmp *c, Value *v_src_dst,
                   size_t offset, BoxType *subf_type)
{
  BoxCont *cont = & v_src_dst->cont;

  switch (cont->categ) {
  case BOXCONTCATEG_GREG:
  case BOXCONTCATEG_LREG:
    {
      BoxInt reg = cont->value.reg;
      int is_greg = (cont->categ == BOXCONTCATEG_GREG);
      cont->categ = BOXCONTCATEG_PTR;
      cont->value.ptr.offset = offset;
      cont->value.ptr.reg = reg;
      cont->value.ptr.is_greg = is_greg;
      cont->type = BoxType_Get_Cont_Type(subf_type);
      v_src_dst->type = BoxType_Link(subf_type);
      return v_src_dst;
    }
  case BOXCONTCATEG_PTR:
    cont->value.ptr.offset += offset;
    cont->type = BoxType_Get_Cont_Type(subf_type);
    v_src_dst->type = BoxType_Link(subf_type);
    return v_src_dst;
  case BOXCONTCATEG_IMM:
    BoxCmp_Log_Fatal(c, "Immediate objects not supported, yet!");
    break;
  }

  abort();
  return NULL;
}

/**
 * @brief Extract the member of a point object.
 * @param c The compiler object.
 * @param v_dst Where the member is stored.
 * @param v_src The input point object.
 * @param memb Name of the member to extract.
 * @return If successful, return @p v_dst. Otherwise, @p v_dst is destroyed and
 *   @c NULL is returned.
 */
static Value *
My_Point_Get_Member(BoxCmp *c, Value *v_dst, Value *v_src, const char *memb)
{
  char first = memb[0];
  if (first != '\0' && memb[1] == '\0') {
    BoxGOp g_op;

    switch(first) {
    case 'x': g_op = BOXGOP_PPTRX; break;
    case 'y': g_op = BOXGOP_PPTRY; break;
    default:
      return NULL;
    }

    c->compiler->Setup_Value_As_Temp(v_dst, Box_Get_Core_Type(BOXTYPEID_PTR));
    BoxLIR_Append_GOp(c->lir, g_op, 2, & v_dst->cont, & v_src->cont);
    v_dst->kind = VALUEKIND_TARGET;
    return Value_Get_Subfield(c, v_dst, (size_t) 0,
                              Box_Get_Core_Type(BOXTYPEID_SREAL));
  }
  c->compiler->Destroy_Value(v_dst);
  return NULL;
}

namespace Box {

/**
 * @note Destroy @p v_src and return a new Value.
 */
Value *
Compiler::Emit_Struc_Member_Get(Value *v_src, const char *memb)
{
  /* If v_struc is a subtype, then expand it (subtypes do not have members) */
  v_src = Emit_Subtype_Expansion(v_src);

  if (v_src->cont.type == BOXTYPEID_POINT) {
    Value *v_dst = Create_Value();
    if (My_Point_Get_Member(c, v_dst, v_src, memb)) {
      Destroy_Value(v_src);
      return v_dst;
    }
    Destroy_Value(v_dst);
    return NULL;
  }

  BoxType *struct_type = BoxType_Get_Stem(v_src->type);
  BoxType *node_type = BoxType_Find_Structure_Member(struct_type, memb);

  if (node_type) {
    size_t offset;
    BoxType *member_type;
    BoxBool result = BoxType_Get_Structure_Member(node_type, NULL,
                                                  & offset, 0, & member_type);
    if (result)
      return Value_Get_Subfield(c, v_src, offset, member_type);
  }
  Destroy_Value(v_src);
  return NULL;
}

void
Compiler::ValueStrucIter_Init(ValueStrucIter *vsi, Value *v_struc)
{
  BoxType *node, *t_struc;

  t_struc = BoxType_Get_Stem(v_struc->type);
  BoxTypeIter_Init(& vsi->type_iter, t_struc);
  vsi->has_next = BoxTypeIter_Get_Next(& vsi->type_iter, & node);
  vsi->index = 0;

  if (vsi->has_next) {
    BoxBool success;
    Value *v_member;
    size_t offset;

    Init_Value(& vsi->v_member);
    Setup_Value_As_Weak_Copy(& vsi->v_member, v_struc);
    success = BoxType_Get_Structure_Member(node, NULL, & offset,
                                           NULL, & vsi->t_member);
    assert(success);

    v_member = Value_Get_Subfield(c, & vsi->v_member, 0, vsi->t_member);
    assert(v_member == & vsi->v_member);
  }
}

void
Compiler::ValueStrucIter_Do_Next(ValueStrucIter *vsi)
{
  BoxType *prev_member_type = vsi->t_member;
  BoxType *node;

  vsi->has_next = BoxTypeIter_Get_Next(& vsi->type_iter, & node);
  ++vsi->index;

  if (vsi->has_next) {
    size_t offset;
    size_t delta_offset = BoxType_Get_Size(prev_member_type);
    Value *v_member;

    BoxBool success = BoxType_Get_Structure_Member(node, NULL, & offset,
                                                   NULL, & vsi->t_member);
    assert(success);

    v_member =
      Value_Get_Subfield(c, & vsi->v_member, delta_offset, vsi->t_member);
    assert(v_member == & vsi->v_member);
  }
}

void
Compiler::ValueStrucIter_Finish(ValueStrucIter *vsi)
{
  Finish_Value(& vsi->v_member);
}

/**
 * @note Destroy @p src and @p dst, return a new Value.
 */
Value *
Compiler::Emit_Value_Move(Value *v_dst, Value *v_src)
{
  BoxTypeCmp match = BoxType_Compare(v_dst->type, v_src->type);
  if (match == BOXTYPECMP_DIFFERENT) {
    BoxCmp_Log_Err(c, "Cannot move objects of type '%T' into objects of type "
                   "'%T'", v_src->type, v_dst->type);
    Destroy_Value(v_src);
    return v_dst;
  }

  if (match == BOXTYPECMP_MATCHING)
    v_src = Emit_Value_Expansion(v_src, v_dst->type);

  if (v_dst->cont.type == BOXTYPEID_OBJ) {
    /* Object types must be copied and destoyed */

    /* Put addresses in registers. EXAMPLE: o[ro2 + 4] is transformed to ro3
     * through "lea ro3, o[ro2 + 4]"
     */
    v_src = Emit_Reduce_Ptr_Offset(v_src);
    v_dst = Emit_Reduce_Ptr_Offset(v_dst);

    // We try to use the method provided by the user, if possible.
    if (Try_Emit_Conversion(v_dst, v_src))
      return v_dst;

    // OK, we couldn't find a user defined conversion.
    // We leave the copy operation to the Box memory management system.
    BoxTypeId type_id = BoxVM_Install_Type(c->vm, v_src->type);
    Value v_type_id;
    BoxCont ri0;
    Init_Value(& v_type_id);
    Setup_Value_As_Imm_Int(& v_type_id, type_id);
    BoxCont_Set(& ri0, "ri", 0);
    BoxLIR_Append_GOp(c->lir, BOXGOP_TYPEOF,
                      2, & ri0, & v_type_id.cont);
    BoxLIR_Append_GOp(c->lir, BOXGOP_RELOC,
                      3, & v_dst->cont, & v_src->cont, & ri0);
    Finish_Value(& v_type_id);

  } else if (v_dst->cont.type == BOXTYPEID_PTR) {
    /* For pointers we need to pay special care: reference counts! */
    BoxLIR_Append_GOp(c->lir, BOXGOP_REF,
                      2, & v_dst->cont, & v_src->cont);

  } else {
    /* All the other types can be moved "quickly" with a mov operation */
    BoxLIR_Append_GOp(c->lir, BOXGOP_MOV,
                      2, & v_dst->cont, & v_src->cont);
  }

  Destroy_Value(v_src);
  return v_dst;
}

/**
 * @note Destroy @p src and @p dst, return new Value.
 */
Value *
Compiler::Emit_Value_Assignment(Value *v_dst, Value *v_src)
{
  Value *v_new;
  char *var_name;

  assert(v_dst->kind == VALUEKIND_VAR_NAME);

  var_name = Value_Steal_Name(v_dst);
  Finish_Value(v_dst);
  Setup_Value_As_Var(v_dst, v_src->type);
  v_new = Namespace_Add_Value(NMSPFLOOR_DEFAULT, var_name, v_dst);
  Box_Mem_Free(var_name);
  Destroy_Value(v_dst);

  /* We play it safe here: we avoid copying src only when src is a temporary
   * value and is stored in register form.
   */
  if (v_src->kind == VALUEKIND_TEMP &&
      v_src->cont.type == BOXTYPEID_OBJ &&
      v_src->cont.categ == BOXCONTCATEG_LREG) {
    BoxInt reg = v_src->cont.value.reg;
    if (reg > 0) {
      /* We then avoid allocating a dst object and copying src to dst. */
      BoxLIR_Append_GOp(c->lir, BOXGOP_REF, 2, & v_new->cont, & v_src->cont);
      Destroy_Value(v_src);
      return v_new;
    }
  }

  /* Otherwise go for a full copy of the object. */
  Emit_Value_Alloc(v_new);
  return Emit_Value_Move(v_new, v_src);
}

/**
 * Emits the conversion from the source expression 'v', to the given type 't'
 * @note Destroy @p v_src, return new Value.
 */
static Value *
My_Expand_Species(BoxCmp *c, Value *v_src, BoxType *t_dst)
{
  Value *v_dst = c->compiler->Create_Value();
  c->compiler->Setup_Value_As_Temp(v_dst, t_dst);

  if (c->compiler->Try_Emit_Conversion(v_dst, v_src))
    return v_dst;

  if (c->compiler->Emit_Call(v_dst, v_src) == BOXTASK_OK)
    return v_dst;

  BoxCmp_Log_Err(c, "Don't know how to convert objects of type %T to %T.",
                 v_src->type, t_dst);
  c->compiler->Destroy_Value(v_src);
  return v_dst;
}

/**
 * Expands the value 'src' as prescribed by the species 'expansion_type'.
 * @note Destroy @p v_src and return a new Value.
 */
Value *
Compiler::Emit_Value_Expansion(Value *v_src, BoxType *t_dst)
{
  BoxType *t_src = v_src->type;

  if (t_src == t_dst)
    return v_src;

  t_src = BoxType_Resolve(t_src,
    static_cast<BoxTypeResolve>(BOXTYPERESOLVE_IDENT
                                | BOXTYPERESOLVE_SPECIES), 0);
  t_dst = BoxType_Resolve(t_dst, BOXTYPERESOLVE_IDENT, 0);

  if (t_src == t_dst)
    return v_src;

  switch (BoxType_Get_Class(t_dst)) {
  case BOXTYPECLASS_INTRINSIC:
    LOG_FATAL("Type forbidden in species conversions.");
    abort();

  case BOXTYPECLASS_SPECIES:
    {
      BoxType *t_species_memb = BoxType_Get_Species_Target(t_dst);

      if (t_species_memb) {
        BoxTypeCmp match = BoxType_Compare(t_species_memb, t_dst);

        if (match != BOXTYPECMP_DIFFERENT) {
          if (match == BOXTYPECMP_MATCHING)
            v_src = Emit_Value_Expansion(v_src, t_species_memb);
          return My_Expand_Species(c, v_src, t_species_memb);
        }
      }

      LOG_FATAL("type '%T' is not compatible with '%T'.", t_src, t_dst);
      abort();
    }

  case BOXTYPECLASS_STRUCTURE:
    {
      BoxTypeCmp comparison = BoxType_Compare(t_dst, t_src);

      /* We check that the comparison can actually be done */
      if (comparison == BOXTYPECMP_DIFFERENT) {
        LOG_FATAL("Expansion involves incompatible types!");
        abort();
      }

      /* We have to expand the structure: we have to create a new structure
       * which can contain the expanded one.
       */
      if (comparison == BOXTYPECMP_MATCHING) { /* need expansion */
        ValueStrucIter dst_iter, src_iter;
        Value *v_dst = Create_Value();
        Setup_Value_As_Temp(v_dst, t_dst);

        ValueStrucIter_Init(& dst_iter, v_dst);
        ValueStrucIter_Init(& src_iter, v_src);

        for (; dst_iter.has_next && src_iter.has_next;
             ValueStrucIter_Do_Next(& dst_iter),
               ValueStrucIter_Do_Next(& src_iter)) {
          Destroy_Value(Emit_Value_Move(Weak_Copy_Value(& dst_iter.v_member),
                                        Weak_Copy_Value(& src_iter.v_member)));
        }

        assert(dst_iter.has_next == src_iter.has_next);

        Destroy_Value(v_src);
        ValueStrucIter_Finish(& dst_iter);
        ValueStrucIter_Finish(& src_iter);
        return v_dst;
      }

      return v_src;
    }

  case BOXTYPECLASS_ANY:
    /* The code below implements boxing of data. */
    {
      BoxCont ri0, src_type_id_cont;
      BoxTypeId src_type_id = BoxVM_Install_Type(c->vm, v_src->type);
      Value *v_dst = Create_Value();

      /* Set up a container representing the register ri0 and one representing
       * the type integer.
       */
      BoxCont_Set(& ri0, "ri", 0);
      BoxCont_Set(& src_type_id_cont, "ii", (BoxInt) src_type_id);

      /* Create a new ANY type. */
      Setup_Value_As_Temp(v_dst, Box_Get_Core_Type(BOXTYPEID_ANY));

      /* Generate the boxing instructions. */
      if (!BoxType_Is_Empty(v_src->type)) {
        /* The object has associated data: get data pointer in v_src_ptr. */
        Value *v_src_ptr = Weak_Copy_Value(v_src);

        /* If src is an immediate, we move it into a register, so we can use
         * the lea instruction to build a pointer to it. We then keep the
         * register allocated until we have finished with the boxing operation.
         * Note that the box instruction copies objects passed with NULL-block
         * pointers. This means that fast types are always copied.
         */
        if (v_src_ptr->kind == VALUEKIND_IMM)
          Emit_Make_Temp(v_src_ptr, v_src_ptr);

        /* Get a pointer to the object and use it in the boxing operation. */
        v_src_ptr = Value_Cast_To_Ptr_2(v_src_ptr);
        BoxLIR_Append_GOp(c->lir, BOXGOP_TYPEOF,
                          2, & ri0, & src_type_id_cont);
        BoxLIR_Append_GOp(c->lir, BOXGOP_BOX,
                          3, & v_dst->cont, & v_src_ptr->cont, & ri0);
        Destroy_Value(v_src_ptr);

      } else {
        BoxLIR_Append_GOp(c->lir, BOXGOP_TYPEOF,
                          2, & ri0, & src_type_id_cont);
        BoxLIR_Append_GOp(c->lir, BOXGOP_BOX,
                          2, & v_dst->cont, & ri0);
      }

      Destroy_Value(v_src);
      return v_dst;
    }

  default:
    LOG_FATAL("Emit_Value_Expansion not fully implemented!");
    abort();
  }

  return NULL;
}

static void
My_Setup_Parent_Or_Child(BoxCmp *c, Value *v, BoxType *t, int is_parent)
{
  if (!BoxType_Is_Empty(t)) {
    BoxVMCode *p = c->cur_proc;
    BoxVMRegNum ro_num =
      is_parent ? BoxVMCode_Get_Parent_Reg(p) : BoxVMCode_Get_Child_Reg(p);
    ValContainer vc = {VALCONTTYPE_LREG, ro_num, 0};
    c->compiler->Setup_Value_Container(v, Box_Get_Core_Type(BOXTYPEID_PTR),
                                       & vc);
    v = c->compiler->Emit_Value_Cast(v, t);
    v->kind = VALUEKIND_TARGET;
  } else {
    c->compiler->Setup_Value_As_Temp(v, t);
    v->kind = VALUEKIND_TARGET;
  }
}

void
Compiler::Setup_Value_As_Parent(Value *v, BoxType *parent_t)
{
  return My_Setup_Parent_Or_Child(c, v, parent_t, /* is_parent */ 1);
}

void
Compiler::Setup_Value_As_Child(Value *v, BoxType *child_t)
{
  return My_Setup_Parent_Or_Child(c, v, child_t, /* is_parent */ 0);
}

static Value *
My_Get_Ptr_To_New_Value(BoxCmp *c, BoxType *t)
{
  if (BoxType_Is_Fast(t)) {
    /* Create a structure type containing just one item of type t, allocate
     * that and get a pointer to it.
     * NOTE: this can be improved. We should not keep generating new
     *  structures each time the function is called, but rather cache them!
     */
    Value *v = c->compiler->Create_Value();
    BoxType *t_struc = BoxType_Create_Structure();
    BoxType_Add_Member_To_Structure(t_struc, t, NULL);
    c->compiler->Setup_Value_As_Temp(v, t_struc);
    return c->compiler->Emit_Cast_To_Ptr(v);
  } else {
    Value *v = c->compiler->Create_Value();
    c->compiler->Setup_Value_As_Temp(v, t);
    return c->compiler->Emit_Cast_To_Ptr(v);
  }
}

/**
 * @note Destroy @p v_parent and return a new Value.
 */
Value *
Compiler::Emit_Subtype_Build(Value *v_parent, const char *subtype_name)
{
  BoxType *found_subtype;
  Value *v_subtype = NULL;

  /* If the method cannot be found, it could be a method of the child
   * of the type. We then resolve the type to its child and try again.
   * X.GetPoint = Point
   * X.GetPoint[].Norm[] (Norm is a method of Point and not a method
   *                       of GetPoint)
   */
  while (1) {
    found_subtype = BoxType_Find_Subtype(v_parent->type, subtype_name);
    if (found_subtype)
      break;

    if (BoxType_Is_Subtype(v_parent->type)) {
      v_parent = Emit_Subtype_Expansion(v_parent);
      if (!v_parent)
        return NULL;
    } else {
      BoxCmp_Log_Err(c, "Type '%T' has not a subtype of name '%s'",
                     v_parent->type, subtype_name);
      Destroy_Value(v_parent);
      return NULL;
    }
  }

  assert(found_subtype);

  /* First, we create the subtype object (a pair of pointers) */
  v_subtype = Create_Value();
  Setup_Value_As_Temp(v_subtype, found_subtype);

  BoxType *t_child;
  BoxType_Get_Subtype_Info(found_subtype, NULL, NULL, & t_child);
  if (!BoxType_Is_Empty(t_child)) {
    /* Next, we create the child and get a pointer to it */
    Value *v_ptr, *v_subtype_child;
    v_subtype_child = My_Get_Ptr_To_New_Value(c, t_child);

    /* We now create a Value corresponding to the first pointer (the child)
     * and transfer the child pointer to the subtype.
     */
    v_ptr = Weak_Copy_Value(v_subtype);
    v_ptr = Value_Get_Subfield(c, v_ptr, /* offset */ 0,
                               Box_Get_Core_Type(BOXTYPEID_PTR));
    Destroy_Value(Emit_Value_Move(v_ptr, v_subtype_child));
  }

  /* We now create the value for the parent Pointer in the subtype */
  if (!BoxType_Is_Empty(v_parent->type)) {
    Value *v_ptr = Weak_Copy_Value(v_subtype);
    v_ptr = Value_Get_Subfield(c, v_ptr, /*offset*/ sizeof(BoxPtr),
                               Box_Get_Core_Type(BOXTYPEID_PTR));
    Value *v_subtype_parent = Weak_Copy_Value(v_parent);
    v_subtype_parent = Emit_Cast_To_Ptr(v_subtype_parent);
    Destroy_Value(Emit_Value_Move(v_ptr, v_subtype_parent));
  }

  Destroy_Value(v_parent);

  return v_subtype;
}

/**
 * @brief Destroy @p v_subtype, return a new Value.
 */
static Value *
My_Value_Subtype_Get(BoxCmp *c, Value *v_subtype, bool get_child)
{
  Value *v_ret = NULL;

  if (c->compiler->Want_Instance(v_subtype)) {
    if (BoxType_Is_Subtype(v_subtype->type)) {
      BoxType *t_parent, *t_child;
      BoxBool success = BoxType_Get_Subtype_Info(v_subtype->type, NULL,
                                                 & t_parent, & t_child);
      assert(success);
      BoxType *t_ret = get_child ? t_child : t_parent;
      if (BoxType_Is_Empty(t_ret)) {
        v_ret = c->compiler->Create_Value();
        c->compiler->Setup_Value_As_Temp(v_ret, t_ret);
      } else {
        size_t offset = get_child ? 0 : sizeof(BoxPtr);
        v_ret = c->compiler->Weak_Copy_Value(v_subtype);
        v_ret = Value_Get_Subfield(c, v_ret, offset,
                                   Box_Get_Core_Type(BOXTYPEID_PTR));
        v_ret = c->compiler->Emit_Value_Cast(v_ret, t_ret);
        v_ret->attr.own_register = v_subtype->attr.own_register;
        v_subtype->attr.own_register = 0;
      }
    } else {
      const char *what = get_child ? "child" : "parent";
      BoxCmp_Log_Err(c, "Cannot get the %s of '%T': this is not a subtype!",
                     what, v_subtype->type);
    }
  }

  c->compiler->Destroy_Value(v_subtype);
  return v_ret;
}

Value *
Compiler::Emit_Get_Subtype_Parent(Value *v_subtype)
{
  return My_Value_Subtype_Get(c, v_subtype, /*get_child*/false);
}

/**
 * @brief Destroy @p v_subtype, return a new Value.
 */
Value *
Compiler::Emit_Get_Subtype_Child(Value *v_subtype)
{
  return My_Value_Subtype_Get(c, v_subtype, /*get_child*/true);
}

/**
 * @note Destroy @p v_src, return new Value.
 */
Value *
Compiler::Emit_Subtype_Expansion(Value *v_src)
{
  if (Value_Is_Value(v_src)) {
    if (BoxType_Is_Subtype(v_src->type)) {
      bool subtype_was_target = (v_src->kind == VALUEKIND_TARGET);
      v_src = Emit_Get_Subtype_Child(v_src);
      if (subtype_was_target)
        v_src = Value_Promote_Temp_To_Target(v_src);
      return v_src;
    }
  }

  return v_src;
}

Value *
Compiler::Emit_Raise_Instance(Value *v)
{
  if (Value_Is_Value(v)) {
    BoxType *t = BoxType_Resolve(v->type, BOXTYPERESOLVE_IDENT, 0);
    BoxType *unraised_type = BoxType_Unraise(t);
    if (unraised_type) {
      (void) BoxType_Unlink(v->type);
      v->type = unraised_type;
      return v;
    } else {
      LOG_ERR("Raising operator is applied to a non-raised type.");
      Destroy_Value(v);
      return NULL;
    }
  } else {
    LOG_ERR("Raising operator got invalid operand.");
    Destroy_Value(v);
    return NULL;
  }
}

Value *
Compiler::Emit_Reference_Instance(Value *v)
{
  if (!Value_Is_Value(v)) {
    LOG_ERR("Invalid operand to reference operator.");
    Destroy_Value(v);
    return NULL;
  }

  v = Value_Cast_To_Ptr_2(v);
  v->type = BoxType_Create_Pointer(v->type);
  v->cont.type = BOXTYPEID_PTR;
  v->kind = VALUEKIND_TEMP;
  return v;
}

Value *
Compiler::Emit_Dereference_Instance(Value *v)
{
  BoxType *deref_type;

  if (!Value_Is_Value(v)) {
    LOG_ERR("Invalid operand to reference operator.");
    return NULL;
  }

  deref_type = BoxType_Dereference_Pointer(v->type);
  if (!deref_type) {
    LOG_ERR("Cannot dereference objects of type  `%T'", v->type);
    return NULL;
  }

  v = Emit_Value_Cast(v, deref_type);
  v->kind = VALUEKIND_TARGET;

  /* Check for NULL pointers. */
  BoxLIR_Append_GOp(c->lir, BOXGOP_NOTNUL, 1, & v->cont, & v->cont);
  return v;
}

}
