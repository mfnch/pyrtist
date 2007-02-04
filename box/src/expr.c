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

#include "expr.h"
#include "compiler.h"

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
