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
#include <string.h>

#include <box/mem.h>
#include <box/obj.h>
#include <box/core.h>
#include <box/messages.h>
#include <box/exception.h>

#include <box/types_priv.h>


/**
 * @brief Get the header from a pointer as returned by BoxObj_Alloc().
 */
#define MY_GET_HEADER_FROM_OBJ(obj) \
  ((BoxObjHeader *) ((char *) (obj) - sizeof(BoxObjHeader)))

/**
 * @brief Get the object (as returned by BoxObj_Alloc()) from the header.
 *
 * This is the inverse of #MY_GET_HEADER_FROM_OBJ.
 */
#define MY_GET_OBJ_FROM_HEADER(hdr) \
  ((void *) ((char *) (hdr) + sizeof(BoxObjHeader)))


/* Forward references */
static BoxBool My_Init_Obj(BoxPtr *src, BoxType *t);
static void My_Finish_Obj(BoxPtr *src, BoxType *t);
static BoxBool My_Copy_Obj(BoxPtr *dst, BoxPtr *src, BoxType *t);

/* Initialize a subtype. */
void
BoxSubtype_Init(BoxSubtype *st) {
  BoxPtr_Nullify(& st->parent);
  BoxPtr_Nullify(& st->child);
}

/* Finalize a subtype. */
void
BoxSubtype_Finish(BoxSubtype *st) {
  (void) BoxPtr_Unlink(& st->parent);
  (void) BoxPtr_Unlink(& st->child);
}

/* Finalize a subtype. */
void
BoxSubtype_Copy(BoxSubtype *dst, BoxSubtype *src) {
  *dst = *src;
}

/* Finalize an Any object. */
void BoxAny_Finish(BoxAny *any)
{
  (void) BoxPtr_Unlink(& any->ptr);
  BoxPtr_Init(& any->ptr);
  /*TODO: unlink type as well!*/
  any->type = NULL;
}

/* Copy an Any object. */
void BoxAny_Copy(BoxAny *dst, BoxAny *src) {
  *dst = *src;
  (void) BoxPtr_Link(& src->ptr);
  /* Note that the type is implicitly referenced by the object. */
}

/* Change the boxed object stored inside the given any object. */
BoxBool BoxAny_Box(BoxPtr *any, BoxPtr *obj, BoxType *t) {
  BoxAny *any_sptr = BoxPtr_Get_Target(any);
  BoxPtr_Unlink(& any_sptr->ptr);
  any_sptr->ptr = *BoxPtr_Link(obj);
  any_sptr->type = t;
  if (any_sptr->ptr.block == NULL)
    printf("NULL-block pointer when boxing\n");
  return BOXBOOL_TRUE;
}

/* Retrieve the boxed object stored inside the given any object. */
BoxBool BoxAny_Unbox(BOXOUT BoxPtr *obj, BoxPtr *any, BoxType *t) {
  return BOXBOOL_FALSE;
}

/* Initialize a block of memory addressed by src and with type t. */
static BoxBool
My_Init_Obj(BoxPtr *src, BoxType *t) {
  while (1) {
    switch (t->type_class) {
    case BOXTYPECLASS_SUBTYPE_NODE:
      BoxSubtype_Init((BoxSubtype *) BoxPtr_Get_Target(src));
      return BOXBOOL_TRUE;

    case BOXTYPECLASS_PRIMARY:
    case BOXTYPECLASS_INTRINSIC:
      return BOXBOOL_TRUE;

    case BOXTYPECLASS_IDENT:
      {
        BoxCallable *callable;
        BoxType *node;
        BoxType *rt;
        
        /* Initialise first from the type we are deriving from. */
        rt = BoxType_Resolve(t, BOXTYPERESOLVE_IDENT, 1);
        if (!My_Init_Obj(src, rt))
          return BOXBOOL_FALSE;

        node = BoxType_Find_Own_Combination_With_Id(t, BOXCOMBTYPE_AT,
                                                    BOXTYPEID_INIT, NULL);
        if (node && BoxType_Get_Combination_Info(node, NULL, & callable))
        {
          /* Now do our own initialization... */
          BoxException *excp = BoxCallable_Call1(callable, src);
          if (!excp)
            return BOXBOOL_TRUE;

          BoxException_Destroy(excp);
          My_Finish_Obj(src, rt);
          return BOXBOOL_FALSE;
        }

        return BOXBOOL_TRUE;
      }

    case BOXTYPECLASS_RAISED:
      t = BoxType_Resolve(t, BOXTYPERESOLVE_RAISED, 0);
      break;

    case BOXTYPECLASS_STRUCTURE:
      {
        BoxBool success;
        BoxTypeIter ti;
        BoxType *node;
        size_t idx, failure_idx;

        /* If things go wrong, failure_idx will be set to the index of
         * the member at which a failure occurred. This will be used to
         * finalize all the previous members of the structure (in order to
         * avoid ending up with a partially initialized structure).
         */
        success = BOXBOOL_TRUE;
 
        for (BoxTypeIter_Init(& ti, t), idx = 0;
             BoxTypeIter_Get_Next(& ti, & node);
             idx++) {
          size_t offset;
          BoxType *type;
          BoxPtr member_ptr;

          BoxType_Get_Structure_Member(node, NULL, & offset, NULL, & type);
          BoxPtr_Add_Offset(& member_ptr, src, offset);
          if (!My_Init_Obj(& member_ptr, type))
          {
            success = BOXBOOL_FALSE;
            failure_idx = idx;
            break;
          }
        }

        BoxTypeIter_Finish(& ti);

        /* No failure, exit successfully! */
        if (success)
          return BOXBOOL_TRUE;

        /* Failure: finalize whatever was initialized and exit with failure. */
        for (BoxTypeIter_Init(& ti, t), idx = 0;
             BoxTypeIter_Get_Next(& ti, & node) && idx < failure_idx;
             idx++) {
          size_t offset;
          BoxType *type;
          BoxPtr member_ptr;

          BoxType_Get_Structure_Member(node, NULL, & offset, NULL, & type);
          BoxPtr_Add_Offset(& member_ptr, src, offset);
          My_Finish_Obj(& member_ptr, type);
        }

        BoxTypeIter_Finish(& ti);
      }

      return BOXBOOL_FALSE;

    case BOXTYPECLASS_SPECIES:
      t = BoxType_Resolve(t, BOXTYPERESOLVE_SPECIES, 0);
      break;

    case BOXTYPECLASS_ENUM:
      /* TO BE IMPLEMENTED */
      return BOXBOOL_FALSE;

    case BOXTYPECLASS_FUNCTION:
      /* TO BE IMPLEMENTED */
      return BOXBOOL_FALSE;

    case BOXTYPECLASS_POINTER:
      BoxPtr_Init((BoxPtr *) BoxPtr_Get_Target(src));
      return BOXBOOL_TRUE;

    case BOXTYPECLASS_ANY:
      BoxAny_Init((BoxAny *) BoxPtr_Get_Target(src));
      return BOXBOOL_TRUE;

    default:
      return BOXBOOL_FALSE;
    }
  }

  return BOXBOOL_TRUE;
}

/* Generic finalization function for objects. */
static void
My_Finish_Obj(BoxPtr *src, BoxType *t) {
  while (1) {
    switch (t->type_class) {
    case BOXTYPECLASS_SUBTYPE_NODE:
      BoxSubtype_Finish((BoxSubtype *) BoxPtr_Get_Target(src));
      return;

    case BOXTYPECLASS_PRIMARY:
    case BOXTYPECLASS_INTRINSIC:
      return;

    case BOXTYPECLASS_IDENT:
      {
        BoxCallable *callable;
        BoxType *node =
          BoxType_Find_Own_Combination_With_Id(t, BOXCOMBTYPE_AT,
                                               BOXTYPEID_FINISH, NULL);

        if (node && BoxType_Get_Combination_Info(node, NULL, & callable))
        {
          /* Finalize the object. Ignore exceptions! */
          BoxException *excp = BoxCallable_Call1(callable, src);
          if (excp)
            BoxException_Destroy(excp);
        }

        t = BoxType_Resolve(t, BOXTYPERESOLVE_IDENT, 1);
      }
      break;

    case BOXTYPECLASS_RAISED:
      t = BoxType_Resolve(t, BOXTYPERESOLVE_RAISED, 0);
      break;

    case BOXTYPECLASS_STRUCTURE:
      {
        BoxTypeIter ti;
        BoxType *node;

        for (BoxTypeIter_Init(& ti, t);
             BoxTypeIter_Get_Next(& ti, & node);) {
          size_t offset;
          BoxType *type;
          BoxPtr member_ptr;

          BoxType_Get_Structure_Member(node, NULL, & offset, NULL, & type);
          BoxPtr_Add_Offset(& member_ptr, src, offset);
          My_Finish_Obj(& member_ptr, type);
        }

        BoxTypeIter_Finish(& ti);
        return;
      }

    case BOXTYPECLASS_SPECIES:
      t = BoxType_Resolve(t, BOXTYPERESOLVE_SPECIES, 0);
      break;

    case BOXTYPECLASS_ENUM:
      /* TO BE IMPLEMENTED */
    case BOXTYPECLASS_FUNCTION:
      /* TO BE IMPLEMENTED */
      return;

    case BOXTYPECLASS_POINTER:
      (void) BoxPtr_Unlink((BoxPtr *) BoxPtr_Get_Target(src));
      BoxPtr_Init((BoxPtr *) BoxPtr_Get_Target(src));
      return;

    case BOXTYPECLASS_ANY:
      BoxAny_Finish((BoxAny *) BoxPtr_Get_Target(src));
      return;

    default:
      return;
    }
  }
}

/* Generic function to copy objects. */
static BoxBool
My_Copy_Obj(BoxPtr *dst, BoxPtr *src, BoxType *t) {
  /* Check we are not copying the object over itself. */
  if (dst->ptr == src->ptr)
    return BOXBOOL_TRUE;

  /* We do a loop to avoid nested calls when a type needs to be resolved: we
   * rather resolve and try again.
   */
  while (1) {
    switch (t->type_class) {
    case BOXTYPECLASS_PRIMARY:
    case BOXTYPECLASS_INTRINSIC:
      (void) memcpy(dst->ptr, src->ptr, BoxType_Get_Size(t));
      return BOXBOOL_TRUE;

    case BOXTYPECLASS_IDENT:
      {
        BoxCallable *callable;
        BoxType *node;
        BoxType *rt;

        /* First, copy the derived type. */
        rt = BoxType_Resolve(t, BOXTYPERESOLVE_IDENT, 1);

        node = BoxType_Find_Own_Combination(t, BOXCOMBTYPE_COPY, t, NULL);

        if (!node)
          return My_Copy_Obj(dst, src, rt);

        if (node && BoxType_Get_Combination_Info(node, NULL, & callable))
        {
          /* Now do our own initialization... */
          BoxException *excp = BoxCallable_Call2(callable, dst, src);
          if (!excp)
            return BOXBOOL_TRUE;

          BoxException_Destroy(excp);
          My_Finish_Obj(dst, rt);
          return BOXBOOL_FALSE;
        }
      }

      return BOXBOOL_TRUE;

    case BOXTYPECLASS_RAISED:
      t = BoxType_Resolve(t, BOXTYPERESOLVE_RAISED, 0);
      assert(t->type_class != BOXTYPECLASS_RAISED);
      break;

    case BOXTYPECLASS_STRUCTURE:
      {
        BoxBool success;
        BoxTypeIter ti;
        BoxType *node;
        size_t idx, failure_idx;

        /* If something goes wrong, idx contains the index for which copy
         * failed, so that we can finalise all the partially copied members.
         */
        success = BOXBOOL_TRUE;

        for (BoxTypeIter_Init(& ti, t), idx = 0;
             BoxTypeIter_Get_Next(& ti, & node);
             idx++) {
          size_t offset;
          BoxType *memb_type;
          BoxPtr dst_memb_ptr, src_memb_ptr;

          BoxType_Get_Structure_Member(node, NULL, & offset, NULL,
                                       & memb_type);
          BoxPtr_Add_Offset(& dst_memb_ptr, dst, offset);
          BoxPtr_Add_Offset(& src_memb_ptr, src, offset);
          if (!My_Copy_Obj(& dst_memb_ptr, & src_memb_ptr, memb_type)) {
            success = BOXBOOL_FALSE;
            failure_idx = idx;
            break;
          }
        }

        BoxTypeIter_Finish(& ti);

        /* No failure, exit successfully! */
        if (success)
          return BOXBOOL_TRUE;

        /* Failure: finalize whatever was copied and exit with failure. */
        for (BoxTypeIter_Init(& ti, t), idx = 0;
             BoxTypeIter_Get_Next(& ti, & node) && idx < failure_idx;
             idx++) {
          size_t offset;
          BoxType *memb_type;
          BoxPtr dst_memb_ptr;

          BoxType_Get_Structure_Member(node, NULL, & offset, NULL,
                                       & memb_type);
          BoxPtr_Add_Offset(& dst_memb_ptr, dst, offset);
          My_Finish_Obj(& dst_memb_ptr, memb_type);
        }

        BoxTypeIter_Finish(& ti);

        return BOXBOOL_FALSE;
      }

    case BOXTYPECLASS_SPECIES:
      t = BoxType_Resolve(t, BOXTYPERESOLVE_SPECIES, 0);
      assert(t->type_class != BOXTYPECLASS_SPECIES);
      break;

    case BOXTYPECLASS_ENUM:
    case BOXTYPECLASS_FUNCTION:
    case BOXTYPECLASS_POINTER:
      return BOXBOOL_FALSE;

    case BOXTYPECLASS_ANY:
      BoxAny_Copy((BoxAny *) BoxPtr_Get_Target(dst),
                  (BoxAny *) BoxPtr_Get_Target(src));
      return BOXBOOL_TRUE;

    default:
      MSG_FATAL("Unexpected type class (%d) in My_Copy_Obj", t->type_class);
      return BOXBOOL_FALSE;
    }
  }
 
  /* Should never get here... */
  return BOXBOOL_FALSE;
}

/* Add a reference to an object and return it. */
BoxSPtr BoxSPtr_Link(BoxSPtr src) {
  if (src) {
    BoxObjHeader *head = MY_GET_HEADER_FROM_OBJ(src);
    assert(head->num_refs > 0);
    head->num_refs++;
    return src;
  } else
    return NULL;
}

/* This function allows unlinking an object and performing some extra
 * operations before object destruction.
 */
BoxBool BoxSPtr_Unlink_Begin(BoxSPtr src) {
  BoxObjHeader *head = MY_GET_HEADER_FROM_OBJ(src);
  BoxSPtr ret;

  if (head->num_refs == 1)
    return BOXBOOL_TRUE;

  assert(head->num_refs > 1);
  ret = BoxSPtr_Unlink(src);
  assert(ret);
  return BOXBOOL_FALSE;
}

/* Function to be used in conjunction with BoxSPtr_Unlink_Begin. */
void BoxSPtr_Unlink_End(BoxSPtr src) {
  BoxSPtr ret = BoxSPtr_Unlink(src);
  assert(!ret);
} 

/* Reference the given object. */
BoxPtr *BoxPtr_Link(BoxPtr *src) {
  if (!BoxPtr_Is_Detached(src)) {
    BoxObjHeader *head = BoxPtr_Get_Block(src);
    assert(head->num_refs >= 1);
    head->num_refs++;
  }

  return src;
}

/* Remove a reference to an object, destroying it, if unreferenced. */
BoxBool BoxPtr_Unlink(BoxPtr *src) {
  BoxObjHeader *head = BoxPtr_Get_Block(src);
  size_t num_refs;

  if (!head)
    return BOXBOOL_TRUE;

  num_refs = head->num_refs;

  if (num_refs > 1) {
    head->num_refs--;
    return BOXBOOL_TRUE;

  } else {
    BoxPtr src_container;

    assert(num_refs == 1);

    /* Destroy the containing object. */
    src_container.block = src->block;
    src_container.ptr = MY_GET_OBJ_FROM_HEADER(head);
    My_Finish_Obj(& src_container, head->type);

    if (head->type)
      (void) BoxType_Unlink(head->type);
    head->num_refs = 0;
    Box_Mem_Free(head);
    return BOXBOOL_FALSE;
  }
}

/* Get the type of the allocated object. */
BoxType *
BoxSPtr_Get_Type(BoxSPtr obj) {
  if (obj) {
    BoxObjHeader *head = MY_GET_HEADER_FROM_OBJ(obj);
    assert(head->num_refs >= 1);
    return head->type;
  } else
    return NULL;
}

/* Remove a reference to an object, destroying it, if unreferenced. */
BoxSPtr BoxSPtr_Unlink(BoxSPtr src) {
  if (src) {
    BoxPtr src_ptr;
    BoxPtr_Init_From_SPtr(& src_ptr, src);

    if (BoxPtr_Unlink(& src_ptr))
      return src;
  }

  return NULL;
}

/* Allocate space for an object of the given type. */
BoxSPtr BoxSPtr_Alloc(BoxType *t) {
  size_t obj_size = BoxType_Get_Size(t);
  return BoxSPtr_Raw_Alloc(t, obj_size);
}

/* Raw allocation function. */
BOXOUT BoxSPtr BoxSPtr_Raw_Alloc(BoxType *t, size_t obj_size) {
  size_t total_size;
  if (Box_Mem_Sum(& total_size, sizeof(BoxObjHeader), obj_size)) {
    void *whole = Box_Mem_Alloc(total_size);
    if (whole) {
      BoxObjHeader *head = whole;
      void *ptr = MY_GET_OBJ_FROM_HEADER(whole);
      head->num_refs = 1;
      head->type = (t) ? BoxSPtr_Link(t) : NULL;
      return ptr;
    }
  }

  return NULL;
}

/* Allocate and initialize an object of the given type; return its pointer. */
BoxSPtr BoxSPtr_Create(BoxType *t) {
  BoxSPtr obj = BoxSPtr_Alloc(t);

  BoxPtr obj_ptr;
  BoxPtr_Init_From_SPtr(& obj_ptr, obj);

  if (obj) {
    if (My_Init_Obj(& obj_ptr, t))
      return obj;

    (void) BoxSPtr_Unlink(obj);
  }

  return NULL;
}

/* Create a new object of the given type and return a pointer to it. */
BoxBool
BoxPtr_Create_Obj(BOXOUT BoxPtr *ptr, BoxType *t) {
  BoxSPtr obj = BoxSPtr_Create(t);
  BoxPtr_Init_From_SPtr(ptr, obj);
  return (obj != NULL);
}

/* Copy an object of the given type. */
BoxBool
BoxPtr_Copy_Obj(BoxPtr *dst, BoxPtr *src, BoxType *t) {
  return My_Copy_Obj(dst, src, t);
}

/* Try to convert a BoxPtr object to a simple single pointer. */
void *
BoxPtr_Get_SPtr(const BoxPtr *src) {
  if (src) {
    void *sptr = (char *) src->block + sizeof(BoxObjHeader);
    if (sptr == src->ptr)
      return sptr;
  }

  return NULL;
}

/* Combine two objects from their types and pointers. */
BoxBool
Box_Combine(BoxType *t_parent, BoxPtr *parent,
            BoxType *t_child, BoxPtr *child, BoxException **exception) {
  BoxType *comb_node, *expand_type;
  BoxTypeCmp expand;
  BoxCallable *cb;

  if (!(t_parent && t_child))
    return BOXBOOL_FALSE;

  comb_node = BoxType_Find_Combination(t_parent, BOXCOMBTYPE_AT, t_child,
                                       & expand);
  if (!comb_node)
    return BOXBOOL_FALSE;

  if (!BoxType_Get_Combination_Info(comb_node, & expand_type, & cb))
    MSG_FATAL("Failed getting combination info in dynamic call.");

  if (expand == BOXTYPECMP_MATCHING) {
    *exception = BoxException_Create_Raw("Dynamic expansion of type is not "
                                         "yet implemented");
    return BOXBOOL_TRUE;
  }

  /* Make sure we are not passing NULL objects... */
  if (!(parent && child)) {
    if (!parent && !BoxType_Is_Empty(t_parent)) {
      *exception = BoxException_Create_Raw("Empty parent in dynamic "
                                           "combination");
      return BOXBOOL_TRUE;
    }

    if (!child && !BoxType_Is_Empty(t_child)) {
      *exception = BoxException_Create_Raw("Empty child in dynamic "
                                           "combination");
      return BOXBOOL_TRUE;
    }
  }

  *exception = BoxCallable_Call2(cb, parent, child);
  return BOXBOOL_TRUE;
}

/* Combine two objects boxed as BoxAny objects. */
BoxBool
Box_Combine_Any(BoxAny *parent, BoxAny *child, BoxException **except) {
  return Box_Combine(BoxAny_Get_Type(parent), BoxAny_Get_Obj(parent),
                     BoxAny_Get_Type(child), BoxAny_Get_Obj(child), except);
}
