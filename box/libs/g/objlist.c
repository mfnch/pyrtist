/****************************************************************************
 * Copyright (C) 2008 by Matteo Franchin                                    *
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "types.h"
#include "g.h"
#include "buffer.h"
#include "objlist.h"

#ifdef CIRCULAR_INDEX
#  undef CIRCULAR_INDEX
#endif

#define CIRCULAR_INDEX(num_items, index) \
  ( (index > 0) ? ( (index - 1) % num_items ) + 1 : \
                  num_items - ( (-index) % num_items ) )

Task objlist_init(ObjList *ol, Int obj_size) {
  Int tot_obj_size = sizeof(ObjListItem) + obj_size;
  assert(obj_size >= 0);
  if (buff_create(& ol->ol, tot_obj_size, 8))
    return BOXTASK_OK;
  return BOXTASK_FAILURE;
}

void objlist_destroy(ObjList *ol) {
  int i, n = buff_numitem(& ol->ol);
  for (i = 1; i <= n; i++) {
    ObjListItem *oli = buff_itemptr(& ol->ol, ObjListItem, i);
    free(oli->name);
  }
  buff_free(& ol->ol);
}

Task objlist_clear(ObjList *ol) {
  int i, n = buff_numitem(& ol->ol);
  for(i=1; i <= n; i++) {
    ObjListItem *oli = buff_itemptr(& ol->ol, ObjListItem, i);
    free(oli->name);
  }
  if (buff_clear(& ol->ol)) return BOXTASK_OK;
  return BOXTASK_FAILURE;
}

Task objlist_dup(ObjList *dest, ObjList *src) {
  int i, n;
  if (!buff_dup(& dest->ol, & src->ol)) return BOXTASK_OK;
  /* We need to copy all the string names: they cannot be shared! */
  n = buff_numitem(& dest->ol);
  for(i=1; i <= n; i++) {
    ObjListItem *dest_oli = buff_itemptr(& dest->ol, ObjListItem, i);
    /* Ignoring the out of memory errors should be fine anyway! */
    if (dest_oli->name != (char *) NULL)
      dest_oli->name = strdup(dest_oli->name);
  }
  return BOXTASK_OK;
}

void *objlist_find(ObjList *ol, const char *name) {
  int i, n = buff_numitem(& ol->ol);
  if (name == NULL)
    return NULL;
  for (i = 1; i <= n; i++) {
    ObjListItem *oli = buff_itemptr(& ol->ol, ObjListItem, i);
    if (oli->name != NULL) {
      if (strcmp(oli->name, name) == 0)
        return (void *) oli + sizeof(ObjListItem);
    }
  }
  return NULL;
}

static ObjListItem *My_ObjList_Get_OLI(ObjList *ol, size_t index) {
  size_t n = buff_numitem(& ol->ol);
  if (n > 0) {
    ObjListItem *oli;
    index = CIRCULAR_INDEX(n, index);
    assert(index >= 1 && index <= n);
    oli = buff_itemptr(& ol->ol, ObjListItem, index);
    return oli;

  } else
    return NULL;
}

void *objlist_get(ObjList *ol, size_t index) {
  ObjListItem *oli = My_ObjList_Get_OLI(ol, index);
  return (oli != NULL) ? (void *) oli + sizeof(ObjListItem) : NULL;
}

const char *objlist_get_name(ObjList *ol, size_t index) {
  ObjListItem *oli = My_ObjList_Get_OLI(ol, index);
  return (oli != NULL) ? oli->name : NULL;
}

Task objlist_add(ObjList *ol, void *obj, char *name) {
  ObjListItem *oli;
  void *dest_obj;

  if (name != NULL) {
    if (objlist_find(ol, name) != NULL) {
      g_error("Another object with the same name exists!");
      return BOXTASK_FAILURE;
    }

    name = strdup(name);
    if (name == NULL) {
      g_error("pointlist_add: strdup failed!");
      return BOXTASK_FAILURE;
    }
  }

  if (!buff_bigenough(& ol->ol, ol->ol.numel+1)) {
    free(name);
    return BOXTASK_FAILURE;
  }

  ++ol->ol.numel;
  oli = buff_lastitemptr(& ol->ol, ObjListItem);
  dest_obj = (void *) oli + sizeof(ObjListItem);
  (void) memcpy(dest_obj, obj, ol->ol.elsize - sizeof(ObjListItem));
  oli->name = name;
  return BOXTASK_OK;
}

Task objlist_iter(ObjList *ol, ObjListIterator it, void *data) {
  int i, n = buff_numitem(& ol->ol);
  for(i=1; i <= n; i++) {
    ObjListItem *oli = buff_itemptr(& ol->ol, ObjListItem, i);
    void *obj = (void *) oli + sizeof(ObjListItem);
    if (it(i, oli->name, obj, data) != BOXTASK_OK)
      return BOXTASK_FAILURE;
  }
  return BOXTASK_OK;
}

Int objlist_num(ObjList *ol) {
  return buff_numitem(& ol->ol);
}

