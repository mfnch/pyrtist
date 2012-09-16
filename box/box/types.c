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
#include <stddef.h>

// TODO: remove this
#include <stdio.h>


#include <box/types.h>
#include <box/types_priv.h>

#include <box/messages.h>
#include <box/mem.h>
#include <box/obj.h>
#include <box/callable.h>

#include <box/core_priv.h>
#include <box/callable_priv.h>


int num_type_nodes = 0;
size_t total_size_of_types = 0;

/* Generic allocation function for BoxType objects. This function allocates a
 * type header plus a the type data, whose size and composition depend on the
 * particular type class.
 */
void *BoxType_Alloc(BoxType **t, BoxTypeClass tc) {
  size_t additional = 0, total;
  size_t data_offset = offsetof(BoxTypeBundle, data);

  switch (tc) {
  case BOXTYPECLASS_STRUCTURE_NODE:
    additional = sizeof(BoxTypeStructureNode); break;
  case BOXTYPECLASS_SPECIES_NODE:
    additional = sizeof(BoxTypeSpeciesNode); break;
  case BOXTYPECLASS_COMB_NODE:
    additional = sizeof(BoxTypeCombNode); break;
  case BOXTYPECLASS_SUBTYPE_NODE:
    additional = sizeof(BoxTypeSubtypeNode); break;
  case BOXTYPECLASS_PRIMARY: additional = sizeof(BoxTypePrimary); break;
  case BOXTYPECLASS_INTRINSIC: additional = sizeof(BoxTypeIntrinsic); break;
  case BOXTYPECLASS_IDENT: additional = sizeof(BoxTypeIdent); break;
  case BOXTYPECLASS_RAISED: additional = sizeof(BoxTypeRaised); break;
  case BOXTYPECLASS_STRUCTURE: additional = sizeof(BoxTypeStructure); break;
  case BOXTYPECLASS_SPECIES: additional = sizeof(BoxTypeSpecies); break;
  case BOXTYPECLASS_FUNCTION: additional = sizeof(BoxTypeFunction); break;
  case BOXTYPECLASS_POINTER: additional = sizeof(BoxTypePointer); break;
  case BOXTYPECLASS_ANY: additional = 0; break;
  default:
    MSG_FATAL("Unknown type class in BoxType_Alloc");
    return NULL;
  }

  if (Box_Mem_Sum(& total, data_offset, additional)) {
    BoxTypeDesc *td = BoxSPtr_Raw_Alloc(box_core_types.type_type, total);

    num_type_nodes++;
    total_size_of_types += total;

    if (!td)
      MSG_FATAL("Cannot allocate memory for type object.");

    td->type_class = tc;
    *t = td;
    return & ((BoxTypeBundle *) td)->data;
  }   

  MSG_FATAL("Integer overflow in BoxType_Alloc");
  return NULL;
}

/* Transition function needed to make the old and new type interchangable. */
BoxTypeId BoxType_Get_Id(BoxType *t) {
  BoxTypeId id = (t ? t->type_id : BOXTYPEID_NONE);
  assert((id == BOXTYPEID_NONE) == (t == NULL));
  return id;
}

void BoxType_Set_Id(BoxType *t, BoxTypeId id) {
  t->type_id = id;
}


/* Get the type class of a given type. The type class is effectively the
 * type of type (the answer to whether the type is a struct, a species, etc.)
 */
BoxTypeClass BoxType_Get_Class(BoxType *type) {
  return type->type_class;
}

/* Get the data part of a type. The size and composition of the data type of
 * a given type changes depending on the type class.
 */
void *BoxType_Get_Data(BoxType *t) {
  return & ((BoxTypeBundle *) t)->data;
}

/**
 * Return the BoxTypeNode object associated to a node type or NULL, if the
 * given type is not a node type.
 */
BoxTypeNode *MyType_Get_Node(BoxType *t) {
  void *td = BoxType_Get_Data(t);
  switch (t->type_class) {
  case BOXTYPECLASS_SPECIES_NODE:
    return & ((BoxTypeSpeciesNode *) td)->node;
  case BOXTYPECLASS_STRUCTURE_NODE:
    return & ((BoxTypeStructureNode *) td)->node;
  case BOXTYPECLASS_COMB_NODE: return & ((BoxTypeCombNode *) td)->node;
  case BOXTYPECLASS_SUBTYPE_NODE: return & ((BoxTypeSubtypeNode *) td)->node;
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
 *   BOX_MAX_NUM_REFS_IN_TYPE.
 * @param refs Location where to write the references. This is an array of
 *   BoxSPtr objects which should be able to contain at least
 *   BOX_MAX_NUM_REFS_IN_TYPE elements.
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
 * BoxType *refs[BOX_MAX_NUM_REFS_IN_TYPE];
 * int num_refs, num_mems;
 * MyType_Get_Refs(type, & num_refs, refs, & num_mems, mems);
 * @endcode
 */
static void MyType_Get_Refs(BoxType *t, int *num_refs, BoxSPtr *refs,
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
    refs[2] = ((BoxTypeCombNode *) tdata)->callable;
    *num_refs = 3;
    return;
  case BOXTYPECLASS_SUBTYPE_NODE:
    refs[0] = ((BoxTypeSubtypeNode *) tdata)->node.next;
    refs[1] = ((BoxTypeSubtypeNode *) tdata)->type;
    *num_refs = 2;
    mems[0] = ((BoxTypeSubtypeNode *) tdata)->name;
    *num_mems = 1;
    return;
  case BOXTYPECLASS_PRIMARY:
  case BOXTYPECLASS_INTRINSIC:
    return;
  case BOXTYPECLASS_IDENT:
    refs[0] = ((BoxTypeIdent *) tdata)->source;
    refs[1] = ((BoxTypeIdent *) tdata)->combs.node.next;
    refs[2] = ((BoxTypeIdent *) tdata)->subtypes.node.next;
    *num_refs = 3;
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

/* Finalization function for type objects. */
static BoxException *My_Type_Finish(BoxPtr *parent) {
  void *mems[BOX_MAX_NUM_MEMS_IN_TYPE];
  BoxSPtr refs[BOX_MAX_NUM_REFS_IN_TYPE];
  int num_refs, num_mems, i;

  BoxType *t = BoxPtr_Get_Target(parent);
  MyType_Get_Refs(t, & num_refs, refs, & num_mems, mems);

  for (i = 0; i < num_mems; i++)
    Box_Mem_Free(mems[i]);

  for (i = 0; i < num_refs; i++)
    BoxSPtr_Unlink(refs[i]);

  return NULL;
}

/* Register initialization and finalization for types. */
BoxBool Box_Register_Type_Combs(BoxCoreTypes *ct) {
  BoxCallable *callable =
    BoxCallable_Create_Undefined(ct->type_type, ct->finish_type);
  callable = BoxCallable_Define_From_CCall1(callable, My_Type_Finish);
  if (!callable)
    return BOXBOOL_FALSE;

  if (BoxType_Define_Combination(ct->type_type, BOXCOMBTYPE_AT,
                                 ct->finish_type, callable))
    return BOXBOOL_TRUE;

  BoxSPtr_Unlink(callable);
  return BOXBOOL_FALSE;
}

/* Append one BoxTypeNode item to a top BoxTypeNode item. This is used in
 * structures, enums, etc. to add members.
 */
void BoxTypeNode_Append_Node(BoxTypeNode *top_node, BoxType *item) {
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
void BoxTypeNode_Prepend_Node(BoxTypeNode *top_node, BoxType *item) {
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
BoxType *BoxType_Create_Primary(BoxTypeId id, size_t size, size_t alignment) {
  BoxType *t;
  BoxTypePrimary *td = BoxType_Alloc(& t, BOXTYPECLASS_PRIMARY);
  td->id = id;
  td->size = size;
  td->alignment = alignment;  
  return t;
}

/* Create a new intrinsic type with the given size and alignment. */
BoxType *BoxType_Create_Intrinsic(size_t size, size_t alignment) {
  BoxType *t;
  BoxTypeIntrinsic *ti = BoxType_Alloc(& t, BOXTYPECLASS_INTRINSIC);
  ti->size = size;
  ti->alignment = alignment;  
  return t;
}

/* Create a new identifier type. */
BOXOUT BoxType *
BoxType_Create_Ident(BOXIN BoxType *source, const char *name) {
  BoxType *t;
  BoxTypeIdent *ta = BoxType_Alloc(& t, BOXTYPECLASS_IDENT);
  ta->name = Box_Mem_Strdup(name);
  ta->source = source;
  ta->combs.node.next = NULL;
  ta->combs.node.previous = NULL;
  ta->subtypes.node.next = NULL;
  ta->subtypes.node.previous = NULL;
  return t;
}

/* Add a child type to the namespace of a parent type. */
void BoxType_Add_Type(BoxType *parent, BoxType *child) {
}

/* Create a new raised type. */
BoxType *BoxType_Create_Raised(BoxType *source) {
  BoxType *t;
  BoxTypeRaised *td = BoxType_Alloc(& t, BOXTYPECLASS_RAISED);
  td->source = source;
  return t;
}

/* Get the target type of a raised type. */
BoxType *BoxType_Unraise(BoxType *raised) {
  if (raised->type_class == BOXTYPECLASS_RAISED) {
    BoxTypeRaised *td = BoxType_Get_Data(raised);
    return td->source;
  }
  return NULL;
}

/* Create a new structure type. */
BoxType *BoxType_Create_Structure(void) {
  BoxType *t;
  BoxTypeStructure *td = BoxType_Alloc(& t, BOXTYPECLASS_STRUCTURE);
  td->size = 0;
  td->alignment = 0;
  td->num_items = 0;
  td->node.next = NULL;
  td->node.previous = NULL;
  return t;
}

/* Add a member to a structure type defined with BoxType_Create_Structure. */
void BoxType_Add_Member_To_Structure(BoxType *structure, BoxType *member,
                                     const char *member_name) {
  BoxType *t;
  size_t msize, malgn, ssize;
  BoxTypeStructureNode *td;
  BoxTypeStructure *std = BoxType_Get_Data(structure);
  char *dup_member_name = (member_name ? Box_Mem_Strdup(member_name) : NULL);

  /* Box_Mem_Strdup failed. */
  if (member_name && !dup_member_name)
    MSG_FATAL("Cannot allocate memory for structure member type object.");

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
  td->name = dup_member_name;
  td->size = msize;
  td->offset = Box_Mem_Align_Offset(ssize, malgn);
  td->type = BoxType_Link(member);

  /* Add the member to the structure. */
  std->num_items++;

  /* Alignment is the maximum of the alignments of the members. */
  if (malgn > std->alignment)
    std->alignment = malgn;

  std->size = Box_Mem_Get_Multiple_Size(td->offset + msize, std->alignment);
  BoxTypeNode_Append_Node(& std->node, t);
}

/* Get information on a structure member as obtained from
 * BoxTypeIter_Get_Next.
 */
BoxBool
BoxType_Get_Structure_Member(BoxType *node, char **name, size_t *offset,
                             size_t *size, BoxType **type) {
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
BoxType *BoxType_Get_Structure_Member_Type(BoxType *node) {
  BoxType *t;

  if (BoxType_Get_Structure_Member(node, NULL, NULL, NULL, & t))
    return t;

  return NULL;
}

/* Get the type of a species member as obtained from BoxTypeIter_Get_Next. */
BoxType *BoxType_Find_Structure_Member(BoxType *s, const char *name) {
  BoxTypeIter ti;
  BoxType *t;
  char *member_name = NULL;

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
size_t BoxType_Get_Structure_Num_Members(BoxType *t) {
  if (t->type_class == BOXTYPECLASS_STRUCTURE_NODE) {
    BoxTypeStructure *s = BoxType_Get_Data(t);
    return s->num_items;

  } else
    return 0;
}

/* Create a new species type. */
BOXOUT BoxType *BoxType_Create_Species(void) {
  BoxType *t;
  BoxTypeSpecies *td = BoxType_Alloc(& t, BOXTYPECLASS_SPECIES);
  td->num_items = 0;
  td->node.next = NULL;
  td->node.previous = NULL;
  return t;
}

/* Add a member to a species type defined with BoxType_Create_Species. */
void BoxType_Add_Member_To_Species(BoxType *species, BoxType *member) {
  BoxType *t;
  BoxTypeSpecies *std = BoxType_Get_Data(species);
  BoxTypeSpeciesNode *td;

  /* Now create the member. */
  td = BoxType_Alloc(& t, BOXTYPECLASS_SPECIES_NODE);
  td->type = BoxType_Link(member);

  /* Add the member to the structure. */
  std->num_items++;

  BoxTypeNode_Append_Node(MyType_Get_Node(species), t);
}

/* Get information on a species member as obtained from BoxTypeIter_Get_Next.
 */
BoxType *BoxType_Get_Species_Member_Type(BoxType *node) {
  if (node->type_class == BOXTYPECLASS_SPECIES_NODE) {
    BoxTypeSpeciesNode *sn = BoxType_Get_Data(node);
    return sn->type;
  }

  return NULL;
}

/* Get the target type of a species. */
BoxType *BoxType_Get_Species_Target(BoxType *species) {
  return BoxType_Resolve(species, BOXTYPERESOLVE_SPECIES, 1);
}

/* Create a new function type. */
BOXOUT BoxType *
BoxType_Create_Function(BoxType *parent, BoxType *child) {
  BoxType *t;
  BoxTypeFunction *td = BoxType_Alloc(& t, BOXTYPECLASS_FUNCTION);
  /* A function ``Fn = In -> Out'' does only weak-reference ``In'' and ``Out''.
   * This is fine as - in order to use the function - one object of type ``In''
   * and one object of type ``Out'' must exist already. The only case where
   * we may have problems is the case where the user extracts the types from
   * ``Fn''. We then do not provide any function to allow the user to do this.
   * Later, when a proper GC will have been implemented, we can change this is
   * necessary.
   */
  td->child = BoxType_Link(child);
  td->parent = BoxType_Link(parent);
  return t;
}

/* Create a new pointer type. */
BoxType *BoxType_Create_Pointer(BoxType *source) {
  BoxType *t;
  BoxTypePointer *td = BoxType_Alloc(& t, BOXTYPECLASS_POINTER);
  td->source = source;
  return t;
}

/* Create a new pointer type. */
BoxType *BoxType_Create_Any(void) {
  BoxType *t;
  (void) BoxType_Alloc(& t, BOXTYPECLASS_ANY);
  return t;
}

/**
 * Get the iterator over the subtypes of the given identifier type.
 * @param type An identifier type or a subtype node.
 * @param iter A pointer where to store the iterator.
 * @return BOXBOOL_TRUE for success, BOXTYPE_FALSE for failure.
 */
BoxBool BoxType_Get_Subtypes(BoxType *type, BoxTypeIter *iter) {
  if (type->type_class == BOXTYPECLASS_IDENT) {
    BoxTypeIdent *td = BoxType_Get_Data(type);
    iter->current_node = td->subtypes.node.next;
    return BOXBOOL_TRUE;

  } else if (type->type_class == BOXTYPECLASS_SUBTYPE_NODE) {
    BoxTypeSubtypeNode *td = BoxType_Get_Data(type);
    iter->current_node = td->subtypes.node.next;
    return BOXBOOL_TRUE;

  } else
    return BOXBOOL_FALSE;
}

/* Add a subtype type for a given type. */
BoxType *BoxType_Create_Subtype(BoxType *parent, const char *name,
                                BoxType *type) {
  BoxType *sn;
  BoxTypeSubtypeNode *sd;
  BoxTypeNode *node;

  if (parent->type_class == BOXTYPECLASS_IDENT) {
    BoxTypeIdent *td = BoxType_Get_Data(parent);
    node = & td->subtypes.node;

  } else if (parent->type_class == BOXTYPECLASS_SUBTYPE_NODE) {
    BoxTypeSubtypeNode *td = BoxType_Get_Data(parent);
    node = & td->subtypes.node;

  } else
    return NULL;
  
  sd = BoxType_Alloc(& sn, BOXTYPECLASS_SUBTYPE_NODE);
  sd->name = Box_Mem_Strdup(name);
  sd->type = type ? BoxType_Link(type) : NULL;
  sd->parent = parent;
  sd->combs.node.next = NULL;
  sd->combs.node.previous = NULL;
  sd->subtypes.node.next = NULL;
  sd->subtypes.node.previous = NULL;
  BoxTypeNode_Append_Node(node, sn);
  return sn;
}

/* Find a subtype of the given type. */
BoxType *BoxType_Find_Own_Subtype(BoxType *parent, const char *name) {
  BoxTypeIter ti;
  if (BoxType_Get_Subtypes(parent, & ti)) {
    BoxType *t;
    for (; BoxTypeIter_Get_Next(& ti, & t);) {
      BoxTypeSubtypeNode *node = BoxType_Get_Data(t);
      assert(t->type_class == BOXTYPECLASS_SUBTYPE_NODE);
      if (strcmp(name, node->name) == 0)
        return t;
    }
  }

  return NULL;
}

BoxType *BoxType_Find_Subtype(BoxType *parent, const char *name) {
  BoxType *found_subtype, *former_parent;

  do {
    /* Find a combination for the parent type, if found return. */
    found_subtype = BoxType_Find_Own_Subtype(parent, name);

    if (found_subtype)
      return found_subtype;

    /* Remember the parent. */
    former_parent = parent;

    /* If not found, resolve the parent and try again. */
    parent = BoxType_Resolve(parent,
                             (BOXTYPERESOLVE_IDENT | BOXTYPERESOLVE_SPECIES
                              | BOXTYPERESOLVE_RAISED),
                             1);

  } while (parent != former_parent);

  return NULL;
}

/* Get details about a combination found with BoxType_Find_Combination. */
BoxBool BoxType_Get_Subtype_Info(BoxType *subtype, char **name,
                                 BoxType **parent, BoxType **type) {
  if (subtype->type_class == BOXTYPECLASS_SUBTYPE_NODE) {
    BoxTypeSubtypeNode *td = BoxType_Get_Data(subtype);
    if (name)
      *name = td->name;
    if (parent)
      *parent = td->parent;
    if (type)
      *type = td->type;
    return BOXBOOL_TRUE;
  }
  return BOXBOOL_FALSE;
}

/* Register the type for a given subtype, if not given during creation. */
BoxBool BoxType_Register_Subtype(BoxType *subtype, BoxType *type) {
  if (subtype->type_class == BOXTYPECLASS_SUBTYPE_NODE) {
    BoxTypeSubtypeNode *td = BoxType_Get_Data(subtype);
    if (td->type != NULL)
      return BOXBOOL_FALSE;
    td->type = type ? BoxType_Link(type) : NULL;
    return BOXBOOL_TRUE;
  }
  return BOXBOOL_FALSE; 
}

/* Get the size of the type 't'. */
size_t BoxType_Get_Size(BoxType *t) {
  size_t size;
  if (BoxType_Get_Size_And_Alignment(t, & size, NULL))
    return size;
  else
    return 0;
}

/* Get the size and the aligment of a given type. */
BoxBool
BoxType_Get_Size_And_Alignment(BoxType *t, size_t *size, size_t *algn) {
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

    case BOXTYPECLASS_SUBTYPE_NODE:
      *size = sizeof(BoxSubtype);
      *algn = __alignof__(BoxSubtype);
      return BOXBOOL_TRUE;

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
      /* Get species' node for the target. */
      t = ((BoxTypeSpecies *) td)->node.previous;

      if (!t) {
        printf("Empty species!\n");
        return BOXBOOL_FALSE;
      }

      /* Get the node's type. */
      t = ((BoxTypeSpeciesNode *) BoxType_Get_Data(t))->type;
      break; /* resolve and retry... */

#if 0
    case BOXTYPECLASS_ENUM:
#endif

    case BOXTYPECLASS_FUNCTION:
      *size = sizeof(BoxCallable);
      *algn = __alignof__(BoxCallable);
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
BoxType *BoxType_Resolve(BoxType *t, BoxTypeResolve resolve, int num) {
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

    case BOXTYPECLASS_SUBTYPE_NODE:
      if ((resolve & BOXTYPERESOLVE_SUBTYPE) == 0)
        return t;
      else
        t = ((BoxTypeSubtypeNode *) BoxType_Get_Data(t))->type;
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
      else {
        BoxType *tg = ((BoxTypeSpecies *) BoxType_Get_Data(t))->node.previous;
        if (!tg)
          return t;
        t = ((BoxTypeSpeciesNode *) BoxType_Get_Data(tg))->type;
      }
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

    default:
      MSG_FATAL("BoxType_Resolve: unknown type class %d",
                t->type_class);
    }

    if (num == 1)
      return t;

    else if (num > 1)
      num--;
    
  } while (1);
}

/* Initialize an iterator for iteration over the members of the given type. */
void BoxTypeIter_Init(BoxTypeIter *ti, BoxType *t) {
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
BoxBool BoxTypeIter_Get_Next(BoxTypeIter *ti, BoxType **next) {
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
BoxTypeCmp BoxType_Compare(BoxType *left, BoxType *right) {
  if (left == right)
    return BOXTYPECMP_SAME;

  left = BoxType_Resolve(left, BOXTYPERESOLVE_IDENT, 0);
  right = BoxType_Resolve(right, 
                          BOXTYPERESOLVE_IDENT | BOXTYPERESOLVE_SPECIES, 0);
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
    BoxType *node;
    for (BoxTypeIter_Init(& iter, left);
         BoxTypeIter_Get_Next(& iter, & node);) {
      BoxType *memb = BoxType_Get_Species_Member_Type(node);
      
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
      BoxType *lnode, *rnode;
      
      BoxTypeIter_Init(& liter, left);
      BoxTypeIter_Init(& riter, right);

      if (BoxType_Get_Structure_Num_Members(left)
          == BoxType_Get_Structure_Num_Members(right)) {
        BoxTypeCmp ret = BOXTYPECMP_EQUAL;

        for (; (BoxTypeIter_Get_Next(& liter, & lnode)
                && BoxTypeIter_Get_Next(& riter, & rnode)); ) {
          BoxType *lmemb = BoxType_Get_Structure_Member_Type(lnode),
                  *rmemb = BoxType_Get_Structure_Member_Type(rnode);

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

  default:
    MSG_ERROR("BoxType_Compare: not fully implemented!");
    return BOXTYPECMP_DIFFERENT;
  }
}

/* Get the stem type of a type. */
BoxType *BoxType_Get_Stem(BoxType *type) {
  return BoxType_Resolve(type, BOXTYPERESOLVE_IDENT
                               | BOXTYPERESOLVE_SPECIES
                               | BOXTYPERESOLVE_RAISED, 0);
}

/* Get the container type associated with a given type.
 * The container type is strictly related to the way the compiler handles types
 * (e.g. register types).
 */
BoxContType BoxType_Get_Cont_Type(BoxType *t) {
  BoxType *stem;

  if (!t)
    return BOXCONTTYPE_VOID;

  stem = BoxType_Get_Stem(t);

  /* Char, Int, ... are V.I.P. objects which have their own container types. */
  if (stem->type_class == BOXTYPECLASS_PRIMARY) {
    BoxTypePrimary *td = BoxType_Get_Data(stem);
    BoxTypeId id = td->id;

    /* Here we assume BoxTypeCont and BoxTypeId are defined consistently. */
    if (id >= BOXTYPEID_CHAR && id <= BOXTYPEID_PTR)
      return (BoxContType) id;
    else
      return (td->size == 0) ? BOXCONTTYPE_VOID : BOXCONTTYPE_OBJ;

  } else if (stem->type_class == BOXTYPECLASS_INTRINSIC) {
    BoxTypeIntrinsic *td = BoxType_Get_Data(stem);
    return (td->size == 0) ? BOXCONTTYPE_VOID : BOXCONTTYPE_OBJ;

  } else
    return (BoxType_Get_Size(stem) == 0) ? BOXCONTTYPE_VOID : BOXCONTTYPE_OBJ;
}

/* Whether the type is a void type (contains nothing). */
BoxBool BoxType_Is_Empty(BoxType *t) {
  return (BoxType_Get_Size(t) == 0);
}

/* Get a string representation of the given type. */
char *BoxType_Get_Repr(BoxType *t) {
  switch (t->type_class) {
  case BOXTYPECLASS_STRUCTURE_NODE:
  case BOXTYPECLASS_SPECIES_NODE:
  case BOXTYPECLASS_ENUM_NODE:
  case BOXTYPECLASS_COMB_NODE:
    return Box_Mem_Strdup("<invalid>");

  case BOXTYPECLASS_SUBTYPE_NODE:
    {
      BoxTypeSubtypeNode *td = BoxType_Get_Data(t);
      return Box_SPrintF("%~s.%s", BoxType_Get_Repr(td->parent), td->name);
    }

  case BOXTYPECLASS_IDENT:
    {
      BoxTypeIdent *td = BoxType_Get_Data(t);
      return Box_Mem_Strdup(td->name);
    }

  case BOXTYPECLASS_PRIMARY:
    {
      BoxTypePrimary *td = BoxType_Get_Data(t);
      return Box_SPrintF("<primary:id=%d,size=%d,align=%d>",
                         (int) td->id, (int) td->size, (int) td->alignment);
    }

  case BOXTYPECLASS_INTRINSIC:
    {
      BoxTypeIntrinsic *td = BoxType_Get_Data(t);
      return Box_SPrintF("<intrinsic:size=%d,align=%d>",
                         (int) td->size, (int) td->alignment);
    }

  case BOXTYPECLASS_RAISED:
    {
      BoxTypeRaised *td = BoxType_Get_Data(t);
      return Box_SPrintF("^%~s", BoxType_Get_Repr(td->source));
    }

  case BOXTYPECLASS_SPECIES:
    {
      char *str = NULL;
      BoxTypeIter iter;
      BoxType *node;

      for (BoxTypeIter_Init(& iter, t);
           BoxTypeIter_Get_Next(& iter, & node);) {
        BoxType *memb_type = BoxType_Get_Species_Member_Type(node);
        char *memb_repr;

        memb_repr = (memb_type
                     ? BoxType_Get_Repr(memb_type)
                     : Box_Mem_Strdup("<err>"));

        str = (str ? Box_SPrintF("%~s=>%~s", str, memb_repr) : memb_repr);
      }

      return Box_SPrintF("(%~s)", str);
    }

  case BOXTYPECLASS_STRUCTURE:
    {
      char *str = NULL;
      BoxTypeIter iter;
      BoxType *node, *prev_type;
      BoxBool has_prev_type = BOXBOOL_FALSE;

      for (BoxTypeIter_Init(& iter, t);
           BoxTypeIter_Get_Next(& iter, & node);) {
        char *memb_name;
        BoxType *memb_type;
        char *memb_repr;

        if (BoxType_Get_Structure_Member(node, & memb_name, NULL,
                                         NULL, & memb_type)) {
          if (!memb_name)
            memb_repr = BoxType_Get_Repr(memb_type);

          else if (has_prev_type && memb_type == prev_type)
            memb_repr = Box_Mem_Strdup(memb_name);

          else
            memb_repr= Box_SPrintF("%~s %s",
                                   BoxType_Get_Repr(memb_type), memb_name);

          has_prev_type = BOXBOOL_TRUE;
          prev_type = memb_type;

        } else {
          memb_repr = Box_Mem_Strdup("<err>");
          has_prev_type = BOXBOOL_FALSE;
        }

        str = (str ? Box_SPrintF("%~s, %~s", str, memb_repr) : memb_repr);
      }

      return Box_SPrintF("(%~s)", str);
    }

  case BOXTYPECLASS_ENUM:
  default:
    return NULL;
  }
}
