/****************************************************************************
 * Copyright (C) 2013 by Matteo Franchin                                    *
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

/**
 * @file bltinarray.c
 * @brief Implementation of heterogeneous arrays (Box's Array object).
 */

#include "bltinarray_priv.h"

/**
 * @brief Finalizer for the array items, used in BoxArray_Init().
 */
static void My_Array_Item_Finalizer(void *item) {
  BoxAny_Finish((BoxAny *) item);
}

/**
 * @brief Initialize a #BoxArray object.
 */
static void BoxArray_Init(BoxArray *a) {
  BoxArr_Init(& a->arr, sizeof(BoxAny), 2);
  BoxArr_Set_Finalizer(& a->arr, My_Array_Item_Finalizer);
}

/**
 * @brief Finalize a #BoxArray object.
 */
static void BoxArray_Finish(BoxArray *a) {
  BoxArr_Finish(& a->arr);  
}

/* Implementation of (.[)@Array. */
static BoxTask My_Init_At_Array(BoxVMX *vm) {
  BoxArray *a = BoxVMX_Get_Parent_Target(vm);
  BoxArray_Init(a);
  return BOXTASK_OK;
}

/* Implementation of (].)@Array. */
static BoxTask My_Finish_At_Array(BoxVMX *vm) {
  BoxArray *a = BoxVMX_Get_Parent_Target(vm);
  BoxArray_Finish(a);
  return BOXTASK_OK;
}

/* Implementation of Array(=)Array. */
static BoxTask My_Array_Copy_Array(BoxVMX *vm) {
  BoxArray *dst = BoxVMX_Get_Parent_Target(vm),
           *src = BoxVMX_Get_Child_Target(vm);
  size_t num_src_items = BoxArr_Get_Num_Items(& src->arr);
  BoxAny *dst_items, *src_items;

  if (num_src_items < 1) {
    BoxArray_Init(dst);
    return BOXTASK_OK;
  }

  src_items = (BoxAny *) BoxArr_Get_First_Item(& src->arr);
  if (!src_items)
    return BOXTASK_FAILURE;

  BoxArray_Init(dst);

  dst_items = (BoxAny *) BoxArr_MPush(& dst->arr, NULL, num_src_items);
  if (dst_items) {
    size_t i;
    for (i = 0; i < num_src_items; i++)
      BoxAny_Copy(& dst_items[i], & src_items[i]);
    return BOXTASK_OK;
  }

  BoxArray_Finish(dst);
  return BOXTASK_FAILURE;
}

/* Implementation of Any@Array. */
static BoxTask My_Any_At_Array(BoxVMX *vm) {
  BoxArray *a = BoxVMX_Get_Parent_Target(vm);
  BoxAny *any = BoxVMX_Get_Child_Target(vm);
  BoxAny *new_item = BoxArr_Push(& a->arr, NULL);
  if (new_item) {
    BoxAny_Copy(new_item, any);
    return BOXTASK_OK;
  }
  return BOXTASK_FAILURE;
}

/* Implementation of Array@Num. */
static BoxTask My_Array_At_Num(BoxVMX *vm) {
  BoxInt *num = BoxVMX_Get_Parent_Target(vm);
  BoxArray *a = BoxVMX_Get_Child_Target(vm);
  *num += BoxArr_Get_Num_Items(& a->arr);
  return BOXTASK_OK;
}

void BoxArray_Register_Combs(void) {
  BoxType *arr = Box_Get_Core_Type(BOXTYPEID_ARRAY),
          *num = Box_Get_Core_Type(BOXTYPEID_NUM);

  Bltin_Proc_Def_With_Id(arr, BOXTYPEID_INIT, My_Init_At_Array);
  Bltin_Proc_Def_With_Id(arr, BOXTYPEID_FINISH, My_Finish_At_Array);
  Bltin_Comb_Def(arr, BOXCOMBTYPE_COPY, arr, My_Array_Copy_Array);
  Bltin_Proc_Def_With_Id(arr, BOXTYPEID_ANY, My_Any_At_Array);

  Bltin_Proc_Def_With_Id(num, BOXTYPEID_ARRAY, My_Array_At_Num);
}
