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
#include "compiler.h"
#include "value.h"
#include "autogen.h"
#include "vmsymstuff.h"
#include "tsdesc.h"
#include "combs.h"

/* FIXME: there is a flaw in the design of the Value datastructure.
 *  We keep track of references to Value objects, but we shouldn't do this.
 *  We should rather keep track of resources used by Value objects.
 *  For example, we should keep track of registers used by a Value object
 *  (i.e. keep a reference counter for registers, so that we know how many
 *  Value-s are referring to a given register). I think this should simplify
 *  considerably the compiler code and should make the code cleaner.
 *  The notion of weak copy of a Value should then become unnecessary.
 *  Moreover, Value will become much simpler to use: one should need just to
 *  use them as ordinary objects (remember to call _Init and _Finish methods
 *  properly), rather than having to define how many reference counts every
 *  function consumes, etc...
 *  We should redesign registers.c and value.c with this in mind and also
 *  paying attention to make sure we are not initialising and destroying
 *  continuously BoxArr objects for every couple of [ ] the compiler
 *  encounters.
 */
void Value_Init(Value *v, CmpProc *proc) {
  v->proc = proc;
  v->kind = VALUEKIND_ERR;
  v->type = NULL;
  v->name = NULL;
  v->attr.new_or_init = 0;
  v->attr.own_register = 0;
  v->attr.ignore = 0;
  v->num_ref = 1;
}

Value *Value_New(CmpProc *proc) {
  Value *v = BoxMem_Safe_Alloc(sizeof(Value));
  Value_Init(v, proc);
  v->attr.new_or_init = 1;
  return v;
}

static void My_Value_Finalize(Value *v) {
  if (v->name != NULL)
    BoxMem_Free(v->name);

  switch(v->kind) {
  case VALUEKIND_ERR:
  case VALUEKIND_TYPE_NAME:
  case VALUEKIND_VAR_NAME:
  case VALUEKIND_TYPE:
  case VALUEKIND_IMM:
    return;

  case VALUEKIND_TARGET:
  case VALUEKIND_TEMP:
    switch (v->value.cont.categ) {
    case BOXCONTCATEG_LREG:
      if (v->attr.own_register) {
        if (v->value.cont.value.reg >= 0)
          Reg_Release(& v->proc->reg_alloc,
                      v->value.cont.type, v->value.cont.value.reg);
      }
      return;

    case BOXCONTCATEG_GREG:
      return;

    case BOXCONTCATEG_PTR:
      if (v->attr.own_register) {
        assert(!v->value.cont.value.ptr.is_greg);
        Reg_Release(& v->proc->reg_alloc,
                    BOXTYPE_OBJ, v->value.cont.value.ptr.reg);
      }
      return;

    default:
      MSG_WARNING("My_Value_Finalize: Destruction not implemented!");
      return;
    }
  }
}

void Value_Unlink(Value *v) {
  if (v != NULL) {
    if (v->num_ref > 1)
      --v->num_ref;

    else {
      assert(v->num_ref == 1);
      My_Value_Finalize(v);
      v->num_ref = 0;
      if (v->attr.new_or_init)
        BoxMem_Free(v);
    }
  }
}

void Value_Link(Value *v) {
  v->num_ref += 1;
}

/* Determine if the given value can be recycled, otherwise return Value_New()
 * REFERENCES: return: new, v: -1;
 */
Value *Value_Recycle(Value *v) {
  CmpProc *proc = v->proc->cmp->cur_proc; /* Be careful! We want to operate on
                                             the cur_proc, not the proc of the
                                             old value!!! */
  if (v->num_ref == 1) {
    /* one reference means nobody else owns the object, so we can do
     * whatever we want with it!
     */
    int new_or_init = v->attr.new_or_init;
    Value_Init(v, proc);
    v->attr.new_or_init = new_or_init;
    Value_Link(v); /* XXX this seems to be wrong to me */
    return v;

  } else
    return Value_New(proc);
}

const char *ValueKind_To_Str(ValueKind vk) {
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

int Value_Want(Value *v, int num_wanted, ValueKind *wanted) {
  char *wanted_str = NULL;
  int i;

  for(i = 0; i < num_wanted; i++)
    if (v->kind == wanted[i]) return 1;

  for(i = 0; i < num_wanted; i++) {
    if (i == 0)
      wanted_str = printdup("%s", ValueKind_To_Str(wanted[i]));
    else {
      char *sep = (i < num_wanted - 1) ? ", " : " or ";
      wanted_str = printdup("%~s%s%s", wanted_str, sep,
                            ValueKind_To_Str(wanted[i]));
    }
  }

  MSG_ERROR("Expected %~s, but got %s.",
            wanted_str, ValueKind_To_Str(v->kind));
  return 0;
}

int Value_Want_Value(Value *v) {
  if (Value_Is_Value(v))
    return 1;

  else if (Value_Is_Err(v)) {
    return 0;

  } else {
    if (v->name != NULL) {
      MSG_ERROR("'%s' is undefined: an expression with both value and type is"
                " expected here.", v->name);
    } else {
      MSG_ERROR("Got '%s', but an expression with both value and type is "
                "expected here.", ValueKind_To_Str(v->kind));
    }
    return 0;
  }
}

int Value_Want_Has_Type(Value *v) {
  if (Value_Has_Type(v))
    return 1;

  else if (Value_Is_Err(v)) {
    return 0;

  } else {
    if (v->name != NULL) {
      MSG_ERROR("'%s' is undefined: an expression with definite type is "
                "expected here.", v->name);
    } else {
      MSG_ERROR("Got '%s', but an expression with definite type is "
                "expected here.", ValueKind_To_Str(v->kind));
    }
    return 0;
  }
}

/** Whether we can change the content of the register associated with v
 * (if any).
 */
static int My_Value_Can_Reuse_Reg(Value *v) {
  return v->num_ref == 1 && v->attr.own_register;
}

void Value_Setup_As_Weak_Copy(Value *v_copy, Value *v) {
  v_copy->proc = v->proc;
  v_copy->kind = v->kind;
  v_copy->type = BoxType_Link(v->type);
  v_copy->value.cont = v->value.cont;
  v_copy->name = (v->name == NULL) ? NULL : BoxMem_Strdup(v->name);
  v_copy->attr.own_register = 0;
  v_copy->attr.ignore = 0;
}

void Value_Setup_As_Var_Name(Value *v, const char *name) {
  v->kind = VALUEKIND_VAR_NAME;
  v->name = BoxMem_Strdup(name);
}

void Value_Setup_As_Type_Name(Value *v, const char *name) {
  v->kind = VALUEKIND_TYPE_NAME;
  v->name = BoxMem_Strdup(name);
}

void Value_Setup_As_Type(Value *v, BoxTypeId t) {
  v->kind = VALUEKIND_TYPE;
  v->type = BoxType_Link(BoxType_From_Id(& v->proc->cmp->ts, t));
  v->value.cont.type = BoxType_Get_Cont_Type(v->type);
}

void Value_Setup_As_Imm_Char(Value *v, Char c) {
  v->kind = VALUEKIND_IMM;
  v->type = BoxType_Link(BoxType_From_Id(& v->proc->cmp->ts,BOXTYPE_CHAR));
  BoxCont_Set(& v->value.cont, "ic", c);
}

void Value_Setup_As_Imm_Int(Value *v, Int i) {
  v->kind = VALUEKIND_IMM;
  v->type = BoxType_Link(BoxType_From_Id(& v->proc->cmp->ts,
                                         v->proc->cmp->bltin.species_int));
  BoxCont_Set(& v->value.cont, "ii", i);
}

void Value_Setup_As_Imm_Real(Value *v, Real r) {
  v->kind = VALUEKIND_IMM;
  v->type = BoxType_Link(BoxType_From_Id(& v->proc->cmp->ts,
                                         v->proc->cmp->bltin.species_real));
  BoxCont_Set(& v->value.cont, "ir", r);
}

void Value_Setup_As_Void(Value *v) {
  v->kind = VALUEKIND_IMM;
  v->type = BoxType_Link(BoxType_From_Id(& v->proc->cmp->ts, BOXTYPE_VOID));
  v->value.cont.type = BOXCONTTYPE_VOID;
}

void Value_Setup_As_Temp(Value *v, BoxXXXX *t) {
  ValContainer vc = {VALCONTTYPE_LREG, -1, 0};
  Value_Setup_Container(v, t, & vc);
  Value_Emit_Allocate(v);
}

void Value_Setup_As_Temp_Old(Value *v, BoxTypeId t) {
  ValContainer vc = {VALCONTTYPE_LREG, -1, 0};
  Value_Setup_Container_Old(v, t, & vc);
  Value_Emit_Allocate(v);
}

void Value_Setup_As_Var(Value *v, BoxXXXX *t) {
  BoxCmp *c = v->proc->cmp;
  CmpProc *p = c->cur_proc;
  if (CmpProc_Get_Style(p) == CMPPROCSTYLE_MAIN) {
    /* We are creating the main procedure so variables should get into global
     * registers
     */
    ValContainer vc = {VALCONTTYPE_GVAR, -1, 0};
    Value_Setup_Container(v, t, & vc);

  } else {
    /* We are not in the main: variables should go into local registers */
    ValContainer vc = {VALCONTTYPE_LVAR, -1, 0};
    Value_Setup_Container(v, t, & vc);
  }

  Value_Emit_Allocate(v);
}

void Value_Setup_As_String(Value *v_str, const char *str) {
  BoxTask success;
  BoxCmp *c = v_str->proc->cmp;
  size_t l, addr;
  Value v_str_data;
  ValContainer vc = {VALCONTTYPE_GPTR, 0, 0};

  l = strlen(str) + 1;
  addr = BoxVM_Data_Add(c->vm, str, l, BOXTYPE_OBJ);
  assert(addr >= 0);

  vc.addr = addr;
  Value_Init(& v_str_data, v_str->proc);
  Value_Setup_Container_Old(& v_str_data, BOXTYPE_OBJ, & vc);

  Value_Setup_As_Temp_Old(v_str, c->bltin.string);

  Value_Unlink(Value_Emit_Call(v_str, & v_str_data, & success));
  if (success != BOXTASK_OK) {
    MSG_FATAL("Value_Setup_As_String: Failure while emitting string.");
    assert(0);
  }
}

/* Create a new empty container. */
void Value_Setup_Container_Old(Value *v, BoxTypeId type, ValContainer *vc) {
  RegAlloc *ra = & v->proc->reg_alloc;
  int use_greg;

  v->type = BoxType_Link(BoxType_From_Id(& v->proc->cmp->ts, type));
  v->value.cont.type = BoxType_Get_Cont_Type(v->type);

  switch(vc->type_of_container) {
  case VALCONTTYPE_IMM:
    v->kind = VALUEKIND_IMM;
    v->value.cont.categ = BOXCONTCATEG_IMM;
    return; break;

  case VALCONTTYPE_LREG:
    v->kind = VALUEKIND_TEMP;
    v->value.cont.categ = BOXCONTCATEG_LREG;
    if (vc->which_one < 0) {
      /* Automatically chooses the local register.
         NOTE: if cont.type == BOXTYPE_VOID this function returns 0, meaning
           that for this type there is no need for registers. */
      Int reg = Reg_Occupy(ra, v->value.cont.type);
      assert(reg >= 0);
      v->value.cont.value.reg = reg;
      v->attr.own_register = (reg > 0);
      return;

    } else {
      /* The user wants a particular register to be chosen */
      v->value.cont.value.reg = vc->which_one;
      return;
    }
    break;

  case VALCONTTYPE_GVAR:
    v->kind = VALUEKIND_TARGET;
    v->value.cont.categ = BOXCONTCATEG_GREG;
    if (vc->which_one < 0) {
      Int reg = -GVar_Occupy(ra, v->value.cont.type);
      /* Automatically choses the local variables */
      assert(reg <= 0);
      v->value.cont.value.reg = reg;
      return;

    } else {
      /* The user wants a particolar variable to be chosen */
      v->value.cont.value.reg = vc->which_one;
      return;
    }
    break;

  case VALCONTTYPE_LVAR:
    v->kind = VALUEKIND_TARGET;
    v->value.cont.categ = BOXCONTCATEG_LREG;
    if (vc->which_one < 0) {
      /* Automatically choses the local variables */
      Int reg = -Var_Occupy(ra, v->value.cont.type, 0);
      assert(reg <= 0);
      v->value.cont.value.reg = reg;
      return;

    } else {
      /* The user wants a particolar variable to be chosen */
      v->value.cont.value.reg = vc->which_one;
      return;
    }
    break;

  case VALCONTTYPE_GREG:
    v->value.cont.categ = BOXCONTCATEG_GREG;
    v->value.cont.value.reg = vc->which_one;
    return;
    break;

  case VALCONTTYPE_GPTR:
  case VALCONTTYPE_LRPTR:
  case VALCONTTYPE_LVPTR:
    use_greg = (vc->type_of_container == VALCONTTYPE_GPTR);
    v->kind = VALUEKIND_TARGET;
    v->value.cont.categ = BOXCONTCATEG_PTR;
    v->value.cont.value.ptr.is_greg = use_greg;
    v->value.cont.value.ptr.reg = vc->which_one;
    v->value.cont.value.ptr.offset = vc->addr;
    if (use_greg || vc->addr >= 0) return;

    if (vc->type_of_container == VALCONTTYPE_LRPTR) {
      Int reg = Reg_Occupy(ra, BOXTYPE_OBJ);
      v->value.cont.value.ptr.reg = reg;
      assert(reg >= 1);
      return;

    } else {
      Int reg = -Var_Occupy(ra, BOXTYPE_OBJ, 0);
      v->value.cont.value.ptr.reg = reg;
      assert(reg < 0);
    }

    return;
    break;

  default:
    MSG_FATAL("Value_Setup_Container: wrong type of container!");
    assert(0);
  }
}

void Value_Setup_Container(Value *v, BoxXXXX *t, ValContainer *vc) {
  Value_Setup_Container_Old(v, BoxType_Get_Id(t), vc);
}

void Value_Emit_Allocate(Value *v) {
  switch(v->kind) {
  case VALUEKIND_ERR:
    return;
  case VALUEKIND_TEMP:
  case VALUEKIND_TARGET:
    if (v->value.cont.type == BOXCONTTYPE_OBJ) {
      BoxCmp *c = v->proc->cmp;
      CmpProc *proc = c->cur_proc;
      BoxVMAllocID alloc_id;

      assert(v->proc == proc);

      /* Get the alloc ID for the type */
      alloc_id = TS_Get_AllocID(c, v->type);

      if (alloc_id == BOXVMALLOCID_NONE) {
        Value v_size;

        Value_Init(& v_size, proc);
        Value_Setup_As_Imm_Int(& v_size, BoxType_Get_Size(v->type));
        CmpProc_Assemble(proc, BOXGOP_MALLOC,
                         2, & v->value.cont, & v_size.value.cont);

      } else {
        /* The 'create' instruction automatically invokes the creator
         * when necessary
         */
        Value v_alloc_id;
        Value_Init(& v_alloc_id, proc);
        Value_Setup_As_Imm_Int(& v_alloc_id, alloc_id);
        CmpProc_Assemble(proc, BOXGOP_CREATE,
                         2, & v->value.cont, & v_alloc_id.value.cont);
      }
    }
    return;

  default:
    MSG_FATAL("Value_Emit_Allocate: invalid argument (%s).",
              ValueKind_To_Str(v->kind));
    assert(0);
  }
}

/* Doesn't unlink v, for coherence with Value_Emit_Unlink. */
void Value_Emit_Link(Value *v) {
  BoxTypeId cont_type = v->value.cont.type;
  if (cont_type == BOXTYPE_OBJ || cont_type == BOXTYPE_PTR) {
    assert(v->value.cont.categ == BOXCONTCATEG_LREG
           || v->value.cont.categ == BOXCONTCATEG_GREG);
    /* ^^^ for now we don't support the general case... */
    CmpProc_Assemble(v->proc, BOXGOP_MLN, 1, & v->value.cont);
  }
}

/* Doesn't unlink v, since the function is called by Value_Unlink in the
 * finalisation.
 */
void Value_Emit_Unlink(Value *v) {
  BoxTypeId cont_type = v->value.cont.type;
  if (cont_type == BOXTYPE_OBJ || cont_type == BOXTYPE_PTR) {
    assert(v->value.cont.categ == BOXCONTCATEG_LREG
           || v->value.cont.categ == BOXCONTCATEG_GREG);
    /* ^^^ for now we don't support the general case... */
    CmpProc_Assemble(v->proc, BOXGOP_MUNLN, 1, & v->value.cont);
  }
}

/* REFERENCES: v: -1 */
void Value_Emit_CJump(Value *v, BoxVMSymID jump_label) {
  BoxCmp *c = v->proc->cmp;
  CmpProc_Assemble_CJump(c->cur_proc, jump_label, & v->value.cont);
  Value_Unlink(v);
}

/* REFERENCES: return: new, v_ptr: -1; */
Value *Value_To_Temp(Value *v) {
  ValContainer vc = {VALCONTTYPE_LREG, -1, 0};
  BoxCmp *c = v->proc->cmp;

  switch (v->kind) {
  case VALUEKIND_ERR:
  case VALUEKIND_TEMP:
    Value_Link(v);
    return v;

  case VALUEKIND_VAR_NAME:
  case VALUEKIND_TYPE_NAME:
    MSG_ERROR("Got %s ('%s'), but a defined type or value is expected here!",
              ValueKind_To_Str(v->kind), v->name);
    return Value_Recycle(v); /* Return an error value (Value_Recycle
                                leaves v with an error, if not initialised
                                with a Value_Setup_* function) */

  case VALUEKIND_TYPE:
    {
      BoxXXXX *t = BoxType_Link(v->type);
      v = Value_Recycle(v);
      Value_Setup_Container(v, t, & vc);
      (void) BoxType_Unlink(t);
      Value_Emit_Allocate(v);
      return v;
    }

  case VALUEKIND_IMM:
  case VALUEKIND_TARGET:
    {
      BoxXXXX *t = BoxType_Link(v->type);
      BoxCont cont = v->value.cont;
      v = Value_Recycle(v);
      Value_Setup_Container(v, t, & vc);
      BoxType_Unlink(t);
      CmpProc_Assemble(c->cur_proc, BOXGOP_MOV,
                       2, & v->value.cont, & cont);
      return v;
    }
  }

  assert(0);
  return NULL;
}

Value *Value_To_Temp_Or_Target(Value *v) {
  if (v->kind == VALUEKIND_TARGET) {
    Value_Link(v);
    return v;

  } else
    return Value_To_Temp(v);
}

Value *Value_Promote_Temp_To_Target(Value *v) {
  if (v->kind == VALUEKIND_TEMP)
    v->kind = VALUEKIND_TARGET;
  return v;
}

void Value_Set_Ignorable(Value *v, int ignorable) {
  v->attr.ignore = ignorable;
}

int Value_Is_Err(Value *v) {
  return (v->kind == VALUEKIND_ERR);
}

int Value_Is_Temp(Value *v) {
  return (v->kind == VALUEKIND_TEMP);
}

int Value_Is_Var_Name(Value *v) {
  return (v->kind == VALUEKIND_VAR_NAME);
}

int Value_Is_Type_Name(Value *v) {
  return (v->kind == VALUEKIND_TYPE_NAME);
}

int Value_Is_Target(Value *v) {
  return (v->kind == VALUEKIND_TARGET);
}

int Value_Is_Value(Value *v) {
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
    return (BoxType_Compare(BoxType_From_Id(& v->proc->cmp->ts, BOXTYPE_VOID),
                            v->type)
            != BOXTYPECMP_DIFFERENT);

  else
    return 0;
}

int Value_Has_Type(Value *v) {
  switch(v->kind) {
  case VALUEKIND_TYPE_NAME:
  case VALUEKIND_VAR_NAME:
  case VALUEKIND_ERR:
    return 0;
  default:
    return 1;
  }
}

/* REFERENCES: parent: 0, child: 0; */
void Value_Emit_Call_From_SymID(BoxVMSymID sym_id,
                                Value *parent, Value *child) {
  BoxCmp *c = parent->proc->cmp;

  assert(parent != NULL && child != NULL);
  assert(c == child->proc->cmp);

  if (parent->value.cont.type != BOXCONTTYPE_VOID) {
    BoxGOp op = (parent->value.cont.type == BOXCONTTYPE_OBJ
                 && parent->value.cont.categ != BOXCONTCATEG_PTR) ?
                BOXGOP_MOV : BOXGOP_LEA;
    CmpProc_Assemble(c->cur_proc, op,
                     2, & c->cont.pass_parent, & parent->value.cont);
  }

  if (child->value.cont.type != BOXCONTTYPE_VOID) {
    Value *v_to_pass = Value_To_Temp_Or_Target(child);
    BoxGOp op = (child->value.cont.type == BOXCONTTYPE_OBJ
                 && child->value.cont.categ != BOXCONTCATEG_PTR) ?
                BOXGOP_REF : BOXGOP_LEA;
    CmpProc_Assemble(c->cur_proc, op,
                     2, & c->cont.pass_child, & v_to_pass->value.cont);
    Value_Unlink(v_to_pass);
  }

  CmpProc_Assemble_Call(c->cur_proc, sym_id);
}

/* REFERENCES: return: new, parent: 0, child: -1; */
static Value *My_Emit_Call(Value *parent, Value *child, BoxTask *success) {
  BoxCmp *c = parent->proc->cmp;
  TS *ts = & c->ts;
  BoxTypeId found_procedure, expansion_for_child;
  BoxVMSymID sym_id;
  BoxTask dummy;

  assert(parent != NULL && child != NULL);

  success = (success != NULL) ? success : & dummy;

  if (Value_Is_Err(parent) || Value_Is_Err(child)) {
    /* In case of error silently exits. */
    Value_Unlink(child);
    *success = BOXTASK_OK;
    return (Value *) NULL;
  }

  assert(c == child->proc->cmp);

  /* We expand the child, since things like X.Y@Z are not allowed: in other
   * words, subtypes can never be children of any type.
   */
  child = Value_Expand_Subtype(child);

  /* Types derived from Void are always ignored */
  if (BoxType_Compare(child->type, BoxType_From_Id(ts, BOXTYPE_VOID))) {
    Value_Unlink(child);
    *success = BOXTASK_OK;
    return NULL;
  }

  /* Now we search for the procedure associated with *child */
  BoxXXXX *child_new = child->type;
  BoxXXXX *parent_new = parent->type;
  BoxTypeCmp expand;
  BoxXXXX *found_combination = 
    BoxType_Find_Combination(parent_new, BOXCOMBTYPE_AT, child_new, & expand);

  found_procedure =
    BoxTS_Procedure_Search(ts, & expansion_for_child,
                           BoxType_Get_Id(child->type), BOXCOMBTYPE_AT,
                           BoxType_Get_Id(parent->type),
                           TSSEARCHMODE_INHERITED);

  assert((found_procedure != BOXTYPE_NONE) == (found_combination != NULL));

  /* If the procedure is not there we try to auto-generate it */
  if (!found_combination) {
    /* Note that we never generate X@Y where X requires expansion.
     * Therefore, it is safe to set expansion_for_child to BOXTYPE_NONE.
     */
    expansion_for_child = BOXTYPE_NONE;

    found_procedure =
      Auto_Generate_Procedure(c, BoxType_Get_Id(child->type),
                              BoxType_Get_Id(parent->type));
    if (found_procedure == BOXTYPE_NONE) {
      *success = BOXTASK_FAILURE;
      return child; /* return child as it may be processed further */
    }
  }

  if (expansion_for_child != BOXTYPE_NONE) {
    child = Value_Expand(child, expansion_for_child);
    if (child == NULL) {
      *success = BOXTASK_ERROR;
      return (Value *) NULL; /* ERROR: Value_Expand does unlink child */
    }
  }

#if 0
  BoxCallable cb;
  if (BoxType_Get_Combination_Info(found_combination, NULL, & cb)) {
    if (BoxCallable_Get_VM_CallNum(cb, BoxVM *vm, BoxVMCallNum *cn)) {

    }
  }
#endif

  sym_id = TS_Procedure_Get_Sym(ts, found_procedure);
  Value_Emit_Call_From_SymID(sym_id, parent, child);

  *success = BOXTASK_OK;
  Value_Unlink(child);
  return (Value *) NULL;
}

Value *Value_Emit_Call(Value *parent, Value *child, BoxTask *success) {
  return My_Emit_Call(parent, child, success);
}

BoxTask Value_Emit_Call_Or_Blacklist(Value *parent, Value *child) {
  BoxTask t;
  Value_Unlink(My_Emit_Call(parent, child, & t));
  return t;
}

/*
 * REFERENCES: return: new, v_ptr: -1;
 */
Value *Value_Cast_From_Ptr(Value *v_ptr, BoxTypeId new_type) {
  BoxCmp *c = v_ptr->proc->cmp;

  assert(v_ptr->value.cont.type == BOXCONTTYPE_PTR);

  if (v_ptr->num_ref == 1) {
    BoxCont *cont = & v_ptr->value.cont;
    BoxTypeId new_cont_type = TS_Get_Cont_Type(& c->ts, new_type);

    switch(cont->categ) {
    case BOXCONTCATEG_GREG:
    case BOXCONTCATEG_LREG:
      v_ptr->type = BoxType_Link(BoxType_From_Id(& c->ts, new_type));
      cont->type = new_cont_type;
      if (new_cont_type == BOXTYPE_OBJ || new_cont_type == BOXTYPE_PTR)
        return v_ptr;

      else {
        int is_greg = (cont->categ == BOXCONTCATEG_GREG);
        BoxInt reg = cont->value.reg;
        cont->categ = BOXCONTCATEG_PTR;
        cont->value.ptr.reg = reg;
        cont->value.ptr.is_greg = is_greg;
        cont->value.ptr.offset = 0;
      }
      return v_ptr;

    case BOXCONTCATEG_PTR:
      if (My_Value_Can_Reuse_Reg(v_ptr)) {
        MSG_FATAL("Value_Cast_From_Ptr: cannot reuse register, yet!");
        /* not implemented */

      } else {
        BoxCont v_ptr_cont = v_ptr->value.cont;
        Value_Unlink(v_ptr);
        v_ptr = Value_New(c->cur_proc);
        Value_Setup_As_Temp_Old(v_ptr, BOXTYPE_PTR);
        CmpProc_Assemble(c->cur_proc, BOXGOP_REF, 2,
                         & v_ptr->value.cont, & v_ptr_cont);
        assert(v_ptr->value.cont.categ == BOXCONTCATEG_LREG);
        return Value_Cast_From_Ptr(v_ptr, new_type);
      }

    default:
      MSG_FATAL("Value_Cast_From_Ptr: unexpected container category!");
      assert(0);
    }

  } else {
    MSG_FATAL("Value_Cast_From_Ptr: not implemented, yet!");
    assert(0);
  }

  assert(0);
  return NULL;
}

Value *Value_Cast_To_Ptr(Value *v) {
  BoxCmp *c = v->proc->cmp;

  if (v->value.cont.type == BOXCONTTYPE_OBJ
      && v->value.cont.categ != BOXCONTCATEG_PTR) {
    /* This is the case where we already have the pointer to the object
     * stored inside a register. We then have two cases:
     *  - such register is already in use. we cannot just change the
     *    value into a Ptr value. We rather have to move the pointer
     *    to a new register.
     *  - the register is fully owned by us, we can recycle it!
     */
    if (v->num_ref > 1) {
      MSG_FATAL("Value_Cast_To_Ptr: not implemented, yet!");
      return v;

    } else {
      assert(v->num_ref == 1);
      assert(v->value.cont.categ == BOXCONTCATEG_LREG
             || v->value.cont.categ == BOXCONTCATEG_GREG);
      /* We own the sole reference to v, which is a temporary quantity:
       * in other words we can do whathever we want with it!
       */
      v->type = BoxType_Link(BoxType_From_Id(& v->proc->cmp->ts, BOXTYPE_PTR));
      v->value.cont.type = BOXCONTTYPE_PTR;
      return v;
    }

  } else {
    /* We have to get the pointer with a lea instruction. */
    BoxCont v_cont = v->value.cont;
    Value_Unlink(v);
    v = Value_New(c->cur_proc);
    Value_Setup_As_Temp_Old(v, BOXTYPE_PTR);
    CmpProc_Assemble(c->cur_proc, BOXGOP_LEA, 2, & v->value.cont, & v_cont);
    return v;
  }
}

/*
 * REFERENCES: return: new, v_obj: -1;
 */
Value *Value_To_Straight_Ptr(Value *v_obj) {
  assert(v_obj->value.cont.type == BOXCONTTYPE_OBJ);

  if (v_obj->value.cont.categ == BOXCONTCATEG_PTR) {
    ValContainer vc = {VALCONTTYPE_LREG, -1, 0};
    Value *v_ret;
    BoxCont cont = v_obj->value.cont;
    BoxXXXX *t = BoxType_Link(v_obj->type);
    CmpProc *cur_proc = v_obj->proc->cmp->cur_proc;

    Value_Unlink(v_obj);
    v_ret = Value_New(cur_proc);
    Value_Setup_Container(v_ret, t, & vc);
    BoxType_Unlink(t);

    assert(v_ret->value.cont.type == BOXCONTTYPE_OBJ);
    CmpProc_Assemble(v_ret->proc, BOXGOP_LEA, 2, & v_ret->value.cont, & cont);
    return v_ret;

  } else
    return v_obj;
}

/** Return a sub-field of an object type. 'offset' is the address of the
 * subfield with respect to the address of the given object 'v_obj',
 * 'subf_type' is the type of the sub-field.
 * REFERENCES: return: new, v_obj: -1;
 */
Value *Value_Get_Subfield(Value *v_obj, size_t offset, BoxXXXX *subf_type) {
  BoxCont *cont;

  if (v_obj->num_ref > 1) {
    /* Here we cannot re-use the register, we have to copy it to a new one */
    BoxCmp *c = v_obj->proc->cmp;
    Value *v_copy = Value_New(c->cur_proc);
    Value_Setup_As_Weak_Copy(v_copy, v_obj);
    Value_Unlink(v_obj);
    v_obj = v_copy;
  }

  cont = & v_obj->value.cont;

  switch(cont->categ) {
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
      v_obj->type = BoxType_Link(subf_type);
      return v_obj;
    }

  case BOXCONTCATEG_PTR:
    cont->value.ptr.offset += offset;
    cont->type = BoxType_Get_Cont_Type(subf_type);
    v_obj->type = BoxType_Link(subf_type);
    return v_obj;

  case BOXCONTCATEG_IMM:
    break;
  }
  MSG_FATAL("Value_Get_Subfield: immediate objects not supported, yet!");
  return NULL;
}

/** Get the next member of a structure. If the given object 'v_memb' is a
 * structure, then returns its first member. If it is a member of a structure,
 * then returns the next member of the same structure.
 * REFERENCES: return: new, v_memb: -1;
 */
Value *Value_Struc_Get_Next_Member(Value *v_memb, BoxTypeId *t_memb) {
  BoxCmp *cmp = v_memb->proc->cmp;
  TS *ts = & cmp->ts;
  BoxTypeId t_next = *t_memb;
  size_t delta_offset = 0;

  if (t_next == BOXTYPE_NONE)
    t_next = TS_Get_Core_Type(ts, BoxType_Get_Id(v_memb->type));

  else
    delta_offset = BoxType_Get_Size(v_memb->type);

  t_next = BoxTS_Get_Next_Struct_Member(ts, t_next);
  if (TS_Is_Member(ts, t_next)) {
    *t_memb = t_next;
    (void) BoxTS_Resolve_Once(ts, & t_next, TS_KS_NONE);
    return Value_Get_Subfield(v_memb, delta_offset,
                              BoxType_From_Id(ts, t_next));

  } else {
    *t_memb = BOXTYPE_NONE;
    return v_memb;
  }
}

static Value *My_Point_Get_Member(Value *v_point, const char *memb) {
  int first = memb[0];
  if (first != '\0') {
    if (memb[1] == '\0') {
      BoxGOp g_op = -1;
      switch(first) {
      case 'x': g_op = BOXGOP_PPTRX; break;
      case 'y': g_op = BOXGOP_PPTRY; break;
      }
      if (g_op != -1) {
        BoxCmp *c = v_point->proc->cmp;
        Value *v_memb = Value_New(c->cur_proc);
        Value_Setup_As_Temp_Old(v_memb, BOXTYPE_PTR);
        CmpProc_Assemble(v_memb->proc, g_op, 2,
                         & v_memb->value.cont, & v_point->value.cont);
        Value_Unlink(v_point);
        v_memb->kind = VALUEKIND_TARGET;
        return
          Value_Get_Subfield(v_memb, (size_t) 0,
                             BoxType_From_Id(& c->ts, c->bltin.species_real));
      }
    }
  }
  Value_Unlink(v_point);
  return NULL;
}

Value *Value_Struc_Get_Member(Value *v_struc, const char *memb) {
  /* If v_struc is a subtype, then expand it (subtypes do not have members) */
  v_struc = Value_Expand_Subtype(v_struc);

  if (v_struc->value.cont.type == BOXCONTTYPE_POINT)
    return My_Point_Get_Member(v_struc, memb);

  BoxXXXX *struct_type = BoxType_Get_Stem(v_struc->type);
  BoxXXXX *node_type = BoxType_Find_Structure_Member(struct_type, memb);

  if (node_type) {
    size_t offset;
    BoxXXXX *member_type;
    BoxBool result = BoxType_Get_Structure_Member(node_type, NULL,
                                                  & offset, 0, & member_type);
    if (result)
      return Value_Get_Subfield(v_struc, offset, member_type);

  }

  Value_Unlink(v_struc);
  return NULL;
}

void ValueStrucIter_Init(ValueStrucIter *vsi, Value *v_struc, CmpProc *proc) {
  Value *v_member = & vsi->v_member;

  Value_Init(v_member, proc);
  Value_Setup_As_Weak_Copy(v_member, v_struc);

  vsi->t_member = BOXTYPE_NONE;
  v_member = Value_Struc_Get_Next_Member(v_member, & vsi->t_member);
  assert(& vsi->v_member == v_member);
  vsi->has_next = (vsi->t_member != BOXTYPE_NONE);
  vsi->index = 0;
}

void ValueStrucIter_Do_Next(ValueStrucIter *vsi) {
  Value *v_member = & vsi->v_member;
  assert(vsi->has_next);
  v_member = Value_Struc_Get_Next_Member(v_member, & vsi->t_member);
  assert(& vsi->v_member == v_member);
  vsi->has_next = (vsi->t_member != BOXTYPE_NONE);
  ++vsi->index;
}

void ValueStrucIter_Finish(ValueStrucIter *vsi) {
  Value_Unlink(& vsi->v_member);
}

ValueStrucIter *ValueStrucIter_New(Value *v_struc, CmpProc *proc) {
  ValueStrucIter *vsi = BoxMem_Safe_Alloc(sizeof(ValueStrucIter));
  ValueStrucIter_Init(vsi, v_struc, proc);
  return vsi;
}

void ValueStrucIter_Destroy(ValueStrucIter *vsi) {
  ValueStrucIter_Finish(vsi);
  BoxMem_Free(vsi);
}

/*
 * REFERENCES: src: -1, dest: 0;
 */
BoxTask Value_Move_Content(Value *dest, Value *src) {
  BoxCmp *c = src->proc->cmp;
  BoxTypeCmp match = BoxType_Compare(dest->type, src->type);
  if (match == BOXTYPECMP_DIFFERENT) {
    MSG_ERROR("Cannot move objects of type '%~s' into objects of type '%~s'",
              BoxType_Get_Repr(src->type), BoxType_Get_Repr(dest->type));
    return BOXTASK_ERROR;
  }

  if (match == BOXTYPECMP_MATCHING)
    src = Value_Expand(src, BoxType_Get_Id(dest->type));

  if (dest->value.cont.type == BOXCONTTYPE_OBJ) {
    /* Object types must be copied and destoyed */

    /* Put addresses in registers. EXAMPLE: o[ro2 + 4] is transformed to ro3
     * through "lea ro3, o[ro2 + 4]"
     */
    Value_Link(dest);
    src = Value_To_Straight_Ptr(src);
    dest = Value_To_Straight_Ptr(dest);

    /* We try to use the method provided by the user, if possible */
    Value_Link(src);
    Value_Link(dest);
    if (BoxCmp_Opr_Try_Emit_Conversion(c, dest, src) == BOXTASK_OK) {
      /* OK, we found a user defined conversion and we used it! Now we just
       * need to return!
       */
      Value_Unlink(src);
      Value_Unlink(dest);
      return BOXTASK_OK;

    } else {
      /* Get the alloc ID for the source value */
      BoxVMAllocID id = TS_Get_AllocID(c, src->type);

      if (id != BOXVMALLOCID_NONE) {
        /* We leave the copy operation to the Box memory management system */
        Value v_id;
        Value_Init(& v_id, c->cur_proc);
        Value_Setup_As_Imm_Int(& v_id, id);
        CmpProc_Assemble(c->cur_proc, BOXGOP_RELOC,
                         3, & dest->value.cont, & src->value.cont,
                         & v_id.value.cont);
        Value_Unlink(& v_id);
        Value_Unlink(src);
        Value_Unlink(dest);
        return BOXTASK_OK;

      } else {
        /* Now we move src to dest, copying it byte by byte */
        Value v_size;
        Value_Init(& v_size, c->cur_proc);
        Value_Setup_As_Imm_Int(& v_size, BoxType_Get_Size(dest->type));

        CmpProc_Assemble(c->cur_proc, BOXGOP_MCOPY,
                         3, & dest->value.cont, & src->value.cont,
                         & v_size.value.cont);
        Value_Unlink(& v_size);
        Value_Unlink(src);
        Value_Unlink(dest);
        return BOXTASK_OK;
      }
    }

  } else if (dest->value.cont.type == BOXCONTTYPE_PTR) {
    /* For pointers we need to pay special care: reference counts! */
    CmpProc_Assemble(dest->proc, BOXGOP_REF,
                     2, & dest->value.cont, & src->value.cont);

  } else {
    /* All the other types can be moved "quickly" with a mov operation */
    CmpProc_Assemble(dest->proc, BOXGOP_MOV,
                     2, & dest->value.cont, & src->value.cont);
  }

  Value_Unlink(src);
  return BOXTASK_OK;
}

/** Emits the conversion from the source expression 'v', to the given type 't'
 * REFERENCES: return: new, src: -1;
 */
static Value *My_Emit_Conversion(BoxCmp *c, Value *src, BoxXXXX *dest) {
  Value *v_dest = Value_New(c->cur_proc);
  Value_Setup_As_Temp(v_dest, dest);
  Value_Link(src);
  Value_Link(v_dest); /* We want to return a new reference! */
  if (BoxCmp_Opr_Try_Emit_Conversion(c, v_dest, src) == BOXTASK_OK) {
    Value_Unlink(src);
    return v_dest;

  } else {
    BoxTask t;
    Value_Link(v_dest);
    Value_Link(src);
    Value_Unlink(Value_Emit_Call(v_dest, src, & t));
    if (t == BOXTASK_OK)
      return v_dest;

    else {
      MSG_ERROR("Don't know how to convert objects of type %~s to %~s.",
                BoxType_Get_Repr(src->type), BoxType_Get_Repr(dest));
      Value_Unlink(v_dest); /* Unlink, since we are not returning it! */
      return NULL;
    }
  }
}

/** Expands the value 'src' as prescribed by the species 'expansion_type'.
 * REFERENCES: return: new, src: -1;
 */
static Value *
Value_Expand_New(Value *src, BoxXXXX *t_dst_new) {
  BoxCmp *c = src->proc->cmp;
  BoxXXXX *t_src_new = src->type;

  if (t_src_new == t_dst_new)
    return src;

  t_src_new =
    BoxType_Resolve(t_src_new,
                    BOXTYPERESOLVE_IDENT | BOXTYPERESOLVE_SPECIES, 0);
  t_dst_new = BoxType_Resolve(t_dst_new, BOXTYPERESOLVE_IDENT, 0);

  if (t_src_new == t_dst_new)
    return src;

  switch (BoxType_Get_Class(t_dst_new)) {
  case BOXTYPECLASS_INTRINSIC: /* t_src != t_dst */
    MSG_FATAL("Value_Expand: type forbidden in species conversions.");
    assert(0);

  case BOXTYPECLASS_SPECIES:
    {
      BoxXXXX *t_species_memb = BoxType_Get_Species_Target(t_dst_new);

      if (t_species_memb) {
        BoxTypeCmp match = BoxType_Compare(t_species_memb, t_dst_new);

        if (match != BOXTYPECMP_DIFFERENT) {
          if (match == BOXTYPECMP_MATCHING) {
            Value *dest = Value_Expand_New(src, t_species_memb);
            Value_Unlink(src);
            src = dest;
          }

          return My_Emit_Conversion(c, src, t_species_memb);
        }
      }

      MSG_FATAL("Value_Expand: type '%~s' is not compatible with '%~s'.",
                BoxType_Get_Repr(t_src_new), BoxType_Get_Repr(t_dst_new));
      assert(0);
    }

#if 0
  case TS_KIND_POINTER:
    MSG_ERROR("Not implemented yet!"); return BOXTASK_FAILURE;
    /*if (td2->tot == TOT_PTR_TO) break;
    return 0;*/

  case TS_KIND_PROC:
    MSG_ERROR("Not implemented yet!"); return BOXTASK_FAILURE;
    /*if (td2->tot != TOT_PROCEDURE) return 0;
    return (
          Tym_Compare_Types(td1->parent, td2->parent)
      && Tym_Compare_Types(td1->target, td2->target) );*/
#endif

  case BOXTYPECLASS_STRUCTURE:
    {
      BoxTypeCmp comparison = BoxType_Compare(t_dst_new, t_src_new);

      /* We check that the comparison can actually be done */
      if (comparison == BOXTYPECMP_DIFFERENT) {
        MSG_FATAL("Value_Expand: Expansion involves incompatible types!");
        assert(0);
      }

      /* We have to expand the structure: we have to create a new structure
       * which can contain the expanded one.
       */
      if (comparison == BOXTYPECMP_MATCHING) { /* need expansion */
        BoxTypeId t_dst_memb, t_src_memb;
        CmpProc *cur_proc = src->proc->cmp->cur_proc;
        Value *v_dst = Value_New(cur_proc),
              *v_dst_memb = Value_New(cur_proc),
              *v_src_memb = Value_New(src->proc);
        Value_Setup_As_Temp(v_dst, t_dst_new);
        Value_Setup_As_Weak_Copy(v_dst_memb, v_dst);
        Value_Setup_As_Weak_Copy(v_src_memb, src);

        t_dst_memb = BOXTYPE_NONE;
        t_src_memb = BOXTYPE_NONE;
        do {
          v_dst_memb = Value_Struc_Get_Next_Member(v_dst_memb, & t_dst_memb);
          v_src_memb = Value_Struc_Get_Next_Member(v_src_memb, & t_src_memb);
          if (t_dst_memb != BOXTYPE_NONE) {
            assert(t_src_memb != BOXTYPE_NONE);
            Value_Link(v_src_memb);
            Value_Move_Content(v_dst_memb, v_src_memb);
            continue;

          } else
            break;

        } while(1);

        assert(t_src_memb == BOXTYPE_NONE);

        Value_Unlink(src);
        Value_Unlink(v_dst_memb);
        Value_Unlink(v_src_memb);
        return v_dst;
      }

      return src;
    }

  default:
    MSG_FATAL("Value_Expand: not fully implemented!");
    assert(0);
  }

  return NULL;
}

Value *Value_Expand(Value *src, BoxTypeId expansion_type) {
  BoxCmp *c = src->proc->cmp;
  TS *ts = & c->ts;
  BoxXXXX *expansion_type_new = BoxType_From_Id(ts, expansion_type);
  return Value_Expand_New(src, expansion_type_new);
}

#if 0
void My_Setup_From_Gro(Value *v, BoxTypeId t, BoxInt gro_num) {
  BoxCmp *c = v->proc->cmp;
  TS *ts = & c->ts;
  if (!TS_Is_Empty(ts, t)) {
    Value v_ptr;
    ValContainer vc = {VALCONTTYPE_GREG, gro_num, 0};
    Value_Init(& v_ptr, c->cur_proc);
    Value_Setup_Container_Old(& v_ptr, BOXTYPE_PTR, & vc);

    Value_Setup_As_Temp_Old(v, BOXTYPE_PTR);
    CmpProc_Assemble(c->cur_proc, BOXGOP_MOV,
                     2, & v->value.cont, & v_ptr.value.cont);
    Value_Unlink(& v_ptr);
    v = Value_Cast_From_Ptr(v, t);
    v->kind = VALUEKIND_TARGET;

  } else {
    Value_Setup_As_Temp_Old(v, t);
    v->kind = VALUEKIND_TARGET;
  }
}
#endif

void My_Family_Setup(Value *v, BoxXXXX *t, int is_parent) {
  BoxCmp *c = v->proc->cmp;

  assert(v->proc == c->cur_proc);

  if (!BoxType_Is_Empty(t)) {
    CmpProc *p = v->proc->cmp->cur_proc;
    BoxVMRegNum ro_num =
      is_parent ? CmpProc_Get_Parent_Reg(p) : CmpProc_Get_Child_Reg(p);
    ValContainer vc = {VALCONTTYPE_LREG, ro_num, 0};
    Value_Setup_Container_Old(v, BOXTYPE_PTR, & vc);
    v = Value_Cast_From_Ptr(v, BoxType_Get_Id(t));
    v->kind = VALUEKIND_TARGET;

  } else {
    Value_Setup_As_Temp(v, t);
    v->kind = VALUEKIND_TARGET;
  }
}

void Value_Setup_As_Parent(Value *v, BoxXXXX *parent_t) {
  return My_Family_Setup(v, parent_t, /* is_parent */ 1);
}

void Value_Setup_As_Child(Value *v, BoxXXXX *child_t) {
  return My_Family_Setup(v, child_t, /* is_parent */ 0);
}

static Value *My_Get_Ptr_To_New_Value(CmpProc *proc, BoxXXXX *t) {
  TS *ts = & proc->cmp->ts;
  BoxTypeId t_old = BoxType_Get_Id(t);
  if (TS_Is_Fast(ts, t_old)) {
    /* Create a structure type containing just one item of type t, allocate
     * that and get a pointer to it.
     * NOTE: this can be improved. We should not keep generating new
     *  structures each time the function is called, but rather cache them!
     */
    Value *v = Value_New(proc);
    BoxTypeId t_struc = BoxTS_Begin_Struct(ts);
    BoxTS_Add_Struct_Member(ts, t_struc, t_old, NULL);
    Value_Setup_As_Temp_Old(v, t_struc);
    return Value_Cast_To_Ptr(v);

  } else {
    Value *v = Value_New(proc);
    Value_Setup_As_Temp(v, t);
    return Value_Cast_To_Ptr(v);
  }
}

Value *Value_Subtype_Build(Value *v_parent, const char *subtype_name) {
  BoxCmp *c = v_parent->proc->cmp;
  TS *ts = & c->ts;
  BoxXXXX *found_subtype;
  Value *v_subtype = NULL;

  /* If the method cannot be found, it could be a method of the child
   * of the type. We then resolve the type to its child and try again.
   * X.GetPoint = Point
   * X.GetPoint[].Norm[] (Norm is a method of Point and not a method
   *                       of GetPoint)
   */
  while (1) {
    found_subtype = BoxType_Find_Subtype(v_parent->type, subtype_name);
    if (found_subtype != NULL)
      break;

    if (TS_Is_Subtype(ts, BoxType_Get_Id(v_parent->type))) {
      v_parent = Value_Expand_Subtype(v_parent);
      if (v_parent == NULL)
        return NULL;

    } else {
      MSG_ERROR("Type '%~s' has not a subtype of name '%s'",
                BoxType_Get_Repr(v_parent->type), subtype_name);
      Value_Unlink(v_parent);
      return NULL;
    }
  }

  assert(found_subtype != NULL);

  /* First, we create the subtype object (a pair of pointers) */
  v_subtype = Value_New(c->cur_proc);
  Value_Setup_As_Temp(v_subtype, found_subtype);

  BoxXXXX *t_child;
  BoxType_Get_Subtype_Info(found_subtype, NULL, NULL, & t_child);
  if (!BoxType_Is_Empty(t_child)) {
    /* Next, we create the child and get a pointer to it */
    Value *v_ptr = Value_New(c->cur_proc),
          *v_subtype_child;
    v_subtype_child = My_Get_Ptr_To_New_Value(c->cur_proc, t_child);

    /* We now create a Value corresponding to the first pointer (the child)
     * and transfer the child pointer to the subtype.
     */
    Value_Setup_As_Weak_Copy(v_ptr, v_subtype);
    v_ptr = Value_Get_Subfield(v_ptr, /* offset */ 0,
                               BoxType_From_Id(ts, BOXTYPE_PTR));
    (void) Value_Move_Content(v_ptr, v_subtype_child);
    Value_Unlink(v_ptr);
  }

  /* We now create the value for the parent Pointer in the subtype */
  if (!BoxType_Is_Empty(v_parent->type)) {
    Value *v_subtype_parent = Value_New(c->cur_proc),
          *v_ptr = Value_New(c->cur_proc);
    Value_Setup_As_Weak_Copy(v_ptr, v_subtype);
    v_ptr = Value_Get_Subfield(v_ptr, /*offset*/ sizeof(BoxPtr),
                               BoxType_From_Id(ts, BOXTYPE_PTR));
    Value_Setup_As_Weak_Copy(v_subtype_parent, v_parent);
    v_subtype_parent = Value_Cast_To_Ptr(v_subtype_parent);
    (void) Value_Move_Content(v_ptr, v_subtype_parent);
    Value_Unlink(v_ptr);
#if 0
    Value_Emit_Link(v_parent); /* The subtype gets a reference
                                  to its parent */
#endif
  }

  Value_Unlink(v_parent);

  return v_subtype;
}

static Value *My_Value_Subtype_Get(Value *v_subtype, int get_child) {
  BoxCmp *c = v_subtype->proc->cmp;
  TS *ts = & c->ts;
  Value *v_ret = NULL;

  if (Value_Want_Value(v_subtype)) {
    BoxTypeId t_subtype = BoxType_Get_Id(v_subtype->type);
    if (TS_Is_Subtype(ts, t_subtype)) {
      BoxXXXX *t_parent, *t_child;
      BoxBool success = BoxType_Get_Subtype_Info(v_subtype->type, NULL,
                                                 & t_parent, & t_child);
      assert(success);
      BoxXXXX *t_ret = get_child ? t_child : t_parent;
      if (BoxType_Is_Empty(t_ret)) {
        v_ret = Value_New(c->cur_proc);
        Value_Setup_As_Temp(v_ret, t_ret);

      } else {
        size_t offset = get_child ? 0 : sizeof(BoxPtr);
        v_ret = Value_New(c->cur_proc);
        /* FIXME: see Value_Init */
        Value_Setup_As_Weak_Copy(v_ret, v_subtype);          
        v_ret = Value_Get_Subfield(v_ret, offset,
                                   BoxType_From_Id(ts, BOXTYPE_PTR));
        v_ret = Value_Cast_From_Ptr(v_ret, BoxType_Get_Id(t_ret));

        /* Temporary fix: transfer ownership of register, if needed */
        if (v_subtype->num_ref == 1) {
          v_ret->attr.own_register = v_subtype->attr.own_register;
          v_subtype->attr.own_register = 0;
        }
      }

    } else {
      const char *what = get_child ? "child" : "parent";
      MSG_ERROR("Cannot get the %s of '%~s': this is not a subtype!",
                what, TS_Name_Get(ts, t_subtype));
    }
  }

  Value_Unlink(v_subtype);
  return v_ret;
}

Value *Value_Subtype_Get_Child(Value *v_subtype) {
  return My_Value_Subtype_Get(v_subtype, 1);
}

Value *Value_Subtype_Get_Parent(Value *v_subtype) {
  return My_Value_Subtype_Get(v_subtype, 0);
}

Value *Value_Expand_Subtype(Value *v) {
  if (Value_Is_Value(v)) {
    BoxCmp *c = v->proc->cmp;
    if (TS_Is_Subtype(& c->ts, BoxType_Get_Id(v->type))) {
      int subtype_was_target = (v->kind == VALUEKIND_TARGET);
      v = Value_Subtype_Get_Child(v);
      if (subtype_was_target)
        v = Value_Promote_Temp_To_Target(v);
      return v;
    }
  }

  return v;
}

Value *Value_Raise(Value *v) {
  if (Value_Is_Value(v)) {
    BoxXXXX *unraised_type = BoxType_Unraise(v->type);
    if (unraised_type) {
      BoxType_Unlink(v->type);
      v->type = unraised_type;
      return v;

    } else {
      Value_Unlink(v);
      MSG_ERROR("Raising operator is applied to a non-raised type.");
      return NULL;
    }

  } else {
    Value_Unlink(v);
    MSG_ERROR("Raising operator got invalid operand.");
    return NULL;
  }
}
