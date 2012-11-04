/****************************************************************************
 * Copyright (C) 2012 by Matteo Franchin                                    *
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
 * @file ntypes.h
 * @brief Declaration of BoxType and the associated functionality.
 *
 * This header defines the BoxType object and most of the functions needed
 * to create BoxType objects, manipulate them and retrieve informations from
 * them.
 */

#ifndef _BOX_TYPES_H
#  define _BOX_TYPES_H

#  include <stdlib.h>

#  define BOX_USE_NEW_OBJ 1

#  include <box/core.h>

/**
 * Integers associated to the fundamental types. These constant values are
 * used internally for caching combinations and - in general - for speeding
 * up the type system (or at least speeding up usage of fundamental types).
 */
typedef enum {
  BOXTYPEID_NONE  =-1,
  BOXTYPEID_FAST_LOWER = 0,
  BOXTYPEID_CHAR  = 0,
  BOXTYPEID_INT   = 1,
  BOXTYPEID_REAL  = 2,
  BOXTYPEID_POINT = 3,
  BOXTYPEID_PTR   = 4,
  BOXTYPEID_FAST_UPPER = 4,
  BOXTYPEID_OBJ   = 5,
  BOXTYPEID_VOID,
  BOXTYPEID_INIT,
  BOXTYPEID_FINISH,
  BOXTYPEID_COPY,
  BOXTYPEID_BEGIN,
  BOXTYPEID_END,
  BOXTYPEID_PAUSE,
  BOXTYPEID_CPTR,
  BOXTYPEID_TYPE,
  BOXTYPEID_MAX_VAL,
} BoxTypeId;


#  include <box/container.h>

/**
 * Type class (whether a type is a structure, a species, etc).
 */
typedef enum {
  BOXTYPECLASS_NONE,
  BOXTYPECLASS_STRUCTURE_NODE,
  BOXTYPECLASS_SPECIES_NODE,
  BOXTYPECLASS_ENUM_NODE,
  BOXTYPECLASS_COMB_NODE,
  BOXTYPECLASS_SUBTYPE_NODE,
  BOXTYPECLASS_PRIMARY,
  BOXTYPECLASS_INTRINSIC,
  BOXTYPECLASS_IDENT,
  BOXTYPECLASS_RAISED,
  BOXTYPECLASS_STRUCTURE,
  BOXTYPECLASS_SPECIES,
  BOXTYPECLASS_ENUM,
  BOXTYPECLASS_FUNCTION,
  BOXTYPECLASS_POINTER,
  BOXTYPECLASS_ANY,
} BoxTypeClass;

/**
 * A type in the Box type system. This is currently implemented as a
 * pointer to an opaque structure.
 */
typedef struct BoxType_struct BoxType;

/**
 * Combination type.
 */
typedef enum {
  BOXCOMBTYPE_AT,
  BOXCOMBTYPE_COPY,
  BOXCOMBTYPE_MOVE
} BoxCombType;

/**
 * Initialize a BoxPtr pointer from a single pointer (BoxSPtr).
 */
#define BoxPtr_Init_From_SPtr(obj, sptr) \
  do {(obj)->block = (char *) (sptr) - sizeof(BoxObjHeader); \
      (obj)->ptr = (sptr);} while(0)

/**
 * Initialize a BoxPtr pointer from a single pointer (BoxSPtr).
 */
#define BoxPtr_Add_Offset(dst, src, offset)                       \
  do {(dst)->block = (src)->block;                                \
      (dst)->ptr = (char *) (src)->ptr + (offset);} while(0)


/**
 * Value which determines the relationship between two types left and right.
 * This is what is returned by BoxType_Compare.
 */
typedef enum {
  BOXTYPECMP_DIFFERENT = 0x0, /**< the two types are different. */
  BOXTYPECMP_MATCHING  = 0x1, /**< right can be expanded to left. */
  BOXTYPECMP_EQUAL     = 0x3, /**< right and left are equal. */
  BOXTYPECMP_SAME      = 0x7  /**< right and left are the same type. */
} BoxTypeCmp;

/**
 * Type used to specify which type classes should be resolved in
 * BoxType_Resolve.
 */
typedef enum {
  BOXTYPERESOLVE_IDENT   = 0x01,
  BOXTYPERESOLVE_SPECIES = 0x02,
  BOXTYPERESOLVE_RAISED  = 0x04,
  BOXTYPERESOLVE_POINTER = 0x08,
  BOXTYPERESOLVE_SUBTYPE = 0x10
} BoxTypeResolve;

/*****************************************************************************
 * TYPE CREATION ROUTINES                                                    *
 *****************************************************************************/

/* Transition functions. */
BOXEXPORT BoxTypeId
BoxType_Get_Id(BoxType *t);

BOXEXPORT void
BoxType_Set_Id(BoxType *t, BoxTypeId id);

/**
 * Get the type class of a given type. The type class is effectively the
 * type of type (the answer to whether the type is a struct, a species, etc.)
 * @param t The input type.
 * @return The type class of t.
 */
BOXEXPORT BoxTypeClass
BoxType_Get_Class(BoxType *t);

/**
 * Get the data part of a type. The size and composition of the data type of
 * a given type changes depending on the type class.
 */
BOXEXPORT void *
BoxType_Get_Data(BoxType *t);

/**
 * Remove a reference to the given type.
 */
#define BoxType_Unlink(t) ((BoxType *) BoxSPtr_Unlink(t))

/**
 * Add a reference to the given type.
 */
#define BoxType_Link(t) ((BoxType *) BoxSPtr_Link(t))

/**
 * Create an instance of a primary type with the given id, size and alignment.
 * An primary type is one of the builtin core types.
 * @param id The ID associated to the primary type.
 * @param size The size of the type (in bytes).
 * @param alignment The alignment for the type.
 * @return A new primary type (or BOXTYPE_NONE if errors occurred).
 */
BOXEXPORT BoxType *
BoxType_Create_Primary(BoxTypeId id, size_t size, size_t alignment);

/**
 * Create a new intrinsic type with the given name, size and alignment.
 * An intrinsic type can be imagined as a descriptor for an atomic
 * (i.e. indivisible) portion of memory.
 * @param size The size of the type (in bytes).
 * @param alignment The alignment for the type.
 * @return A new intrinsic type (or BOXTYPE_NONE if errors occurred).
 */
BOXEXPORT BoxType *
BoxType_Create_Intrinsic(size_t size, size_t alignment);

/**
 * Create a new type identifier from the type 'source'. A type identifier
 * is a type which can be referred using a name. It can be put inside the
 * namespace of a parent type and has itself a namespace which can contain
 * children types. An identifier type can also have combinations and
 * subtypes. It is thus a central concept for organising the type hierarchy
 * in the Box language.
 * @param source The target type to which this identifier refers to.
 * @param name The name to use for identifying the type.
 * @return A new type identifier (or BOXTYPE_NONE in case of errors).
 */
BOXEXPORT BOXOUT BoxType *
BoxType_Create_Ident(BOXIN BoxType *source, const char *name);

/**
 * Create a new raised type from the type 'source'. The new type will be
 * similar to t, but incompatible: BoxType_Compare will not match the two
 * types. As a consequence, the new raised type will not share the combinations
 * of its source type. Notice, however, that an object whose type is raised
 * can be unraised to obtain an object having the source type.
 * @param source The source type.
 * @return A new raised type.
 */
BOXEXPORT BoxType *
BoxType_Create_Raised(BoxType *source);

/**
 * Un-raise a raised type. For example, if r is the raised type of t, then
 * BoxType_Unraise(r) returns t.
 * @param raised A raised type.
 * @return The unraised type.
 */
BOXEXPORT BoxType *
BoxType_Unraise(BoxType *raised);

/**
 * Create an empty structure. Members can be added with
 * BoxType_Add_Member_To_Structure.
 */
BOXEXPORT BOXOUT BoxType *
BoxType_Create_Structure(void);

/**
 * Add a member to a structure type defined with BoxType_Create_Structure.
 * Note that this function does not check for members with duplicate name.
 * @param structure A type created with BoxType_Create_Structure.
 * @param member The type of the member.
 * @param member_name The member name.
 */
BOXEXPORT void
BoxType_Add_Member_To_Structure(BoxType *structure, BoxType *member,
                                const char *member_name);

/**
 */
BOXEXPORT BoxType *
BoxType_Get_Species_Target(BoxType *node);

/**
 * Get information on a structure member as obtained from
 * #BoxTypeIter_Get_Next.
 */
BOXEXPORT BoxBool
BoxType_Get_Structure_Member(BoxType *node, char **name, size_t *offset,
                             size_t *size, BoxType **type);

/**
 * Get the type of a structure member obtained from BoxTypeIter_Get_Next.
 * This is a convenience function to be used as a replacement of
 * BoxType_Get_Struct_Member in the case where only the member type is needed.
 * @param node The type node as obtained from BoxTypeIter_Get_Next.
 * @return The type of the member.
 */
BOXEXPORT BoxType *
BoxType_Get_Structure_Member_Type(BoxType *node);

/**
 * Find the member of a structure with the given name.
 * @param structure The input structure.
 * @param name The name of the structure member.
 * @return If the member is found, return the type node of the member (which
 *   can be used with BoxType_Get_Structure_Member), otherwise return NULL.
 */
BOXEXPORT BoxType *
BoxType_Find_Structure_Member(BoxType *structure, const char *name);

/**
 * Get the number of members of the structure.
 */
BOXEXPORT size_t
BoxType_Get_Structure_Num_Members(BoxType *t);

/**
 * Create an empty species. Members can be added with
 * BoxType_Add_Member_To_Species.
 */
BOXEXPORT BOXOUT BoxType *
BoxType_Create_Species(void);

/**
 * Add a member to a species type defined with BoxType_Create_Species.
 */
BOXEXPORT void
BoxType_Add_Member_To_Species(BoxType *species, BoxType *member);

/**
 * Get the type of a species member as obtained from BoxTypeIter_Get_Next.
 * @param node The type node as obtained from BoxTypeIter_Get_Next.
 * @return The type of the member.
 */
BOXEXPORT BoxType *
BoxType_Get_Species_Member_Type(BoxType *node);

/**
 * Create a species of type '(*=>Dest)' (everything is converted to 'Dest').
 */
BOXEXPORT BoxType *
BoxType_Create_Star_Species(BoxType *dest);

/** Create an empty enum type. */
BOXEXPORT BoxType *
BoxType_Create_Enum(void);

/** Add a member to an enum type cerated with BoxType_New_Enum. */
BOXEXPORT void
BoxType_Add_Member_To_Enum(BoxType *member, const char *member_name);

/**
 * Create a new function type taking 'child' as an argument and working
 * on 'parent'.
 * @param parent The type of the value returned by the function.
 * @param child The type of the argument of the function.
 * @return A new type corresponding to the specified function.
 */
BOXEXPORT BOXOUT BoxType *
BoxType_Create_Callable(BoxType *parent, BoxType *child);

/**
 * @brief Get the parent of a callable type.
 *
 * @param callable The callable type.
 * @return Return the callable parent type.
 * @note If @p callable is @c NULL, then @c NULL is returned.
 */
BOXEXPORT BoxType *
BoxType_Get_Callable_Parent(BoxType *callable);

/**
 * @brief Get the child of a callable type.
 *
 * @param callable The callable type.
 * @return Return the callable child type.
 * @note If @p callable is @c NULL, then @c NULL is returned.
 */
BOXEXPORT BoxType *
BoxType_Get_Callable_Child(BoxType *callable);

/**
 * Create a new pointer type to 'source'.
 * @param target The type of the pointer target.
 * @return A new pointer type.
 */
BOXEXPORT BoxType *
BoxType_Create_Pointer(BoxType *target);

/**
 * Create a new Any type.
 */
BOXEXPORT BoxType *
BoxType_Create_Any(void);

/**
 * Resolve the given type. Resolution is an operation on types which allow
 * obtaining the original type one type refers to. For example, resolving a
 * species allows obtaining the target type of the species and resolving an
 * identifier type allows obtaining the underlying type.
 * @param type Type to resolve.
 * @param resolve What type class should be resolved. This is a sum of one
 *   or more BoxTypeResolve masks, one for each class that should be resolved.
 * @param num Number of resolutions (0 means as many as necessary).
 * @return The resolved type.
 */
BOXEXPORT BoxType *
BoxType_Resolve(BoxType *type, BoxTypeResolve resolve, int num);

/*****************************************************************************
 * TYPE FINE-TUNING ROUTINES                                                 *
 *****************************************************************************/

/**
 * Add a child type to the namespace of a parent type.
 * Note that both the child and the parent must be identifier types.
 * @param parent The parent type.
 * @param child The child type.
 * @see BoxType_Create_Ident
 */
BOXEXPORT void
BoxType_Add_Type(BoxType *parent, BOXIN BoxType *child);

/**
 * Add a subtype type for a given type.
 * Note that the parent must be an identifier type.
 * @param parent The parent type.
 * @param name The name of the subtype.
 * @param subtype The type of the subtype or NULL to leave the subtype
 *   untyped. BoxType_Register_Subtype can be used to provide the type
 *   later.
 * @return Return the subtype node. Note that the caller is not give a
 *   reference to the node. In other words, the caller may need to use
 *   BoxType_Link to claim a reference to the subtype.
 */
BOXEXPORT BoxType *
BoxType_Create_Subtype(BoxType *parent, const char *name, BoxType *type);

/**
 * Find a subtype of the given type.
 * @param parent The type whose subtype we are trying to find.
 * @param name The name of the subtype.
 * @return A new subtype node or NULL, if the subtype could not be found.
 */
BOXEXPORT BoxType *
BoxType_Find_Subtype(BoxType *parent, const char *name);

/**
 * This function is similar to #BoxType_Find_Subtype, but does not try to
 * find subtypes inherited by types from which @p parent derives.
 * @see BoxType_Find_Subtype
 */
BOXEXPORT BoxType *
BoxType_Find_Own_Subtype(BoxType *parent, const char *name);

/**
 * Get details about a subtype found with BoxType_Find_Combination.
 */
BOXEXPORT BoxBool
BoxType_Get_Subtype_Info(BoxType *subtype, char **name,
                         BoxType **parent, BoxType **type);

/**
 * Register the type for a given subtype, if not given during creation.
 * @param subtype A subtype created with BoxType_Create_Subtype, giving
 *   NULL for the type.
 * @param type The type to associate to the subtype.
 * @return Whether the operation was successful.
 */
BOXEXPORT BoxBool
BoxType_Register_Subtype(BoxType *subtype, BoxType *type);


/** Identifier used to determine the state of a Box. */
typedef unsigned int BoxBoxState;

/** Return a Box state identifier from its string representation. */
BoxBoxState BoxType_Get_State(BoxType *t, const char *source);

/*****************************************************************************
 * TYPE ENQUIRY ROUTINES                                                     *
 *****************************************************************************/

/**
 * Get the size of the type 't'.
 */
BOXEXPORT size_t
BoxType_Get_Size(BoxType *t);

/**
 * Get the size and the aligment of a given input type.
 * @param t The input type.
 * @param size Where to store the size of the type.
 * @param algn Where to store the alignment of the type.
 * @return BOXTYPE_TRUE if size/alignment were retrieved successfully.
 */
BOXEXPORT BoxBool
BoxType_Get_Size_And_Alignment(BoxType *t, size_t *size, size_t *algn);

/**
 * Create the string representation of the Type 't'. The returned string
 * has to be freed with BoxMem_Free.
 */
char *BoxType_Get_Repr(BoxType *t);

/**
 * Compare right to left and return a BoxTypeCmp value
 */
BOXEXPORT BoxTypeCmp
BoxType_Compare(BoxType *left, BoxType *right);

/**
 * Type iterator. Allows to iter through the types that do have members,
 * such as structures, species and enums.
 * @see BoxTypeIter_Init
 */
typedef struct BoxTypeIter_struct {
  BoxType *current_node;
} BoxTypeIter;

/**
 * Initialize an iterator for iteration over the members of the given type.
 * @param ti Pointer to location of memory to initialize.
 * @param t Iteration is done over the members of this type.
 */
BOXEXPORT void
BoxTypeIter_Init(BoxTypeIter *ti, BoxType *t);

/**
 * Iterate over the next member of the provided iterator.
 * @param ti Pointer to an initialized iterator.
 * @param next This is a pointer where to put the next item in the iteration.
 * @return BOXBOOL_TRUE if a new item was retrieved and written in ``*next``,
 *   BOXBOOL_FALSE otherwise.
 *
 * EXAMPLE: The idea is to use this as:
 *
 *  BoxTypeIter ti;
 *  BoxType *t;
 *  for (BoxTypeIter_Init(& ti, parent); BoxTypeIter_Get_Next(& ti, & t);) {
 *    BoxType_Get_Structure_Member(& t, ...);
 *    ...
 *  }
 *
 */
BOXEXPORT BoxBool
BoxTypeIter_Get_Next(BoxTypeIter *ti, BoxType **next);

/**
 * Finalize an iterator initialized with BoxTypeIter_Init.
 */
#define BoxTypeIter_Finish(iter) \
  do {(void) (iter);} while(0)

/**
 * Whether an iterator has more items to read with BoxTypeIter_Get_Next.
 * @param ti Pointer to an initialized, possibly "used" iterator.
 * @return BOXBOOL_TRUE if the iterator has some items (in other words, if the
 *   next call to BoxTypeIter_Get_Next is going to return BOXBOOL_TRUE),
 *   BOXBOOL_FALSE otherwise.
 */
BOXEXPORT BoxBool
BoxTypeIter_Has_Items(BoxTypeIter *ti);

/**
 * Get the stem type of a type.
 * The stem type for a type t is the most basic type which can describe what is
 * contained inside t. It is used by the compiler to determine how to handle
 * the object (which type of register to use, for example).
 * In practice, the stem type is obtained by resolving species, identifiers and
 * raised types (with BoxType_Resolve).
 * @param type The input type.
 * @return The stem type of type (or NULL in case of failure).
 */
BOXEXPORT BoxType *
BoxType_Get_Stem(BoxType *type);

/**
 * Get the container type associated with a given type.
 * The container type is strictly related to the way the compiler handles types
 * (e.g. register types).
 * @param t The input type.
 * @return The container of t.
 */
BOXEXPORT BoxContType
BoxType_Get_Cont_Type(BoxType *t);

/**
 * Return whether the type is a void type (contains nothing).
 * @param t The input type.
 * @return Whether t is an empty type.
 */
BOXEXPORT BoxBool
BoxType_Is_Empty(BoxType *t);

/**
 * Get a string representation of the given type.
 * @param t The input type.
 * @return A freshly allocated string containing the representation of t.
 *   The string must be freed by the user with Box_Mem_Free.
 */
BOXEXPORT char *
BoxType_Get_Repr(BoxType *t);

#endif /* _BOX_TYPES_H */
