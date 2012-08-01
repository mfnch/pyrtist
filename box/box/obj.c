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
static BoxBool My_Init_Obj(void *src, BoxType t);
static void My_Finish_Obj(void *src, BoxType t);

/* Initialize a block of memory addressed by src and with type t. */
static BoxBool My_Init_Obj(void *src, BoxType t) {
  while(1) {
    switch (t->type_class) {
    case BOXTYPECLASS_PRIMARY:
    case BOXTYPECLASS_INTRINSIC:
      return BOXBOOL_TRUE;

    case BOXTYPECLASS_IDENT:
      {
        BoxCallable *callable;
        BoxType node =
          BoxType_Find_Combination_With_Id(t, BOXCOMBTYPE_AT,
                                           BOXTYPEID_INIT, NULL);
        if (!node)
          return BOXBOOL_FALSE;

        if (BoxType_Get_Combination(node, NULL, & callable))
        {
          BoxPtr parent;
          BoxException *excp = BoxCallable_Call1(callable, & parent);
          if (!excp)
            return BOXBOOL_TRUE;

          BoxException_Destroy(excp);
        }
      }
      return BOXBOOL_FALSE;

    case BOXTYPECLASS_RAISED:
      t = BoxType_Resolve(t, BOXTYPERESOLVE_RAISED, 0);
      break;

    case BOXTYPECLASS_STRUCTURE:
      {
        BoxTypeIter ti;
        BoxType node;
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
          BoxType type;

          BoxType_Get_Structure_Member(node, NULL, & offset, NULL, & type);
          if (!My_Init_Obj(src + offset, type))
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
          BoxType type;

          BoxType_Get_Structure_Member(node, NULL, & offset, NULL, & type);
          My_Finish_Obj(src + offset, type);
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
      BoxPtr_Init((BoxPtr *) src);
      return BOXBOOL_TRUE;

    case BOXTYPECLASS_ANY:
      BoxAny_Init((BoxAny *) src);
      return BOXBOOL_TRUE;

    default:
      return BOXBOOL_FALSE;
    }
  };

  return BOXBOOL_TRUE;
}

static void My_Finish_Obj(void *src, BoxType t) {
}

/* Add a reference to an object and return it. */
BoxSPtr BoxSPtr_Link(BoxSPtr src) {
  BoxObjHeader *head = src - sizeof(BoxObjHeader);
  assert(head->num_refs > 0);
  head->num_refs++;
  return src;
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
BoxSPtr BoxSPtr_Unlink(BoxSPtr src) {
  BoxObjHeader *head = src - sizeof(BoxObjHeader);
  assert(head->num_refs > 0);

  if (head->num_refs > 1) {
    head->num_refs--;
    return src;

  } else {
    if (head->type)
      (void) BoxSPtr_Unlink(head->type);
    BoxMem_Free(head);
    return NULL;
  }
}

/* Allocate space for an object of the given type. */
BoxSPtr BoxSPtr_Alloc(BoxType t) {
  size_t obj_size = BoxType_Get_Size(t);
  return BoxSPtr_Raw_Alloc(t, obj_size);
}

/* Raw allocation function. */
BoxSPtr BoxSPtr_Raw_Alloc(BoxType t, size_t obj_size) {
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
BoxSPtr BoxSPtr_Create(BoxType t) {
  BoxSPtr obj = BoxSPtr_Alloc(t);
  if (obj) {
    if (My_Init_Obj(obj, t))
      return obj;

    (void) BoxSPtr_Unlink(obj);
  }

  return NULL;
}
