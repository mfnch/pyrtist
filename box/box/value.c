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
#include "new_compiler.h"
#include "value.h"

void Value_Init(Value *v, BoxCmp *cmp) {
  v->cmp = cmp;
  v->kind = VALUEKIND_ERR;
  v->type = BOXTYPE_NONE;
  v->name = NULL;
  v->attr.new_or_init = 0;
  v->attr.own_register = 0;
  v->attr.own_reference = 0;
  v->attr.ignore = 0;
  v->num_ref = 1;
}

Value *Value_New(BoxCmp *cmp) {
  Value *v = BoxMem_Safe_Alloc(sizeof(Value));
  Value_Init(v, cmp);
  v->attr.new_or_init = 1;
  return v;
}

static void My_Value_Finalize(Value *v) {
  if (v->name != NULL)
    BoxMem_Free(v->name);

  switch(v->kind) {
  case VALUEKIND_ERR:
    return;
  case VALUEKIND_IDENTIFIER:
    return;
  case VALUEKIND_TYPE:
    return;
  case VALUEKIND_IMM:
    return;
  case VALUEKIND_TEMP:
    assert(v->value.cont.categ == BOXCONTCATEG_LREG
           && v->value.cont.value.reg >= 0);
    if (v->attr.own_register)
      Reg_Release(& v->cmp->regs,
                  v->value.cont.type, v->value.cont.value.reg);
    return;
  case VALUEKIND_TARGET:
    return;
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
  if (v->num_ref == 1) {
    /* one reference means nobody else owns the object, so we can do
     * whatever we want with it!
     */
    int new_or_init = v->attr.new_or_init;
    Value_Init(v, v->cmp);
    v->attr.new_or_init = new_or_init;
    Value_Link(v); /* Must return a new reference */
    return v;

  } else
    return Value_New(v->cmp);
}

const char *ValueKind_To_Str(ValueKind vk) {
  switch(vk) {
  case VALUEKIND_ERR: return "an error expression";
  case VALUEKIND_IDENTIFIER: return "an undefined identifier";
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
  switch(v->kind) {
  case VALUEKIND_IMM: case VALUEKIND_TEMP: case VALUEKIND_TARGET:
    return 1;
  default:
    if (v->name != NULL) {
      MSG_ERROR("%s is undefined: an expression with both value and type is "
                "expected here.", v->name);
    } else {
      MSG_ERROR("Got %s, but an expression with both value and type is "
                "expected here.", ValueKind_To_Str(v->kind));
    }
    return 0;
  }
}

void Value_Set_Identifier(Value *v, const char *name) {
  v->kind = VALUEKIND_IDENTIFIER;
  v->name = BoxMem_Strdup(name);
}

void Value_Set_Imm_Char(Value *v, Char c) {
  v->kind = VALUEKIND_IMM;
  v->type = BOXTYPE_CHAR;
  BoxCont_Set(& v->value.cont, "ic", c);
}

void Value_Set_Imm_Int(Value *v, Int i) {
  v->kind = VALUEKIND_IMM;
  v->type = BOXTYPE_INT;
  BoxCont_Set(& v->value.cont, "ii", i);
}

void Value_Set_Imm_Real(Value *v, Real r) {
  v->kind = VALUEKIND_IMM;
  v->type = BOXTYPE_REAL;
  BoxCont_Set(& v->value.cont, "ir", r);
}

/* Create a new empty container. */
void Value_Container_Init(Value *v, BoxType type, ValContainer *vc) {
  BoxCmp *c = v->cmp;
  int use_greg;

  v->type = type;
  v->value.cont.type = TS_Core_Type(& c->ts, type);

  switch(vc->type_of_container) {
  case VALCONTTYPE_IMM:
    v->kind = VALUEKIND_IMM;
    v->value.cont.categ = CONT_IMM;
    return; break;

  case VALCONTTYPE_LREG:
    v->kind = VALUEKIND_TEMP;
    v->value.cont.categ = BOXCONTCATEG_LREG;
    if (vc->which_one < 0) {
      /* Automatically chooses the local register */
      Int reg = Reg_Occupy(& c->regs, v->value.cont.type);
      if (reg < 1) {
        MSG_FATAL("Value_Container_Init: Reg_Occupy failed!");
        assert(0);
      }
      v->value.cont.value.reg = reg;
      v->attr.own_register = 1;
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
      Int reg = -GVar_Occupy(& c->regs, v->value.cont.type);
      /* Automatically choses the local variables */
      if (reg >= 0) {
        MSG_FATAL("Value_Container_Init: GVar_Occupy failed!");
        assert(0);
      }
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
      Int reg = -Var_Occupy(& c->regs, v->value.cont.type, 0);
      if (reg >= 0) {
        MSG_FATAL("Value_Container_Init: Var_Occupy failed!");
        assert(0);
      }
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
    v->value.cont.value.ptr.reg = vc->addr;
    v->value.cont.value.ptr.offset = vc->which_one;
    if (use_greg || vc->addr >= 0) return;

    if (vc->type_of_container == VALCONTTYPE_LRPTR) {
      Int reg = Reg_Occupy(& c->regs, TYPE_OBJ);
      v->value.cont.value.ptr.reg = reg;
      if (reg < 1) {
        MSG_FATAL("Value_Container_Init: Reg_Occupy failed!");
        assert(0);
      }
      return;

    } else {
      Int reg = -Var_Occupy(& c->regs, TYPE_OBJ, 0);
      v->value.cont.value.ptr.reg = reg;
      if (reg >= 0) {
        MSG_FATAL("Value_Container_Init: Var_Occupy failed!");
        assert(0);
      }
    }

    return;
    break;

  default:
    MSG_FATAL("Value_Container_Init: wrong type of container!");
    assert(0);
  }
}

Value *Value_Make_Temp(Value *v) {
  if (v->kind != VALUEKIND_TEMP) {
    Value old_v = *v;
    ValContainer vc = {VALCONTTYPE_LREG, -1, 0};
    v = Value_Recycle(v);
    Value_Container_Init(v, old_v.type, & vc);
    CmpProc_Assemble(v->cmp->cur_proc, BOXGOP_MOV,
                     2, & v->value.cont, & old_v.value.cont);
    return v;

  } else {
    Value_Link(v);
    return v;
  }
}

int Value_Is_Temp(Value *v) {
  return (v->kind == VALUEKIND_TEMP);
}

int Value_Is_Identifier(Value *v) {
  return (v->kind == VALUEKIND_IDENTIFIER);
}

int Value_Is_Target(Value *v) {
  return (v->kind == VALUEKIND_TARGET);
}
