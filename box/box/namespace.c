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
#include "messages.h"
#include "hashtable.h"
#include "array.h"
#include "namespace.h"
#include "value.h"
#include "combs.h"

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
  BoxInt num_floors_left = BoxArr_Num_Items(& ns->floors);
  if (num_floors_left != 1)
    MSG_WARNING("num floors = %I at Namespace destruction!", num_floors_left);

  Namespace_Floor_Down(ns);
  assert(BoxArr_Num_Items(& ns->floors) == 0);
  BoxArr_Finish(& ns->floors);
  BoxHT_Finish(& ns->ht);
}

Namespace *Namespace_New(void) {
  Namespace *ns = Box_Mem_Safe_Alloc(sizeof(Namespace));
  Namespace_Init(ns);
  return ns;
}

void Namespace_Destroy(Namespace *ns) {
  Namespace_Finish(ns);
  Box_Mem_Free(ns);
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

typedef struct {
  BoxType     *parent;    /**< Type of the parent. */
  BoxType     *comb_node; /**< Node associated to the combination. */
} MyProcedureNmspItem;

typedef struct {
  void         *data;    /**< Data passed to the callback */
  NmspCallback callback; /**< Callback function */
} MyCallbackNmspItem;

static void
My_NmspItem_Finish(Namespace *ns, size_t floor_idx, NmspItem *item) {
  switch(item->type) {
  case NMSPITEMTYPE_VALUE:
    if (((Value *) item->data)->num_ref != 1) {
      Value *v = (Value *) item->data;
      if (v->name != NULL)
        MSG_WARNING("'%s' unlinked, but ref count == %I",
                    v->name, v->num_ref - 1);
      else
        MSG_WARNING("Object unlinked, but ref count == %I", v->num_ref - 1);
    }
    Value_Unlink((Value *) item->data);
    return;

  case NMSPITEMTYPE_PROCEDURE:
    {
      MyProcedureNmspItem *p = (MyProcedureNmspItem *) item->data;
      if (floor_idx > 1)
        BoxType_Undefine_Combination(p->parent, p->comb_node);
      Box_Mem_Free(p);
      return;
    }

  case NMSPITEMTYPE_CALLBACK:
    {
      MyCallbackNmspItem *d = (MyCallbackNmspItem *) item->data;
      if (d->callback != NULL)
        d->callback(ns, item, d->data);
      return;
    }

  default:
    printf("My_NmspItem_Finish: don't know how to remove item!");
  }
}

void Namespace_Floor_Down(Namespace *ns) {
  NmspFloorData floor_data;
  NmspItem *item;
  size_t floor_idx;

  BoxArr_Pop(& ns->floors, & floor_data);
  floor_idx = BoxArr_Get_Num_Items(& ns->floors);

  for(item = floor_data.first_item; item != NULL;) {
    NmspItem *item_to_del = item;
    item = item->next;
    My_NmspItem_Finish(ns, floor_idx, item_to_del);
    if (item_to_del->ht_item != NULL)
      BoxHT_Remove_By_HTItem(& ns->ht, item_to_del->ht_item);
    else
      Box_Mem_Free(item_to_del);
  }
}

NmspItem *Namespace_Add_Item(Namespace *ns, NmspFloor floor,
                             const char *item_name) {
  NmspItem dummy, *new_item;
  BoxHTItem *hi = NULL;
  NmspFloorData *floor_data = My_Get_Floor(ns, floor);

  if (item_name != NULL) {
    size_t item_name_len = strlen(item_name) + 1;
    hi = BoxHT_Insert_Obj(& ns->ht, item_name, item_name_len,
                          & dummy, sizeof(NmspItem));
    new_item = (NmspItem *) hi->object;
  } else
    new_item = (NmspItem *) Box_Mem_Safe_Alloc(sizeof(NmspItem));

  new_item->ht_item = hi;
  new_item->next = floor_data->first_item;
  floor_data->first_item = new_item;
  return new_item;
}

NmspItem *Namespace_Get_Item(Namespace *ns, NmspFloor floor,
                             const char *item_name) {
  size_t item_name_len;
  assert(item_name != NULL);
  item_name_len = strlen(item_name) + 1;
  BoxHTItem *ht_item;
  if (BoxHT_Find(& ns->ht, (char *) item_name, item_name_len, & ht_item)) {
    return (NmspItem *) ht_item->object;
  } else
    return NULL;
}

/* REFERENCES: v: 0; */
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

void Namespace_Add_Procedure(Namespace *ns, NmspFloor floor,
                             BoxType *parent, BoxType *comb_node) {
  NmspItem *new_item = Namespace_Add_Item(ns, floor, NULL);
  MyProcedureNmspItem *p = Box_Mem_Safe_Alloc(sizeof(MyProcedureNmspItem));
  assert(new_item);
  new_item->type = NMSPITEMTYPE_PROCEDURE;
  new_item->data = p;
  p->parent = parent;
  p->comb_node = comb_node;
}

void Namespace_Add_Callback(Namespace *ns, NmspFloor floor,
                            NmspCallback callback, void *data) {
  NmspItem *new_item = Namespace_Add_Item(ns, floor, (char *) NULL);
  assert(new_item != NULL);
  new_item->type = NMSPITEMTYPE_CALLBACK;
  new_item->data = callback;
}
