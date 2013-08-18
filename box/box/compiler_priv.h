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
#  include "registers.h"
#  include "value.h"
#  include "namespace.h"
#  include "operator.h"
#  include "builtins.h"


/**
 * @brief Implementation for #BoxCmp.
 */
struct BoxCmp_struct {
  struct {
    unsigned int
               own_vm :1; /**< Was the VM created by us, or given from
                               outside? */
  }          attr;      /**< Attributes of the compiler */
  BoxVM      *vm;       /**< The target of the compilation */
  BoxArr     stack;     /**< Used during compilation to pass around
                             expressions */
  BltinStuff bltin;     /**< Builtin types, etc. */
  Namespace  ns;        /**< The namespace */
  BoxVMCode  main_proc, /**< Main procedure in the module */
             *cur_proc; /**< Procedure on which we are working now */
  Operator   convert,   /**< Conversion operator */
             bin_ops[ASTBINOP__NUM_OPS], /**< Table of binary operators */
             un_ops[ASTUNOP__NUM_OPS];   /**< Table of unary operators */
  BoxSrcPos  src_pos;   /**< Recent position in source while parsing AST */
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
};

#endif /* _BOX_COMPILER_PRIV_H */
