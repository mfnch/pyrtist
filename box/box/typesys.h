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

#  include <box/types.h>
#  include <box/callable.h>

#  include "occupation.h"
#  include "hashtable.h"
#  include "vm_private.h"
#  include "vmalloc.h"

typedef enum {
  TS_KIND_INTRINSIC=1,
  TS_KIND_ALIAS,
  TS_KIND_RAISED,
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
  TS_KS_RAISED=8,
  TS_KS_ANONYMOUS=16
} TSKindSelect;

typedef struct {
  BoxType *new_type;
  TSKind  kind;
  BoxInt  size,
          alignment;
  char    *name;
  void    *val;
  BoxTypeId target,
          first_proc;
  union {
    BoxInt  array_size;
    BoxTypeId last,
            member_next;
    struct {
      BoxCombType combine;
      BoxTypeId     parent;
      BoxUInt     sym_num;
    } proc;
    struct {
      BoxTypeId parent;
      char    *child_name;
    } subtype;
  } data;
} TSDesc;

/* Used to initialise the structure TSDesc */
#define TS_TSDESC_INIT(tsdesc)           \
  do {                                   \
    (tsdesc)->new_type = NULL;           \
    (tsdesc)->val = NULL;                \
    (tsdesc)->name = NULL;               \
    (tsdesc)->first_proc = BOXTYPE_NONE; \
  } while(0)

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

#define BOX_SIZE_UNKNOWN ((BoxInt) -1)

void TS_Init(BoxTS *ts);

void TS_Finish(BoxTS *ts);

/* Transition function to allow setting the new style type correspoding
 * to an old style type.
 */
void TS_Set_New_Style_Type(BoxTS *ts, BoxTypeId old_type, BoxType *new_type);
BoxType *BoxType_From_Id(BoxTS *ts, BoxTypeId id);

/* Transition function to get the new style type associated to an old style
 * type.
 */
BoxType *TS_Get_New_Style_Type(BoxTS *ts, BoxTypeId old_type);

/** Should disappear soon */
void TS_Init_Builtin_Types(BoxTS *ts);

BoxInt TS_Get_Size(BoxTS *ts, BoxTypeId t);

/** If 't' is a special type (BOXTYPE_CREATE, BOXTYPE_DESTROY, etc.)
 * return 't', otherwise return BOXTYPE_NONE.
 */
BoxTypeId TS_Is_Special(BoxTypeId t);

/** Resolve types (obsolete version).
 * ks is a combination (with operator |) of TSKindSelect
 * values which specify what exactly has to be resolved.
 */
BoxTypeId BoxTS_Obsolete_Resolve_Once(BoxTS *ts, BoxTypeId t, TSKindSelect ks);

/** Resolve the type in '*t' following the instructions in 'ks'.
 * 'ks' specifies what to resolve (see 'TSKindSelect'). The type to resolve
 * is taken from '*t' and the resolved type is written in '*t'.
 * 1 is returned if the type was resolved, 0 is returned otherwise.
 * This allows using something like:
 *
 *   BoxTypeId t = ...;
 *   do {
 *     ...
 *   } while (BoxTS_Resolve_Once(ts, & t, ks));
 */
int BoxTS_Resolve_Once(BoxTS *ts, BoxTypeId *t, TSKindSelect ks);

/** TS_Resolve_Once is applied until the type is fully resolved. */
BoxTypeId TS_Resolve(BoxTS *ts, BoxTypeId t, TSKindSelect select);

/** Return the core type of the provided type 't'.
 * This means that alias, species and raised types are resolved and
 * the underlying fundamental type is returned. For example, if:
 *
 *   MyType = ++(Int a, b, c)
 *
 * then the core type is just (Int a, b, c). This is what MyType intrinsically
 * is. This function is used to perform operations where the structure
 * of the type is needed, such as constructing the destructor of MyType.
 */
BoxTypeId TS_Get_Core_Type(BoxTS *ts, BoxTypeId t);

/** Get the fundamental type (Char, Int, ..., Ptr) used to store objects of
 * the given type. Returns BOXTYPE_INT for integers, BOXTYPE_REAL for reals,
 * etc. BOXTYPE_OBJ is returned for objects which are not Char, Int, Real,
 * Point, Ptr.
 */
BoxTypeId TS_Get_Cont_Type(BoxTS *ts, BoxTypeId t);

TSKind TS_Get_Kind(BoxTS *ts, BoxTypeId t);

/** Box types can be subtivided into two cathegories: fast types and
 * slow types. A type is fast if the Box VM has a corresponding register
 * type, otherwise it is slow.
 */
int TS_Is_Fast(BoxTS *ts, BoxTypeId t);

#define TS_Is_Member(ts, t) (TS_Get_Kind((ts), (t)) == TS_KIND_MEMBER)
#define TS_Is_Subtype(ts, t) (TS_Get_Kind((ts), (t)) == TS_KIND_SUBTYPE)
#define TS_Is_Structure(ts, t) (TS_Get_Kind((ts), (t)) == TS_KIND_STRUCTURE)
#define TS_Is_Empty(ts, t) (TS_Get_Size((ts), (t)) == 0)

BoxTypeId BoxTS_New_Intrinsic_With_Name(BoxTS *ts, size_t size,
                                      size_t alignment, const char *name);

/** Convenient function to define an intrinsic Box type out of a C type. */
#define BOXTS_NEW_INTRINSIC(ts, type) \
  BoxTS_New_Intrinsic((ts), sizeof(type), __alignof__(type))

/** Create a new unregistered subtype: a subtype is unregistered when
 * the parent is not aware of its existance. An unregistered type is defined
 * only giving its name and its parent type. When the subtype is registered
 * the parent type becomes aware of it. In order for the registration to be
 * completed the full type of the subtype must be specified.
 */
BoxTypeId TS_Subtype_New(BoxTS *ts, BoxTypeId parent_type,
                       const char *child_name);

/** Get the child type of the given subtype (return BOXTYPE_NONE, if the
 * subtype has not been yet registered)
 */
BoxTypeId TS_Subtype_Get_Child(BoxTS *ts, BoxTypeId st);

/**< Return whether the subtype is already registered */
int TS_Subtype_Is_Registered(BoxTS *ts, BoxTypeId st);

/** Register a previously created (and still unregistered) subtype.
 */
BoxTask
TS_Subtype_Register(BoxTS *ts, BoxTypeId subtype, BoxTypeId subtype_type);

/** Find the registered subtype with name child among the subtypes of type
 * parent. The found subtype is put inside *subtype. If no subtype is found
 * then BOXTYPE_NONE is returned inside *subtype.
 */
BoxTypeId TS_Subtype_Find(BoxTS *ts, BoxTypeId parent, const char *name);

/** Create the string representation of the Type 't'. The returned string
 * has to be freed by the user.
 */
char *TS_Name_Get(BoxTS *ts, BoxTypeId t);

/* Transition function: alternative to BoxTS_New_Alias. */
BoxTypeId BoxTS_New_Alias_With_Name(BoxTS *ts, BoxTypeId origin,
                                  const char *name);

/** Create a new raised type from the type t. The new type (in *d) will be
 * similar to t, but incompatible: TS_Compare will not match the two types.
 */
BoxTypeId BoxTS_New_Raised(BoxTS *ts, BoxTypeId t_origin);

/** Function called to create an empty structure. Members can be added
 * with BoxTS_Add_Struct_Member.
 */
BoxTypeId BoxTS_Begin_Struct(BoxTS *ts);

/** Add a member to a structure type defined with TS_Structure_Begin. */
void BoxTS_Add_Struct_Member(BoxTS *ts, BoxTypeId structure,
                             BoxTypeId member_type, const char *member_name);

/** Function called to create an empty species. Members can be added
 * with TS_Species_Add.
 */
BoxTypeId BoxTS_Begin_Species(BoxTS *ts);

/** Add a member to a species type defined with TS_Species_Begin. */
void BoxTS_Add_Species_Member(BoxTS *ts, BoxTypeId species, BoxTypeId member);

BoxTypeId BoxTS_New_Any(BoxTS *ts);

#endif
