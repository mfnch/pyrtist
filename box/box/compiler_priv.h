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

namespace Box {
  class Compiler {
  private:
    // Old compiler.
    BoxCmp *c;

    /// Pool of #Value objects currently allocated by the compiler.
    BoxArr active_values_;

    /// Position in the active_value_ array at the beginning of tracking.
    BoxArr value_pos_;

  public:
    Compiler(BoxCmp *old_compiler);
    ~Compiler();

    /**
     * @brief Compile from the given abstract syntax tree.
     * @param ast Abstract syntax tree of the program to compile.
     * @return Whether it is safe to execute the compiled code.
     */
    bool Compile(BoxAST *ast);

    // Value maniputation (implemented in value.cc).
    Value *Create_Value();
    Value *Destroy_Value(Value *v);

    // Leak check functions.
    void Begin_Leak_Check();
    int End_Leak_Check();
    Value *Track_Value(Value *v);
    Value *Untrack_Value(Value *v);

    // Value manipulation functionality.
    Value *Value_To_Straight_Ptr(Value *v_obj);
    BoxTask Value_Move_Content(Value *dest, Value *src);

  private:
    /**
     * @brief Remove items from the compiler stack.
     */
    void Remove_Any(int num_items_to_remove);

    /**
     * @brief Used to push an error into the stack.
     * @details Errors are propagated. For example: a binary operation where
     * one of the two arguments is an error returns silently an error into the
     * stack.
     */
    void Push_Error(int num_errors);

    /**
     * @brief Manage the compiler stack in case of errors.
     * @details Check the last @p items_to_pop stack entries for errors. If all
     * these entries have no errors, then do nothing and return true. If one or
     * more of these entries have errors, then removes all of them from the
     * stack, push the given number of errors and return true.
     */
    bool Pop_Errors(int items_to_pop, int errors_to_push);

    /**
     * @brief Pushes the given value into the compiler stack.
     */
    void Push_Value(Value *v);

    /**
     * @brief Pops the last value in the compiler stack and returns it.
     */
    Value *Pop_Value();

    /**
     * @brief Get a value from the compiler stack.
     */
    Value *Get_Value(int pos);

    // Support for the AST node compilation.
    void Compile_Any(BoxASTNode *node);
    void Compile_Subtype_Value(BoxASTNodeSubtype *node);
    void Compile_Subtype_Type(BoxASTNodeSubtype *node);
    void Compile_Box_Generic(BoxASTNode *box_node,
                             BoxType *t_child, BoxType *t_parent);
    Value *Compile_Value_Assignment(Value *left, Value *right);
    Value *Compile_Type_Assignment(Value *v_name, Value *v_type);
    void Compile_Struct_Value(BoxASTNodeCompound *compound);
    void Compile_Struct_Type(BoxASTNodeCompound *compound);
    void Compile_Species_Type(BoxASTNodeCompound *compound);
    void Compile_Paren(BoxASTNode *expr);

    // AST node methods (each method compiles the corresponding AST node).
#  define BOXASTNODE_DEF(NODE, Node) \
    void Compile_##Node(BoxASTNode *node);
#  include "astnodes.h"
#  undef BOXASTNODE_DEF
  };
}

/**
 * @brief Implementation for #BoxCmp.
 */
struct BoxCmp_struct {
  /* This object will eventually swallow all the content of BoxCmp. */
  Box::Compiler *compiler;

  BoxAST     *ast;      /**< Abstract syntax tree. */
  BoxASTNode *ast_node; /**< Current source AST node. */
  BoxLIR     *lir;      /**< LIR tree. */
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
    Value    *create,   /**< Value for BOXTYPE_CREATE */
             *begin,    /**< Value for BOXTYPE_BEGIN */
             *end,      /**< Value for BOXTYPE_END */
             *pause;    /**< Value for BOXTYPE_PAUSE */
  }          value;     /**< Bunch of values, which we do not want
                             to allocate again and again (just to make the
                             compiler a little bit faster and memory
                             efficient). */
  struct {
    BoxCont  pass_child,  /**< Container used to pass child to procedures */
             pass_parent;
                          /**< Container used to pass parent to procedures */
  }          cont;        /**< Constant containers (allocated once for all
                               just for efficiency) */
  struct {
    unsigned int
             own_vm :1,   /**< Do we own the VM? */
             is_sane :1;  /**< Is the output of compilation sane? */
  }          attr;        /**< Attributes of the compiler */
};

BOX_BEGIN_DECLS

/**
 * @brief Initialize an unset #BoxCmp structure.
 */
void BoxCmp_Init(BoxCmp *c, BoxVM *target_vm);

/**
 * @brief Finalize a #BoxCmp structure initialized with BoxCmp_Init().
 */
void BoxCmp_Finish(BoxCmp *c);

BOX_END_DECLS

#endif /* _BOX_COMPILER_PRIV_H */
