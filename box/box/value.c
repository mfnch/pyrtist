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

#include <stdlib.h>
#include <assert.h>

#include "types.h"
#include "mem.h"
#include "print.h"
#include "messages.h"
#include "container.h"
#include "compiler.h"
#include "value.h"

void Value_Init(Value *v, CmpProc *proc) {
  v->proc = proc;
  v->kind = VALUEKIND_ERR;
  v->type = BOXTYPE_NONE;
  v->name = NULL;
  v->attr.new_or_init = 0;
  v->attr.own_register = 0;
  v->attr.own_reference = 0;
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
    return;
  case VALUEKIND_TYPE_NAME:
  case VALUEKIND_VAR_NAME:
    return;
  case VALUEKIND_TYPE:
    return;
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

      if (v->attr.own_reference) {
        assert(v->value.cont.type == BOXTYPE_OBJ);
        assert(v->value.cont.categ == BOXCONTCATEG_LREG); /* for now we don't support the general case... */
        CmpProc_Assemble(v->proc, BOXGOP_MUNLN, 1, & v->value.cont);
      }
      return;

    case BOXCONTCATEG_PTR:
      if (v->attr.own_register) {
        assert(!v->value.cont.value.ptr.is_greg);
        Reg_Release(& v->proc->reg_alloc,
                    BOXTYPE_OBJ, v->value.cont.value.ptr.reg);
      }

      if (v->attr.own_reference) {
        MSG_WARNING("My_Value_Finalize: should we unlink also for members?");
      }
      return;

    default:
      MSG_WARNING("My_Value_Finalize: Destruction not implemented!");
      return;
    }
  }
}

void Value_Unlink(Value *v) {
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

void Value_Link(Value *v) {
  v->num_ref += 1;
}

/** Determine if the given value can be recycled, otherwise return Value_New()
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
    Value_Link(v); /* Must return a new reference */
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

void Value_Setup_As_Weak_Copy(Value *v_copy, Value *v) {
  v_copy->proc = v->proc;
  v_copy->kind = v->kind;
  v_copy->type = v->type;
  v_copy->value.cont = v->value.cont;
  v_copy->name = (v->name == NULL) ? NULL : BoxMem_Strdup(v->name);
  v_copy->attr.own_register = 0;
  v_copy->attr.own_reference = 0;
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

void Value_Setup_As_Type(Value *v, BoxType t) {
  v->kind = VALUEKIND_TYPE;
  v->type = t;
  v->value.cont.type = TS_Get_Cont_Type(& v->proc->cmp->ts, t);
}

void Value_Setup_As_Imm_Char(Value *v, Char c) {
  v->kind = VALUEKIND_IMM;
  v->type = BOXTYPE_CHAR;
  BoxCont_Set(& v->value.cont, "ic", c);
}

void Value_Setup_As_Imm_Int(Value *v, Int i) {
  v->kind = VALUEKIND_IMM;
  v->type = v->proc->cmp->bltin.species_int;
  BoxCont_Set(& v->value.cont, "ii", i);
}

void Value_Setup_As_Imm_Real(Value *v, Real r) {
  v->kind = VALUEKIND_IMM;
  v->type = v->proc->cmp->bltin.species_real;
  BoxCont_Set(& v->value.cont, "ir", r);
}

void Value_Setup_As_Void(Value *v) {
  v->kind = VALUEKIND_IMM;
  v->type = BOXTYPE_VOID;
  v->value.cont.type = BOXCONTTYPE_VOID;
}

void Value_Setup_As_Temp(Value *v, BoxType t) {
  ValContainer vc = {VALCONTTYPE_LREG, -1, 0};
  Value_Setup_Container(v, t, & vc);
  Value_Emit_Allocate(v);
}

void Value_Setup_As_String(Value *v_str, const char *str) {
  BoxCmp *c = v_str->proc->cmp;
  size_t l, addr;
  Value v_str_data;
  ValContainer vc = {VALCONTTYPE_GPTR, 0, 0};

  l = strlen(str) + 1;
  addr = BoxVM_Data_Add(c->vm, str, l, BOXTYPE_OBJ);
  assert(addr >= 0);

  vc.addr = addr;
  Value_Init(& v_str_data, v_str->proc);
  Value_Setup_Container(& v_str_data, BOXTYPE_OBJ, & vc);

  Value_Setup_As_Temp(v_str, c->bltin.string);
  if (Value_Emit_Call(v_str, & v_str_data) != BOXTASK_OK) {
    MSG_FATAL("Value_Setup_As_String: Failure while emitting string.");
    assert(0);
  }

  Value_Unlink(& v_str_data);
}

/* Create a new empty container. */
void Value_Setup_Container(Value *v, BoxType type, ValContainer *vc) {
  RegAlloc *ra = & v->proc->reg_alloc;
  int use_greg;

  v->type = type;
  v->value.cont.type = TS_Get_Cont_Type(& v->proc->cmp->ts, type);

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
    v->value.cont.categ = CAT_GREG;
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

void Value_Emit_Allocate(Value *v) {
  switch(v->kind) {
  case VALUEKIND_ERR:
    return;
  case VALUEKIND_TEMP:
  case VALUEKIND_TARGET:
    if (v->value.cont.type == BOXCONTTYPE_OBJ) {
      BoxCmp *c = v->proc->cmp;
      CmpProc *proc = c->cur_proc;
      Value v_size, v_alloc_type;

      assert(v->attr.own_reference == 0);
      assert(v->proc == proc);

      Value_Init(& v_size, proc);
      Value_Setup_As_Imm_Int(& v_size, TS_Get_Size(& c->ts, v->type));
      Value_Init(& v_alloc_type, proc);
      Value_Setup_As_Imm_Int(& v_alloc_type, v->type);
      CmpProc_Assemble(proc, BOXGOP_MALLOC,
                       3, & v->value.cont, & v_size.value.cont,
                       & v_alloc_type.value.cont);
      v->attr.own_reference = 1;

      /* Now let's invoke the creator */
      (void) Value_Emit_Call(v, & c->value.create);
    }
    return;

  default:
    MSG_FATAL("Value_Emit_Allocate: invalid argument (%s).",
              ValueKind_To_Str(v->kind));
    assert(0);
  }
}

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
    return Value_Recycle(v); /* Return an error value (Value_Recycle set the
                                leaves v with an error, if not initialised
                                with a Value_Setup_* function) */

  case VALUEKIND_TYPE:
    {
      BoxType t = v->type;
      v = Value_Recycle(v);
      Value_Setup_Container(v, t, & vc);
      Value_Emit_Allocate(v);
      return v;
    }

  case VALUEKIND_IMM:
  case VALUEKIND_TARGET:
    {
      Value old_v = *v;
      v = Value_Recycle(v);
      Value_Setup_Container(v, old_v.type, & vc);
      CmpProc_Assemble(c->cur_proc, BOXGOP_MOV,
                       2, & v->value.cont, & old_v.value.cont);
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
    return (TS_Compare(& v->proc->cmp->ts, BOXTYPE_VOID, v->type)
            != TS_TYPES_UNMATCH);

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
                BOXGOP_MOV : BOXGOP_LEA;
    CmpProc_Assemble(c->cur_proc, op,
                     2, & c->cont.pass_child, & v_to_pass->value.cont);
    Value_Unlink(v_to_pass);
  }

  CmpProc_Assemble_Call(c->cur_proc, sym_id);
}

BoxTask Value_Emit_Call(Value *parent, Value *child) {
  BoxCmp *c = parent->proc->cmp;
  BoxType found_procedure, expansion_for_child;
  BoxVMSymID sym_id;

  assert(parent != NULL && child != NULL);
  assert(c == child->proc->cmp);

  /* Now we search for the procedure associated with *child */
  TS_Procedure_Inherited_Search(& c->ts, & found_procedure,
                                & expansion_for_child,
                                parent->type, child->type, 1);

  if (found_procedure == BOXTYPE_NONE)
    return BOXTASK_FAILURE;

  if (expansion_for_child != BOXTYPE_NONE) {
    Value_Link(child); /* XXX */
    child = Value_Expand(child, expansion_for_child);
    if (child == NULL)
      return BOXTASK_ERROR;
  }

  TS_Procedure_Sym_Num_Get(& c->ts, & sym_id, found_procedure);

  Value_Emit_Call_From_SymID(sym_id, parent, child);
  return BOXTASK_OK;
}

/**
 * REFERENCES: return: new, v_obj: -1;
 */
Value *Value_To_Straight_Ptr(Value *v_obj) {
  assert(v_obj->value.cont.type == BOXTYPE_OBJ);

  if (v_obj->value.cont.categ == BOXCONTCATEG_PTR) {
    ValContainer vc = {VALCONTTYPE_LREG, -1, 0};
    Value *v_ret;
    BoxCont cont = v_obj->value.cont;

    Value_Unlink(v_obj);
    v_ret = Value_New(v_obj->proc->cmp->cur_proc);
    Value_Setup_Container(v_ret, v_obj->type, & vc);

    assert(v_ret->value.cont.type == BOXTYPE_OBJ);
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
Value *Value_Get_Subfield(Value *v_obj, size_t offset, BoxType subf_type) {
  BoxCont *cont = & v_obj->value.cont;

  if (v_obj->num_ref > 1) {
    MSG_FATAL("Value_Get_Subfield: ref count > 1: not implemented, yet!");
    assert(0);
  }

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
      cont->type = TS_Get_Cont_Type(& v_obj->proc->cmp->ts, subf_type);
      v_obj->type = subf_type;
      return v_obj;
    }

  case BOXCONTCATEG_PTR:
    cont->value.ptr.offset += offset;
    cont->type = TS_Get_Cont_Type(& v_obj->proc->cmp->ts, subf_type);
    v_obj->type = subf_type;
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
Value *Value_Struc_Get_Next_Member(Value *v_memb, BoxType *t_memb) {
  BoxCmp *cmp = v_memb->proc->cmp;
  TS *ts = & cmp->ts;
  BoxType t_next = *t_memb;

  t_next = (t_next != BOXTYPE_NONE) ? t_next : v_memb->type;
  t_next = TS_Get_Next_Member(ts, t_next);
  if (TS_Is_Member(ts, t_next)) {
    size_t offset = TS_Get_Size(ts, t_next);
    *t_memb = t_next;
    t_next = TS_Resolve_Once(ts, t_next, TS_KS_NONE);
    return Value_Get_Subfield(v_memb, offset, t_next);

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
        Value_Setup_As_Temp(v_memb, BOXTYPE_PTR);
        CmpProc_Assemble(v_memb->proc, g_op, 2,
                         & v_memb->value.cont, & v_point->value.cont);
        Value_Unlink(v_point);
        v_memb->kind = VALUEKIND_TARGET;
        return Value_Get_Subfield(v_memb, (size_t) 0, c->bltin.species_real);
      }
    }
  }
  Value_Unlink(v_point);
  return NULL;
}

Value *Value_Struc_Get_Member(Value *v_struc, const char *memb) {
  BoxCmp *cmp = v_struc->proc->cmp;
  if (v_struc->value.cont.type == BOXTYPE_POINT)
    return My_Point_Get_Member(v_struc, memb);
  BoxType t_memb = TS_Member_Find(& cmp->ts, v_struc->type, memb);
  if (t_memb != BOXTYPE_NONE) {
    size_t offset;
    t_memb = TS_Member_Get(& cmp->ts, t_memb, & offset);
    return Value_Get_Subfield(v_struc, offset, t_memb);

  } else {
    Value_Unlink(v_struc);
    return NULL;
  }
}

/**
 * REFERENCES: src: -1, dest: 0;
 */
BoxTask Value_Move_Content(Value *dest, Value *src) {
  BoxCmp *c = src->proc->cmp;
  TS *ts = & c->ts;
  TSCmp match = TS_Compare(ts, dest->type, src->type);
  if (match == TS_TYPES_UNMATCH) {
    MSG_ERROR("Cannot move objects of type '%~s' into objects of type '%~s'",
              TS_Name_Get(ts, src->type), TS_Name_Get(ts, dest->type));
    return BOXTASK_ERROR;
  }

  if (match == TS_TYPES_EXPAND)
    src = Value_Expand(src, dest->type);

  if (dest->value.cont.type == BOXCONTTYPE_OBJ) {
    /* Object types must be copied and destoyed */
    Value v_size;

    /* Put addresses in registers. EXAMPLE: o[ro2 + 4] is transformed to ro3
     * through "lea ro3, o[ro2 + 4]"
     */
    Value_Link(dest);
    src = Value_To_Straight_Ptr(src);
    dest = Value_To_Straight_Ptr(dest);

    /* First let's destroy the 'dest' obj (it is going to be overwritten) */
    (void) Value_Emit_Call(dest, & c->value.destroy);

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
      /* Now we move src to dest, copying it byte by byte */
      Value_Init(& v_size, c->cur_proc);
      Value_Setup_As_Imm_Int(& v_size, TS_Get_Size(& c->ts, dest->type));
      CmpProc_Assemble(c->cur_proc, BOXGOP_MCOPY,
                       3, & dest->value.cont, & src->value.cont,
                       & v_size.value.cont);
      Value_Unlink(& v_size);
      Value_Unlink(src);
      Value_Unlink(dest);
      return BOXTASK_OK;
    }

  } else {
    /* All the other types can be moved "quickly" with a mov operation */
    CmpProc_Assemble(dest->proc, BOXGOP_MOV,
                     2, & dest->value.cont, & src->value.cont);
  }

  Value_Unlink(src);
  return BOXTASK_OK;
}

/** Expands the value 'src' as prescribed by the species 'expansion_type'.
 * REFERENCES: return: new, src: -1;
 */
Value *Value_Expand(Value *src, BoxType expansion_type) {
  BoxCmp *c = src->proc->cmp;
  TS *ts = & c->ts;
  BoxInt t_src, t_dst;

//   assert(e->is.typed && e->is.value);

  t_dst = expansion_type;
  t_src = src->type;

#ifdef DEBUG_SPECIES_EXPANSION
  printf("Expand: '%s' --> '%s'\n",
         Tym_Type_Names(e->type), Tym_Type_Names(species));
#endif

  if (t_src == t_dst)
    return src;

  t_src = TS_Resolve(ts, t_src, TS_KS_ALIAS | TS_KS_SPECIES);
  t_dst = TS_Resolve(ts, t_dst, TS_KS_ALIAS);

  if (t_src == t_dst)
    return src;

  switch(TS_Get_Kind(ts, t_dst)) {
  case TS_KIND_INTRINSIC: /* t_src != t_dst */
    MSG_FATAL("Value_Expand: type forbidden in species conversions.");
    assert(0);

  case TS_KIND_SPECIES:
    {
      BoxType t_species_memb = TS_Get_Next_Member(ts, t_dst);
      for(t_species_memb = TS_Get_Next_Member(ts, t_dst);
          t_species_memb != BOXTYPE_NONE;
          t_species_memb = TS_Get_Next_Member(ts, t_species_memb)) {
        TSCmp match = TS_Compare(ts, t_species_memb, t_dst);
        if (match != TS_TYPES_UNMATCH) {
          if (match == TS_TYPES_EXPAND) {
            Value *dest = Value_Expand(src, t_species_memb);
            Value_Unlink(src);
            src = dest;
          }
          return BoxCmp_Opr_Emit_Conversion(c, src, t_species_memb);
        }
      }

      MSG_FATAL("Value_Expand: type '%~s' is not compatible with '%~s'.",
                TS_Name_Get(ts, t_src), TS_Name_Get(ts, t_dst));
      assert(0);
    }

#if 0
  case TS_KIND_POINTER:
    MSG_ERROR("Not implemented yet!"); return Failed;
    /*if (td2->tot == TOT_PTR_TO) break;
    return 0;*/

  case TS_KIND_ARRAY:
    switch(TS_Compare(cmp->ts, type1, type2)) {
    case TS_TYPES_EQUAL:
    case TS_TYPES_MATCH:
      return Success;
    case TS_TYPES_EXPAND:
      MSG_ERROR("Expansion of array of species is not implemented yet!");
      return Failed;
    default:
      MSG_ERROR("Value_Expand: Expansion to array involves "
                "an incompatible type.");
      return Failed;
    }

  case TS_KIND_PROC:
    MSG_ERROR("Not implemented yet!"); return Failed;
    /*if (td2->tot != TOT_PROCEDURE) return 0;
    return (
          Tym_Compare_Types(td1->parent, td2->parent)
      && Tym_Compare_Types(td1->target, td2->target) );*/
#endif

  case TS_KIND_STRUCTURE:
    {
      TSCmp comparison = TS_Compare(ts, t_dst, t_src);

      /* We check that the comparison can actually be done */
      if (comparison == TS_TYPES_UNMATCH) {
        MSG_FATAL("Value_Expand: Expansion involves incompatible types!");
        assert(0);
      }

      /* We have to expand the structure: we have to create a new structure
       * which can contain the expanded one.
       */
      if (comparison == TS_TYPES_EXPAND) { /* need expansion */
        BoxType t_dst_memb, t_src_memb;
        Value *v_dst = Value_New(src->proc), /* same proc as src */
              *v_dst_memb = Value_New(src->proc),
              *v_src_memb = Value_New(src->proc);
        Value_Setup_As_Temp(v_dst, t_dst);
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
