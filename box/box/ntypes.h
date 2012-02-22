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

/** A type in the Box type system. */
typedef BoxTypeDesc *BoxType;

/** Value which determines the relationship between two types left and right.
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

/** Create a new parameter type to be used in the definition of parametric
 * types.
 */
BoxType BoxType_New_Parameter(void);

BoxType BoxType_New_Parametric(void);

/** Create a new intrinsic type with the given size and alignment. */
BoxType BoxType_New_Intrinsic(size_t size, size_t alignment, const char *name);

/** Create a new alias type from the type 'source'. */
BoxType BoxType_New_Alias(BoxType source, const char *name);

/** Create a new raised type from the type 'source'. The new type will be
 * similar to t, but incompatible: BoxType_Compare will not match the two
 * types.
 */
BoxType BoxType_New_Raised(BoxType source);

/** Create an empty structure. Members can be added with
 * BoxType_Add_Member_To_Structure.
 */
BoxType BoxType_New_Structure(void);

/** Add a member to a structure type defined with BoxType_New_Structure. */
void BoxType_Add_Member_To_Structure(BoxType structure, BoxType member,
                                     const char *member_name);

/** Create an empty species. Members can be added with
 * BoxType_Add_Member_To_Species.
 */
BoxType BoxType_New_Species(void);

/** Add a member to a species type defined with TS_Species_Begin. */
void BoxType_Add_Species_Member(BoxType species, BoxType member);

/** Create a species of type '(*=>Dest)' (everything is converted to 'Dest').
 */
BoxType BoxType_New_Star_Species(BoxType dest);

/** Create an empty enum type. */
void BoxType_New_Enum(void);

/** Add a member to an enum type cerated with BoxType_New_Enum. */
void BoxType_Add_Member_To_Enum(BoxType member, const char *member_name);

/** Create a new function type taking 'child' as an argument and working
 * on 'parent'.
 */
BoxType BoxType_New_Function(BoxType child, BoxType parent);

/** Create a new pointer type to 'source'. */
BoxType BoxType_New_Pointer(BoxType source);

/** Create a new Any type. */
BoxType BoxType_New_Any(void);

/*****************************************************************************
 * TYPE FINE-TUNING ROUTINES                                                 *
 *****************************************************************************/

/** Set the namespace for a type 'source'. */
void BoxType_Set_Namespace(BoxType source, BoxType namespace);

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

/** Get the size of the type 't'. */
size_t BoxType_Get_Size(BoxType t);

/** Create the string representation of the Type 't'. The returned string
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
