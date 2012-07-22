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

/**
 * Maximum number of types linked to a type.
 * This is a constant value useful when using the function MyType_Get_Refs.
 */
#define BOX_MAX_NUM_TYPES_IN_TYPE 3

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
 * Intrinsic Box type: basically a piece of memory handled opaquely by C
 * initializers, finalizers, etc. (e.g. Int, Real, Str, ...)
 */
typedef struct {
  size_t  size,
          alignment;
} BoxTypeIntrinsic;

/**
 * A type identifier: basically a node in the type tree which allows the type
 * to be visible and used in the Box language.
 */
typedef struct {
  char         *name;
  BoxType      source;
#if 0
  BoxNamespace namespace;
  BoxCombs     combs;
  BoxSubtypes  subtypes;
#endif
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
 * List node (used in structures, enums, etc.)
 */
typedef struct {
  BoxType next, previous;
} BoxTypeNode;

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
