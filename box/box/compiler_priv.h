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
#define BOX_CHECK_VALUE_LEAKS 0

// Whether to cache Value allocation in the compiler.
#define BOX_USE_VALUE_CACHE 1


namespace Box {

  /// @brief Return the name (a string) corresponding to the given ValueKind.
  const char *ValueKind_To_Str(ValueKind vk);

  union ValueOrChain {
    Value value;
    ValueOrChain *next_in_chain;
  };

  class Compiler {
  public:
    // Old compiler.
    BoxVM        *vm_;         ///< The target of compilation.
    BoxAST       *ast_;        ///< Abstract syntax tree.

  private:
    Operator     convert_,     ///< Conversion operator.
                 bin_ops_[BOXASTBINOP_NUM_OPS], ///< Table of binary operators.
                 un_ops_[BOXASTUNOP_NUM_OPS];   ///< Table of unary operators.
    BoxCont      pass_child_,  ///< Container used to pass child to procedures.
                 pass_parent_; ///< Container used to pass parent to procedures.
    BoxVMCode    main_proc_,   ///< Main procedure in the module.
                 *cur_proc_;   ///< Procedure on which we are working now.
    BoxLIR       *lir_;        ///< LIR tree.
    BoxArr       stack_;       ///< Used during compilation to pass around
                               ///  expressions.
    BoxASTNode   *ast_node_;   ///< Current source AST node.
    Namespace    ns;           ///< The namespace.

    // The following objects manage allocation of #Value objects.
    BoxAllocPool value_pool_;  ///< Pool of allocated Values.
    BoxArr       active_values_;
                               ///< #Value objects currently tracked.
    BoxArr       value_pos_;   ///< Position in the active_value_ array at the
                               ///< beginning of tracking.
    ValueOrChain *free_value_chain_;
                               ///< Chain to free Value in the value_pool_.

    struct {
      unsigned int
                 own_vm :1,    ///< Do we own the VM?
                 is_sane :1;   ///< Is the output of compilation sane?
    }            attr_;        ///< Attributes of the compiler.

  public:
    Compiler(BoxVM *target_vm);
    ~Compiler();

    /**
     * @brief Submit a compiler message (error, warning, etc).
     */
    void Log(BoxSrc *src, BoxLogLevel level, const char *fmt, ...);

    /// @brief Create and append a 1-argument LIR node.
    void Append_LIR0(BoxGOp g_op)
    {
      BoxLIR_Append_GOp(lir_, g_op, 0);
    }

    /// @brief Create and append a 1-argument LIR node.
    void Append_LIR1(BoxGOp g_op, BoxCont *c1)
    {
      BoxLIR_Append_GOp(lir_, g_op, 1, c1);
    }

    /// @brief Create and append a 2-arguments LIR node.
    void Append_LIR2(BoxGOp g_op, BoxCont *c1, BoxCont *c2)
    {
      BoxLIR_Append_GOp(lir_, g_op, 2, c1, c2);
    }

    /// @brief Create and append a 3-arguments LIR node.
    void Append_LIR3(BoxGOp g_op, BoxCont *c1, BoxCont *c2, BoxCont *c3)
    {
      BoxLIR_Append_GOp(lir_, g_op, 3, c1, c2, c3);
    }

    /// @brief Set the current node for message position tracking.
    BoxASTNode *Set_Cur_Node(BoxASTNode *cur_ast_node);

    BoxTypeId Install_Type(BoxType *t)
    {
      return BoxVM_Install_Type(vm_, t);
    }

    /**
     * @brief Compile from the given abstract syntax tree.
     * @param ast Abstract syntax tree of the program to compile.
     * @return Whether it is safe to execute the compiled code.
     */
    bool Compile(BoxAST *ast);

    /**
     * @brief Compile and install the main procedure.
     * @return Return the call number for the installed procedure.
     */
    BoxVMCallNum Install() {return Install(& main_proc_);}

    /**
     * @brief Compile and install the given procedure.
     * @return Return the call number for the installed procedure.
     */
    BoxVMCallNum Install(BoxVMCode *p);

    // Shorthands for submitting warning/error messages to the compiler.
#   define LOG_WARN(...) \
      Log(NULL, BOXLOGLEVEL_WARNING, __VA_ARGS__)
#   define LOG_ERR(...) \
      Log(NULL, BOXLOGLEVEL_ERROR, __VA_ARGS__)
#   define LOG_FATAL(...) \
      Log(NULL, BOXLOGLEVEL_FATAL, __VA_ARGS__)

    ///////////////////////////////////////////////////////////////////////////
    // Value manipulation functionality (value.cc).
    Value *Init_Value(Value *v);
    Value *Finish_Value(Value *v);
    Value *Create_Value();
    Value *Destroy_Value(Value *v);
    Value *Weak_Copy_Value(Value *v_src);
    void Begin_Leak_Check();
    int End_Leak_Check();
    Value *Track_Value(Value *v);
    Value *Untrack_Value(Value *v);
    Value *Move_Value(Value *v_dst, Value *v_src);
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
    void Setup_Value_As_Parent_Or_Child(Value *v, BoxType *t, bool is_parent);
    void Setup_Value_As_Parent(Value *v, BoxType *parent_t);
    void Setup_Value_As_Child(Value *v, BoxType *child_t);
    void Setup_Value_As_LReg(Value *v, BoxType *type);
    void Emit_Value_Alloc(Value *v);
    void Emit_Link(Value *v);
    void Emit_Unlink(Value *v);
    BoxLIRNodeOpBranch *Emit_Conditional_Jump(Value *v);
    Value *Emit_Make_Temp(Value *v_dst, Value *v_src);
    Value *Emit_Load_Into_Reg(Value *v_dst, Value *v_src);
    Value *Temp_As_Target(Value *v);
    Value *Value_Cast_To_Ptr_2(Value *v);
    Value *Emit_Value_Cast(Value *v_ptr, BoxType *t);
    void Emit_Call(BoxVMCallNum call_num, Value *parent, Value *child);
    BoxTask Emit_Call(Value *parent, BoxTypeId tid_child);
    BoxTask Emit_Call(Value *parent, Value *child);
    Value *Emit_Cast_To_Ptr(Value *v);
    Value *Emit_Get_Subfield(Value *v_src_dst, size_t offset,
                             BoxType *subf_type);
    Value *Emit_Get_Struc_Member(Value *v_src, const char *memb);
    Value *Emit_Reduce_Ptr_Offset(Value *v_obj);
    Value *Emit_Value_Move(Value *v_dst, Value *v_src);
    Value *Emit_Value_Assignment(Value *v_dst, Value *v_src);
    Value *Emit_Value_Expansion(Value *src, BoxType *t_dst);
    Value *Emit_Subtype_Build(Value *v_parent, const char *subtype_name);
    Value *Emit_Get_Subtype_Parent_Or_Child(Value *v_subtype, bool get_child);
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
    void Init_Operators();
    void Finish_Operators();
    /// @brief Get a binary operator.
    Operator *Get_Bin_Op(BoxASTBinOp bin_op)
    {
      assert(bin_op >= 0 && bin_op < BOXASTBINOP_NUM_OPS);
      return & bin_ops_[bin_op];
    }

    /// @brief Get a unary operator.
    Operator *Get_Un_Op(BoxASTUnOp un_op)
    {
      assert(un_op >= 0 && un_op < BOXASTUNOP_NUM_OPS);
      return & un_ops_[un_op];
    }

    Value *Emit_Operation(Operation *opn, Value *v_left, Value *v_right);
    Value *Emit_BinOp(BoxASTBinOp op, Value *v_left, Value *v_right);
    Value *Emit_UnOp(BoxASTUnOp op, Value *v);
    bool Try_Emit_Conversion(Value *dest, Value *src);

    ///////////////////////////////////////////////////////////////////////////
    // Name-space functionality (namespace.cc).
    void Namespace_Init();
    void Namespace_Finish();
    void Namespace_Floor_Up();
    void Namespace_Floor_Down();
    NmspItem *Namespace_Add_Item(NmspFloor floor, const char *item_name);
    NmspItem *Namespace_Get_Item(NmspFloor floor, const char *item_name);
    Value *Namespace_Add_Value(NmspFloor floor,
                               const char *item_name, Value *v);
    Value *Namespace_Get_Value(NmspFloor floor, const char *item_name);
    void Namespace_Add_Procedure(NmspFloor floor,
                                 BoxType *parent, BoxType *comb_node);
    void Namespace_Add_Callback(NmspFloor floor,
                                NmspCallback callback, void *data);

    bool Is_Var_Name(Value *v) {return (v->kind == VALUEKIND_VAR_NAME);}
    bool Is_Type_Name(Value *v) {return (v->kind == VALUEKIND_TYPE_NAME);}
    bool Is_Target(Value *v) {return (v->kind == VALUEKIND_TARGET);}
    bool Is_Err(Value *v) {return (v->kind == VALUEKIND_ERR);}
    bool Is_Temp(Value *v) {return (v->kind == VALUEKIND_TEMP);}
    bool Is_Instance(Value *v)
    {
      switch(v->kind) {
      case VALUEKIND_IMM: case VALUEKIND_TEMP: case VALUEKIND_TARGET:
        return true;
      default:
        return false;
      }
    }
    bool Is_Ignorable(Value *v) {
      int ignore = ((v->kind == VALUEKIND_ERR)
                    || (v->kind == VALUEKIND_TYPE)
                    || v->attr.ignore);
      if (ignore)
        return true;
      if (Is_Instance(v))
        return (BoxType_Compare(Box_Get_Core_Type(BOXTYPEID_VOID), v->type)
                != BOXTYPECMP_DIFFERENT);
      return false;
    }
    Value *Set_Ignorable(Value *v, bool ignorable=true) {
      v->attr.ignore = ignorable;
      return v;
    }
    bool Has_Type(Value *v)
    {
      switch(v->kind) {
      case VALUEKIND_TYPE_NAME: case VALUEKIND_VAR_NAME: case VALUEKIND_ERR:
        return false;
      default:
        return true;
      }
    }

    ///////////////////////////////////////////////////////////////////////////
    // Built-ins (builtins.cc).
    void Init_Builtins();
    void Finish_Builtins();
    /// @brief Define a new intrinsic type with the given name and size.
    BoxType *Create_Type_Raw(const char *type_name,
                             size_t type_size, size_t alignment);
    /// @brief Define a new intrinsic type from a given C type.
#   define Create_Type(type_name, type) \
      Create_Type_Raw((type_name), sizeof(type), __alignof__(type))

    ///////////////////////////////////////////////////////////////////////////
    // Built-ins (bltinio.cc).
    /// @brief Register the builtin IO functions.
    void Register_IO_Builtins();
    /// @brief To be called when destroying the compiler data structures. */
    void Unregister_IO_Builtins();

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

    ///////////////////////////////////////////////////////////////////////////
    // Private value manipulation functionality (value.cc).
    Value *Emit_Species_Expansion(Value *v_src, BoxType *t_dst);
  };
}

#endif /* _BOX_COMPILER_PRIV_H */
