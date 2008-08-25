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

/** @file expr.h
 * @brief The functions to handle declarations and instantiations of boxes.
 *
 * Some words.
 */

#ifndef _EXPR_H
#  define _EXPR_H

#  include "typesys.h"
#  include "virtmach.h"
#  include "container.h"


typedef struct {
  struct {
    unsigned int imm     : 1; /* l'espressione e' immediata? */
    unsigned int value   : 1; /* Possiede un valore determinato? */
    unsigned int typed   : 1; /* Possiede tipo? */
    unsigned int ignore  : 1; /* Va ignorata o passata alla box? */
    unsigned int target  : 1; /* si puo' assegnare un valore all'espressione?*/
    unsigned int gaddr   : 1; /* addr e' un registro globale (o locale)? */
    unsigned int allocd  : 1; /* l'oggetto e' stato allocato? (va liberato?) */
    unsigned int release : 1; /* il registro va rilasciato automaticamente? */
    unsigned int error   : 1; /* An invalid expression coming from an error
                                 which has already been communicated to
                                 the user: we should propagate it silently! */
  } is;

  Int    addr;
  Int    type;     /* The type of the expression */
  Int    resolved; /* = Tym_Type_Resolve_All(type) */
  AsmArg  categ;
  union {
    Int  i;
    Real  r;
    Point p;
    Int   reg;
    Name  nm;
  } value;

} Expr;

/** For backward compatibility */
#define Expression Expr

/** Return the error state of the Expr referenced by the pointer 'e' */
#define Expr_Is_Error(e) ((e)->is.error)

/** Set *e to be the result of an error, which has already beed signaled
 * to the user and thus need to be propagated silently.
 */
void Expr_New_Error(Expr *e);

/** Clear the content of the expression '*e'. This function must be called
 * by every function which creates an Expr type. Rationale: if we add new
 * entries to Expr we can just edit this function to be sure that every
 * Expr instance is initialised correctly.
 */
void Expr_Background(Expr *e);

/** Create a new type-expression */
void Expr_New_Type(Expr *e, Int type);

/** Create a new Void expression */
void Expr_New_Void(Expr *e);

/** Create an expression with value, corresponding to a local register 0
 * having a suitable type.
 */
void Expr_New_Value(Expr *e, Type t);

/** Create a new immediate character expression. */
void Expr_New_Imm_Char(Expr *e, Char c);

/** Create a new immediate int expression. */
void Expr_New_Imm_Int(Expr *e, Int i);

/** Create a new immediate real expression. */
void Expr_New_Imm_Real(Expr *e, Real r);

/** Create a new immediate point expression. */
void Expr_New_Imm_Point(Expr *e, Point *p);

/** See the function Expr_Attr_Set */
typedef enum {
  EXPR_ATTR_TARGET=1,
  EXPR_ATTR_IGNORE=2,
  EXPR_ATTR_RELEASE=4,
  EXPR_ATTR_ALLOCD=8,
  EXPR_ATTR_ERROR=16
} ExprAttr;

/** Change the attributes of an expression.
 * mask specifies what attributes have to be changed.
 * value specifies the values of these attributes.
 * Both the parameters are supposed to be obtained combining one or more
 * EXPR_ATTR_* attributes with the bitwise or operator |.
 */
void Expr_Attr_Set(Expr *e, Int mask, Int value);

/** Print the content of the given expression */
void Expr_Print(Expr *e, FILE *out);

/** Checks that the given expression has type */
Task Expr_Must_Have_Type(Expr *e);

/** Checks that the given expression has value */
Task Expr_Must_Have_Value(Expr *e);

/** Get/Set the container of the expression *e. These function will disappear
 * as soon as Expr will be asjusted to contain a Cont object.
 */
void Expr_Cont_Get(Cont *c, Expr *e);
void Expr_Cont_Set(Expr *e, Cont *c);

/** Returns the allocation type for the given expression.
 * The allocation type is an integer associated to the type, which identifies
 * the allocation/deallocation mechanism: types with the same allocation
 * type are destroyed by calling the same procedure.
 */
Int Expr_Allocation_Type(Expr *e);

/** Change the type of an expression if asmissible (using Cont_Cast).
 * This is often used to transform a generic pointer into a pointer
 * to a specific type.
 */
void Expr_Cast(Expr *e, Type t);

/** Convert an object register/variable into a pointer.
 * example: this correspond to the transition from ro3 --> o[ro3]
 */
void Expr_To_Ptr(Expr *e);

/** Given an expression *s, construct in *m the expression
 * corresponding to its member with name m_name.
 * Obviously *s has to be a structure and m_name must be
 * the name of a member of this structure.
 * Once the member has been obtained *s is released.
 */
Task Expr_Struc_Member(Expr *m, Expr *s, Name *m_name);

/** Given an array expression and an index expression,
 * build up an expression containing the corresponding member
 * of the array.
 */
Task Expr_Array_Member(Expr *memb, Expr *array, Expr *index);


/** This function gets the parent and the child of the given procedure
 * out of the global registers used for passing the respective pointers.
 * This function should be used at the beginning of a function
 * to construct the expressions for the child and parent.
 */
void Expr_Parent_And_Child(Expr *e_parent, Expr *e_child, Type t_proc);

/** Called when the / stands as a prefix for the expression *e. */
Task Expr_Ignore(Expr *e);

/** This function creates in 'subtype' a the subtype of kind 'child'
 * of 'parent'.
 */
Task Expr_Subtype_Create(Expr *subtype, Expr *parent, Name *child);

/** Creates an expression containing the parent of 'subtype'.
 */
Task Expr_Subtype_Get_Parent(Expr *parent, Expr *subtype);

/** Creates an expression containing the child of 'subtype'.
 */
Task Expr_Subtype_Get_Child(Expr *child, Expr *subtype);

/** If *e is a subtype, then it is converted to its own child;
 * otherwise nothing is done.
 */
Task Expr_Resolve_Subtype(Expr *e);

/** This type is used to specify a container (see the macros CONTAINER_...) */
typedef struct {
  int type_of_container;
  int which_one;
  int addr;
} Container;

enum {
  CONTAINER_TYPE_IMM = 0,
  CONTAINER_TYPE_LREG, CONTAINER_TYPE_LVAR,
  CONTAINER_TYPE_GREG, CONTAINER_TYPE_GVAR,
  CONTAINER_TYPE_GPTR,
  CONTAINER_TYPE_LRPTR, CONTAINER_TYPE_LVPTR,
  CONTAINER_TYPE_ARG, CONTAINER_TYPE_STACK
};

/** Create a new empty container.
 * NOTE: At the end this should substitute Cmp_Expr_LReg.
 */
void Expr_Container_New(Expr *e, Type type, Container *c);

/** Allocate space for an empty container.
 * When expressions are created with Expr_Container_New, only the container
 * is created, while the object has not a space in memory associated with it.
 * This function allocates space for the passed expression.
 * Obviously only non-intrinsic types require to be allocated:
 * intrinsic types can be stored inside their container (register).
 */
void Expr_Alloc(Expr *e);

/* These macros are used in functions such as Expr_Container_New
 * or Expr_Container_Change to specify the container for an expression.
 */

/* The container is immediate (contains directly the value
 * of the integer, real, etc.)
 */
#define CONTAINER_IMM (& (Container) {CONTAINER_TYPE_IMM, 0})

/* In the function Cmp_Expr_Container_Change this will behaves similar
 * to CONTAINER_LREG_AUTO, but with one difference. If the expression
 * is already contained in a local register:
 *  - macro CONTAINER_LREG_AUTO: will keep the expression
 *    in its current container;
 *  - macro CONTAINER_LREG_FORCE: will choose another local register
 *    and use this as the new container.
 */
#define CONTAINER_LREG_FORCE (& (Container) {CONTAINER_TYPE_LREG, -2})

/* A local register automatically chosen (and reserved) */
#define CONTAINER_LREG_AUTO (& (Container) {CONTAINER_TYPE_LREG, -1})

/* A well defined local register */
#define CONTAINER_LREG(n) \
  (& (Container) {CONTAINER_TYPE_LREG, (n) > 0 ? (n) : 0})

/* A well defined local variable */
#define CONTAINER_LVAR(n) \
  (& (Container) {CONTAINER_TYPE_LVAR, (n) > 0 ? (n) : 0})

/* A local variable automatically chosen (and reserved) */
#define CONTAINER_LVAR_AUTO \
  (& (Container) {CONTAINER_TYPE_LVAR, -1})

/* A well defined global register */
#define CONTAINER_GREG(n) \
  (& (Container) {CONTAINER_TYPE_GREG, (n) > 0 ? (n) : 0})

/* A well defined global variable */
#define CONTAINER_GVAR(n) \
  (& (Container) {CONTAINER_TYPE_GVAR, (n) > 0 ? (n) : 0})

/* A global variable automatically chosen (and reserved) */
#define CONTAINER_GVAR_AUTO \
  (& (Container) {CONTAINER_TYPE_GVAR, -1})

/* Global pointer to object */
#define CONTAINER_GPTR(gr, offset) \
  (& (Container) {CONTAINER_TYPE_GPTR, offset, gr})

/* Local pointer to object */
#define CONTAINER_LRPTR(lr, offset) \
  (& (Container) {CONTAINER_TYPE_LRPTR, lr, offset})

/* Local pointer to object */
#define CONTAINER_LRPTR_AUTO \
  (& (Container) {CONTAINER_TYPE_LRPTR, 0, -1})

/* A well defined global variable */
#define CONTAINER_ARG(num) \
  (& (Container) {CONTAINER_TYPE_ARG, num})

/* Use this macro if you want to specify to use the stack as a container */
#define CONTAINER_STACK (& (Container) {CONTAINER_TYPE_STACK, 0})

#endif
