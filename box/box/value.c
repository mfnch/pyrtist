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
#include "messages.h"
#include "container.h"
#include "new_compiler.h"
#include "value.h"

void Value_Init(Value *v) {
  v->kind = VALUEKIND_ERR;
  v->type = BOXTYPE_NONE;
  v->attr.new_or_init = 1;
  v->attr.own_register = 0;
  v->attr.own_reference = 0;
  v->attr.ignore = 0;
  v->num_ref = 1;
}

Value *Value_New(void) {
  Value *v = BoxMem_Safe_Alloc(sizeof(Value));
  Value_Init(v);
  v->attr.new_or_init = 0;
  return v;
}

void Value_Unlink(Value *v) {
  if (v->num_ref > 1)
    --v->num_ref;

  else {
    assert(v->num_ref == 1);
    /* do something */
    v->num_ref = 0;
    if (v->attr.new_or_init)
      BoxMem_Free(v);
  }
}

void Value_Link(Value *v) {
  v->num_ref += 1;
}

void Value_Set_Imm_Char(Value *v, Char c) {
  v->kind = VALUEKIND_IMM;
  v->type = BOXTYPE_CHAR;
  BoxCont_Set(& v->cont, "ic", c);
}

void Value_Set_Imm_Int(Value *v, Int i) {
  v->kind = VALUEKIND_IMM;
  v->type = BOXTYPE_INT;
  BoxCont_Set(& v->cont, "ii", i);
}

void Value_Set_Imm_Real(Value *v, Real r) {
  v->kind = VALUEKIND_IMM;
  v->type = BOXTYPE_REAL;
  BoxCont_Set(& v->cont, "ir", r);
}

/* Create a new empty container. */
void Value_Container_Init(BoxCmp *c, Value *v,
                          BoxType type, ValContainer *vc) {
  int intrinsic;
  Int type_of_register, resolved;

/*  e->is.typed = 1;
  e->is.value = 1;
  e->is.ignore = 0;
  e->is.allocd = 0;
  e->type = type;
  e->resolved = resolved = Tym_Type_Resolve_All(type);
  intrinsic = (resolved < NUM_INTRINSICS);
  type_of_register = (intrinsic) ? resolved : TYPE_OBJ;

  e->is.imm = 0;
  e->is.target = 0;
  e->is.release = 0;*/

  v->type = type;
  v->cont.type = TS_Core_Type(& c->ts, type);

  switch(vc->type_of_container) {
  case VALCONTTYPE_IMM:
    v->kind = VALUEKIND_IMM;
    v->cont.categ = CONT_IMM;
    return; break;

  case VALCONTTYPE_LREG:
    v->kind = VALUEKIND_TEMP;
    v->cont.categ = BOXCONTCATEG_LREG;
    v->attr.own_register = 1;
    if (vc->which_one < 0) {
      /* Automatically chooses the local register */
      Int reg = Reg_Occupy(& c->regs, v->cont.type);
      if (reg < 1) {
        MSG_FATAL("Value_Container_Init: Reg_Occupy failed!");
      }
      v->cont.value.reg = reg;
      v->attr.own_register = 1;
      return;

    } else {
      /* The user wants a particular register to be chosen */
      v->cont.value.reg = vc->which_one;
      return;
    }
    break;

  case VALCONTTYPE_GVAR:
    v->cont.categ = CONT_GREG;
    v->kind = VALUEKIND_TARGET;
    if (vc->which_one < 0) {
      Int reg = -GVar_Occupy(& c->regs, type_of_register);
      /* Automatically choses the local variables */
      if (reg >= 0) {
        MSG_FATAL("Value_Container_Init: GVar_Occupy failed!");
      }
      v->cont.value.reg = reg;
      return;

    } else {
      /* The user wants a particolar variable to be chosen */
      v->cont.value.reg = vc->which_one;
      return;
    }
    break;

#if 0
  case VALCONTTYPE_LVAR:
    v->cont.categ = CONT_LREG;
    v->attr.is_target = 1;
    if (vc->which_one < 0) {
      /* Automatically choses the local variables */
      Int reg = -Var_Occupy(type_of_register, Box_Num());
      if (reg >= 0) {
        MSG_FATAL("Value_Container_Init: Var_Occupy failed!");
      }
      v->cont.reg = reg;
      return;

    } else {
      /* The user wants a particolar variable to be chosen */
      v->cont.reg = c->which_one;
      return;
    }
    break;
#endif

  case VALCONTTYPE_GREG:
    v->cont.categ = CAT_GREG;
    v->cont.value.reg = vc->which_one;
    return;
    break;

  case VALCONTTYPE_GPTR:
  case VALCONTTYPE_LRPTR:
  case VALCONTTYPE_LVPTR:
  {
    int is_gaddr = (vc->type_of_container == VALCONTTYPE_GPTR);
    assert(0);

#if 0
    v->cont.is.gaddr = is_gaddr;
    v->cont.is.target = 1;
    v->cont.categ = CONT_PTR;
    v->cont.addr = c->addr;         /* X in o[roX + N]*/
    v->cont.value.i = c->which_one; /* X in o[roN + X] */

    if (is_gaddr || c->addr >= 0) return;

    if (c->type_of_container == VALCONTTYPE_LRPTR) {
      if ( (e->addr = Reg_Occupy(TYPE_OBJ)) < 1 ) {
        MSG_FATAL("Value_Container_Init: Reg_Occupy failed!");
      }
      e->is.release = 1;
      return;

    } else {
      if ( (e->addr = -Var_Occupy(TYPE_OBJ, Box_Num())) >= 0 ) {
        MSG_FATAL("Value_Container_Init: Var_Occupy failed!");
      }
    }
#endif
    return;
    break;
  }

  default:
    MSG_FATAL("Value_Container_Init: wrong type of container!");
  }
}

void Value_Make_Temp(BoxCmp *c, Value *v) {
  if (v->kind != VALUEKIND_TEMP) {
    Value old_v = *v;
    ValContainer vc = {VALCONTTYPE_LREG, -1, 0};
    Value_Init(v);
    Value_Container_Init(c, v, old_v.type, & vc);
    CmpProc_Assemble(c->cur_proc, BOXGOP_MOV, 2, & v->cont, & old_v.cont);
  }
}
