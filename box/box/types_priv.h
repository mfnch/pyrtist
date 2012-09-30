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

#  include <box/types.h>
#  include <box/combs.h>
#  include <box/core.h>


/**
 * Maximum number of types linked to a type.
 * This is a constant value useful when using the function MyType_Get_Refs.
 */
#define BOX_MAX_NUM_REFS_IN_TYPE 4

/**
 * Maximum number of allocations for a type.
 * This is a constant value useful when using the function MyType_Get_Refs.
 */
#define BOX_MAX_NUM_MEMS_IN_TYPE 3

/**
 * @brief List node (used in structures, enums, etc.)
 */
typedef struct {
  BoxType *next, *previous;
} BoxTypeNode;

/**
 * @brief Intrinsic Box type.
 *
 * This is basically a piece of memory handled opaquely by C initializers,
 * finalizers, etc. (e.g. Int, Real, Str, ...)
 */
typedef struct {
  BoxTypeId id;
  size_t    size,
            alignment;
} BoxTypePrimary;

/**
 * @brief Intrinsic Box type.
 *
 * This is basically a piece of memory handled opaquely by C initializers,
 * finalizers, etc. (e.g. Int, Real, Str, ...)
 */
typedef struct {
  size_t  size,
          alignment;
} BoxTypeIntrinsic;

/**
 * @brief Collection of combinations allowing combination searches.
 *
 * This object is used internally by the type system.
 */
typedef struct BoxCombs_struct {
  BoxTypeNode node;
} BoxCombs;

/**
 * @brief Collection of subtypes which allow fast search (hash table).
 *
 * This object is used internally by the type system.
 */
typedef struct BoxSubtypes_struct {
  BoxTypeNode node;
} BoxSubtypes;

/**
 * @brief Subtype node.
 */
typedef struct {
  BoxTypeNode node;
  char        *name;
  BoxType     *parent;
  BoxType     *type;
  BoxCombs    combs;
  BoxSubtypes subtypes;
} BoxTypeSubtypeNode;

/**
 * @brief A type identifier.
 *
 * This is basically a node in the type tree which allows the type to be
 * visible and used in the Box language.
 */
typedef struct {
  char         *name;
  BoxType      *source;
  /*BoxNamespace namespace;*/
  BoxCombs     combs;
  BoxSubtypes  subtypes;
} BoxTypeIdent;

/**
 * @brief A raised type.
 *
 * A raised type is a type which is identical to the source type it refers to,
 * but is treated as different when matching combinations. Object whose type is
 * raised can be un-raised, e.g. transformed to object of the original type.
 */
typedef struct {
  BoxType *source;
} BoxTypeRaised;

/**
 * @brief Structure type.
 *
 * Objects of this type contain a fixed number of objects of other types, which
 * can be referred by name.
 */
typedef struct {
  BoxTypeNode node;
  size_t      size,
              alignment,
              num_items;
} BoxTypeStructure;

/**
 * @brief Structure node: basically, a structure member.
 */
typedef struct {
  BoxTypeNode node;
  char        *name;
  size_t      offset,
              size;
  BoxType     *type;
} BoxTypeStructureNode;

/**
 * @brief Species type.
 */
typedef struct {
  BoxTypeNode node;
  size_t      num_items;
} BoxTypeSpecies;

/**
 * @brief Species node.
 */
typedef struct {
  BoxTypeNode node;
  BoxType     *type;
} BoxTypeSpeciesNode;

/**
 * @brief Combination node.
 */
typedef struct {
  BoxTypeNode node;
  BoxType     *child;
  BoxCombType comb_type;
  BoxCallable *callable;
} BoxTypeCombNode;

/**
 * @brief A function type: a type for something which can be called with an
 * input and returns an output.
 */
typedef struct {
  BoxType *child,
          *parent;
} BoxTypeFunction;

/**
 * @brief Pointer type.
 */
typedef struct {
  BoxType *source;
} BoxTypePointer;

struct BoxTypeDesc_struct {
  BoxTypeClass   type_class;
  BoxTypeId      type_id;

#if 0
  struct {
    unsigned int can_self_ref : 1, /**< Can reference itself (directly or
                                      indirectly). For garbage collection. */
                 is_global    : 1; /**< Whether the type is global. Global 
                                      types do not need to be referenced by
                                      their instances. */
  }              attr;
#endif
};

typedef struct BoxTypeBundle_struct {
  /* The header. */
  BoxTypeDesc            header;

  /* The data part. */
  union {
    BoxTypeStructureNode structure_node;
    BoxTypeSpeciesNode   species_node;
    BoxTypeCombNode      comb_node;
    BoxTypeSubtypeNode   subtype_node;
    BoxTypePrimary       primary;
    BoxTypeIntrinsic     intrinsic;
    BoxTypeIdent         ident;
    BoxTypeRaised        raised;
    BoxTypeStructure     structure;
    BoxTypeSpecies       species;
    BoxTypeFunction      function;
    BoxTypePointer       pointer;
  }                      data;
} BoxTypeBundle;

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
void *BoxType_Alloc(BoxType **t, BoxTypeClass tc);

/**
 * Append one BoxTypeNode item to a top BoxTypeNode item. This is used
 * internally in the implementation of structures, enums, etc. to add members.
 * This is an internal function of the type system.
 */
void BoxTypeNode_Append_Node(BoxTypeNode *node, BoxType *item);

/**
 * Prepend one BoxTypeNode item to a top BoxTypeNode item. This is similar
 * to BoxTypeNode_Append_Node, but the item is inserted at the other end of
 * the linked list.
 */
void
BoxTypeNode_Prepend_Node(BoxTypeNode *node, BoxType *item);

/**
 * @brief Remove a #BoxTypeNode from a top #BoxTypeNode item.
 *
 * @param top_node The top node, identifying the linked list to which @p this
 *   belongs to.
 * @param this The item to remove from the linked list.
 * @return The removed node. This may be destroyed or inserted into another
 *   linked list.
 */
BoxTypeNode *
BoxTypeNode_Remove_Node(BoxTypeNode *top_node, BoxType *this);

#endif /* _BOX_TYPES_PRIV_H */
