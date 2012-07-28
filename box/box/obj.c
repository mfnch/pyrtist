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

/**
 * Object header. Every object allocation includes some extra space to contain
 * This structure, which contains the type of the object and the number of
 * references that other objects make to it.
 */
typedef struct BoxObjHeader_struct {
  size_t  num_refs;
  BoxType type;
} BoxObjHeader;

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

/* Remove a reference to an object, destroying it, if unreferenced. */
BoxSPtr BoxSPtr_Unlink(BoxSPtr src) {
  BoxObjHeader *head = src - sizeof(BoxObjHeader);
  assert(head->num_refs > 0);

  if (head->num_refs > 1) {
    head->num_refs--;
    return src;

  } else {
    (void) BoxSPtr_Unlink(head->type);
    BoxMem_Free(head);
    return NULL;
  }
}

/* Allocate space for an object of the given type. */
BoxSPtr BoxSPtr_Alloc(BoxType t) {
  size_t total_size, obj_size = BoxType_Get_Size(t);
  if (Box_Mem_Sum(& total_size, sizeof(BoxObjHeader), obj_size)) {
    void *whole = Box_Mem_Alloc(total_size);
    if (whole) {
      BoxObjHeader *head = whole;
      void *ptr = (char *) whole + sizeof(BoxObjHeader);
      head->num_refs = 1;
      head->type = BoxSPtr_Link(t);
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
