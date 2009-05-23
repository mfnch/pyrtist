/****************************************************************************
 * Copyright (C) 2009 by Matteo Franchin                                    *
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
#include <string.h>
#include <assert.h>

#include "mem.h"
#include "hashtable.h"
#include "array.h"
#include "namespace.h"
#include "value.h"

typedef struct {
  NmspItem *first_item;
} NmspFloorData;

void Namespace_Init(Namespace *ns) {
  BoxHT_Init_Default(& ns->ht, 1024);
  BoxHT_Set_Attr(& ns->ht, BOXHTATTR_COPY_KEYS | BOXHTATTR_COPY_OBJS,
                 BOXHTATTR_COPY_KEYS | BOXHTATTR_COPY_OBJS);
  BoxArr_Init(& ns->floors, sizeof(NmspFloorData), 16);
  Namespace_Floor_Up(ns);
}

void Namespace_Finish(Namespace *ns) {
  Namespace_Floor_Down(ns);
  assert(BoxArr_Num_Items(& ns->floors) == 0);
  BoxArr_Finish(& ns->floors);
  BoxHT_Finish(& ns->ht);
}

Namespace *Namespace_New(void) {
  Namespace *ns = BoxMem_Safe_Alloc(sizeof(Namespace));
  Namespace_Init(ns);
  return ns;
}

void Namespace_Destroy(Namespace *ns) {
  Namespace_Finish(ns);
  BoxMem_Free(ns);
}

static NmspFloorData *My_Get_Floor(Namespace *ns, NmspFloor f) {
  if (f == NMSPFLOOR_DEFAULT)
    return (NmspFloorData *) BoxArr_Last_Item_Ptr(& ns->floors);

  else
    return (NmspFloorData *) BoxArr_Item_Ptr(& ns->floors, f);
}

void Namespace_Floor_Up(Namespace *ns) {
  NmspFloorData *floor_data =
    (NmspFloorData *) BoxArr_Push(& ns->floors, NULL);
  floor_data->first_item = NULL;
}

static void My_NmspItem_Finish(NmspItem *item) {
  switch(item->type) {
  case NMSPITEMTYPE_VALUE:
    Value_Unlink((Value *) item->data);
    return;

  default:
    printf("My_NmspItem_Finish: don't know how to remove item!");
  }
}

void Namespace_Floor_Down(Namespace *ns) {
  NmspFloorData floor_data;
  NmspItem *item;

  BoxArr_Pop(& ns->floors, & floor_data);

  for(item = floor_data.first_item; item != NULL;) {
    NmspItem *item_to_del = item;
    item = item->next;
    My_NmspItem_Finish(item_to_del);
    BoxHT_Remove_By_HTItem(& ns->ht, item_to_del->ht_item);
  }
}

NmspItem *Namespace_Add_Item(Namespace *ns, NmspFloor floor,
                             const char *item_name) {
  size_t item_name_len = strlen(item_name) + 1;
  NmspItem dummy, *new_item;
  BoxHTItem *hi;
  NmspFloorData *floor_data = My_Get_Floor(ns, floor);

  hi = BoxHT_Insert_Obj(& ns->ht, item_name, item_name_len,
                        & dummy, sizeof(NmspItem));
  new_item = (NmspItem *) hi->object;
  new_item->ht_item = hi;
  new_item->next = floor_data->first_item;
  floor_data->first_item = new_item;
  return new_item;
}

NmspItem *Namespace_Get_Item(Namespace *ns, NmspFloor floor,
                             const char *item_name) {
  size_t item_name_len = strlen(item_name) + 1;
  BoxHTItem *ht_item;
  if (BoxHT_Find(& ns->ht, (char *) item_name, item_name_len, & ht_item)) {
    return (NmspItem *) ht_item->object;
  } else
    return NULL;
}

void Namespace_Add_Value(Namespace *ns, NmspFloor floor,
                         const char *item_name, Value *v) {
  NmspItem *new_item = Namespace_Add_Item(ns, floor, item_name);
  assert(new_item != NULL);
  new_item->type = NMSPITEMTYPE_VALUE;
  new_item->data = v;
  Value_Link(v); /* we now own a further reference to the value */
}

Value *Namespace_Get_Value(Namespace *ns, NmspFloor floor,
                           const char *item_name) {
  NmspItem *new_item = Namespace_Get_Item(ns, floor, item_name);
  Value *v;
  if (new_item == NULL) return NULL;
  assert(new_item->type == NMSPITEMTYPE_VALUE);
  v = (Value *) new_item->data;
  Value_Link(v); /* we also return a new reference to the value */
  return v;
}
