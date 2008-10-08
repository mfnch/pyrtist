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
#  include "compiler.h"

#else
#  ifndef _BOX_H
#    define _BOX_H

typedef struct {
  UInt cur_sheet;
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
  UInt   sheet;        /**< Number of the procedure where the box is */
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
Int Box_Num(void);
Int Box_Def_Num(void);

/** Used to specify the Box we are referring to. For example, consider:
 *
 * CODE:  Int[ Real[ Int[ ..., *, ...]   ]   ]
 * DEPTH: 3  [  2  [ 1  [      0     ] 1 ] 2 ] 3
 *
 * where the asterix * indicates where we are in the code.
 * If we want to refer to a variable defined inside the first Int to the left
 * we should use a BoxDepth equal to 2. For the Real[] depth we should use 1,
 * and so on. BoxDepth allows to specify the scoping when referring to
 * variables.
 */
typedef Int BoxDepth;

/** The depth of the current Box, see BoxDepth */
#define BOX_DEPTH_UPPER 0

/** This function returns the pointer to the structure Box
 * corresponding to the box with depth 'depth'.
 * NOTE: Specifying a negative value for 'depth' is equivalent to specify
 *  the value 0 (BOX_DEPTH_UPPER).
 */
Box *Box_Get(BoxDepth depth);

/** Create in '*e_parent' the expression corresponding to the parent
 * of the box whose depth level is 'depth'
 */
Task Box_Parent_Get(Expr *e_parent, BoxDepth depth);

/** Create in '*e_child' the expression corresponding to the child
 * of the box whose depth level is 'depth'
 */
Task Box_Child_Get(Expr *e_child, BoxDepth depth);

/** Returns the depth of the n-th to last definition box: Box_Def_Depth(0)
 * returns the depth of the current (last) definition box (-1 if no definition
 * box has been opened, yet).
 */
BoxDepth Box_Def_Depth(int n);

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
Task Box_NParent_Get(Expr *e_parent, Int level, BoxDepth depth);

/** Invoked when a new procedure is being defined. */
Task Box_Procedure_Begin(Type parent, Type child, Type procedure);

/** Invoked to finalise and register a procedure created with
 * Box_Procedure_Begin.
 */
Task Box_Procedure_End(UInt *call_num);

/** Similar to Box_Procedure_Begin but also register the procedure
 * so that it can be found and used by Box code.
 */
Task Box_Def_Begin(Type parent, Type child, int kind);

Task Box_Def_End(void);

Task Box_Main_Begin(void);
void Box_Main_End(void);

/** Decides whether a function should emit any error messages
 * or just be silent.
 */
typedef enum {BOX_MSG_SILENT, BOX_MSG_VERBOSE} BoxMsg;

#if 0
/* OBSOLETE */
/** This function is an interface for the function TS_Procedure_Search.
 * Indeed, the purpose of the first is very similar to the purpose
 * of the second: the function searches the procedure of type 'procedure'
 * which belongs to the opened box 'b'.
 * If a suitable procedure is found, its actual type is assigned
 * to '*prototype' and its symbol number is put into '*sym_num'.
 * 'verbosity' decides whether or not warning messages should be emmitted.
 * RETURN VALUE: the function returns 1 if the procedure was found,
 *  otherwise it returns 0.
 */
int Box_Procedure_Search(Box *b, Int procedure, Type *prototype,
                         UInt *sym_num, BoxMsg verbosity);
#endif

/** This function handles the procedures of the box corresponding to depth.
 * It generates the assembly code that calls the procedure.
 * '*e' is the object passed to the box whose depth is 'depth'.
 * 'second' is 0, if the box has just been created, otherwise it is 1:
 * Example:
 *  b = Box[ ... ] <-- just created, second = 0
 *  a = b[ ... ]   <-- old box, second = 1
 * 'auto_define' allows the automatic definition of non found procedures
 * (the definition should then be provided by the user later in the code).
 * The function returns Success, even if the procedure is not found:
 * check the content of *found for this purpose.
 */
Task Box_Procedure_Call(Expr *child, BoxDepth depth, BoxMsg verbosity);

/** Temporary hacks */
Task Box_Hack1(Expr *parent, Expr *child, int kind, BoxMsg verbosity);
Task Box_Hack2(Expr *parent, Type child, int kind, BoxMsg verbosity);

/** This function calls a procedure without value, such as (;), ([) or (]).
 * It does not complain if such a procedure is not defined for 'type'.
 * In such cases the behaviour is the following: if 'auto_define' is 1,
 * then a procedure is automatically declared (the user then has to define
 * it later), otherwise the procedure call is ignored.
 * In any case '*found' is set to 1 if the procedure has been somehow called.
 */
Task Box_Procedure_Call_Void(Type type, BoxDepth depth, BoxMsg verbosity);

Task Box_Instance_Begin(Expr *e, int kind);
Task Box_Instance_End(Expr *e);
Int Box_Search_Opened(Int type, BoxDepth depth);

Task Sym_Explicit_New(Symbol **sym, Name *nm, BoxDepth depth);
#  endif
#endif
