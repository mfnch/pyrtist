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

#include <box/mem.h>
#include <box/obj.h>
#include <box/core.h>
#include <box/types_priv.h>


/* Forward references */
static BoxBool My_Init_Obj(BoxPtr *src, BoxXXXX *t);
static void My_Finish_Obj(BoxPtr *src, BoxXXXX *t);

/* Finalize an Any object. */
void BoxAny_Finish(BoxAny *any)
{
  (void) BoxPtr_Unlink(& any->ptr);
  BoxPtr_Init(& any->ptr);
  //TODO: unlink type as well!
  any->type = NULL;
}

/* Initialize a block of memory addressed by src and with type t. */
static BoxBool My_Init_Obj(BoxPtr *src, BoxXXXX *t) {
  while (1) {
    switch (t->type_class) {
    case BOXTYPECLASS_PRIMARY:
    case BOXTYPECLASS_INTRINSIC:
      return BOXBOOL_TRUE;

    case BOXTYPECLASS_IDENT:
      {
        BoxCallable *callable;
        BoxXXXX *node =
          BoxType_Find_Combination_With_Id(t, BOXCOMBTYPE_AT,
                                           BOXTYPEID_INIT, NULL);
        
        /* Initialise first from the type we are deriving from. */
        t = BoxType_Resolve(t, BOXTYPERESOLVE_IDENT, 1);
        if (!My_Init_Obj(src, t))
          return BOXBOOL_FALSE;

        if (node && BoxType_Get_Combination_Info(node, NULL, & callable))
        {
          /* Now do our own initialization... */
          BoxException *excp = BoxCallable_Call1(callable, src);
          if (!excp)
            return BOXBOOL_TRUE;

          BoxException_Destroy(excp);
          My_Finish_Obj(src, t);
          return BOXBOOL_FALSE;
        }
      }

    case BOXTYPECLASS_RAISED:
      t = BoxType_Resolve(t, BOXTYPERESOLVE_RAISED, 0);
      break;

    case BOXTYPECLASS_STRUCTURE:
      {
        BoxTypeIter ti;
        BoxXXXX *node;
        size_t idx, failure_idx;

        /* If things go wrong, the integer above will be set with the index of
         * the member at which a failure occurred. This will be used to
         * finalized all the previous members of the structure (in order to
         * avoid ending up with a partially initialized structure).
         */
        failure_idx = 0;

        for (BoxTypeIter_Init(& ti, t), idx = 0;
             BoxTypeIter_Get_Next(& ti, & node);
             idx++) {
          size_t offset;
          BoxXXXX *type;
          BoxPtr member_ptr;

          BoxType_Get_Structure_Member(node, NULL, & offset, NULL, & type);
          BoxPtr_Add_Offset(& member_ptr, src, offset);
          if (!My_Init_Obj(& member_ptr, type))
          {
            failure_idx = idx;
            break;
          }
        }

        BoxTypeIter_Finish(& ti);

        /* No failure, exit successfully! */
        if (!failure_idx)
          return BOXBOOL_TRUE;

        /* Failure: finalize whatever was initialized and exit with failure. */
        for (BoxTypeIter_Init(& ti, t), idx = 0;
             BoxTypeIter_Get_Next(& ti, & node) && idx < failure_idx;
             idx++) {
          size_t offset;
          BoxXXXX *type;
          BoxPtr member_ptr;

          BoxType_Get_Structure_Member(node, NULL, & offset, NULL, & type);
          BoxPtr_Add_Offset(& member_ptr, src, offset);
          My_Finish_Obj(& member_ptr, type);
        }
      }

      return BOXBOOL_FALSE;

    case BOXTYPECLASS_SPECIES:
      t = BoxType_Resolve(t, BOXTYPERESOLVE_SPECIES, 0);
      break;

    case BOXTYPECLASS_ENUM:
      /* TO BE IMPLEMENTED */
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
static void My_Finish_Obj(BoxPtr *src, BoxXXXX *t) {
  while (1) {
    switch (t->type_class) {
    case BOXTYPECLASS_PRIMARY:
    case BOXTYPECLASS_INTRINSIC:
      return;

    case BOXTYPECLASS_IDENT:
      {
        BoxCallable *callable;
        BoxXXXX *node =
          BoxType_Find_Combination_With_Id(t, BOXCOMBTYPE_AT,
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
        BoxXXXX *node;
        size_t idx;

        for (BoxTypeIter_Init(& ti, t), idx = 0;
             BoxTypeIter_Get_Next(& ti, & node);
             idx++) {
          size_t offset;
          BoxXXXX *type;
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

/* Add a reference to an object and return it. */
BoxSPtr BoxSPtr_Link(BoxSPtr src) {
  if (src) {
    BoxObjHeader *head = src - sizeof(BoxObjHeader);
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
  BoxObjHeader *head = src - sizeof(BoxObjHeader);
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

/* Remove a reference to an object, destroying it, if unreferenced. */
BoxBool BoxPtr_Unlink(BoxPtr *src) {
  BoxObjHeader *head = BoxPtr_Get_Block(src);
  size_t num_refs = head->num_refs;

  if (num_refs > 1) {
    head->num_refs--;
    return BOXBOOL_TRUE;

  } else {
    BoxPtr src_container;

    assert(num_refs == 1);

    /* Destroy the containing object. */
    src_container.block = src->block;
    src_container.ptr = src->block + sizeof(BoxObjHeader);
    My_Finish_Obj(& src_container, head->type);

    if (head->type)
      (void) BoxSPtr_Unlink(head->type);
    BoxMem_Free(head);
    return BOXBOOL_FALSE;
  }
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
BoxSPtr BoxSPtr_Alloc(BoxXXXX *t) {
  size_t obj_size = BoxType_Get_Size(t);
  return BoxSPtr_Raw_Alloc(t, obj_size);
}

/* Raw allocation function. */
BOXOUT BoxSPtr BoxSPtr_Raw_Alloc(BoxXXXX *t, size_t obj_size) {
  size_t total_size;
  if (Box_Mem_Sum(& total_size, sizeof(BoxObjHeader), obj_size)) {
    void *whole = Box_Mem_Alloc(total_size);
    if (whole) {
      BoxObjHeader *head = whole;
      void *ptr = (char *) whole + sizeof(BoxObjHeader);
      head->num_refs = 1;
      head->type = (t) ? BoxSPtr_Link(t) : NULL;
      return ptr;
    }
  }

  return NULL;
}

/* Allocate and initialize an object of the given type; return its pointer. */
BoxSPtr BoxSPtr_Create(BoxXXXX *t) {
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
