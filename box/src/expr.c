/***************************************************************************
 *   Copyright (C) 2006 by Matteo Franchin                                 *
 *   fnch@libero.it                                                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

/* $Id$ */

#include <assert.h>
#include <ctype.h>

#include "types.h"
#include "messages.h"
#include "typesys.h"
#include "compiler.h"
#include "registers.h"
#include "expr.h"
#include "str.h"
#include "mem.h"

/* This fuction creates an expression with type, but without value.
 */
void Expr_New_Type(Expr *e, Int type) {
  e->type = type;
  e->resolved = Tym_Type_Resolve_All(type);
  e->value.i = 0;
  e->addr = 0;
  e->categ = 0;
  e->is.typed = 1;
  e->is.value = 0;
  e->is.ignore = 0;
  e->is.imm = 0;
  e->is.target = 0;
  e->is.gaddr = 0;
  e->is.allocd = 0;
  e->is.release = 0;
}

/** Put inside *e a the Void value. */
void Expr_New_Void(Expr *e) {
  e->type = TYPE_VOID;
  e->resolved = TYPE_VOID;
  e->is.typed = 1;
  e->is.value = 0;
  e->is.ignore = 0;
  e->is.imm = 0;
  e->is.target = 0;
}

/** Create an expression with value, corresponding to a local register 0
 * having a suitable type.
 */
void Expr_New_Value(Expr *e, Type t) {
  e->is.typed = 1;
  e->type = t;
  e->resolved = TS_Resolve(cmp->ts, t, 1, 1);
  e->is.value = (TS_Size(cmp->ts, t) > 0) ? 1 : 0;
  e->categ = CAT_LREG;
  e->value.i = 0;
  e->is.imm = e->is.ignore = 0;
  e->is.target = e->is.gaddr = 0;
  e->is.allocd = e->is.release = 0;
  e->addr = 0;
}

/* Prints the details about the specified expression *e.
 */
void Expr_Print(Expr *e, FILE *out) {
  char buffer[128], *value = buffer;
  static const char *asm_arg_str[4] = {
    "global register",
    "local register",
    "pointer to location",
    "immediate value"
  };

  if ( ! e->is.typed ) {
    fprintf(out, "Name(name=\"%s\")\n", e->value.nm.text);
    return;
  }

  if ( e->categ == CAT_IMM ) {
    switch(e->resolved) {
    case TYPE_CHAR:
      sprintf(buffer, "INT("SChar")", (Char) e->value.i);
      break;
    case TYPE_INTG:
      sprintf(buffer, "INT("SIntg")", e->value.i);
      break;
    case TYPE_REAL:
      sprintf(buffer, "REAL("SReal")", e->value.r);
      break;
    default:
      value = "UNKNOWN";
      break;
    }

  } else {
    sprintf(buffer, "INT("SIntg")", e->value.i);
  }

  fprintf(out,
    "Expression(type="SIntg"=\"%s\", resolved="SIntg"=\"%s\", "
    "categ=%d=\"%s\", %s, imm=%c, value=%c, typed=%c, ignore=%c, target=%c, "
    "gaddr=%c, allocd=%c, release=%c)\n",
    e->type, Tym_Type_Names(e->type),
    e->resolved, Tym_Type_Names(e->resolved),
    e->categ,
    (e->categ >= 0) && (e->categ < 4) ? asm_arg_str[e->categ] : "ERROR!",
    value,
    e->is.imm ? '1' : '0',
    e->is.value ? '1' : '0',
    e->is.typed ? '1' : '0',
    e->is.ignore ? '1' : '0',
    e->is.target ? '1' : '0',
    e->is.gaddr ? '1' : '0',
    e->is.allocd ? '1' : '0',
    e->is.release ? '1' : '0'
  );
}

/* Checks that the given expression has type */
Task Expr_Must_Have_Type(Expr *e) {
  if (! e->is.typed) {
    MSG_ERROR("Expression has no type.");
    return Failed;
  }
  return Success;
}

Task Expr_Must_Have_Value(Expr *e) {
  if (! (e->is.typed && e->is.value)) {
    MSG_ERROR("Expression has no value.");
    return Failed;
  }
  return Success;
}

/* Converts an expression into a pointer */
void Expr_To_Ptr(Expr *e) {
  assert(e->categ != CAT_IMM);
  switch(e->categ) {
  case CAT_GREG:
  case CAT_LREG:
    /* *e is a register o a local/global variable:
     * we have to pass from the object being referenced by roN
     * (roN is the N-th object register) to o[roN]
     */
    e->is.gaddr = (e->categ == CAT_GREG ? 1 : 0);
    e->categ = CAT_PTR;
    e->addr = e->value.i;
    e->value.i = 0;
    e->is.imm = e->is.ignore = 0;
    e->is.value = e->is.target = 1;
    e->is.allocd = 0; e->is.release = 1;
    return;

  case CAT_PTR:
    /* *e is already a pointer (i.e. something like o[roN+M]
     * There is nothing to do then!
     */
    return;
  default:
    assert(0);
  }
}

/* Given a Point expression and the name of its possible member
 * ("x" or "y"), constructs an expression for the corresponding
 * member in *m. *s is then released.
 */
static Task Expr_Point_Member(Expr *m, Expr *s, Name *m_name) {
  assert(s->resolved < NUM_INTRINSICS);
  if (s->resolved != TYPE_POINT) goto member_not_found;

  if (m_name->length == 1) {
    Int addr;
    switch(tolower(*(m_name->text))) {
    case 'x':
      TASK( Cmp_Expr_To_LReg(s) );
      TASK( Cmp_Complete_Ptr_1(s) );
      Cmp_Assemble(ASM_PPTRX_P, s->categ, s->value.i);
      break;
    case 'y':
      TASK( Cmp_Expr_To_LReg(s) );
      TASK( Cmp_Complete_Ptr_1(s) );
      Cmp_Assemble(ASM_PPTRY_P, s->categ, s->value.i);
      break;
    default:
      goto member_not_found;
    }
    TASK( Cmp_Expr_Destroy_Tmp(s) );

    /* ATTENZIONE: devo mettere ro0 in un registro locale temporaneo */
    if ( (addr = Reg_Occupy(TYPE_OBJ)) < 1 ) return Failed;
    Cmp_Assemble(ASM_MOV_OO, CAT_LREG, addr, CAT_LREG, 0);

    m->resolved = m->type = TYPE_REAL;
    m->categ = CAT_PTR;
    m->is.gaddr = 0;
    m->addr = addr;
    m->value.i = 0;
    m->is.imm = m->is.ignore = 0;
    m->is.value = m->is.target = 1;
    m->is.allocd = 0;
    m->is.release = 1;
    return Success;
  }

member_not_found:
  MSG_ERROR( "'%N' is not an intrinsic member of '%s'!",
   m, Tym_Type_Name(s->type) );
  return Failed;
}


/* Given a structure expression and the name of its member,
 * constructs an expression for the corresponding member in *m.
 * *s is then released.
 */
Task Expr_Struc_Member(Expr *m, Expr *s, Name *m_name) {
  Type t = s->resolved, m_type, tm;
  Int m_addr;
  char *str_m_name;

  if (! s->is.value) {
    MSG_ERROR("Requested member '%N' of a non-valued expression!", m_name);
    return Failed;
  }

  /* Determino se si tratta di un oggetto intrinseco */
  if (t < NUM_INTRINSICS)
    return Expr_Point_Member(m, s, m_name);

  str_m_name = Name_To_Str(m_name);
  TS_Member_Find(cmp->ts, & tm, s->type, str_m_name);
  Mem_Free(str_m_name);
  if (tm == TYPE_NONE) {
    MSG_ERROR("'%s' has no member with name '%N'",
     Tym_Type_Name(s->type), m_name);
    return Failed;
  }

  TS_Member_Get(cmp->ts, & m_type, & m_addr, tm);

  *m = *s;
  Expr_To_Ptr(m);
  m->type = m_type;
  m->resolved = Tym_Type_Resolve_All(m_type);
  m->value.i += m_addr;
  return Success;
}

Task Expr_Array_Member(Expr *member, Expr *array, Expr *index) {
  Type m_type;
  Int m_size;
  TASK( Expr_Must_Have_Type(array) );
  TASK( Expr_Must_Have_Value(index) );
  TASK( TS_Array_Member(cmp->ts, & m_type, array->type, (Int *) NULL) );
  if (index->resolved != TYPE_INT) {
    MSG_ERROR("Indices of arrays must be integers.");
    return Failed;
  }
  *member = *array;
  Expr_To_Ptr(member);
  member->type = m_type;
  member->resolved = Tym_Type_Resolve_All(m_type);
  m_size = TS_Size(cmp->ts, m_type);
  if (index->categ == CAT_IMM) {
    member->value.i += m_size*index->value.i;
  } else {
    MSG_ERROR("Index of array is non-constant: not implemented yet!");
    member->value.i = 0;
  }
  return Success;
}

/** This function gets the parent and the child of the given procedure
 * out of the global registers used for passing the respective pointers.
 * This function should be used at the beginning of a function
 * to construct the expressions for the child and parent.
 */
void Expr_Parent_And_Child(Expr *e_parent, Expr *e_child, Type t_proc) {
  Type t_parent, t_child;
  TS_Procedure_Info(cmp->ts, & t_parent, & t_child, (int *) NULL,
   (UInt *) NULL, t_proc);

  Expr_New_Value(e_parent, t_parent);
  if (e_parent->is.value) {
    /* Here we occupy a TYPE_OBJ even if the type is TYPE_INT, etc.
     * This is because we have to get the object from the register go1
     */
    e_parent->is.target = 1;
    e_parent->categ = CAT_PTR;
    e_parent->is.gaddr = 0;
    e_parent->addr = Reg_Occupy(TYPE_OBJ);
    e_parent->value.i = 0;
    Cmp_Assemble(ASM_MOV_OO, CAT_LREG, e_parent->addr, CAT_GREG, 1);
  }

  Expr_New_Value(e_child, t_child);
  if (e_child->is.value) {
    /* See previous comment! */
    e_child->is.target = 1;
    e_child->categ = CAT_PTR;
    e_child->is.gaddr = 0;
    e_child->addr = Reg_Occupy(TYPE_OBJ);
    e_child->value.i = 0;
    Cmp_Assemble(ASM_MOV_OO, CAT_LREG, e_child->addr, CAT_GREG, 2);
  }
}


/******************************************************************************/


#if 0
/** Put inside *e a the given immediate Char value. */
void Expr_New_Imm_Char(Expression *e, Char c) {
  Expr_New_Container(e, TYPE_CHAR, CONTAINER_IMM);
  e->value.i = (Intg) c;
}

/** Put inside *e a the given immediate Int value. */
void Cmp_Expr_New_Imm_Intg(Expression *e, Int i) {
  Expr_New_Container(e, TYPE_INT, CONTAINER_IMM);
  e->value.i = i;
}

/** Put inside *e a the given immediate Real value. */
void Expr_New_Imm_Real(Expression *e, Real r) {
  Expr_New_Container(e, TYPE_REAL, CONTAINER_IMM);
  e->value.r = r;
}

/** Put inside *e a the given immediate Point value. */
void Expr_New_Imm_Point(Expression *e, Point *p) {
  Expr_New_Container(e, TYPE_POINT, CONTAINER_IMM);
  e->value.p = *p;
}

/** Create a new empty container.
 * NOTE: At the end this should substitute Cmp_Expr_LReg and Cmp_Expr_Create.
 */
Task Expr_New_Container(Expression *e, Intg type, Container *c) {
  int intrinsic;
  Intg type_of_register, resolved;

  e->is.typed = 1;
  e->is.value = 1;
  e->is.ignore = 0;
  e->is.allocd = 0;
  e->type = type;
  e->resolved = resolved = Tym_Type_Resolve_All(type);
  intrinsic = (resolved < NUM_INTRINSICS);
  type_of_register = (intrinsic) ? resolved : TYPE_OBJ;

  e->is.imm = 0;
  e->is.target = 0;
  e->is.release = 0;

  switch( c->type_of_container ) {
  case CONTAINER_TYPE_IMM:
    e->is.imm = 1;
    e->categ = CAT_IMM;
    return Success;
    break;

  case CONTAINER_TYPE_LREG:
    e->categ = CAT_LREG;
    if ( c->which_one < 0 ) {
      /* Automatically choses the local register */
      if ( (e->value.i = Reg_Occupy(type_of_register)) < 1 ) return Failed;
      e->is.release = 1;
      return Success;

    } else {
      /* The user wants a particolar register to be chosen */
      e->value.i = c->which_one;
      return Success;
    }
    break;

  case CONTAINER_TYPE_LVAR:
    e->categ = CAT_LREG;
    e->is.target = 1;
    if ( c->which_one < 0 ) {
      /* Automatically choses the local variables */
      if ( (e->value.i = -Var_Occupy(type_of_register, cmp_box_level)) >= 0 ) return Failed;
      return Success;

    } else {
      /* The user wants a particolar variable to be chosen */
      e->value.i = c->which_one;
      return Success;
    }
    break;

  case CONTAINER_TYPE_GREG:
    e->categ = CAT_GREG;
    e->value.i = c->which_one;
    return Success;
    break;

  case CONTAINER_TYPE_GVAR:
    e->categ = CAT_GREG;
    e->value.i = -(c->which_one);
    return Success;
    break;

  default:
    MSG_ERROR("Cmp_Expr_Container_New: wrong type of container!");
  }
  return Failed;
}
#endif
