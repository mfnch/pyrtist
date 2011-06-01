/****************************************************************************
 * Copyright (C) 2008-2011 by Matteo Franchin                               *
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

/** @file typesys.h
 * @brief The type system for the Box compiler.
 *
 * Some words.
 */

#ifndef _TYPESYS_H
#  define _TYPESYS_H

#  include <stdlib.h>

#  include "types.h"
#  include "occupation.h"
#  include "hashtable.h"
#  include "virtmach.h"
#  include "vmalloc.h"

typedef enum {
  TS_KIND_INTRINSIC=1,
  TS_KIND_ALIAS,
  TS_KIND_DETACHED,
  TS_KIND_SPECIES,
  TS_KIND_STRUCTURE,
  TS_KIND_ENUM,
  TS_KIND_MEMBER,
  TS_KIND_ARRAY,
  TS_KIND_PROC,
  TS_KIND_METHOD,
  TS_KIND_POINTER,
  TS_KIND_SUBTYPE
} TSKind;

enum {
  TS_TYPE_NONE = TYPE_NONE,
  TS_SIZE_UNKNOWN = TYPE_NONE
};

/** Used with the TS_Resolve & co. to specify how the type should be resolved.
 * NOTE: TS_KS_ANONYMOUS has a special interpretation. It is used to specify
 *  that only the anonymous types should be resolved, while the types with
 *  name should not be resolved. If TS_KS_ANONYMOUS is not used, then both
 *  anonymous and named types are resolved.
 */
typedef enum {
  TS_KS_NONE=0,
  TS_KS_ALIAS=1,
  TS_KS_SPECIES=2,
  TS_KS_SUBTYPE=4,
  TS_KS_DETACHED=8,
  TS_KS_ANONYMOUS=16
} TSKindSelect;

typedef struct {
  TSKind  kind;
  BoxInt  size;
  char    *name;
  void    *val;
  BoxType target,
          first_proc;
  union {
    BoxInt  array_size;
    BoxType last,
            member_next;
    struct {
      BoxComb combine;
      BoxType parent;
      BoxUInt sym_num;
    } proc;
    struct {
      BoxType parent;
      char    *child_name;
    } subtype;
  } data;
} TSDesc;

/* Used to initialise the structure TSDesc */
#define TS_TSDESC_INIT(tsdesc) \
  (tsdesc)->val = (char *) NULL; \
  (tsdesc)->name = (char *) NULL; \
  (tsdesc)->first_proc = TS_TYPE_NONE

typedef struct {
  BoxOcc type_descs;
  BoxHT  members;
  BoxHT  subtypes;
  BoxArr name_buffer;
} BoxTS;

typedef BoxTS TS;

typedef enum {
  TS_TYPES_EQUAL=7,
  TS_TYPES_MATCH=3,
  TS_TYPES_EXPAND=1,
  TS_TYPES_UNMATCH=0
} TSCmp;

/** Used by the old Tym_* functions, to be removed in the future! */
extern BoxTS *last_ts;

void TS_Init(BoxTS *ts);

void TS_Finish(BoxTS *ts);

/** Should disappear soon */
void TS_Init_Builtin_Types(BoxTS *ts);

BoxInt TS_Get_Size(BoxTS *ts, BoxType t);

/** True if 't' is an anonymous type: anonymous types are types without
 * a name. A type can be named using 'TS_Name_Set'.
 */
int TS_Is_Anonymous(BoxTS *ts, BoxType t);

/** If 't' is a special type (BOXTYPE_CREATE, BOXTYPE_DESTROY, etc.)
 * return 't', otherwise return BOXTYPE_NONE.
 */
BoxType TS_Is_Special(BoxType t);

/** Resolve types (useful for comparisons).
 * select is a combination (with operator |) of TSKindSelect
 * values which specify what exactly has to be resolved.
 */
BoxType TS_Resolve_Once(BoxTS *ts, BoxType t, TSKindSelect select);

/** TS_Resolve_Once is applied until the type is fully resolved. */
BoxType TS_Resolve(BoxTS *ts, BoxType t, TSKindSelect select);

/** Return the core type of the provided type 't'.
 * This means that alias, species and detached types are resolved and
 * the underlying fundamental type is returned. For example, if:
 *
 *   MyType = ++(Int a, b, c)
 *
 * then the core type is just (Int a, b, c). This is what MyType intrinsically
 * is. This function is used to perform operations where the structure
 * of the type is needed, such as constructing the destructor of MyType.
 */
BoxType TS_Get_Core_Type(BoxTS *ts, BoxType t);

/** Get the fundamental type (Char, Int, ..., Ptr) used to store objects of
 * the given type. Returns BOXTYPE_INT for integers, BOXTYPE_REAL for reals,
 * etc. BOXTYPE_OBJ is returned for objects which are not Char, Int, Real,
 * Point, Ptr.
 */
BoxType TS_Get_Cont_Type(BoxTS *ts, BoxType t);

TSKind TS_Get_Kind(BoxTS *ts, BoxType t);

/** Box types can be subtivided into two cathegories: fast types and
 * slow types. A type is fast if the Box VM has a corresponding register
 * type, otherwise it is slow.
 */
int TS_Is_Fast(BoxTS *ts, BoxType t);

/** A structure is fast if it contains only fast types. fast structures do not
 * need to have an iterator. They do not require to propagate the basic
 * methods.
 * NOTE: be careful, a fast structure is not a fast type itself.
 */
int TS_Structure_Is_Fast(BoxTS *ts, BoxType structure);

#define TS_Is_Member(ts, t) (TS_Get_Kind((ts), (t)) == TS_KIND_MEMBER)
#define TS_Is_Subtype(ts, t) (TS_Get_Kind((ts), (t)) == TS_KIND_SUBTYPE)
#define TS_Is_Structure(ts, t) (TS_Get_Kind((ts), (t)) == TS_KIND_STRUCTURE)
#define TS_Is_Empty(ts, t) (TS_Get_Size((ts), (t)) == 0)

BoxInt TS_Align(BoxTS *ts, BoxInt address);

BoxType TS_Intrinsic_New(BoxTS *ts, BoxInt size);

/** Create a new procedure type in p. init tells if the procedure
 * is an initialisation procedure or not.
 */
BoxType BoxTS_Procedure_New(BoxTS *ts, BoxType child,
                            BoxComb comb, BoxType parent);

/** Get information about the procedure p. This information is stored
 * in the destination specified by the given pointers, but this happens
 * only if the pointer is != NULL.
 * @param ts the type-system data structure
 * @param parent the parent of the procedure
 * @param child the children of the procedure
 * @param kind the kind of the procedure
 * @param sym_num the symbol identification (for registered procedures)
 */
void BoxTS_Procedure_Info(BoxTS *ts, BoxType *parent, BoxType *child,
                          BoxComb *comb, BoxVMSymID *sym_num, BoxType p);

/** Register the procedure p. After this function has been called
 * the procedure p will belong to the list of procedures of its parent.
 * sym_num is the symbol associated with the procedure.
 */
void BoxTS_Procedure_Register(BoxTS *ts, BoxType p,
                              BoxVMSymID sym_num);

/** Unregister a procedure which was previously registered with
 * BoxTS_Procedure_Register
 */
void BoxTS_Procedure_Unregister(BoxTS *ts, BoxComb comb, BoxType p);

/** Obtain the symbol identification number of a registered procedure.
 */
BoxUInt TS_Procedure_Get_Sym(BoxTS *ts, BoxType p);

/** Options to be used when searching for procedure */
typedef enum {
  TSSEARCHMODE_INHERITED=1, /**< Search procedures inherited from extended
                                 types */
  TSSEARCHMODE_BLACKLIST=2  /**< If the procedure is not found,
                                 blacklist it! */
} TSSearchMode;

/** Search the given procedure in the list of registered procedures.
 * Return the procedure in *proc, or TS_TYPE_NONE if the procedure
 * has not been found. If the argument of the procedure needs to be expanded
 * *expansion_type is the target type for that expansion. If expansion
 * is not needed then *expansion_type = TS_TYPE_NONE.
 */
BoxType BoxTS_Procedure_Search(BoxTS *ts, BoxType *expansion_type,
                               BoxType child, BoxComb comb, BoxType parent,
                               TSSearchMode mode);

/** Put the procedure in the blacklist. Blacklisted procedure are cannot be
 * registered. The feature is important to ensure that a procedure that
 * was assumed not to be registered when emitting the bytecode is not
 * registered later.
 * @see TS_Procedure_Blacklist_Undo
 */
void TS_Procedure_Blacklist(BoxTS *ts, BoxType child, BoxType type);

/** Undo a blacklist operation.
 * @see TS_Procedure_Blacklist
 */
void TS_Procedure_Blacklist_Undo(BoxTS *ts, BoxType child, BoxType parent);

/** Return 1 if the procedure is blacklisted, 0 otherwise. */
int TS_Procedure_Is_Blacklisted(BoxTS *ts, BoxType child, BoxType parent);

/** Return whether a procedure is registered (does just check if one
 * of the registered procedures of the parent of 'p' has type ID matching
 * with 'p')
 */
int BoxTS_Procedure_Is_Registered(BoxTS *ts, BoxComb comb, BoxType p);

/** Create and register a procedure by calling firt TS_Procedure_New
 * and then TS_Procedure_Register. sym_num is the associated symbol
 * identifier.
 */
BoxInt TS_Procedure_Def(BoxInt proc, BoxInt of_type, BoxInt sym_num);

/** Create a new unregistered subtype: a subtype is unregistered when
 * the parent is not aware of its existance. An unregistered type is defined
 * only giving its name and its parent type. When the subtype is registered
 * the parent type becomes aware of it. In order for the registration to be
 * completed the full type of the subtype must be specified.
 */
BoxType TS_Subtype_New(BoxTS *ts, BoxType parent_type,
                       const char *child_name);

/** Get the parent type of the given subtype */
BoxType TS_Subtype_Get_Parent(BoxTS *ts, BoxType st);

/** Get the child type of the given subtype (return BOXTYPE_NONE, if the
 * subtype has not been yet registered)
 */
BoxType TS_Subtype_Get_Child(BoxTS *ts, BoxType st);

/**< Return whether the subtype is already registered */
int TS_Subtype_Is_Registered(BoxTS *ts, BoxType st);

/** Register a previously created (and still unregistered) subtype.
 */
BoxTask TS_Subtype_Register(BoxTS *ts, BoxType subtype, BoxType subtype_type);

/** Find the registered subtype with name child among the subtypes of type
 * parent. The found subtype is put inside *subtype. If no subtype is found
 * then TS_TYPE_NONE is returned inside *subtype.
 */
BoxType TS_Subtype_Find(BoxTS *ts, BoxType parent, const char *name);

/** Associate the name 'name' to the anonymous type 't'. */
BoxTask TS_Name_Set(BoxTS *ts, BoxType t, const char *name);

/** Create the string representation of the Type 't'. The returned string
 * has to be freed by the user.
 */
char *TS_Name_Get(BoxTS *ts, BoxType t);

/** Create a new alias type from the type 'origin'. */
BoxType TS_Alias_New(BoxTS *ts, BoxType origin);

/** Create a new detached type from the type t. The new type (in *d) will be
 * similar to t, but incompatible: TS_Compare will not match the two types.
 */
BoxType TS_Detached_New(BoxTS *ts, BoxType t_origin);

/** Create a new array type, with 'item_num' items with type 'item'. */
void TS_Array_New(BoxTS *ts, BoxType *array, BoxType item, BoxInt num_items);

/** Function called to create an empty structure. Members can be added
 * with TS_Structure_Add.
 */
BoxType TS_Structure_Begin(BoxTS *ts);

/** Add a member to a structure type defined with TS_Structure_Begin. */
void TS_Structure_Add(BoxTS *ts, BoxType structure, BoxType member_type,
                      const char *member_name);

/** Function called to create an empty species. Members can be added
 * with TS_Species_Add.
 */
void TS_Species_Begin(BoxTS *ts, BoxType *species);

/** Add a member to a species type defined with TS_Species_Begin. */
void TS_Species_Add(BoxTS *ts, BoxType species, BoxType member);

/** Function called to create an empty enumeration. Members can be added
 * with TS_Structure_Enum.
 */
void TS_Enum_Begin(BoxTS *ts, BoxType *enumeration);

/** Add a member to an enumeration type defined with TS_Enum_Begin. */
void TS_Enum_Add(BoxTS *ts, BoxType enumeration, BoxType member);

BoxTask TS_Default_Value(BoxTS *ts, BoxType *dv_t, BoxType t, Data *dv);

/** Given an array type (N)X returns X in *memb and the size N
 * of the array in *array_size.
 * An error is generated if array is not an array type.
 */
BoxTask TS_Array_Member(BoxTS *ts, BoxType *memb, BoxType array,
                        BoxInt *array_size);

/** Search the member 'm_name' from the members of the structure s.
 * If the member is found then return its type number, otherwise return
 * BOXTYPE_NONE
 */
BoxType TS_Member_Find(BoxTS *ts, BoxType s, const char *m_name);

/** Obtain details about a member of a structure (to be used in conjunction
 * with TS_Member_Find)
 */
BoxType TS_Member_Get(BoxTS *ts, BoxType m, size_t *address);

/** Obtain the name of a member from its type descriptor. */
char *TS_Member_Name_Get(BoxTS *ts, BoxType member);

/** If m is a structure/species/enum returns its first member.
 * If m is a member, return the next member.
 * It m is the last member, return the parent structure.
 */
BoxType TS_Get_Next_Member(BoxTS *ts, BoxType m);

/** Counts the member of a structure/species/enum (using TS_Member_Next) */
BoxInt TS_Member_Count(BoxTS *ts, BoxType s);

/** This function tells if a type t2 is contained into a type t1.
 * The return value is the following:
 * - TS_TYPES_EQUAL: the two types are equal;
 * - TS_TYPES_MATCH: the two types are equal;
 * - TS_TYPES_EXPAND: the two types are compatible, but type t2 should
 *    be expanded to type t1;
 * - TS_TYPES_UNMATCH: the two types are not compatible;
 */
TSCmp TS_Compare(BoxTS *ts, BoxType t1, BoxType t2);

/** Object used to iterate over the members of a structure.
 * It should be used like this:
 *
 *   BoxTSStrucIt it;
 *   for (BoxTSStrucIt_Init(ts, & it, s); it.has_more;
          BoxTSStrucIt_Advance(& it)) {
 *     // Member is accessible as ``it.member''
 *     // Member position inside structure as ``it.position''
 *   }
 *   BoxTSStrucIt_Finish(& it);
 *
 */
typedef struct {
  TS      *ts;
  int     has_more;
  BoxType member;
  TSDesc  *td;
  size_t  position;
} BoxTSStrucIt;

/** Initialise the iterator object.
 * @see BoxTSStrucIt
 */
void BoxTSStrucIt_Init(BoxTS *ts, BoxTSStrucIt *it, BoxType s);

/** ``Increment'' the iterator.
 * @see BoxTSStrucIt
 */
void BoxTSStrucIt_Advance(BoxTSStrucIt *it);

/** Finalise the iterator object.
 * @see BoxTSStrucIt
 */
#define BoxTSStrucIt_Finish(i)

#endif
