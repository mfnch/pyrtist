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

/**
 * @file compiler_priv.h
 * @brief Implementation of objects defined in compiler.h.
 */

#ifndef _BOX_COMPILER_PRIV_H
#  define _BOX_COMPILER_PRIV_H

#  include <box/compiler.h>
#  include <box/array.h>
#  include <box/vmcode.h>
#  include <box/srcpos.h>
#  include <box/registers.h>
#  include <box/value.h>
#  include <box/namespace.h>
#  include <box/operator.h>
#  include <box/builtins.h>

#  include <box/lir_priv.h>


/**
 * @brief Implementation for #BoxCmp.
 */
struct BoxCmp_struct {
  BoxAST     *ast;      /**< Abstract syntax tree. */
  BoxASTNode *ast_node; /**< Current source AST node. */
  BoxLIR     lir;       /**< LIR tree. */
  BoxVM      *vm;       /**< The target of the compilation */
  BoxArr     stack;     /**< Used during compilation to pass around
                             expressions */
  BltinStuff bltin;     /**< Builtin types, etc. */
  Namespace  ns;        /**< The namespace */
  BoxVMCode  main_proc, /**< Main procedure in the module */
             *cur_proc; /**< Procedure on which we are working now */
  Operator   convert,   /**< Conversion operator */
             bin_ops[BOXASTBINOP_NUM_OPS], /**< Table of binary operators */
             un_ops[BOXASTUNOP_NUM_OPS];   /**< Table of unary operators */
  struct {
    Value      error,     /**< Error value */
               void_val,  /**< Void value */
               create,    /**< Value for BOXTYPE_CREATE */
               destroy,   /**< Value for BOXTYPE_DESTROY */
               begin,     /**< Value for BOXTYPE_BEGIN */
               end,       /**< Value for BOXTYPE_END */
               pause;     /**< Value for BOXTYPE_PAUSE */
  } value;                /**< Bunch of values, which we do not want
                             to allocate again and again (just to make the
                             compiler a little bit faster and memory
                             efficient). */
  struct {
    BoxCont    pass_child,/**< Container used to pass child to procedures */
               pass_parent;
                          /**< Container used to pass parent to procedures */
  }          cont;        /**< Constant containers (allocated once for all
                               just for efficiency) */
  struct {
    unsigned int
               own_vm :1,  /**< Do we own the VM? */
               is_sane :1; /**< Is the output of compilation sane? */
  }          attr;      /**< Attributes of the compiler */
};


/**
 * @brief Initialize an unset #BoxCmp structure.
 */
void BoxCmp_Init(BoxCmp *c, BoxVM *target_vm);

/**
 * @brief Finalize a #BoxCmp structure initialized with BoxCmp_Init().
 */
void BoxCmp_Finish(BoxCmp *c);

#endif /* _BOX_COMPILER_PRIV_H */
