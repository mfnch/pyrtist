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

/** @file box.h
 * @brief The functions to handle declarations and instantiations of boxes.
 *
 * Some words.
 */

#ifndef _COMPILER_H
/* This file must be included only by "compiler.h" */
#  include "compiler.h"

#else
#  ifndef _BOX_H
#    define _BOX_H

typedef struct {
  UInt cur_proc_num;
  Int num_defs; /**< 0 in global context, 1 inside procedure definitions. */
  Array *box;
} BoxStack;

/** @brief This structure describes an opened box
 */
typedef struct {
  struct {
    unsigned int definition : 1; /**< instance or definition of procedure? */
    unsigned int second : 1; /**< This is 1 only if it is a non-creation box */
  } is;
  UInt   proc_num;     /**< Number of the procedure where the box is */
  UInt   head_sym_num; /**< Symbol ID associated with the header of the proc. */
  Int    type;         /**< Type of the box */
  Expr   parent;       /**< Expression associated with the box */
  Expr   child;        /**< Expression associated with the child of a procedure */
  Symbol *syms;        /**< Child symbols which belong to this box */
  int    label_begin,  /**< Labels located at the beginning */
         label_end;    /**< and at the end of the box */
} Box;

Task Box_Init(void);
void Box_Destroy(void);
Int Box_Depth(void);
Int Box_Def_Num(void);

/** This function calls a procedure without value, such as (;), ([) or (]).
 * It does not complain if such a procedure is not defined for 'type'.
 * In such cases the behaviour is the following: if 'auto_define' is 1,
 * then a procedure is automatically declared (the user then has to define
 * it later), otherwise the procedure call is ignored.
 * In any case '*found' is set to 1 if the procedure has been somehow called.
 */
Task Box_Call_Void_Proc(int *found, Type type, int auto_define);

Task Box_Def_Begin(Int proc_type);
Task Box_Def_End(void);
Task Box_Main_Begin(void);
void Box_Main_End(void);
Task Box_Instance_Begin(Expr *e, int kind);
Task Box_Instance_End(Expr *e);
Intg Box_Search_Opened(Intg type, Intg depth);

/** This function returns the pointer to the structure Box
 * corresponding to the box with depth 'depth'.
 * NOTE: Specifying a negative value for 'depth' is equivalent to specify
 *  the value 0.
 */
Task Box_Get(Box **box, Intg depth);

/** Create in '*e_parent' the expression corresponding to the parent
 * of the box whose depth level is 'depth'
 * NOTE: Specifying a negative value for 'depth' is equivalent to specify
 *  the value 0.
 */
Task Box_Parent_Get(Expr *e_parent, Int depth);

/** Create in '*e_child' the expression corresponding to the child
 * of the box whose depth level is 'depth'
 * NOTE: Specifying a negative value for 'depth' is equivalent to specify
 *  the value 0.
 */
Task Box_Child_Get(Expr *e_child, Int depth);

/** Create in '*e_parent' the expression corresponding to level-th parent
 * of the box whose depth level is 'depth'.
 * NOTE: the first level parent is just the child (which means that
 *  for level=1, the function is equivalent to Box_Child_Get),
 *  the second level parent is just the parent (for level=2 this function
 *  becomes identical to Box_Parent_Get).
 *  For level=3, the function returns the subtype parent of the expression
 *  which would be returned for level=2 (and has to be a subtype).
 *  For level=n+1, the function returns the subtype parent of the expression
 *  which would be returned for level=n (and has to be a subtype).
 */
Task Box_NParent_Get(Expr *e_parent, Int level, Int depth);

Task Sym_Explicit_New(Symbol **sym, Name *nm, Intg depth);
#  endif
#endif
