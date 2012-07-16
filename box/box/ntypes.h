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

#ifndef _BOX_NTYPES_H
#  define _BOX_NTYPES_H

#  include <stdlib.h>

typedef struct BoxTypeDesc_struct BoxTypeDesc;

/**
 * A type in the Box type system. This is currently implemented as a
 * pointer to an opaque structure.
 */
typedef BoxTypeDesc *BoxType;

/**
 * A pointer to a target object decorated with the type of the target.
 * This is using for boxing/unboxing objects.
 */
typedef struct {
  BoxType type;
  BoxPtr  ptr;
} BoxAny;

/**
 * A Box function (can be called from Box).
 */
typedef int BoxFunc;

/**
 * Value which determines the relationship between two types left and right.
 * This is what is returned by BoxType_Compare.
 */
typedef enum {
  BOXTYPECMP_SAME,     /**< right and left are the same type. */
  BOXTYPECMP_EQUAL,    /**< right and left are equal. */
  BOXTYPECMP_MATCHING, /**< right can be expended to left. */
  BOXTYPECMP_DIFFERENT /**< the two types are different. */

} BoxTypeCmp;

/*****************************************************************************
 * TYPE CREATION ROUTINES                                                    *
 *****************************************************************************/

/**
 * Create a new intrinsic type with the given name, size and alignment.
 * An intrinsic type can be imagined as a descriptor for an atomic
 * (i.e. indivisible) portion of memory.
 * @param size The size of the type (in bytes).
 * @param alignment The alignment for the type.
 * @return A new intrinsic type (or BOXTYPE_NONE if errors occurred).
 */
BOXEXPORT BoxType
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
BOXEXPORT BoxType
BoxType_Create_Ident(BoxType source, const char *name);

/**
 * Create a new raised type from the type 'source'. The new type will be
 * similar to t, but incompatible: BoxType_Compare will not match the two
 * types. As a consequence, the new raised type will not share the combinations
 * of its source type. Notice, however, that an object whose type is raised
 * can be unraised to obtain an object having the source type.
 * @param source The source type.
 * @return A new raised type.
 */
BOXEXPORT BoxType
BoxType_Create_Raised(BoxType source);

/**
 * Create an empty structure. Members can be added with
 * BoxType_Add_Member_To_Structure.
 */
BOXEXPORT BoxType
BoxType_Create_Structure(void);

/**
 * Add a member to a structure type defined with BoxType_Create_Structure.
 */
BOXEXPORT void
BoxType_Add_Member_To_Structure(BoxType structure, BoxType member,
                                const char *member_name);

/**
 * Create an empty species. Members can be added with
 * BoxType_Add_Member_To_Species.
 */
BOXEXPORT BoxType
BoxType_Create_Species(void);

/**
 * Add a member to a species type defined with BoxType_Create_Species.
 */
BOXEXPORT void
BoxType_Add_Member_To_Species(BoxType species, BoxType member);

/** Create a species of type '(*=>Dest)' (everything is converted to 'Dest').
 */
BoxType BoxType_New_Star_Species(BoxType dest);

/** Create an empty enum type. */
void BoxType_New_Enum(void);

/** Add a member to an enum type cerated with BoxType_New_Enum. */
void BoxType_Add_Member_To_Enum(BoxType member, const char *member_name);

/**
 * Create a new function type taking 'child' as an argument and working
 * on 'parent'.
 * @param child The type of the argument of the function.
 * @param parent The type of the value returned by the function.
 * @return A new type corresponding to the specified function.
 */
BOXEXPORT BoxType
BoxType_Create_Function(BoxType child, BoxType parent);

/**
 * Create a new pointer type to 'source'.
 * @param target The type of the pointer target.
 * @return A new pointer type.
 */
BOXEXPORT BoxType
BoxType_Create_Pointer(BoxType target);

/**
 * Create a new Any type.
 */
BOXEXPORT BoxType
BoxType_Create_Any(void);

/*****************************************************************************
 * TYPE FINE-TUNING ROUTINES                                                 *
 *****************************************************************************/

/**
 * Add a child type to the namespace of a parent type.
 * Note that both the child and the parent must be identifier types.
 * @param parent The parent type.
 * @param child The child type.
 */
BOXEXPORT void
BoxType_Add_Type(BoxType parent, BoxType child);

/**
 * Add a subtype type for a given type.
 * Note that the parent must be an identifier type.
 * @param parent The parent type.
 * @param name The name of the subtype.
 * @param subtype The type of the subtype.
 */
BOXEXPORT void
BoxType_Create_Subtype(BoxType parent, const char *name, BoxType subtype);

/** Identifier used to determine the state of a Box. */
typedef unsigned int BoxBoxState;

/** Return a Box state identifier from its string representation. */
BoxBoxState BoxType_Get_State(BoxType t, const char *source);

typedef int BoxAction;

/** Define a combination 'child'@'parent' and associate an action with 
 * the procedure.
 */
void BoxType_Define_Combination(BoxType child, BoxType parent,
                                BoxAction *action);

/*****************************************************************************
 * TYPE ENQUIRY ROUTINES                                                     *
 *****************************************************************************/

/**
 * Get the size of the type 't'.
 */
BOXEXPORT size_t
BoxType_Get_Size(BoxType t);

/**
 * Get the size and the aligment of a given input type.
 * @param t The input type.
 * @param size Where to store the size of the type.
 * @param algn Where to store the alignment of the type.
 * @return BOXTYPE_TRUE if size/alignment were retrieved successfully.
 */
BOXEXPORT BoxBool
BoxType_Get_Size_And_Alignment(BoxType t, size_t *size, size_t *algn);

/**
 * Create the string representation of the Type 't'. The returned string
 * has to be freed with BoxMem_Free.
 */
char *BoxType_Get_Repr(BoxType t);

/** Compare right to left and return a BoxTypeCmp value */
BoxTypeCmp BoxType_Compare(BoxType left, BoxType right);

/** Find the procedure 'left'@'right' and return:
 * - 'left' if the procedure was found and the type 'left' is equal to the
 *   procedure left type;
 * - the expansion type, if the procedure was found but 'left' must be
 *   expanded;
 * - BOXTYPE_NONE if the procedure was not found.
 */
BoxType BoxType_Find_Combination(BoxType left, BoxType right);

/** Type iterator. Allows to iter through the types that do have members,
 * such as structures, species and enums.
 * @see BoxTypeIter_Init
 */
typedef struct BoxTypeIter_struct BoxTypeIter;

void BoxTypeIter_Init(BoxTypeIter *ti);

/** The idea is to use this as:

  BoxTypeIter ti;
  BoxType t;
  for (BoxTypeIter(& ti); BoxTypeIter_Get_Next(& ti, & t);) {
    // ...
  }

 */
int BoxTypeIter_Get_Next(BoxTypeIter *ti, BoxType *next);

#endif /* _BOX_NTYPES_H */
