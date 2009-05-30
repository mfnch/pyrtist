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

#  include "types.h"
#  include "cmpptrs.h"
#  include "array.h"
#  include "virtmach.h"
#  include "ast.h"
#  include "registers.h"
#  include "value.h"
#  include "namespace.h"
#  include "operator.h"
#  include "cmpproc.h"
#  include "builtins.h"

struct _box_cmp {
  struct {
    unsigned int
               own_vm :1; /**< Was the VM created by us, or given from
                               outside? */
  }          attr;      /**< Attributes of the compiler */
  BoxVM      *vm;       /**< The target of the compilation */
  BoxArr     stack;     /**< Used during compilation to pass around
                             expressions */
  BoxTS      ts;        /**< The type system */
  BltinStuff bltin;     /**< Builtin types, etc. */
  Namespace  ns;        /**< The namespace */
  CmpProc    main_proc, /**< Main procedure in the module */
             *cur_proc; /**< Procedure on which we are working now */
  Operator   convert,   /**< Conversion operator */
             bin_ops[ASTBINOP__NUM_OPS], /**< Table of binary operators */
             un_ops[ASTUNOP__NUM_OPS];   /**< Table of unary operators */
  struct {
    Value      error,     /**< Error value */
               void_val;  /**< Void value */
  } value;              /**< Bunch of constant values, which we do not want
                             to allocate again and again (just to make the
                             compiler a little bit faster and memory
                             efficient). */
};

void BoxCmp_Init(BoxCmp *c, BoxVM *target_vm);

void BoxCmp_Finish(BoxCmp *c);

BoxCmp *BoxCmp_New(BoxVM *target_vm);

void BoxCmp_Destroy(BoxCmp *c);

/** Steal the VM which is being used as the target for the compilation. */
BoxVM *BoxCmp_Steal_VM(BoxCmp *c);

/** Create the compiler and use it to parse the given file, returning the
 * virtual machine object which was used as the target of the compilation
 * and putting in '*main' the BoxVMCallNum of the main procedure of the
 * compiled source.
 */
BoxVM *Box_Compile_To_VM_From_File(BoxVMCallNum *main, BoxVM *target_vm,
                                   FILE *file, const char *setup_file_name);

void BoxCmp_Compile(BoxCmp *c, ASTNode *program);

#endif /* _COMPILER_H */
