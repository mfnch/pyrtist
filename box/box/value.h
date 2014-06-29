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
 * @file value.h
 * @brief Declaration Value, the central object in the Box compiler.
 *
 * A value object is used by the Box compiler to represent values.
 * It contains information about the actual register (global or local)
 * used to store/manage the object and about the type and the attributes
 * of the value. Integers, reals, etc. (such as '1', '1.23', ...) are translated
 * into Value objects. Variables (such as 'var1', ...) are translated into
 * Value objects. And many operations (such as the extraction of a member of
 * a structure, 'var1.member') are expressed in terms of Value objects.
 */

#ifndef _BOX_VALUE_H
#  define _BOX_VALUE_H

#  include <box/types.h>
#  include <box/container.h>
#  include <box/vmcode.h>
#  include <box/lir.h>
#  include <box/compiler.h>


typedef enum {
  VALUEKIND_ERR,              /**< An error */
  VALUEKIND_VAR_NAME,         /**< A variable name (no type, no value) */
  VALUEKIND_TYPE_NAME,        /**< A variable name (no type, no value) */
  VALUEKIND_TYPE,             /**< A type (no value) */
  VALUEKIND_IMM,              /**< A constant numerical value */
  VALUEKIND_TEMP,             /**< A temporary value */
  VALUEKIND_TARGET            /**< A target value (lvalue) */
} ValueKind;

/* This datastructure has a major design flaw. See: value.c (Value_Init). */
typedef struct {
  BoxVMCode *proc;            /**< The compiler to which this value refers */
  ValueKind kind;             /**< Kind of Value */
  BoxType   *type;            /**< Type of the Value */
  BoxCont   cont;             /**< Container */
  char      *name;            /**< Optional name of the value */
  struct {
    unsigned int
            read_only     :1, /**< Value datastructure is read-only. */
            new_or_init   :1, /**< Created by Value_Create or Value_Init? */
            own_register  :1, /**< Need to release a register during
                                 finalisation? */
            ignore        :1; /**< To be ignored when passed to a Box? */
  }         attr;             /**< Attributes for the Value */
} BoxValue;

/* Just for backward "compatibility". */
typedef BoxValue Value;

BOX_BEGIN_DECLS

/**
 * @brief Initialise a Value.
 * @param v Pointer to memory to be initialised.
 * @return Return @param v.
 */
Value *
Value_Init(Value *v, BoxCmp *c);

/**
 * @brief Allocate a Value object and initialise it by calling Value_Init().
 * @details Notice that the object is aware of its allocation mode (whether it
 * was created with Value_Init() or Value_Create()) and Value_Destroy() will
 * do the right things (e.g. it calls free() only if the object was created
 * with Value_Create()).
 */
Value *
Value_Create(BoxCmp *c);

/**
 * @brief Finalise a #Value (without deallocating it).
 * @param v Pointer to a #Value object.
 * @return Return @param v.
 */
Value *
Value_Finish(Value *v);

/**
 * @brief Destroy a #Value deallocating it, if necessary.
 * @param v Pointer to the #Value object to destroy.
 * @return If the object was deallocated, then @c NULL is returned.
 *   If the object was only finalised, then @param v is returned.
 * @note Read-only objects are ignored.
 */
Value *
Value_Destroy(Value *v);

/**
 * @brief Move the content of one #Value to another #Value.
 * @return Return @p dst.
 * @note The source value is invalid after the move.
 */
Value *
Value_Move(BoxCmp *c, Value *dst, Value *src);

/** Determine if the given value can be recycled, otherwise return
 * Value_Create()
 */
Value *Value_Recycle(Value *v);

typedef enum {
  VALCONTTYPE_IMM = 0,
  VALCONTTYPE_LREG, VALCONTTYPE_LVAR,
  VALCONTTYPE_GREG, VALCONTTYPE_GVAR,
  VALCONTTYPE_GPTR,
  VALCONTTYPE_LRPTR, VALCONTTYPE_LVPTR,
  VALCONTTYPE_ARG, VALCONTTYPE_STACK
} ValContType;

/**
 * @brief Mark the #Value as read only.
 * @note Read only values cannot be finished or altered.
 */
Value *
Value_Mark_RO(Value *v);

/**
 * @brief Undo a call to Value_Mark_RO().
 */
Value *
Value_Unmark_RO(Value *v);

/** Return the name (a string) corresponding to the given ValueKind. */
const char *ValueKind_To_Str(ValueKind vk);

/**
 * @brief Finish a potentially read-only #Value object.
 */
inline Value *
Value_Force_Finish(Value *v)
{
  return Value_Finish(Value_Unmark_RO(v));
}

/**
 * @brief Finish a potentially read-only #Value object.
 */
inline Value *
Value_Force_Destroy(Value *v)
{
  return Value_Destroy(Value_Unmark_RO(v));
}

/**
 * Check that the given value 'v' has both type and value.
 * Return 1 if 'v' has type and value, otherwise return 0.
 */
BoxBool
Value_Want_Value(Value *v);

/** Check that the given value 'v' has a definite type.
 * Return 1 if 'v' has type, otherwise return 0.
 */
int Value_Want_Has_Type(Value *v);

/** This type is used to specify a container (see the macros CONTAINER_...) */
typedef struct {
  ValContType type_of_container;
  int         which_one,
              addr;
} ValContainer;

/** Set the value to Void[] */
void Value_Setup_As_Void(Value *v);

/** Create a "weak" copy of the provided value. A weak copy is a copy which
 * does not own the register nor references to the underlying object.
 * A weak copy is done when returning a variable, since we just want to access
 * the variable, knowing that we "do not own" it.
 */
Value *
Value_Setup_As_Weak_Copy(Value *v_copy, Value *v);

/** Set the value to a variable with the given name. */
void Value_Setup_As_Var_Name(Value *v, const char *name);

/**
 * @brief Set the value to a type with the given name.
 */
Value *
Value_Setup_As_Type_Name(Value *v, const char *name);

/**
 * @brief Set the value to represent the given type.
 */
Value *
Value_Setup_As_Type(Value *v, BoxType *t);

/** Set the value to represent the given immediate char 'c' */
void Value_Setup_As_Imm_Char(Value *v, BoxChar c);

/** Set the value to represent the given immediate int 'i' */
void Value_Setup_As_Imm_Int(Value *v, BoxInt i);

/** Set the value to represent the given immediate real 'r' */
void Value_Setup_As_Imm_Real(Value *v, BoxReal r);

/** Set the value to represent a temporary value of the given type */
void Value_Setup_As_Temp(Value *v, BoxType *t);

/**
 * @brief Create a new local register value.
 *
 * @param v The destination (uninitialized) #Value structure.
 * @param type The type the register is referring to.
 */
BOXEXPORT void
Value_Setup_As_LReg(Value *v, BoxType *type);

/**
 * @brief Set up the #BoxValue to represent a new variable of the given type.
 *
 * @param v An initialized #BoxValue object.
 * @param t The type of the new variable.
 */
BOXEXPORT void
Value_Setup_As_Var(BoxValue *v, BoxType *t);

/** Set the value to represent the given string 'str'. */
void Value_Setup_As_String(Value *v_str, const char *str);

void Value_Setup_Container(Value *v, BoxType *type, ValContainer *vc);

/**
 * @brief Emit code to allocate the given object.
 *
 * Emit the VM code to generate the object @p v is pointing to.
 *
 * @param v The object to allocate.
 */
BOXEXPORT void
BoxValue_Emit_Allocate(Value *v);

#define Value_Emit_Allocate BoxValue_Emit_Allocate

/** Emit a link instruction to the Value 'v' (which is supposed to be
 * an object or a pointer).
 * REFERENCES: v: -1;
 */
void Value_Emit_Link(Value *v);

/** Emit an unlink instruction to the Value 'v' (which is supposed to be
 * an object or a pointer).
 * REFERENCES: v: -1;
 */
void Value_Emit_Unlink(Value *v);

/** Given an If, For type emits a conditional jump to the specified jump
 * label.
 */
BoxLIRNodeOpBranch *
Value_Emit_CJump(Value *v);

/**
 * @brief Create a temporary value from the given value.
 * @param c The compiler object.
 * @param v_dst Where the output temporary value is to be stored.
 * @param v_src Where the input value is stored.
 * @return Return @p v_dst.
 */
Value *
Value_To_Temp(BoxCmp *c, Value *v_dst, Value *v_src);

/**
 * Similar to 'Value_To_Temp' with just one difference: if v is a target
 * do nothing (just return v with a new link to it).
 */
Value *
Value_To_Temp_Or_Target(BoxCmp *c, Value *v_dst, Value *v_src);

/** Promote a temporary expression to a target one.
 * REFERENCES: return: new, v: -1;
 */
Value *Value_Promote_Temp_To_Target(Value *v);

/**
 * @brief Set the ignorable flag of the given value and return it.
 */
Value *
Value_Set_Ignorable(Value *v, BoxBool ignorable);

/** Return 1 if the value represents an error. */
int Value_Is_Err(Value *v);

/** Return 1 if the value is a temporary value (it stores an intermediate
 * result)
 */
int Value_Is_Temp(Value *v);

/**
 * @brief Return whether the #BoxValue is a variable name (has no type/value).
 */
BOXEXPORT BoxBool
BoxValue_Is_Var_Name(BoxValue *v);

/** Return 1 if the value is a type name (has no type/value) */
int Value_Is_Type_Name(Value *v);

/** Return 1 if the value is a lvalue (can be target of assignments) */
int Value_Is_Target(Value *v);

/** Return 1 if the value has both type and value */
int Value_Is_Value(Value *v);

/** Whether the object's value should be ignored or not. */
int Value_Is_Ignorable(Value *v);

/** Return 1, if v has a type */
int Value_Has_Type(Value *v);

/** Emit the code corresponding to a call to the procedure having symbol
 * 'sym_id'.
 */
void Value_Emit_Call_From_Call_Num(BoxVMCallNum call_num,
                                  Value *parent, Value *child);

/** Emits the code corresponding to a call to child@parent. If the procedure
 * cannot be found, then sets *success to BOXTASK_FAILURE and exits without
 * displaying any messages and returns a new reference to child, so that it
 * can be further processed (rationale: if a standard procedure has not
 * been found you may still interpret it differently, rather than giving up
 * immediately). Example:
 *
 *   child = Value_Emit_Call(parent1, child, & success);
 *   if (child != NULL) // success == BOXTASK_FAILURE
 *     child = Value_Emit_Call(parent2, child, & success);
 *   Value_Unlink(child); // remember that Value_Unlink(NULL) is legal.
 *   if (success == BOXTASK_FAILURE)
 *     MSG_ERROR("Neither parent1 nor parent2 has got the procedure.");
 *
 * In the code, do child@parent2, if child@parent1 was not found.
 * If the procedure is found and compiled, sets *success = BOXTASK_OK and
 * returns NULL. In case of error, sets *success = BOXTASK_ERROR and return
 * NULL.
 * REFERENCES: return: new, parent: 0, child: -1;
 */
BoxTask Value_Emit_Call(Value *parent, Value *child);

/**
 * @brief Object used to iterate over the member of a structure.
 */
typedef struct {
  BoxBool     has_next;
  int         index;
  Value       v_member;
  BoxType     *t_member;
  BoxTypeIter type_iter;
} ValueStrucIter;

/**
 * Convenience function to facilitate iteration of the member of a structure
 * value. Here is and example of how it is supposed to be used:
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
void ValueStrucIter_Init(ValueStrucIter *vsi, Value *v_struc, BoxCmp *c);

/**
 * Convenience function to facilitate iteration of the member of a structure
 * value.
 * @see ValueStrucIter_Init
 */
void ValueStrucIter_Do_Next(ValueStrucIter *vsi);

/**
 * Convenience function to facilitate iteration of the member of a structure
 * value.
 * @see ValueStrucIter_Init
 */
void ValueStrucIter_Finish(ValueStrucIter *vsi);

/**
 * Creator corresponding to #ValueStrucIter_Init.
 * @see ValueStrucIter_Init
 * @see ValueStrucIter_Destroy
 */
ValueStrucIter *ValueStrucIter_Create(Value *v_struc, BoxCmp *c);

/**
 * Destructor for #ValueStrucIter_New.
 * @see ValueStrucIter_New
 */
void ValueStrucIter_Destroy(ValueStrucIter *vsi);

/**
 * @brief Extract the member of a structure object.
 * @param c The compiler object.
 * @param v_src_dst Used to pass the structure and to get the member.
 * @param memb Name of the member to extract.
 * @return If successful, return @p v_src_dst. Otherwise, @p v_dst is destroyed
 *   and @c NULL is returned.
 */
Value *
Value_Struc_Get_Member(BoxCmp *c, Value *v_src_dst, const char *memb);

/**
 * @brief Assign a value to a variable.
 *
 * @param dst The destination variable.
 * @param src The value to be assigned to @p dst.
 * @return Whether the operation succeeded.
 * @note REFERENCES: src: -1, dest: 0;
 */
BOXEXPORT BoxTask
Value_Assign(BoxCmp *c, Value *dst, Value *src);

/** Expand the value 'v' in agreement with the provided expansion type.
 * REFERENCES: return: new, src: -1;
 */
Value *Value_Expand(Value *v, BoxType *expansion_type);

/**
 * REFERENCES: v: 0;
 */
void Value_Setup_As_Parent(Value *v, BoxType *parent_t);

/**
 * REFERENCES: v: 0;
 */
void Value_Setup_As_Child(Value *v, BoxType *child_t);

/** Build the subtype 'subtype_name' of 'v_parent'.
 * If the subtype is not found and 'v_parent' is itself a subtype, then
 * resolve it to its child and search among its subtypes.
 * If the subtype cannot be built, then return NULL.
 * REFERENCES: return: new, v_parent: -1;
 */
Value *Value_Subtype_Build(Value *v_parent, const char *subtype_name);

/** Return the child of the subtype 'v_subtype'.
 * REFERENCES: return: new, v_subtype: -1;
 */
Value *Value_Subtype_Get_Child(Value *v_subtype);

/** Return the parent of the subtype 'v_parent'.
 * REFERENCES: return: new, v_subtype: -1;
 */
Value *Value_Subtype_Get_Parent(Value *v_subtype);

/** If 'v' is a subtype, returns its child, otherwise return 'v'.
 * REFERENCES: return: new, v: -1;
 */
Value *Value_Expand_Subtype(Value *v);

/** Raise the value 'v': if 'v = (^Int)[]', '^v' has type 'Int'. */
Value *Value_Raise(Value *v);

/**
 * @brief Create a reference to the given object.
 */
Value *
Value_Reference(Value *v);

/**
 * @brief Dereference the given pointer object.
 */
Value *
Value_Dereference(Value *v);

BOX_END_DECLS

#endif /* _BOX_VALUE_H */
