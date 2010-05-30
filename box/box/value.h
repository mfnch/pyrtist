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

/** @file value.h
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

#ifndef _VALUE_H
#  define _VALUE_H

#  include "types.h"
#  include "container.h"
#  include "cmpproc.h"

typedef enum {
  VALUEKIND_ERR,              /**< An error */
  VALUEKIND_VAR_NAME,         /**< A variable name (no type, no value) */
  VALUEKIND_TYPE_NAME,        /**< A variable name (no type, no value) */
  VALUEKIND_TYPE,             /**< A type (no value) */
  VALUEKIND_IMM,              /**< A constant numerical value */
  VALUEKIND_TEMP,             /**< A temporary value */
  VALUEKIND_TARGET            /**< A target value (lvalue) */
} ValueKind;

typedef struct {
  int       num_ref;          /**< Number of references to this Value */
  CmpProc   *proc;            /**< The compiler to which this value refers */
  ValueKind kind;             /**< Kind of Value */
  BoxType   type;             /**< Type of the Value */
  struct {
    BoxCont   cont;             /**< Container */
  }         value;            /**< Value of the container */
  char      *name;            /**< Optional name of the value */
  struct {
    unsigned int
              new_or_init   :1, /**< Created with Value_New or Value_Init? */
              own_register  :1, /**< Need to release a register during
                                     finalisation? */
              own_reference :1, /**< Do we own a reference to the object? */
              ignore        :1; /**< To be ignored when passed to a Box? */
  }         attr;             /**< Attributes for the Value */
} Value;

/** Initialise a Value 'v' assuming 'v' points to an already allocated memory
 * region able to contain a Value object.
 */
void Value_Init(Value *v, CmpProc *proc);

/** Allocate a Value object and initialise it by calling Value_Init.
 * Notice that the object is aware of its allocation mode (whether it was
 * created with Value_Init or Value_New) and Value_Unlink will correctly
 * call a free() in case Value_New was used. free() won't be called if the
 * object was created with Value_Init.
 */
Value *Value_New(CmpProc *proc);

/** Remove one reference to the Value object 'v', destroying it if there are
 * no more reference to the Value. The object is destroyed coherently to how
 * it was created (Value_New or Value_Init). If Value_New was used, then the
 * object is freed. If Value_Init was used free() is not called.
 */
void Value_Unlink(Value *v);

/** Add a reference to the specified Value. */
void Value_Link(Value *v);

/** Determine if the given value can be recycled, otherwise return Value_New()
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

/** Return the name (a string) corresponding to the given ValueKind. */
const char *ValueKind_To_Str(ValueKind vk);

/** Check that the given value is of the wanted kind. 'wanted' is a pointer
 * to an array of 'num_wanted' ValueKind items. This array specifies what
 * kinds the Value 'v' is expected to have.
 * If the value has one of the wanted kind, returns 1, otherwise print an
 * error message (such as "Got a x expression, but expecting w, y or z.")
 * and returns 0.
 */
int Value_Want(Value *v, int num_wanted, ValueKind *wanted);

/** Check that the given value 'v' has both type and value.
 * Return 1 if 'v' has type and value, otherwise return 0.
 */
int Value_Want_Value(Value *v);

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
void Value_Setup_As_Weak_Copy(Value *v_copy, Value *v);

/** Set the value to a variable with the given name. */
void Value_Setup_As_Var_Name(Value *v, const char *name);

/** Set the value to a type with the given name. */
void Value_Setup_As_Type_Name(Value *v, const char *name);

/** Set the value to represent the given type */
void Value_Setup_As_Type(Value *v, BoxType t);

/** Set the value to represent the given immediate char 'c' */
void Value_Setup_As_Imm_Char(Value *v, Char c);

/** Set the value to represent the given immediate int 'i' */
void Value_Setup_As_Imm_Int(Value *v, Int i);

/** Set the value to represent the given immediate real 'r' */
void Value_Setup_As_Imm_Real(Value *v, Real r);

/** Set the value to represent a temporary value of the given type */
void Value_Setup_As_Temp(Value *v, BoxType t);

/** Set the value to represent the given string 'str'. */
void Value_Setup_As_String(Value *v_str, const char *str);

void Value_Setup_Container(Value *v, BoxType type, ValContainer *vc);

/** Emit the VM code to generate the object 'v' is pointing to. */
void Value_Emit_Allocate(Value *v);

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

/** Return a new temporary Value created from the given Value 'v'.
 * NOTE: return a new value created with Value_New() or a new reference
 *  to 'v', if it can be recycled (has just one reference).
 */
Value *Value_To_Temp(Value *v);

/** Similar to 'Value_To_Temp' with just one difference: if v is a target
 * do nothing (just return v with a new link to it).
 */
Value *Value_To_Temp_Or_Target(Value *v);

/** Promote a temporary expression to a target one.
 * REFERENCES: return: new, v: -1;
 */
Value *Value_Promote_Temp_To_Target(Value *v);

/** Set the ignorable flag for the value. */
void Value_Set_Ignorable(Value *v, int ignorable);

/** Return 1 if the value represents an error. */
int Value_Is_Err(Value *v);

/** Return 1 if the value is a temporary value (it stores an intermediate
 * result)
 */
int Value_Is_Temp(Value *v);

/** Return 1 if the value is a variable name (has no type/value) */
int Value_Is_Var_Name(Value *v);

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
void Value_Emit_Call_From_SymID(BoxVMSymID sym_id,
                                Value *parent, Value *child);

/** Emit the code corresponding to a call to child@parent.
 * REFERENCES: parent: 0, child: -1;
 */
BoxTask Value_Emit_Call(Value *parent, Value *child);

/** Try to call the procedure for the given parent with the given child.
 * If the procedure is not found, than it is blacklisted, so that it won't
 * be possible to define it later.
 * REFERENCES: parent: 0, child: -1;
 */
BoxTask Value_Emit_Call_Or_Blacklist(Value *parent, Value *child);

/** Cast a generic pointer (type BOXTYPE_PTR) to the given type.
 * REFERENCES: return: new, v_ptr: -1;
 */
Value *Value_Cast_From_Ptr(Value *v_ptr, BoxType new_type);

/** Get the next member of a structure. If the given type '*t_memb' is a
 * structure, then return in 'v_memb' its first member. If '*t_memb' is a
 * member (which is the case after one call to this function), then
 * return the next member of the same structure, re-using 'v_memb'.
 * Here is an example which shows how to use the function:
 *
 *  BoxType t_memb = BOXTYPE_NONE;
 *  Value *v_memb = Value_New(...);
 *  Value_Set_As_Weak_Copy(v_memb, v_struc);
 *  do {
 *    v_memb = Value_Struc_Get_Next_Member(v_memb, & t_memb);
 *    if (t_memb == BOXTYPE_NONE)
 *      break;
 *    else {
 *      ... use the member ...
 *    }
 *  }
 *  Value_Unlink(v_memb);
 *
 * REFERENCES: return: new, v_memb: -1;
 */
Value *Value_Struc_Get_Next_Member(Value *v_memb, BoxType *t_memb);

typedef struct {
  int     has_next;
  int     index;
  Value   v_member;
  BoxType t_member;
} ValueStrucIter;

/** Convenience function to facilitate iteration of the member of a structure
 * value. Here is and example of how it is supposed to be used:
 *
 *   ValueStrucIter vsi;
 *   for(ValueStrucIter_Init(& vsi, v_struc, cmpproc);
 *       vsi.has_next; ValueStrucIter_Do_Next(& vsi)) {
 *     // access to 'vsi.member'
 *   }
 *   ValueStrucIter_Finish(& vsi);
 *
 * @see ValueStrucIter_Finish, ValueStrucIter_Do_Next
 */
void ValueStrucIter_Init(ValueStrucIter *vsi, Value *v_struc, CmpProc *proc);

/** Convenience function to facilitate iteration of the member of a structure
 * value.
 * @see ValueStrucIter_Init
 */
void ValueStrucIter_Do_Next(ValueStrucIter *vsi);

/** Convenience function to facilitate iteration of the member of a structure
 * value.
 * @see ValueStrucIter_Init
 */
void ValueStrucIter_Finish(ValueStrucIter *vsi);

/** Creator corresponding to ValueStrucIter_Init.
 * @see ValueStrucIter_Init, ValueStrucIter_Destroy
 */
ValueStrucIter *ValueStrucIter_New(Value *v_struc, CmpProc *proc);

/** Destructor fro ValueStrucIter_New.
 * @see ValueStrucIter_New
 */
void ValueStrucIter_Destroy(ValueStrucIter *vsi);

/** Returns the member 'memb' of the given structure 'v_struc'.
 * Try to transform 'v_struc' into 'v_memb' passing to it the responsabilities
 * for deallocation, etc.
 * REFERENCES: return: new, v_struc: -1;
 */
Value *Value_Struc_Get_Member(Value *v_struc, const char *memb);

/**
 * REFERENCES: dest: 0, src: -1;
 */
BoxTask Value_Move_Content(Value *dest, Value *src);

/** Expand the value 'v' in agreement with the provided expansion type.
 * REFERENCES: return: new, src: -1;
 */
Value *Value_Expand(Value *v, BoxType expansion_type);

/**
 * REFERENCES: v: 0;
 */
void Value_Setup_As_Parent(Value *v, BoxType parent_t);

/**
 * REFERENCES: v: 0;
 */
void Value_Setup_As_Child(Value *v, BoxType child_t);

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

#endif /* _VALUE_H */
