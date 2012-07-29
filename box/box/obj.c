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


BoxBool BoxSPtr_Init(BoxSPtr src) {
  return BOXBOOL_TRUE;
}

void BoxSPtr_Finish(BoxSPtr src) {

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
    if (BoxSPtr_Init(obj))
      return obj;

    (void) BoxSPtr_Unlink(obj);
  }

  return NULL;
}
