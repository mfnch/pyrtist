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

#include "compiler_priv.h"

namespace Box {

typedef struct {
  NmspItem *first_item;
} NmspFloorData;

void
Compiler::Namespace_Init()
{
  BoxHT_Init_Default(& ns.ht, 1024);
  BoxHT_Set_Attr(& ns.ht, BOXHTATTR_COPY_KEYS | BOXHTATTR_COPY_OBJS,
                 BOXHTATTR_COPY_KEYS | BOXHTATTR_COPY_OBJS);
  BoxArr_Init(& ns.floors, sizeof(NmspFloorData), 16);
  Namespace_Floor_Up();
}

void
Compiler::Namespace_Finish()
{
  BoxInt num_floors_left = BoxArr_Num_Items(& ns.floors);
  if (num_floors_left != 1)
    LOG_WARN("num floors = %I at Namespace destruction!", num_floors_left);

  Namespace_Floor_Down();
  assert(BoxArr_Num_Items(& ns.floors) == 0);
  BoxArr_Finish(& ns.floors);
  BoxHT_Finish(& ns.ht);
}

static NmspFloorData *
My_Get_Floor(Namespace *ns, NmspFloor f)
{
  if (f == NMSPFLOOR_DEFAULT)
    return (NmspFloorData *) BoxArr_Last_Item_Ptr(& ns->floors);
  else
    return (NmspFloorData *) BoxArr_Item_Ptr(& ns->floors, f);
}

void
Compiler::Namespace_Floor_Up()
{
  NmspFloorData *floor_data = (NmspFloorData *) BoxArr_Push(& ns.floors, NULL);
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
My_NmspItem_Finish(Compiler *c, Namespace *ns, size_t floor_idx, NmspItem *item)
{
  switch(item->type) {
  case NMSPITEMTYPE_VALUE:
    {
      Value *v = (Value *) item->data;
      c->Destroy_Value(c->Track_Value(v));
      return;
    }
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

void
Compiler::Namespace_Floor_Down()
{
  NmspFloorData floor_data;
  NmspItem *item;
  size_t floor_idx;

  BoxArr_Pop(& ns.floors, & floor_data);
  floor_idx = BoxArr_Get_Num_Items(& ns.floors);

  for(item = floor_data.first_item; item;) {
    NmspItem *item_to_del = item;
    item = item->next;
    My_NmspItem_Finish(this, & ns, floor_idx, item_to_del);
    if (item_to_del->ht_item != NULL)
      BoxHT_Remove_By_HTItem(& ns.ht, item_to_del->ht_item);
    else
      Box_Mem_Free(item_to_del);
  }
}

NmspItem *
Compiler::Namespace_Add_Item(NmspFloor floor, const char *item_name)
{
  NmspItem dummy, *new_item;
  BoxHTItem *hi = NULL;
  NmspFloorData *floor_data = My_Get_Floor(& ns, floor);

  if (item_name != NULL) {
    size_t item_name_len = strlen(item_name) + 1;
    hi = BoxHT_Insert_Obj(& ns.ht, item_name, item_name_len,
                          & dummy, sizeof(NmspItem));
    new_item = (NmspItem *) hi->object;
  } else
    new_item = (NmspItem *) Box_Mem_Safe_Alloc(sizeof(NmspItem));

  new_item->ht_item = hi;
  new_item->next = floor_data->first_item;
  floor_data->first_item = new_item;
  return new_item;
}

NmspItem *
Compiler::Namespace_Get_Item(NmspFloor floor, const char *item_name)
{
  size_t item_name_len;
  assert(item_name != NULL);
  item_name_len = strlen(item_name) + 1;
  BoxHTItem *ht_item;
  if (BoxHT_Find(& ns.ht, (char *) item_name, item_name_len, & ht_item))
    return (NmspItem *) ht_item->object;
  return NULL;
}

Value *
Compiler::Namespace_Add_Value(NmspFloor floor, const char *item_name, Value *v)
{
  NmspItem *new_item = Namespace_Add_Item(floor, item_name);
  Value *new_value = Untrack_Value(Create_Value());
  assert(new_item && new_value);

  Move_Value(new_value, v);
  new_item->type = NMSPITEMTYPE_VALUE;
  new_item->data = new_value;
  return Setup_Value_As_Weak_Copy(Create_Value(), new_value);
}

Value *
Compiler::Namespace_Get_Value(NmspFloor floor, const char *item_name)
{
  NmspItem *new_item = Namespace_Get_Item(floor, item_name);
  Value *v_copy, *v;
  if (!new_item)
    return NULL;
  assert(new_item->type == NMSPITEMTYPE_VALUE);
  v = (Value *) new_item->data;
  v_copy = Create_Value();
  Setup_Value_As_Weak_Copy(v_copy, v);
  return v_copy;
}

void
Compiler::Namespace_Add_Procedure(NmspFloor floor,
                                  BoxType *parent, BoxType *comb_node)
{
  NmspItem *new_item = Namespace_Add_Item(floor, NULL);
  MyProcedureNmspItem *p =
    (MyProcedureNmspItem *) Box_Mem_Safe_Alloc(sizeof(MyProcedureNmspItem));
  assert(new_item);
  new_item->type = NMSPITEMTYPE_PROCEDURE;
  new_item->data = p;
  p->parent = parent;
  p->comb_node = comb_node;
}

void
Compiler::Namespace_Add_Callback(NmspFloor floor,
                                 NmspCallback callback, void *data)
{
  NmspItem *new_item = Namespace_Add_Item(floor, (char *) NULL);
  assert(new_item != NULL);
  new_item->type = NMSPITEMTYPE_CALLBACK;
  new_item->data = (void *) callback;
}

}
