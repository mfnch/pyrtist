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
#include "container.h"

/* This fuction creates an expression with type, but without value.
 */
void Expr_Background(Expr *e) {
  e->type = TS_TYPE_NONE;
  e->resolved = TS_TYPE_NONE;
  e->value.i = 0;
  e->addr = 0;
  e->categ = 0;
  e->is.typed = 0;
  e->is.value = 0;
  e->is.ignore = 0;
  e->is.imm = 0;
  e->is.target = 0;
  e->is.gaddr = 0;
  e->is.allocd = 0;
  e->is.release = 0;
  e->is.error = 0;
}

void Expr_New_Type(Expr *e, Int type) {
  Expr_Background(e);
  e->type = type;
  e->resolved = Tym_Type_Resolve_All(type);
  e->is.typed = 1;
}

/** Put inside *e a the Void value. */
void Expr_New_Void(Expr *e) {
  Expr_Background(e);
  e->type = TYPE_VOID;
  e->resolved = TYPE_VOID;
  e->is.typed = 1;
}

/** Create an expression with value, corresponding to a local register 0
 * having a suitable type.
 */
void Expr_New_Value(Expr *e, Type t) {
  Expr_Background(e);
  e->is.typed = 1;
  e->type = t;
  e->resolved = TS_Core_Type(cmp->ts, t);
  e->is.value = (TS_Size(cmp->ts, t) > 0) ? 1 : 0;
  e->categ = CAT_LREG;
  e->value.i = 0;
  e->is.imm = e->is.ignore = 0;
  e->is.target = e->is.gaddr = 0;
  e->is.allocd = e->is.release = 0;
  e->addr = 0;
}

void Expr_New_Imm_Char(Expr *e, Char c) {
  Expr_Container_New(e, TYPE_CHAR, CONTAINER_IMM);
  e->value.i = (Int) c;
}

void Expr_New_Imm_Int(Expr *e, Int i) {
  Expr_Container_New(e, TYPE_INTG, CONTAINER_IMM);
  e->value.i = i;
}

void Expr_New_Imm_Real(Expr *e, Real r) {
  Expr_Container_New(e, TYPE_REAL, CONTAINER_IMM);
  e->value.r = r;
}

void Expr_New_Imm_Point(Expr *e, Point *p) {
  Expr_Container_New(e, TYPE_POINT, CONTAINER_IMM);
  e->value.p = *p;
}

void Expr_New_Error(Expr *e) {
  Expr_Background(e);
  Expr_Attr_Set(e, EXPR_ATTR_ERROR, EXPR_ATTR_ERROR);
}

/** Change the attributes of an expression
 */
void Expr_Attr_Set(Expr *e, Int mask, Int value) {
  if ((mask & EXPR_ATTR_TARGET) != 0)
    e->is.target = ((value & EXPR_ATTR_TARGET) != 0);
  if ((mask & EXPR_ATTR_IGNORE) != 0)
    e->is.ignore = ((value & EXPR_ATTR_IGNORE) != 0);
  if ((mask & EXPR_ATTR_RELEASE) != 0)
    e->is.release = ((value & EXPR_ATTR_RELEASE) != 0);
  if ((mask & EXPR_ATTR_ALLOCD) != 0)
    e->is.allocd = ((value & EXPR_ATTR_ALLOCD) != 0);
  if ((mask & EXPR_ATTR_ERROR) != 0)
    e->is.allocd = ((value & EXPR_ATTR_ERROR) != 0);
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

  if ( e->is.error ) {
    fprintf(out, "ERROR_Expr\n");
    return;
  }

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
      sprintf(buffer, "INT("SInt")", e->value.i);
      break;
    case TYPE_REAL:
      sprintf(buffer, "REAL("SReal")", e->value.r);
      break;
    default:
      value = "UNKNOWN";
      break;
    }

  } else {
    sprintf(buffer, "INT("SInt")", e->value.i);
  }

  fprintf(out,
    "Expression(type="SInt"=\"%s\", resolved="SInt"=\"%s\", "
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
    MSG_ERROR("Expression '%N' has no type.", & e->value.nm);
    return Failed;
  }
  return Success;
}

Task Expr_Must_Have_Value(Expr *e) {
  if (e->is.typed && e->is.value) return Success;
  if (! e->is.typed) {
    MSG_ERROR("Expression '%N' has no type.", & e->value.nm);
    return Failed;

  } else {
    MSG_ERROR("Expression of type '%~s' has no value.",
              TS_Name_Get(cmp->ts, e->type));
    return Failed;
  }
}

void Expr_Cont_Get(Cont *c, const Expr *e) {
  assert(e->resolved >= 0);
  c->categ = e->categ;
  c->type = e->resolved < TYPE_OBJ ? e->resolved : TYPE_OBJ;
  c->reg = e->value.i;
  c->ptr_reg = e->addr;
  c->extra = & e->value.i;
  c->flags.ptr_is_greg = e->is.gaddr;
}

void Expr_Cont_Set(Expr *e, const Cont *c) {
  assert(e->resolved >= 0);
  assert((e->resolved >= TYPE_OBJ) == (c->type == TYPE_OBJ));
  e->categ = c->categ;
  e->value.i = c->reg;
  e->addr = c->ptr_reg;
  e->is.gaddr = c->flags.ptr_is_greg;
  /* c->extra ??? */
}

void Expr_Container_Move(const Expr *dest, const Expr *src) {
  Cont dest_c, src_c;
  Expr_Cont_Get(& dest_c, dest);
  Expr_Cont_Get(& src_c, src);
  Cont_Move(& dest_c, & src_c);
}

/** Returns the allocation type for the given expression.
 * The allocation type is an integer associated to the type, which identifies
 * the allocation/deallocation mechanism: types with the same allocation
 * type are destroyed by calling the same procedure.
 */
Int Expr_Allocation_Type(Expr *e) {
  return e->type;
  return 0; /* This will be done better in the near future */
}

void Expr_Cast(Expr *e, Type t) {
  Cont c;
  Type r = TS_Core_Type(cmp->ts, t);
  ContType ct = (r > TYPE_OBJ) ? TYPE_OBJ : r;
  Expr_Cont_Get(& c, e);
  Cont_Ptr_Cast(& c, ct);
  e->type = t;
  e->resolved = r;
  Expr_Cont_Set(e, & c);
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

    m->is.typed = 1;
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
  MSG_ERROR("'%N' is not an intrinsic member of '%s'!",
            m_name, Tym_Type_Name(s->type));
  return Failed;
}


/* Given a structure expression and the name of its member,
 * constructs an expression for the corresponding member in *m.
 * *s is then released.
 */
Task Expr_Struc_Member(Expr *m, Expr *s, Name *m_name) {
  Type m_type, tm;
  Int m_addr;
  char *str_m_name;

  if (! s->is.value) {
    MSG_ERROR("Requested member '%N' of a non-valued expression!", m_name);
    return Failed;
  }

  /* If s is a subtype, convert it automatically to its child. */
  Expr_Resolve_Subtype(s);

  /* Determino se si tratta di un oggetto intrinseco */
#if 0
  t = TS_Core_Type(cmp->ts, s->type);
#endif

  if (s->resolved < NUM_INTRINSICS)
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

void Expr_Struc_Iter(Expr *member, Expr *iter, int *n) {
  Int member_type = iter->type;
  ASSERT_TASK( Tym_Structure_Get(& member_type) );

  if (TS_Kind(cmp->ts, iter->type) == TS_KIND_STRUCTURE) {
    Expr first;

    *n = Tym_Struct_Get_Num_Items(iter->type);
    if (*n == 0 ) return;
    assert(*n > 0);

    /* Convert the expression into a pointer expression */
    first = *iter;
    Expr_To_Ptr(& first);

    /* Just change the type, and we get then the first member! */
    first.type = member_type;
    first.resolved = TS_Core_Type(cmp->ts, member_type);

    first.is.ignore = 1; first.is.target = 1;
    first.is.allocd = 0; /* <-- important!!! */
    first.is.release = 0;
    *iter = first;
    /* continue... */


  } else {
    assert( *n > 0 );
    --(*n);
    /* (n == 0) if and only if (member_type == TYPE_NONE) */
    if ((*n == 0) != (member_type == TYPE_NONE)) {
      MSG_FATAL("Expr_Struc_Iter: internal error: *n = %d, member_type = %d",
                *n, member_type);
      return;
    }
    if (*n == 0) return;
    iter->value.i += Tym_Type_Size(iter->resolved);
    iter->type = member_type;
    iter->resolved = TS_Core_Type(cmp->ts, member_type);
  }

  *member = *iter;
  member->type = TS_Resolve_Once(cmp->ts, member->type, TS_KS_NONE);
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
    Cont cont_index;
    Int member_categ, member_reg;

    member_categ = (member->is.gaddr ? CAT_GREG : CAT_LREG);
    member_reg = member->addr;
    Expr_Cont_Get(& cont_index, index);
    Cont_Move(& CONT_NEW_LREG(TYPE_INT, 0), & cont_index);
    Cmp_Assemble(ASM_MUL_II, CAT_LREG, (Int) 0, CAT_IMM, (Int) m_size);
    Cmp_Assemble(ASM_ADD_O, member_categ, member_reg);

    MSG_ERROR("Index of array is non-constant: not implemented yet!");
  }
  return Success;
}

/** This function gets the parent and the child of the given procedure
 * out of the global registers used for passing the respective pointers.
 * This function should be used at the beginning of a function
 * to construct the expressions for the child and parent.
 */
void Expr_Parent_And_Child(Expr *parent_e, Expr *child_e,
                           Type parent_t, Type child_t) {
  Expr_New_Value(parent_e, parent_t);
  if (parent_e->is.value) {
    /* Here we occupy a TYPE_OBJ even if the type is TYPE_INT, etc.
     * This is because we have to get the object from the register go1
     */
    parent_e->is.target = 1;
    parent_e->categ = CAT_PTR;
    parent_e->is.gaddr = 0;
    parent_e->addr = Reg_Occupy(TYPE_OBJ);
    parent_e->value.i = 0;
    Cmp_Assemble(ASM_MOV_OO, CAT_LREG, parent_e->addr, CAT_GREG, 1);
  }

  Expr_New_Value(child_e, child_t);
  if (child_e->is.value) {
    /* See previous comment! */
    child_e->is.target = 1;
    child_e->categ = CAT_PTR;
    child_e->is.gaddr = 0;
    child_e->addr = Reg_Occupy(TYPE_OBJ);
    child_e->value.i = 0;
    Cmp_Assemble(ASM_MOV_OO, CAT_LREG, child_e->addr, CAT_GREG, 2);
  }
}

/* Called when the / stands as a prefix for an expression. */
Task Expr_Ignore(Expr *e) {
  return Cmp_Expr_Destroy_Tmp(e);
}

/* This function creates in 'subtype' a the subtype of kind 'child'
 * of 'parent'.
 */
Task Expr_Subtype_Create(Expr *subtype, Expr *parent, Name *child) {
  Type found_subtype, child_type;
  Expr child_expr; /* child_ptr, parent_ptr */
  int not_void_parent, not_void_child;

  if (!parent->is.typed) {
    MSG_ERROR("The symbol '%N' has no type and cannot have a subtype '%N'",
              & parent->value.nm, child);
    return Failed;
  }

  /* If the method has not been found, it could be a method of the child
   * of the type. We then resolve the type to its child and try again.
   */
  for(;;) {
    TS_Subtype_Find(cmp->ts, & found_subtype, parent->type, child);
    if (found_subtype != TS_TYPE_NONE) break;
    if (TS_Is_Subtype(cmp->ts, parent->type)) {
      TASK( Expr_Resolve_Subtype(parent) );

    } else {
      MSG_ERROR("Type '%~s' has not a subtype of name '%N'",
                TS_Name_Get(cmp->ts, parent->type), child);
      return Failed;
    }
  }

  TS_Subtype_Get_Child(cmp->ts, & child_type, found_subtype);

  /* This is what we do in the following:
      malloc SB  (Allocate the child)
      mov roB, ro0

      malloc 8
      mov roS, ro0
      mov o[ro0], roA (roA is the parent)
      mov o[ro0+4], roB
  */

  not_void_parent = (TS_Size(cmp->ts, parent->type) > 0);
  not_void_child = (TS_Size(cmp->ts, child_type) > 0);

  /* Create the child object */
  if (not_void_child) {
    Expr_Container_New(& child_expr, child_type, CONTAINER_LRPTR_AUTO);
    Expr_Alloc(& child_expr);
  }

  /* Create the subtype object */
  Expr_Container_New(subtype, found_subtype, CONTAINER_LREG_AUTO);
  Expr_Alloc(subtype);

  if (not_void_parent) {
    Cont c_src, c_dest = CONT_NEW_LPTR(CONT_OBJ, subtype->value.i, 0);
    Expr_Cont_Get(& c_src, parent);
    Cont_Ptr_Create(& c_dest, & c_src);
  }

  if (not_void_child) {
    Cmp_Assemble(ASM_MOV_OO, CAT_PTR, sizeof(Obj), CAT_LREG, child_expr.addr);
    Cmp_Expr_Destroy_Tmp(& child_expr);
  }

  return Success;
}

Task Expr_Subtype_Get_Parent(Expr *parent, Expr *subtype) {
  Type parent_type;
  int not_void_parent;

  if (!subtype->is.typed) {
    MSG_ERROR("The symbol '%N' has no type!", & subtype->value.nm);
    return Failed;
  }

  if (!TS_Is_Subtype(cmp->ts, subtype->resolved)) {
    MSG_ERROR("Cannot get the parent of '%~s': this is not a subtype!",
              TS_Name_Get(cmp->ts, subtype->type));
    return Failed;
  }

  TS_Subtype_Get_Parent(cmp->ts, & parent_type, subtype->resolved);

  not_void_parent = (TS_Size(cmp->ts, parent_type) > 0);
  if (not_void_parent) {
    Expr_Container_New(parent, CONT_OBJ, CONTAINER_LREG_AUTO);
    Expr_Container_Move(parent, subtype);
    Expr_Cast(parent, parent_type);
    /* Need to make this a target, since child was obtained originally
     * by invoking Expr_Container_New with CONTAINER_LREG_AUTO and hence
     * is not considered a target, yet.
     */
    Expr_Attr_Set(parent, EXPR_ATTR_TARGET, EXPR_ATTR_TARGET);
    return Success;

  } else {
    Expr_New_Value(parent, parent_type);
    return Success;
  }
}

Task Expr_Subtype_Get_Child(Expr *child, Expr *subtype) {
  Type child_type;
  int not_void_child;

  if (!subtype->is.typed) {
    MSG_ERROR("The symbol '%N' has no type!", & subtype->value.nm);
    return Failed;
  }

  if (!TS_Is_Subtype(cmp->ts, subtype->resolved)) {
    MSG_ERROR("Cannot get the child of '%~s': this is not a subtype!",
              TS_Name_Get(cmp->ts, subtype->type));
    return Failed;
  }

  TS_Subtype_Get_Child(cmp->ts, & child_type, subtype->resolved);

  not_void_child = (TS_Size(cmp->ts, child_type) > 0);
  if (not_void_child) {
    Cont child_cont, subtype_cont;
    Expr_Container_New(child, CONT_OBJ, CONTAINER_LREG_AUTO);
    Expr_Cont_Get(& child_cont, child);
    Expr_Cont_Get(& subtype_cont, subtype);
    Cont_Ptr_Inc(& subtype_cont, & CONT_NEW_INT(sizeof(Obj)));
    Cont_Move(& child_cont, & subtype_cont);
    Expr_Cast(child, child_type);
    /* Need to make this a target, since child was obtained originally
     * by invoking Expr_Container_New with CONTAINER_LREG_AUTO and hence
     * is not considered a target, yet.
     */
    Expr_Attr_Set(child, EXPR_ATTR_TARGET, EXPR_ATTR_TARGET);
    return Success;

  } else {
    Expr_New_Value(child, child_type);
    return Success;
  }
}

Task Expr_Resolve_Subtype(Expr *e) {
  Expr child;
  int ignore = e->is.ignore;
  if (!e->is.typed) return Success;
  if (!TS_Is_Subtype(cmp->ts, e->resolved)) return Success;
  TASK( Expr_Subtype_Get_Child(& child, e) );
  Cmp_Expr_Destroy_Tmp(e);
  *e = child;
  e->is.ignore = ignore;
  return Success;
}

/* Create a new empty container.
 * NOTE: At the end this should substitute Cmp_Expr_LReg.
 */
void Expr_Container_New(Expr *e, Type type, Container *c) {
  int intrinsic;
  Int type_of_register, resolved;

  Expr_Background(e);
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
    return; break;

  case CONTAINER_TYPE_LREG:
    e->categ = CAT_LREG;
    if ( c->which_one < 0 ) {
      /* Automatically choses the local register */
      if ( (e->value.i = Reg_Occupy(type_of_register)) < 1 ) {
        MSG_FATAL("Expr_Container_New: Reg_Occupy failed!");
      }
      e->is.release = 1;
      return;

    } else {
      /* The user wants a particolar register to be chosen */
      e->value.i = c->which_one;
      return;
    }
    break;

  case CONTAINER_TYPE_GVAR:
    e->categ = CAT_GREG;
    e->is.target = 1;
    if (c->which_one < 0) {
      /* Automatically choses the local variables */
      if ( (e->value.i = -GVar_Occupy(type_of_register)) >= 0 ) {
        MSG_FATAL("Expr_Container_New: GVar_Occupy failed!");
      }
      return;

    } else {
      /* The user wants a particolar variable to be chosen */
      e->value.i = c->which_one;
      return;
    }
    break;

  case CONTAINER_TYPE_LVAR:
    e->categ = CAT_LREG;
    e->is.target = 1;
    if ( c->which_one < 0 ) {
      /* Automatically choses the local variables */
      if ( (e->value.i = -Var_Occupy(type_of_register, Box_Num())) >= 0 ) {
        MSG_FATAL("Expr_Container_New: Var_Occupy failed!");
      }
      return;

    } else {
      /* The user wants a particolar variable to be chosen */
      e->value.i = c->which_one;
      return;
    }
    break;

  case CONTAINER_TYPE_GREG:
    e->categ = CAT_GREG;
    e->value.i = c->which_one;
    return;
    break;

  case CONTAINER_TYPE_GPTR:
  case CONTAINER_TYPE_LRPTR:
  case CONTAINER_TYPE_LVPTR:
  {
    int is_gaddr = (c->type_of_container == CONTAINER_TYPE_GPTR);

    e->is.gaddr = is_gaddr;
    e->is.target = 1;
    e->categ = CAT_PTR;
    e->addr = c->addr;         /* X in o[roX + N]*/
    e->value.i = c->which_one; /* X in o[roN + X] */

    if (is_gaddr || c->addr >= 0) return;

    if (c->type_of_container == CONTAINER_TYPE_LRPTR) {
      if ( (e->addr = Reg_Occupy(TYPE_OBJ)) < 1 ) {
        MSG_FATAL("Expr_Container_New: Reg_Occupy failed!");
      }
      e->is.release = 1;
      return;

    } else {
      if ( (e->addr = -Var_Occupy(TYPE_OBJ, Box_Num())) >= 0 ) {
        MSG_FATAL("Expr_Container_New: Var_Occupy failed!");
      }
    }
    return;
    break;
  }

  default:
    MSG_FATAL("Expr_Container_New: wrong type of container!");
  }
}

void Expr_Alloc(Expr *e) {
  int is_intrinsic;
  Type t;
  Int size;

  assert(e->is.typed);
  assert(!e->is.allocd);

  t = e->type;
  size = TS_Size(cmp->ts, t);
  is_intrinsic = TS_Is_Fast(cmp->ts, e->type);

  if (size < 1) return;

  if (e->categ == CAT_PTR) {
    Int ptr_reg = e->addr;
    Int ptr_categ = (e->is.gaddr ? CAT_GREG : CAT_LREG);
    Int at = Expr_Allocation_Type(e);
    Cmp_Assemble(ASM_MALLOC_I, CAT_IMM, size, CAT_IMM, at);
    Cmp_Assemble(ASM_MOV_OO, ptr_categ, ptr_reg, CAT_LREG, 0);
    e->is.allocd = 1;

  } else {
    /* Intrinsic types are contained inside the appropriate registers
    * and the corresponding objects do not need to be allocated.
    * We obviously do not allocate space for types with size == 0.
    */
    if (is_intrinsic)
      return;

    else {
      Int at = Expr_Allocation_Type(e);
      /* If the object is of a user defined type, we must allocate it! */
      Cmp_Assemble(ASM_MALLOC_I, CAT_IMM, size, CAT_IMM, at);
      Cmp_Assemble(ASM_MOV_OO, e->categ, e->value.i, CAT_LREG, 0);
      e->is.allocd = 1;
    }
  }
}

static Int asm_mov[NUM_INTRINSICS] = {
  ASM_MOV_CC, ASM_MOV_II, ASM_MOV_RR, ASM_MOV_PP
};

Task Expr_Move(Expr *dest_e, Expr *src_e) {
  Int t, c;

  assert(dest_e != NULL && src_e != NULL);
  /*assert(dest_e->resolved == src_e->resolved);*/
  t = dest_e->resolved;
  c = dest_e->categ;
  assert(t >= 0);
  assert((src_e->is.typed) && (src_e->is.value) && (c != CAT_IMM));

  /* Qui devo controllare se il tipo ammette un mover user-defined! */

  if (t < NUM_INTRINSICS) {
    /* Sposto una quantita' intrinseca */
    int is_integer;

    is_integer = (src_e->resolved == TYPE_CHAR)
              || (src_e->resolved == TYPE_INT);
    if (src_e->categ == CAT_IMM && !is_integer) {
      ASSERT_TASK( Cmp_Complete_Ptr_1(dest_e) );
      switch ( t ) {
       case TYPE_REAL:
        Cmp_Assemble(ASM_MOV_Rimm,
                     c, dest_e->value.i, CAT_IMM, src_e->value.r);
        break;
       case TYPE_POINT:
        Cmp_Assemble(ASM_MOV_Pimm,
                     c, dest_e->value.i, CAT_IMM, src_e->value.p);
        break;
       default:
        MSG_FATAL("Internal error in Expr_Move");
        return Failed;
      }
      ASSERT_TASK( Cmp_Expr_Destroy_Tmp(src_e) );
      return Success;

    } else {
      TASK( Cmp_Complete_Ptr_2(dest_e, src_e) );
      Cmp_Assemble(asm_mov[t], dest_e->categ, dest_e->value.i,
                   src_e->categ, src_e->value.i );
      ASSERT_TASK( Cmp_Expr_Destroy_Tmp(src_e) );
      return Success;
    }

  } else {
    Cont dest_c, src_c, tmp_c;
    Expr tmp_e;

    /*if (src_e->resolved != dest_e->resolved) {
      MSG_ERROR("Type mismatch in object copy");
      return Failed;
    }*/

    Expr_Container_New(& tmp_e, dest_e->type, CONTAINER_LREG_AUTO);
    Expr_Cont_Get(& tmp_c, & tmp_e);
    Expr_Cont_Get(& dest_c, dest_e);
    Cont_Ptr_Create(& CONT_NEW_LREG(CONT_OBJ, tmp_e.value.i), & dest_c);

    Expr_Cont_Get(& src_c, src_e);
    Cont_Ptr_Create(& CONT_NEW_LREG(CONT_OBJ, 0), & src_c);

    Cmp_Assemble(ASM_MOV_II, CAT_LREG, (Int) 0,
                 CAT_IMM, (Int) Tym_Type_Size(tmp_e.type));
    Cmp_Assemble(ASM_MCOPY_OO, tmp_e.categ, tmp_e.value.i,
                 CAT_LREG, (Int) 0);
    ASSERT_TASK( Cmp_Expr_Destroy_Tmp(& tmp_e) );
    ASSERT_TASK( Cmp_Expr_Destroy_Tmp(src_e) );
    return Success;
  }
}
