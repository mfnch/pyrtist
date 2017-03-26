/****************************************************************************
 * Copyright (C) 2014 by Matteo Franchin                                    *
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

#include <box/index_priv.h>
#include <box/mem.h>


static void
My_Free_Array_Item(void *item)
{
  Box_Mem_Free(*((char **) item));
}

/* Initialization for BoxIndex. */
void
BoxIndex_Init(BoxIndex *idx, uint32_t num_entries)
{
  BoxArr_Init(& idx->names_from_nums, sizeof(char *), num_entries);
  BoxArr_Set_Finalizer(& idx->names_from_nums, My_Free_Array_Item);
  BoxHT_Init_Default(& idx->nums_from_names, num_entries);
  BoxHT_Set_Attr(& idx->nums_from_names,
                 BOXHTATTR_COPY_KEYS | BOXHTATTR_COPY_OBJS,
                 BOXHTATTR_COPY_OBJS);
}

/* Finalization for BoxIndex. */
void
BoxIndex_Finish(BoxIndex *idx)
{
  BoxArr_Finish(& idx->names_from_nums);
  BoxHT_Finish(& idx->nums_from_names);
}

uint32_t
BoxIndex_Get_Idx_From_Name(BoxIndex *idx, const char *name)
{

  size_t name_length = strlen(name);
  BoxHTItem *hi;
  char *name_dup;

  if (BoxHT_Find(& idx->nums_from_names, name, name_length, & hi))
    return (uint32_t) (uintptr_t) hi->object;

  name_dup = Box_Mem_Strdup(name);

  if (!name_dup)
    return 0;

  if (BoxArr_Push(& idx->names_from_nums, & name_dup)) {
    uint32_t num = BoxArr_Get_Num_Items(& idx->names_from_nums);
    if (BoxHT_Insert_Obj(& idx->nums_from_names, name_dup, name_length,
                         (void *) (uintptr_t) num, 0))
      return num;
    (void) BoxArr_Pop(& idx->names_from_nums, NULL);
  }

  Box_Mem_Free(name_dup);
  return 0;
}

const char *
BoxIndex_Get_Name_From_Idx(BoxIndex *idx, uint32_t num)
{
  if (num > 0 && num <= BoxArr_Get_Num_Items(& idx->names_from_nums))
    return *((char **) BoxArr_Get_Item_Ptr(& idx->names_from_nums, num));
  return NULL;
}
