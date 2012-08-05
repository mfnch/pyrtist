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
 * @file types_priv.h
 * @brief Private declaration for types.h
 *
 * Header defining what is needed by the files which implement the Box type
 * system and the functionality around the BoxType object.
 */

#ifndef _BOX_TYPES_PRIV_H
#  define _BOX_TYPES_PRIV_H

#  include <box/ntypes.h>
#  include <box/core.h>

/**
 * Maximum number of types linked to a type.
 * This is a constant value useful when using the function MyType_Get_Refs.
 */
#define BOX_MAX_NUM_REFS_IN_TYPE 3

/**
 * Maximum number of allocations for a type.
 * This is a constant value useful when using the function MyType_Get_Refs.
 */
#define BOX_MAX_NUM_MEMS_IN_TYPE 3

/**
 * Type class (whether a type is a structure, a species, etc).
 */
typedef enum {
  BOXTYPECLASS_NONE,
  BOXTYPECLASS_STRUCTURE_NODE,
  BOXTYPECLASS_SPECIES_NODE,
  BOXTYPECLASS_ENUM_NODE,
  BOXTYPECLASS_COMB_NODE,
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

struct BoxTypeDesc_struct {
  BoxTypeClass   type_class;

  struct {
    unsigned int is_namespace :1,
                 has_subtypes :1,
                 has_combs :1;
  }              attr;
};

/**
 * List node (used in structures, enums, etc.)
 */
typedef struct {
  BoxType next, previous;
} BoxTypeNode;

/**
 * Intrinsic Box type: basically a piece of memory handled opaquely by C
 * initializers, finalizers, etc. (e.g. Int, Real, Str, ...)
 */
typedef struct {
  BoxTypeId id;
  size_t    size,
            alignment;
} BoxTypePrimary;

/**
 * Intrinsic Box type: basically a piece of memory handled opaquely by C
 * initializers, finalizers, etc. (e.g. Int, Real, Str, ...)
 */
typedef struct {
  size_t  size,
          alignment;
} BoxTypeIntrinsic;

/**
 * Collection of combinations allowing combination searches.
 * This object is used internally by the type system.
 */
typedef struct BoxCombs_struct {
  BoxTypeNode node;
} BoxCombs;

/**
 * A type identifier: basically a node in the type tree which allows the type
 * to be visible and used in the Box language.
 */
typedef struct {
  char         *name;
  BoxType      source;
  /*BoxNamespace namespace;*/
  BoxCombs     combs;
  /*BoxSubtypes  subtypes;*/
} BoxTypeIdent;

/**
 * A raised type: a type which is identical to the source type it refers to,
 * but is treated as different when matching combinations. Object whose type
 * is raised can be un-raised, e.g. transformed to object of the original type.
 */
typedef struct {
  BoxType source;
} BoxTypeRaised;

/**
 * Structure type: objects of this type contain a fixed number of objects of
 * other types, which can be referred by name.
 */
typedef struct {
  BoxTypeNode node;
  size_t      size,
              alignment,
              num_items;
} BoxTypeStructure;

/**
 * Structure node: basically, a structure member.
 */
typedef struct {
  BoxTypeNode node;
  char        *name;
  size_t      offset,
              size;
  BoxType     type;
} BoxTypeStructureNode;

/**
 * Species type.
 */
typedef struct {
  BoxTypeNode node;
  size_t      num_items;
} BoxTypeSpecies;

/**
 * Species node.
 */
typedef struct {
  BoxTypeNode node;
  BoxType     type;
} BoxTypeSpeciesNode;

/**
 * Combination node.
 */
typedef struct {
  BoxTypeNode node;
  BoxType     child;
  BoxCombType comb_type;
  BoxCallable *callable;
} BoxTypeCombNode;

/**
 * A function type: a type for something which can be called with an input
 * and returns an output.
 */
typedef struct {
  BoxType child,
          parent;
} BoxTypeFunction;

/**
 * Pointer type.
 */
typedef struct {
  BoxType source;
} BoxTypePointer;

/**
 * Internal function. Not meant to be called by ordinary users.
 */
BoxBool Box_Register_Type_Combs(BoxCoreTypes *core_types);

/**
 * Generic allocation function for BoxType objects. This function allocates a
 * type header plus a the type data, whose size and composition depend on the
 * particular type class.
 * This is an internal function of the type system.
 */
void *BoxType_Alloc(BoxType *t, BoxTypeClass tc);

/**
 * Append one BoxTypeNode item to a top BoxTypeNode item. This is used
 * internally in the implementation of structures, enums, etc. to add members.
 * This is an internal function of the type system.
 */
void BoxTypeNode_Append_Node(BoxTypeNode *node, BoxType item);

/**
 * Prepend one BoxTypeNode item to a top BoxTypeNode item. This is similar
 * to BoxTypeNode_Append_Node, but the item is inserted at the other end of
 * the linked list.
 */
void BoxTypeNode_Prepend_Node(BoxTypeNode *node, BoxType item);

#endif /* _BOX_TYPES_PRIV_H */
