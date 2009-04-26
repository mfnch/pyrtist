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

/** @file compiler.h
 * @brief The compiler of Box.
 *
 * A nice description...
 */

#ifndef _NEW_COMPILER_H
#  define _NEW_COMPILER_H

#include "types.h"
#include "array.h"
#include "virtmach.h"
#include "ast.h"
#include "registers.h"

typedef struct _box_cmp BoxCmp;

/** @brief The CmpProc object.
 */
typedef struct {
  struct {
    unsigned int
                sym      :1, /**< the procedure has an associated symbol */
                proc_id  :1, /**< the procedure has a procedure number */
                proc_name:1, /**< it has a name */
                call_num :1, /**< it has a call number */
                type     :1; /**< it has a type */
  } have;
  BoxCmp        *cmp;        /**< Compiler corresponding to the procedure */
  BoxVMSymID    sym;         /**< Symbol associated with the procedure */
  BoxVMProcNum  proc_id;     /**< Proc. number (needed to write ASM to it) */
  char          *proc_name;  /**< Procedure name */
  BoxVMCallNum  call_num;    /**< Call number (needed to call it from ASM) */
  Type          type;        /**< Type of the procedure */
} CmpProc;

#include "operator.h"
#include "cmpproc.h"

struct _box_cmp {
  BoxArr    stack;     /**< Used during compilation to pass around expressions */
  BoxVM     vm;        /**< The target of the compilation */
  BoxTS     ts;        /**< The type system */
  RegAlloc  regs;      /**< Register occupation handler */
  CmpProc   main_proc, /**< Main procedure in the module */
            *cur_proc; /**< Procedure on which we are working now */
  Operator  bin_ops[ASTBINOP__NUM_OPS], /**< Table of binary operators */
            un_ops[ASTUNOP__NUM_OPS];   /**< Table of unary operators */
};

void BoxCmp_Init(BoxCmp *c);

void BoxCmp_Finish(BoxCmp *c);

BoxCmp *BoxCmp_New(void);

void BoxCmp_Destroy(BoxCmp *c);

void BoxCmp_Compile(BoxCmp *c, ASTNode *program);

#endif /* _COMPILER_H */
