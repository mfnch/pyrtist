/****************************************************************************
 * Copyright (C) 2008 by Matteo Franchin                                    *
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

/* $Id$ */

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
  TSKind kind;
  Int size;
  char *name;
  void *val;
  Type target;
  Type first_proc;
  union {
    Int array_size;
    Type last;
    Type member_next;
    struct {
      int kind;
      Type parent;
      UInt sym_num;
    } proc;
    struct {
      Type parent;
      char *child_name;
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
extern TS *last_ts;

void TS_Init(TS *ts);

void TS_Finish(TS *ts);

/** Should disappear soon */
void TS_Init_Builtin_Types(TS *ts);

Int TS_Get_Size(TS *ts, Type t);

/** True if 't' is an anonymous type: anonymous types are types without
 * a name. A type can be named using 'TS_Name_Set'.
 */
int TS_Is_Anonymous(TS *ts, Type t);

/** Return 1 if 't' is a special type (TYPE_OPEN, TYPE_CLOSE, etc.)
 */
int TS_Is_Special(Type t);

/** Resolve types (useful for comparisons).
 * select is a combination (with operator |) of TSKindSelect
 * values which specify what exactly has to be resolved.
 */
Type TS_Resolve_Once(TS *ts, Type t, TSKindSelect select);

/** TS_Resolve_Once is applied until the type is fully resolved. */
Type TS_Resolve(TS *ts, Type t, TSKindSelect select);

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
Type TS_Get_Core_Type(TS *ts, Type t);

/** Get the fundamental type (Char, Int, ..., Ptr) used to store objects of
 * the given type. Returns BOXTYPE_INT for integers, BOXTYPE_REAL for reals,
 * etc. BOXTYPE_PTR is returned for objects which are not Char, Int, Real,
 * Point, Ptr.
 */
BoxType TS_Get_Cont_Type(TS *ts, BoxType t);

TSKind TS_Get_Kind(TS *ts, Type t);

/** Box types can be subtivided into two cathegories: fast types and
 * slow types. A type is fast if the Box VM has a corresponding register
 * type, otherwise it is slow.
 */
int TS_Is_Fast(TS *ts, Type t);

/** A structure is fast if it contains only fast types. fast structures do not
 * need to have an iterator. They do not require to propagate the basic
 * methods.
 * NOTE: be careful, a fast structure is not a fast type itself.
 */
int TS_Structure_Is_Fast(TS *ts, Type structure);

#define TS_Is_Member(ts, t) (TS_Get_Kind((ts), (t)) == TS_KIND_MEMBER)
#define TS_Is_Subtype(ts, t) (TS_Get_Kind((ts), (t)) == TS_KIND_SUBTYPE)
#define TS_Is_Structure(ts, t) (TS_Get_Kind((ts), (t)) == TS_KIND_STRUCTURE)

Int TS_Align(TS *ts, Int address);

void TS_Intrinsic_New(TS *ts, Type *i, Int size);

/** Create a new procedure type in p. init tells if the procedure
 * is an initialisation procedure or not.
 */
BoxType TS_Procedure_New(TS *ts, BoxType parent, BoxType child, int kind);

/** Get information about the procedure p. This information is stored
 * in the destination specified by the given pointers, but this happens
 * only if the pointer is != NULL.
 * @param ts the type-system data structure
 * @param parent the parent of the procedure
 * @param child the children of the procedure
 * @param kind the kind of the procedure
 * @param sym_num the symbol identification (for registered procedures)
 */
void TS_Procedure_Info(TS *ts, Type *parent, Type *child,
                       int *kind, UInt *sym_num, Type p);

/** Register the procedure p. After this function has been called
 * the procedure p will belong to the list of procedures of its parent.
 * sym_num is the symbol associated with the procedure.
 */
void TS_Procedure_Register(TS *ts, Type p, UInt sym_num);

/** Unregister a procedure which was previously registered with 
 * TS_Procedure_Register 
 */
void TS_Procedure_Unregister(TS *ts, BoxType p);

/** Obtain the symbol identification number of a registered procedure.
 */
UInt TS_Procedure_Get_Sym(TS *ts, Type p);

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
BoxType TS_Procedure_Search(TS *ts, Type *expansion_type,
                            Type child, Type parent, TSSearchMode mode);

/** Put the procedure in the blacklist. Blacklisted procedure are cannot be
 * registered. The feature is important to ensure that a procedure that
 * was assumed not to be registered when emitting the bytecode is not
 * registered later.
 * @see TS_Procedure_Blacklist_Undo
 */
void TS_Procedure_Blacklist(TS *ts, BoxType child, BoxType type);

/** Undo a blacklist operation.
 * @see TS_Procedure_Blacklist
 */
void TS_Procedure_Blacklist_Undo(TS *ts, BoxType child, BoxType parent);

/** Return 1 if the procedure is blacklisted, 0 otherwise. */
int TS_Procedure_Is_Blacklisted(TS *ts, BoxType child, BoxType parent);

/** Return whether a procedure is registered (does just check if one
 * of the registered procedures of the parent of 'p' has type ID matching
 * with 'p')
 */
int TS_Procedure_Is_Registered(TS *ts, BoxType p);

/** Create and register a procedure by calling firt TS_Procedure_New
 * and then TS_Procedure_Register. sym_num is the associated symbol
 * identifier.
 */
Int TS_Procedure_Def(Int proc, int kind, Int of_type, Int sym_num);

/** Create a new unregistered subtype: a subtype is unregistered when
 * the parent is not aware of its existance. An unregistered type is defined
 * only giving its name and its parent type. When the subtype is registered
 * the parent type becomes aware of it. In order for the registration to be
 * completed the full type of the subtype must be specified.
 */
void TS_Subtype_New(TS *ts, Type *new_subtype,
                    Type parent_type, Name *child_name);

/** Register a previously created (and still unregistered) subtype.
 */
Task TS_Subtype_Register(TS *ts, Type subtype, Type subtype_type);

/** Find the registered subtype with name child among the subtypes of type
 * parent. The found subtype is put inside *subtype. If no subtype is found
 * then TS_TYPE_NONE is returned inside *subtype.
 */
void TS_Subtype_Find(TS *ts, Type *subtype, Type parent, Name *child);

/** Associate the name 'name' to the anonymous type 't'. */
Task TS_Name_Set(TS *ts, Type t, const char *name);

/** Create the string representation of the Type 't'. The returned string
 * has to be freed by the user.
 */
char *TS_Name_Get(TS *ts, Type t);

/** Create a new alias type from the type 'origin'. */
void TS_Alias_New(TS *ts, Type *alias, Type origin);

/** Create a new detached type from the type t. The new type (in *d) will be
 * identical to t, but incompatible: TS_Compare will not match the two types.
 */
void TS_Detached_New(TS *ts, Type *detached, Type origin);

/** Create a new array type, with 'item_num' items with type 'item'. */
void TS_Array_New(TS *ts, Type *array, Type item, Int num_items);

/** Function called to create an empty structure. Members can be added
 * with TS_Structure_Add.
 */
void TS_Structure_Begin(TS *ts, Type *structure);

/** Add a member to a structure type defined with TS_Structure_Begin. */
void TS_Structure_Add(TS *ts, Type structure, Type member_type,
                      const char *member_name);

/** Function called to create an empty species. Members can be added
 * with TS_Species_Add.
 */
void TS_Species_Begin(TS *ts, Type *species);

/** Add a member to a species type defined with TS_Species_Begin. */
void TS_Species_Add(TS *ts, Type species, Type member);

/** Function called to create an empty enumeration. Members can be added
 * with TS_Structure_Enum.
 */
void TS_Enum_Begin(TS *ts, Type *enumeration);

/** Add a member to an enumeration type defined with TS_Enum_Begin. */
void TS_Enum_Add(TS *ts, Type enumeration, Type member);

/** Get the child type of a subtype. */
void TS_Subtype_Get_Child(TS *ts, Type *child, Type subtype);

/** Get the parent type of a subtype. */
void TS_Subtype_Get_Parent(TS *ts, Type *parent, Type subtype);

Task TS_Default_Value(TS *ts, Type *dv_t, Type t, Data *dv);

/** Given an array type (N)X returns X in *memb and the size N
 * of the array in *array_size.
 * An error is generated if array is not an array type.
 */
Task TS_Array_Member(TS *ts, Type *memb, Type array, Int *array_size);

/** Search the member 'm_name' from the members of the structure s.
 * If the member is found then return its type number, otherwise return
 * BOXTYPE_NONE
 */
BoxType TS_Member_Find(TS *ts, Type s, const char *m_name);

/** Obtain details about a member of a structure (to be used in conjunction
 * with TS_Member_Find)
 */
BoxType TS_Member_Get(TS *ts, Type m, size_t *address);

/** Obtain the name of a member from its type descriptor. */
char *TS_Member_Name_Get(TS *ts, Type member);

/** If m is a structure/species/enum returns its first member.
 * If m is a member, return the next member.
 * It m is the last member, return the parent structure.
 */
Type TS_Get_Next_Member(TS *ts, Type m);

/** Counts the member of a structure/species/enum (using TS_Member_Next) */
Int TS_Member_Count(TS *ts, Type s);

/** This function tells if a type t2 is contained into a type t1.
 * The return value is the following:
 * - TS_TYPES_EQUAL: the two types are equal;
 * - TS_TYPES_MATCH: the two types are equal;
 * - TS_TYPES_EXPAND: the two types are compatible, but type t2 should
 *    be expanded to type t1;
 * - TS_TYPES_UNMATCH: the two types are not compatible;
 */
TSCmp TS_Compare(TS *ts, Type t1, Type t2);

#  if 1

/* Stringhe corrispondenti ai tipi di tipi */
#define TOT_DESCRIPTIONS { \
 "instance of ", "pointer to", "array of ", "set of types", "procedure " }

/* Enumeration of special procedures */
enum {
  PROC_COPY    = -1,
  PROC_DESTROY = -2,
  PROC_SPECIAL_NUM = 3
};

/* Important builtin types */
extern Int type_Point, type_RealNum, type_IntgNum, type_CharNum;

Int Tym_Type_Size(Int t);
Int Tym_Struct_Get_Num_Items(Int t);
const char *Tym_Type_Name(Int t);
char *Tym_Type_Names(Int t);
Task Tym_Def_Type(Int *new_type,
 Int parent, Name *nm, Int size, Int aliased_type);
Int Tym_Def_Array_Of(Int num, Int type);
Int Tym_Def_Pointer_To(Int type);
Int Tym_Def_Alias_Of(Name *nm, Int type);
int Tym_Compare_Types(Int type1, Int type2, int *need_expansion);
Int Tym_Type_Resolve(Int type, int not_alias, int not_species);
#define Tym_Type_Resolve_Alias(type) Tym_Type_Resolve(type, 0, 1)
#define Tym_Type_Resolve_Species(type) Tym_Type_Resolve(type, 1, 0)
#define Tym_Type_Resolve_All(type) Tym_Type_Resolve(type, 0, 0)
Int Tym_Def_Procedure(Int proc, int second, Int of_type, Int sym_num);
void Tym_Print_Procedure(FILE *stream, Int of_type);
Task Tym_Def_Specie(Int *specie, Int type);
Task Tym_Def_Structure(Int *strc, Int type);
Task Tym_Structure_Get(Int *type);
Task Tym_Specie_Get(Int *type);
Int Tym_Specie_Get_Target(Int type);

/*#define Tym_Def_Explicit(new_type, nm) \
  Tym_Def_Type(new_type, TYPE_NONE, nm, (Int) 0, TYPE_NONE)*/
#define Tym_Def_Explicit_Alias(new_type, nm, type) \
  Tym_Def_Type(new_type, TYPE_NONE, nm, (Int) -1, type)
#define Tym_Def_Intrinsic(new_type, nm, size) \
  Tym_Def_Type(new_type, TYPE_NONE, nm, size, TYPE_NONE)
#  endif

#endif
