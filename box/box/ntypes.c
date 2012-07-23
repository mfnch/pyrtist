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

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <box/types.h>
#include <box/ntypes.h>
#include <box/types_priv.h>

#include <box/messages.h>
#include <box/mem.h>


/* Generic allocation function for BoxType objects. This function allocates a
 * type header plus a the type data, whose size and composition depend on the
 * particular type class.
 */
void *BoxType_Alloc(BoxType *t, BoxTypeClass tc) {
  size_t additional = 0, total;

  switch (tc) {
  case BOXTYPECLASS_STRUCTURE_NODE:
    additional = sizeof(BoxTypeStructureNode); break;
  case BOXTYPECLASS_SPECIES_NODE:
    additional = sizeof(BoxTypeSpeciesNode); break;
  case BOXTYPECLASS_COMB_NODE: additional = sizeof(BoxTypeCombNode); break;
  case BOXTYPECLASS_PRIMARY: additional = sizeof(BoxTypePrimary); break;
  case BOXTYPECLASS_INTRINSIC: additional = sizeof(BoxTypeIntrinsic); break;
  case BOXTYPECLASS_IDENT: additional = sizeof(BoxTypeIdent); break;
  case BOXTYPECLASS_RAISED: additional = sizeof(BoxTypeRaised); break;
  case BOXTYPECLASS_STRUCTURE: additional = sizeof(BoxTypeStructure); break;
  case BOXTYPECLASS_FUNCTION: additional = sizeof(BoxTypeFunction); break;
  case BOXTYPECLASS_POINTER: additional = sizeof(BoxTypePointer); break;
  case BOXTYPECLASS_ANY: additional = 0; break;
  default:
    MSG_FATAL("Unknown type class in BoxType_Alloc");
    return NULL;
  }

  if (Box_Mem_X_Plus_Y(& total, additional, sizeof(BoxTypeDesc))) {
    BoxTypeDesc *td = Box_Mem_RC_Safe_Alloc(total);
    td->type_class = tc;
    *t = td;
    return (void *) td + sizeof(BoxTypeDesc);
  }   

  MSG_FATAL("Integer overflow in BoxType_Alloc");
  return NULL;
}

/* Get the data part of a type. The size and composition of the data type of
 * a given type changes depending on the type class.
 */
void *BoxType_Get_Data(BoxType t) {
  return (void *) t + sizeof(BoxTypeDesc);
}

/**
 * Return the BoxTypeNode object associated to a node type or NULL, if the
 * given type is not a node type.
 */
BoxTypeNode *MyType_Get_Node(BoxType t) {
  void *td = BoxType_Get_Data(t);
  switch (t->type_class) {
  case BOXTYPECLASS_SPECIES_NODE:
    return & ((BoxTypeSpeciesNode *) td)->node;
  case BOXTYPECLASS_STRUCTURE_NODE:
    return & ((BoxTypeStructureNode *) td)->node;
  case BOXTYPECLASS_COMB_NODE: return & ((BoxTypeCombNode *) td)->node;
  case BOXTYPECLASS_STRUCTURE: return & ((BoxTypeStructure *) td)->node;
  case BOXTYPECLASS_SPECIES: return & ((BoxTypeSpecies *) td)->node;
  default: return NULL;
  }
}

/**
 * Get the allocated objects a given type refers to.
 * This function returns the number of types (in *num_refs) and the number of
 * allocations (in *num_mems) associated to the given type. The function does
 * also write the actual references and pointers to the allocated regions
 * in the two provided arrays. This function is internal and allows to
 * traverse the type tree easily. It is used, for example, to deallocate all
 * the resources associated to a given type in BoxType_Unlink.
 * @param t The type 
 * @param num_refs Pointer where the number of references made by the provided
 *   type makes be written (the number of references never exceeds the constant
 *   BOX_MAX_NUM_TYPES_IN_TYPE.
 * @param refs Location where to write the references. This is an array of
 *   BoxType which should be able to contain at least BOX_MAX_NUM_TYPES_IN_TYPE
 *   elements.
 * @param num_mems Pointer where the number of allocations made by the provided
 *   type should be written (the number of references never exceeds
 *   the constant BOX_MAX_NUM_MEMS_IN_TYPE.
 * @param mems Location where to write the pointers to the allocated blocks.
 *   This is an array of pointers which should be able to contain at least
 *   BOX_MAX_NUM_MEMS_IN_TYPE elements.
 *
 * Example:
 * @code
 * void *mems[BOX_MAX_NUM_MEMS_IN_TYPE];
 * BoxType refs[BOX_MAX_NUM_TYPES_IN_TYPE];
 * int num_refs, num_mems;
 * MyType_Get_Refs(type, & num_refs, refs, & num_mems, mems);
 * @endcode
 */
static void MyType_Get_Refs(BoxType t, int *num_refs, BoxType *refs,
                            int *num_mems, void **mems) {
  void *tdata = BoxType_Get_Data(t);

  *num_refs = 0;
  *num_mems = 0;

  switch (t->type_class) {
  case BOXTYPECLASS_NONE:
    break;
  case BOXTYPECLASS_STRUCTURE_NODE:
    refs[0] = ((BoxTypeStructureNode *) tdata)->node.next;
    refs[1] = ((BoxTypeStructureNode *) tdata)->type;
    *num_refs = 2;
    /* NOTE: we do not own the reference to `previous'. */
    mems[0] = ((BoxTypeStructureNode *) tdata)->name;
    *num_mems = 1;
    return;
  case BOXTYPECLASS_SPECIES_NODE:
    refs[0] = ((BoxTypeSpeciesNode *) tdata)->node.next;
    refs[1] = ((BoxTypeSpeciesNode *) tdata)->type;
    *num_refs = 2;
    return;
  case BOXTYPECLASS_COMB_NODE:
    refs[0] = ((BoxTypeCombNode *) tdata)->node.next;
    refs[1] = ((BoxTypeCombNode *) tdata)->child;
    *num_refs = 2;
    return;
  case BOXTYPECLASS_PRIMARY:
  case BOXTYPECLASS_INTRINSIC:
    return;
  case BOXTYPECLASS_IDENT:
    refs[0] = ((BoxTypeIdent *) tdata)->source;
    refs[1] = ((BoxTypeIdent *) tdata)->combs.node.next;
    *num_refs = 2;
    mems[0] = ((BoxTypeIdent *) tdata)->name;
    *num_mems = 1;
    return;
  case BOXTYPECLASS_RAISED:
    refs[0] = ((BoxTypeRaised *) tdata)->source;
    *num_refs = 1;
    return;
  case BOXTYPECLASS_STRUCTURE:
    refs[0] = ((BoxTypeStructure *) tdata)->node.next;
    *num_refs = 1;
    /* NOTE: we do not own the reference to `previous'. */
    return;
  case BOXTYPECLASS_SPECIES:
    refs[0] = ((BoxTypeSpecies *) tdata)->node.next;
    *num_refs = 1;
    /* NOTE: we do not own the reference to `previous'. */
    return;
  case BOXTYPECLASS_FUNCTION:
    refs[0] = ((BoxTypeFunction *) tdata)->child;
    refs[1] = ((BoxTypeFunction *) tdata)->parent;
    *num_refs = 2;
    return;
  case BOXTYPECLASS_POINTER:
    refs[0] = ((BoxTypePointer *) tdata)->source;
    *num_refs = 1;
    return;
  case BOXTYPECLASS_ANY:
    *num_refs = 0;
    return;
  }
}

/* Unlink (remove a reference to) the given type. The memory for the type is
 * released if there are no references left to it.
 */
void BoxType_Unlink(BoxType t) {
  if (t) {
    if (Box_Mem_RC_Get_Num_Refs(t) == 1) {
      /* The object is gonna die (just one ref count).  */

      void *mems[BOX_MAX_NUM_MEMS_IN_TYPE];
      BoxType refs[BOX_MAX_NUM_TYPES_IN_TYPE];
      int num_refs, num_mems, i;

      MyType_Get_Refs(t, & num_refs, refs, & num_mems, mems);

      for (i = 0; i < num_mems; i++)
        BoxMem_Free(mems[i]);

      for (i = 0; i < num_refs; i++)
        BoxType_Unlink(refs[i]);
    }

    Box_Mem_RC_Unlink(t);
  }
}

/* Add a reference to the given type. */
BoxType BoxType_Link(BoxType t) {
  Box_Mem_RC_Link(t);
  return t;
}

/* Append one BoxTypeNode item to a top BoxTypeNode item. This is used in
 * structures, enums, etc. to add members.
 */
void BoxTypeNode_Append_Node(BoxTypeNode *top_node, BoxType item) {
  BoxTypeNode *item_node = MyType_Get_Node(item);
  assert(top_node && item_node);

  /* Adjust the links. */
  item_node->previous = top_node->previous;
  item_node->next = NULL;

  /* Adjust the tail. */
  if (top_node->previous != NULL) {
    BoxTypeNode *previous_node = MyType_Get_Node(top_node->previous);
    assert(previous_node);
    previous_node->next = item;
  }

  /* Adjust the top node. */
  if (top_node->next == NULL)
    top_node->next = item;
  top_node->previous = item;
}

/* Prepend one BoxTypeNode item to a top BoxTypeNode item. This is similar
 * to BoxTypeNode_Append_Node, but the item is inserted at the other end of
 * the linked list.
 */
void BoxTypeNode_Prepend_Node(BoxTypeNode *top_node, BoxType item) {
  BoxTypeNode *item_node = MyType_Get_Node(item);
  assert(top_node && item_node);

  /* Adjust the links. */
  item_node->previous = NULL;
  item_node->next = top_node->next;

  /* Adjust the head. */
  if (top_node->next != NULL) {
    BoxTypeNode *next_node = MyType_Get_Node(top_node->next);
    assert(next_node);
    next_node->previous = item;
  }

  /* Adjust the top node. */
  if (top_node->previous == NULL)
    top_node->previous = item;
  top_node->next = item;
}

/* Create a new primary type with the given id, size and alignment. */
BoxType BoxType_Create_Primary(BoxTypeId id, size_t size, size_t alignment) {
  BoxType t;
  BoxTypePrimary *td = BoxType_Alloc(& t, BOXTYPECLASS_PRIMARY);
  td->id = id;
  td->size = size;
  td->alignment = alignment;  
  return t;
}

/* Create a new intrinsic type with the given size and alignment. */
BoxType BoxType_Create_Intrinsic(size_t size, size_t alignment) {
  BoxType t;
  BoxTypeIntrinsic *ti = BoxType_Alloc(& t, BOXTYPECLASS_INTRINSIC);
  ti->size = size;
  ti->alignment = alignment;  
  return t;
}

/* Create a new identifier type. */
BoxType BoxType_Create_Ident(BoxType source, const char *name) {
  BoxType t;
  BoxTypeIdent *ta = BoxType_Alloc(& t, BOXTYPECLASS_IDENT);
  ta->name = BoxMem_Strdup(name);
  ta->source = source;
  ta->combs.node.next = NULL;
  ta->combs.node.previous = NULL;
  return t;
}

/* Add a child type to the namespace of a parent type. */
void BoxType_Add_Type(BoxType parent, BoxType child) {
  
}


/* Create a new raised type. */
BoxType BoxType_Create_Raised(BoxType source) {
  BoxType t;
  BoxTypeRaised *td = BoxType_Alloc(& t, BOXTYPECLASS_RAISED);
  td->source = source;
  return t;
}

/* Create a new structure type. */
BoxType BoxType_Create_Structure(void) {
  BoxType t;
  BoxTypeStructure *td = BoxType_Alloc(& t, BOXTYPECLASS_STRUCTURE);
  td->size = 0;
  td->alignment = 0;
  td->num_items = 0;
  td->node.next = NULL;
  td->node.previous = NULL;
  return t;
}

/* Add a member to a structure type defined with BoxType_Create_Structure. */
void BoxType_Add_Member_To_Structure(BoxType structure, BoxType member,
                                     const char *member_name) {
  BoxType t;
  size_t msize, malgn, ssize;
  BoxTypeStructure *std = BoxType_Get_Data(structure);
  BoxTypeStructureNode *td;

  /* Let's get the size and alignment of the member type. */
  if (!BoxType_Get_Size_And_Alignment(member, & msize, & malgn))
    MSG_FATAL("Cannot get size and alignment of structure member type");

  /* Need to do this small computation to retrieve the structure size without
   * padding (as std->size is the actual structure size, with padding).
   */
  ssize = 0;
  if (std->node.previous != NULL) {
    BoxTypeStructureNode *ptd = BoxType_Get_Data(std->node.previous);
    ssize = ptd->offset + ptd->size;
  }

  /* Now create the member. */
  td = BoxType_Alloc(& t, BOXTYPECLASS_STRUCTURE_NODE);
  td->name = (member_name ? BoxMem_Strdup(member_name) : NULL);
  td->size = msize;
  td->offset = BoxMem_Align_Offset(ssize, malgn);
  td->type = member;

  /* Add the member to the structure. */
  std->num_items++;

  /* Alignment is the maximum of the alignments of the members. */
  if (malgn > std->alignment)
    std->alignment = malgn;

  std->size = BoxMem_Get_Multiple_Size(td->offset + msize, std->alignment);
  BoxTypeNode_Append_Node(& std->node, t);
}


/* Get information on a structure member as obtained from
 * BoxTypeIter_Get_Next.
 */
BoxBool
BoxType_Get_Structure_Member(BoxType node, char **name, size_t *offset,
                             size_t *size, BoxType *type) {
  if (node->type_class == BOXTYPECLASS_STRUCTURE_NODE) {
    BoxTypeStructureNode *sn = BoxType_Get_Data(node);

    if (name)
      *name = sn->name;
    if (offset)
      *offset = sn->offset;
    if (size)
      *size = sn->size;
    if (type)
      *type = sn->type;

    return BOXBOOL_TRUE;
    
  } else
    return BOXBOOL_FALSE;
}

/* Get the type of a structure member obtained from BoxTypeIter_Get_Next. */
BoxType BoxType_Get_Structure_Member_Type(BoxType node) {
  BoxType t;

  if (BoxType_Get_Structure_Member(node, NULL, NULL, NULL, & t))
    return t;

  return NULL;
}

/* Get the type of a species member as obtained from BoxTypeIter_Get_Next. */
BoxType BoxType_Find_Structure_Member(BoxType s, const char *name) {
  BoxTypeIter ti;
  BoxType t;
  char *member_name;

  for (BoxTypeIter_Init(& ti, s); BoxTypeIter_Get_Next(& ti, & t);) {
    BoxType_Get_Structure_Member(t, & member_name, NULL, NULL, NULL);
    if (strcmp(name, member_name) == 0) {
      BoxTypeIter_Finish(& ti);
      return t;
    }
  }

  BoxTypeIter_Finish(& ti);
  return NULL;
}

/* Get the number of members of a structure. */
size_t BoxType_Get_Structure_Num_Members(BoxType t) {
  if (t->type_class == BOXTYPECLASS_STRUCTURE_NODE) {
    BoxTypeStructure *s = BoxType_Get_Data(t);
    return s->num_items;

  } else
    return 0;
}

/* Get information on a species member as obtained from BoxTypeIter_Get_Next.
 */
BoxType BoxType_Get_Species_Member_Type(BoxType node) {
  if (node->type_class == BOXTYPECLASS_SPECIES_NODE) {
    BoxTypeStructureNode *sn = BoxType_Get_Data(node);
    return sn->type;
  }

  return NULL;
}

/* Create a new species type. */
BoxType BoxType_Create_Species(void) {
  BoxType t;
  BoxTypeSpecies *td = BoxType_Alloc(& t, BOXTYPECLASS_SPECIES);
  td->num_items = 0;
  td->node.next = NULL;
  td->node.previous = NULL;
  return t;
}

/* Add a member to a species type defined with BoxType_Create_Species. */
BOXEXPORT void
BoxType_Add_Member_To_Species(BoxType species, BoxType member) {
  BoxType t;
  size_t msize, malgn;
  BoxTypeSpecies *std = BoxType_Get_Data(species);
  BoxTypeSpeciesNode *td;

  /* Let's get the size and alignment of the member type. */
  if (!BoxType_Get_Size_And_Alignment(t, & msize, & malgn))
    MSG_FATAL("Cannot get size and alignment of species member type");

  /* Now create the member. */
  td = BoxType_Alloc(& t, BOXTYPECLASS_SPECIES_NODE);
  td->type = member;

  /* Add the member to the structure. */
  std->num_items++;

  BoxTypeNode_Append_Node(MyType_Get_Node(species), t);
}

/* Create a new function type. */
BoxType BoxType_Create_Function(BoxType child, BoxType parent) {
  BoxType t;
  BoxTypeFunction *td = BoxType_Alloc(& t, BOXTYPECLASS_FUNCTION);
  td->child = child;
  td->parent = parent;
  return t;
}

/* Create a new pointer type. */
BoxType BoxType_Create_Pointer(BoxType source) {
  BoxType t;
  BoxTypePointer *td = BoxType_Alloc(& t, BOXTYPECLASS_POINTER);
  td->source = source;
  return t;
}

/* Create a new pointer type. */
BoxType BoxType_Create_Any(void) {
  BoxType t;
  (void) BoxType_Alloc(& t, BOXTYPECLASS_ANY);
  return t;
}

/* Get the size of the type 't'. */
size_t BoxType_Get_Size(BoxType t) {
  size_t size;
  if (BoxType_Get_Size_And_Alignment(t, & size, NULL))
    return size;
  else
    return 0;
}

/* Get the size and the aligment of a given type. */
BoxBool BoxType_Get_Size_And_Alignment(BoxType t, size_t *size, size_t *algn) {
  size_t dummy;

  if (!size)
    size = & dummy;

  if (!algn)
    algn = & dummy;

  /* We do a while loop rather than opting for recursive calls.
   * The loop is uglier, but more efficient...
   */
  while (t) {
    void *td = BoxType_Get_Data(t);

    switch (t->type_class) {
    case BOXTYPECLASS_NONE:
    case BOXTYPECLASS_STRUCTURE_NODE:
    case BOXTYPECLASS_SPECIES_NODE:
    case BOXTYPECLASS_ENUM_NODE:
    case BOXTYPECLASS_COMB_NODE:
      return BOXBOOL_FALSE;

    case BOXTYPECLASS_PRIMARY:
      *size = ((BoxTypePrimary *) td)->size;
      *algn = ((BoxTypePrimary *) td)->alignment;
      return BOXBOOL_TRUE;

    case BOXTYPECLASS_INTRINSIC:
      *size = ((BoxTypeIntrinsic *) td)->size;
      *algn = ((BoxTypeIntrinsic *) td)->alignment;
      return BOXBOOL_TRUE;

    case BOXTYPECLASS_IDENT:
      t = ((BoxTypeIdent *) td)->source;
      break; /* resolve and retry... */

    case BOXTYPECLASS_RAISED:
      t = ((BoxTypeRaised *) td)->source;
      break; /* resolve and retry... */

    case BOXTYPECLASS_STRUCTURE:
      *size = ((BoxTypeStructure *) td)->size;
      *algn = ((BoxTypeStructure *) td)->alignment;
      return BOXBOOL_TRUE;

    case BOXTYPECLASS_SPECIES:
      t = ((BoxTypeSpecies *) td)->node.previous;
      break; /* resolve and retry... */

#if 0
    case BOXTYPECLASS_ENUM:
#endif

    case BOXTYPECLASS_FUNCTION:
      *size = sizeof(BoxFunc);
      *algn = __alignof__(BoxFunc);
      return BOXBOOL_TRUE;

    case BOXTYPECLASS_POINTER:
      *size = sizeof(BoxPtr);
      *algn = __alignof__(BoxPtr);
      return BOXBOOL_TRUE;

    case BOXTYPECLASS_ANY:
      *size = sizeof(BoxAny);
      *algn = __alignof__(BoxAny);
      return BOXBOOL_TRUE;

    default:
      return BOXBOOL_FALSE;
    }
  }

  return BOXBOOL_FALSE;
}

/* Resolve the given type. */
BoxType BoxType_Resolve(BoxType t, BoxTypeResolve resolve, int num) {
  if (!t)
    return t;

  do { /* forever */
    switch (t->type_class) {
    case BOXTYPECLASS_NONE:
    case BOXTYPECLASS_STRUCTURE_NODE:
    case BOXTYPECLASS_SPECIES_NODE:
    case BOXTYPECLASS_ENUM_NODE:
    case BOXTYPECLASS_COMB_NODE:
      return NULL;

    case BOXTYPECLASS_IDENT:
      if ((resolve & BOXTYPERESOLVE_IDENT) == 0)
        return t;
      else
        t = ((BoxTypeIdent *) BoxType_Get_Data(t))->source;
      break;

    case BOXTYPECLASS_RAISED:
      if ((resolve & BOXTYPERESOLVE_RAISED) == 0)
        return t;
      else
        t = ((BoxTypeRaised *) BoxType_Get_Data(t))->source;
      break;

    case BOXTYPECLASS_SPECIES:
      if ((resolve & BOXTYPERESOLVE_SPECIES) == 0)
        return t;
      else
        t = ((BoxTypeSpecies *) BoxType_Get_Data(t))->node.previous;
      break;

    case BOXTYPECLASS_PRIMARY:
    case BOXTYPECLASS_INTRINSIC:
    case BOXTYPECLASS_STRUCTURE:      
    case BOXTYPECLASS_ENUM:
    case BOXTYPECLASS_FUNCTION:
    case BOXTYPECLASS_ANY:
      return t;

    case BOXTYPECLASS_POINTER:
      if ((resolve & BOXTYPERESOLVE_POINTER) == 0)
        return t;
      else
        t = ((BoxTypePointer *) BoxType_Get_Data(t))->source;
      break;
    }

    if (num == 1)
      return t;

    else if (num > 1)
      num--;
    
  } while (1);
}

/* Initialize an iterator for iteration over the members of the given type. */
void BoxTypeIter_Init(BoxTypeIter *ti, BoxType t) {
  if (t) {
    BoxTypeNode *node = MyType_Get_Node(t);
    if (node) {
      ti->current_node = node->next;
      return;
    }
  }

  ti->current_node = NULL;
}

/* Iterate over the next member of the provided iterator. If the iterator has
 * a next member, then ``*next`` is set to it and BOXBOOL_TRUE is returned.
 * BOXBOOL_FALSE is returned otherwise.
 */
BoxBool BoxTypeIter_Get_Next(BoxTypeIter *ti, BoxType *next) {
  if (ti && ti->current_node) {
    BoxTypeNode *node = MyType_Get_Node(ti->current_node);
    *next = ti->current_node;
    ti->current_node = node->next;
    return BOXBOOL_TRUE;

  } else {
    *next = NULL;
    return BOXBOOL_FALSE;
  }
}

/* Whether the iterator has more items. */
BoxBool BoxTypeIter_Has_Items(BoxTypeIter *ti) {
  return (ti && ti->current_node);
}

/* Type comparison function. */
BoxTypeCmp BoxType_Compare(BoxType left, BoxType right) {
  if (left == right)
    return BOXTYPECMP_SAME;

  left = BoxType_Resolve(left, BOXTYPERESOLVE_IDENT, 1);
  right = BoxType_Resolve(right, 
                          BOXTYPERESOLVE_IDENT | BOXTYPERESOLVE_SPECIES, 1);
  if (left == right)
    return BOXTYPECMP_EQUAL;

  switch (left->type_class) {
  case BOXTYPECLASS_STRUCTURE_NODE:
  case BOXTYPECLASS_SPECIES_NODE:
  case BOXTYPECLASS_ENUM_NODE:
  case BOXTYPECLASS_COMB_NODE:
  case BOXTYPECLASS_IDENT:
    MSG_FATAL("BoxType_Compare: Invalid type objects.");
    return BOXTYPECMP_DIFFERENT;

  case BOXTYPECLASS_PRIMARY:
    if (right->type_class == BOXTYPECLASS_PRIMARY) {
      BoxTypePrimary *ltd = BoxType_Get_Data(left),
                     *rtd = BoxType_Get_Data(right);      
      return (ltd->id == rtd->id) ? BOXTYPECMP_EQUAL : BOXTYPECMP_DIFFERENT;
    }

    return BOXTYPECMP_DIFFERENT;

  case BOXTYPECLASS_INTRINSIC:
    /* If we got here, we have left != right, which is enough to say the two
     * types are not the same (two intrinsic types are different iff they are
     * not the same type).
     */
    return BOXTYPECMP_DIFFERENT;

  case BOXTYPECLASS_RAISED:
    /* Same as BOXTYPECLASS_INTRINSIC. */
    return BOXTYPECMP_DIFFERENT;

  case BOXTYPECLASS_SPECIES:
  {
    BoxTypeIter iter;
    BoxType node;
    for (BoxTypeIter_Init(& iter, left);
         BoxTypeIter_Get_Next(& iter, & node);) {
      BoxType memb = BoxType_Get_Species_Member_Type(node);
      
      if (BoxType_Compare(memb, right) != BOXTYPECMP_DIFFERENT) {
        if (BoxTypeIter_Has_Items(& iter))
          /* Match with one of the species' sources: need to expand! */
          return BOXTYPECMP_MATCHING;
        else
          /* Match with species' target: no need to expand! */
          return BOXTYPECMP_EQUAL;
      }
    }

    return BOXTYPECMP_DIFFERENT;
  }

  case BOXTYPECLASS_STRUCTURE:
    /* Note that two structures do match when the types of their members do
     * match, even if the names of the members are different. For example,
     *
     *   (Real x, y) == (Real z, hgj)
     *
     * A particular tuple can be raised to make sure it does not match a tuple
     * having the same types of members.
     */
    if (left->type_class == right->type_class) {
      BoxTypeIter liter, riter;
      BoxType lnode, rnode;
      
      BoxTypeIter_Init(& liter, left);
      BoxTypeIter_Init(& riter, right);

      if (BoxType_Get_Structure_Num_Members(left)
          == BoxType_Get_Structure_Num_Members(right)) {
        BoxTypeCmp ret = BOXTYPECMP_EQUAL;

        for (; (BoxTypeIter_Get_Next(& liter, & lnode)
                && BoxTypeIter_Get_Next(& riter, & rnode)); ) {
          BoxType lmemb = BoxType_Get_Structure_Member_Type(lnode),
                  rmemb = BoxType_Get_Structure_Member_Type(rnode);

          ret &= BoxType_Compare(lmemb, rmemb);
          if (ret == BOXTYPECMP_DIFFERENT)
            return BOXTYPECMP_DIFFERENT;
        }

        return ret;
      }
    }

    return BOXTYPECMP_DIFFERENT;

  case BOXTYPECLASS_ENUM:
    assert(0);

#if 0
  case TS_KIND_PROC:
    if (td1->data.proc.combine != td2->data.proc.combine)
      return TS_TYPES_UNMATCH;
      
    if (TS_Compare(ts, td1->data.proc.parent, td2->data.proc.parent)
        != TS_TYPES_EQUAL)
      return TS_TYPES_UNMATCH;
    return TS_Compare(ts, td1->target, td2->target);

  case BOXTYPE_POINTER:
#endif
      
  default:
    MSG_ERROR("TS_Compare: not fully implemented!");
    return BOXTYPECMP_DIFFERENT;
  }
}

#if 0
BoxType_Get_Combination(BoxType child, BoxType parent) {


}
#endif
