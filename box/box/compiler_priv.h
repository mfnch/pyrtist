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
#  include <box/allocpool_priv.h>


// Whether to carry out Value allocation leak checks in the compiler.
#define BOX_CHECK_VALUE_LEAKS 1

// Whether to cache Value allocation in the compiler.
#define BOX_USE_VALUE_CACHE 1


namespace Box {
  union ValueOrChain {
    Value value;
    ValueOrChain *next_in_chain;
  };

  class Compiler {
  private:
    // Old compiler.
    BoxCmp *c;

    /// Pool of #Value objects currently allocated by the compiler.
    BoxArr active_values_;

    /// Position in the active_value_ array at the beginning of tracking.
    BoxArr value_pos_;

    /// Pool of Value which allows to reuse memory.
    BoxAllocPool value_pool_;

    /// Chain to free Value in the value_pool_ pool.
    ValueOrChain *free_value_chain_;

  public:
    Compiler(BoxCmp *old_compiler);
    ~Compiler();

    /**
     * @brief Compile from the given abstract syntax tree.
     * @param ast Abstract syntax tree of the program to compile.
     * @return Whether it is safe to execute the compiled code.
     */
    bool Compile(BoxAST *ast);

    /**
     * @brief Submit a compiler message (error, warning, etc).
     */
    void Log(BoxLogLevel level, const char *fmt, ...);

    // Shorthands for submitting warning/error messages to the compiler.
#   define LOG_WARN(...) \
      Log(BOXLOGLEVEL_WARNING, __VA_ARGS__)
#   define LOG_ERR(...) \
      Log(BOXLOGLEVEL_ERROR, __VA_ARGS__)
#   define LOG_FATAL(...) \
      Log(BOXLOGLEVEL_FATAL, __VA_ARGS__)

    // Value maniputation (implemented in value.cc).
    Value *Create_Value();
    Value *Destroy_Value(Value *v);
    Value *Weak_Copy_Value(Value *v_src);

    // Leak check functions.
    void Begin_Leak_Check();
    int End_Leak_Check();
    Value *Track_Value(Value *v);
    Value *Untrack_Value(Value *v);

    ///////////////////////////////////////////////////////////////////////////
    // Value manipulation functionality (value.cc).
    bool Want_Instance(Value *v);
    bool Want_Type(Value *v);
    void Setup_Value_Container(Value *v, BoxType *type, ValContainer *vc);
    Value *Setup_Value_As_Weak_Copy(Value *v_copy, Value *v);
    void Setup_Value_As_Var_Name(Value *v, const char *name);
    Value *Setup_Value_As_Type_Name(Value *v, const char *name);
    Value *Setup_Value_As_Type(Value *v, BoxType *t);
    void Setup_Value_As_Imm_Char(Value *v, BoxChar c);
    void Setup_Value_As_Imm_Int(Value *v, BoxInt i);
    void Setup_Value_As_Imm_Real(Value *v, BoxReal r);
    void Setup_Value_As_Void(Value *v);
    void Setup_Value_As_Temp(Value *v, BoxType *t);
    void Setup_Value_As_Var(Value *v, BoxType *t);
    void Setup_Value_As_String(Value *v_str, const char *str);
    void Setup_Value_As_Parent(Value *v, BoxType *parent_t);
    void Setup_Value_As_Child(Value *v, BoxType *child_t);
    void Setup_Value_As_LReg(Value *v, BoxType *type);
    void Emit_Value_Alloc(Value *v);
    void Emit_Link(Value *v);
    void Emit_Unlink(Value *v);
    BoxLIRNodeOpBranch *Emit_Conditional_Jump(Value *v);
    Value *Emit_Make_Temp(Value *v_dst, Value *v_src);
    Value *Value_Cast_To_Ptr_2(Value *v);
    Value *Emit_Value_Cast(Value *v_ptr, BoxType *t);
    void Emit_Call(BoxVMCallNum call_num, Value *parent, Value *child);
    BoxTask Emit_Call(Value *parent, BoxTypeId tid_child);
    BoxTask Emit_Call(Value *parent, Value *child);
    Value *Emit_Cast_To_Ptr(Value *v);
    Value *Value_To_Straight_Ptr(Value *v_obj);
    Value *Emit_Struc_Member_Get(Value *v_src, const char *memb);
    Value *Emit_Reduce_Ptr_Offset(Value *v_obj);
    Value *Emit_Value_Move(Value *v_dst, Value *v_src);
    Value *Emit_Value_Assignment(Value *v_dst, Value *v_src);
    Value *Emit_Value_Expansion(Value *src, BoxType *t_dst);
    Value *Emit_Subtype_Build(Value *v_parent, const char *subtype_name);
    Value *Emit_Get_Subtype_Parent(Value *v_subtype);
    Value *Emit_Get_Subtype_Child(Value *v_subtype);
    Value *Emit_Subtype_Expansion(Value *v_src);
    Value *Emit_Raise_Instance(Value *v);
    Value *Emit_Reference_Instance(Value *v);
    Value *Emit_Dereference_Instance(Value *v);

    /**
     * @brief Initialise iteration over the members of a structure.
     * @details Convenience function to facilitate iteration of the member of
     * a structure value. Here is and example of how it is supposed to be used:
     * @code
     *   ValueStrucIter vsi;
     *   for(ValueStrucIter_Init(& vsi, v_struc, vmcode);
     *       vsi.has_next; ValueStrucIter_Do_Next(& vsi)) {
     *     // access to 'vsi.member'
     *   }
     *   ValueStrucIter_Finish(& vsi);
     * @endcode
     * @see ValueStrucIter_Finish
     * @see ValueStrucIter_Do_Next
     */
    void ValueStrucIter_Init(ValueStrucIter *vsi, Value *v_struc);

    /// @brief Iterate to the next member of the structure.
    /// @see ValueStrucIter_Init
    void ValueStrucIter_Do_Next(ValueStrucIter *vsi);

    /// @brief Finalise the ValueStrucIter object.
    /// @see ValueStrucIter_Init
    void ValueStrucIter_Finish(ValueStrucIter *vsi);

    ///////////////////////////////////////////////////////////////////////////
    // Operator functionality (operator.cc).
    Value *Emit_BinOp(BoxASTBinOp op, Value *v_left, Value *v_right);
    bool Try_Emit_Conversion(Value *dest, Value *src);

  private:
    /// @brief Obtain a #Value from the pool.
    /// @see Free_Value
    /// @note This function and Free_Value() do the job of malloc() and free(),
    //    but more efficiently.
    Value *Alloc_Value();

    /// @brief: release a #Value previously obtained with Alloc_Value().
    /// @see Alloc_Value
    void Free_Value(Value *v);

    /// @brief Remove items from the compiler stack.
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
