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

#include <box/types.h>
#include <box/ntypes.h>
#include <box/messages.h>
#include <box/mem.h>

typedef enum {
  BOXTYPECLASS_NONE,
#if 0
  BOXTYPECLASS_PARAMETER,
  BOXTYPECLASS_BASIC,
#endif
  BOXTYPECLASS_INTRINSIC,
  BOXTYPECLASS_ALIAS,
  BOXTYPECLASS_RAISED,
  BOXTYPECLASS_STRUCTURE,
#if 0
  BOXTYPECLASS_SPECIES,
  BOXTYPECLASS_ENUM,
#endif
  BOXTYPECLASS_FUNCTION,
  BOXTYPECLASS_POINTER,
  BOXTYPECLASS_ANY,
  BOXTYPECLASS_LIST_NODE
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
 * Structure type: objects of this type contain a fixed number of objects of
 * other types, which can be referred by name.
 */
typedef struct {
  size_t  size,
          alignment,
          num_items;
  BoxType first,
          last;
} BoxTypeStructure;

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
 * List node (used internally by structures, enums, etc.)
 */
typedef struct {
  BoxType next, previous;
} BoxTypeListNode;

/**
 * Generic allocation function for BoxType objects. This function allocates a
 * type header plus a the type data, whose size and composition depend on the
 * particular type class.
 */
static void *MyType_Alloc(BoxType *t, BoxTypeClass tc) {
  size_t additional = 0, total;

  switch (tc) {
  case BOXTYPECLASS_INTRINSIC: additional = sizeof(BoxTypeIntrinsic); break;
  case BOXTYPECLASS_ALIAS: additional = sizeof(BoxTypeIdent); break;
  case BOXTYPECLASS_RAISED: additional = sizeof(BoxTypeRaised); break;
  case BOXTYPECLASS_STRUCTURE: additional = sizeof(BoxTypeStructure); break;
  case BOXTYPECLASS_FUNCTION: additional = sizeof(BoxTypeFunction); break;
  case BOXTYPECLASS_POINTER: additional = sizeof(BoxTypePointer); break;
  case BOXTYPECLASS_ANY: additional = 0; break;
  case BOXTYPECLASS_LIST_NODE: additional = sizeof(BoxTypeListNode); break;
  default:
    MSG_FATAL("Unknown type class in MyType_Alloc");
    return NULL;
  }

  if (Box_Mem_X_Plus_Y(& total, additional, sizeof(BoxTypeDesc))) {
    BoxTypeDesc *td = Box_Mem_RC_Safe_Alloc(total);
    *t = td;
    return (void *) td + sizeof(BoxTypeDesc);
  }   

  MSG_FATAL("Integer overflow in MyType_Alloc");
  return NULL;
}

/**
 * Get the data part of a type. The size and composition of the data type of
 * a given type changes depending on the type class.
 */
void *MyType_Get_Data(BoxType t) {
  return (void *) t + sizeof(BoxTypeDesc);
}

/**
 * Get the allocated objects a given type refers to.
 */
static void MyType_Get_Refs(BoxType t, int *num_refs, BoxType *refs,
                            int *num_mems, void **mems) {
  void *tdata = MyType_Get_Data(t);

  *num_refs = 0;
  *num_mems = 0;

  switch (t->type_class) {
  case BOXTYPECLASS_NONE:
    break;
  case BOXTYPECLASS_INTRINSIC:
    return;
  case BOXTYPECLASS_ALIAS:
    refs[0] = ((BoxTypeIdent *) tdata)->source;
    *num_refs = 1;
    mems[0] = ((BoxTypeIdent *) tdata)->name;
    *num_mems = 1;
    return;
  case BOXTYPECLASS_RAISED:
    refs[0] = ((BoxTypeRaised *) tdata)->source;
    *num_refs = 1;
    return;
  case BOXTYPECLASS_STRUCTURE:
    refs[0] = ((BoxTypeStructure *) tdata)->first;
    *num_refs = 1;
    /* NOTE: we do not own the reference to `last'. */
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
  case BOXTYPECLASS_LIST_NODE:
    refs[0] = ((BoxTypeListNode *) tdata)->next;
    *num_refs = 1;
    /* NOTE: we do not own the reference to `previous'. */
    return;
  }
}

/* Unlink (remove a reference to) the given type. The memory for the type is
 * released if there are no references left to it.
 */
void BoxType_Unlink(BoxType t) {
  void *mems[3];
  BoxType refs[3];
  int num_refs, num_mems, i;

  MyType_Get_Refs(t, & num_refs, refs, & num_mems, mems);

  for (i = 0; i < num_mems; i++)
    BoxMem_Free(mems[i]);

  for (i = 0; i < num_refs; i++)
    BoxType_Unlink(refs[i]);

  Box_Mem_RC_Unlink(t);
}

BoxType BoxType_Link(BoxType t) {
  Box_Mem_RC_Link(t);
  return t;
}

/* Create a new intrinsic type with the given size and alignment. */
BoxType BoxType_Create_List_Node(BoxType source, BoxTypeListNode **list_node) {
  BoxType t;
  BoxTypeListNode *td = MyType_Alloc(& t, BOXTYPECLASS_LIST_NODE);
  td->source = source;
  *list_node = td;
  return t;
}

/* Create a new intrinsic type with the given size and alignment. */
BoxType BoxType_Create_Intrinsic(size_t size, size_t alignment) {
  BoxType t;
  BoxTypeIntrinsic *ti = MyType_Alloc(& t, BOXTYPECLASS_INTRINSIC);
  ti->size = size;
  ti->alignment = alignment;  
  return t;
}

/* Create a new identifier type. */
BoxType BoxType_Create_Ident(BoxType source, const char *name) {
  BoxType t;
  BoxTypeIdent *ta = MyType_Alloc(& t, BOXTYPECLASS_ALIAS);
  ta->name = BoxMem_Strdup(name);
  ta->source = source;
  return t;
}

/* Create a new raised type. */
BoxType BoxType_Create_Raised(BoxType source) {
  BoxType t;
  BoxTypeRaised *td = MyType_Alloc(& t, BOXTYPECLASS_RAISED);
  td->source = source;
  return t;
}

/* Create a new raised type. */
BoxType BoxType_Create_Structure(void) {
  BoxType t;
  BoxTypeStructure *td = MyType_Alloc(& t, BOXTYPECLASS_STRUCTURE);
  td->size = 0;
  td->alignment = 0;
  td->num_items = 0;
  td->first = NULL;
  td->last = NULL;
  return t;
}

/* Create a new function type. */
BoxType BoxType_Create_Function(BoxType child, BoxType parent) {
  BoxType t;
  BoxTypeFunction *td = MyType_Alloc(& t, BOXTYPECLASS_FUNCTION);
  td->child = child;
  td->parent = parent;
  return t;
}

/* Create a new pointer type. */
BoxType BoxType_Create_Pointer(BoxType source) {
  BoxType t;
  BoxTypePointer *td = MyType_Alloc(& t, BOXTYPECLASS_POINTER);
  td->source = source;
  return t;
}

/* Create a new pointer type. */
BoxType BoxType_Create_Any(void) {
  BoxType t;
  (void) MyType_Alloc(& t, BOXTYPECLASS_ANY);
  return t;
}




#if 0

BoxBool BoxType_Compare(BoxType left, BoxType right, BoxTypeCmp *cmp);

BoxType_Get_Combination(BoxType child, BoxType parent) {


}





#endif
