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
BOXEXPORT BoxException *
Box_Runtime_Init_At_Array(BoxPtr *parent, BoxPtr *child) {
  BoxArray *a = BoxPtr_Get_Target(parent);
  BoxArray_Init(a);
  return NULL;
}

/* Implementation of (].)@Array. */
BOXEXPORT BoxException *
Box_Runtime_Finish_At_Array(BoxPtr *parent, BoxPtr *child) {
  BoxArray *a = BoxPtr_Get_Target(parent);
  BoxArray_Finish(a);
  return NULL;
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

/* Implementation of Any@Get. */
static BoxTask My_Any_At_Get(BoxVMX *vm) {
  BoxAny *get = BoxVMX_Get_Parent_Target(vm);
  BoxAny *arg = BoxVMX_Get_Child_Target(vm);
  BoxPtr *get_obj = BoxAny_Get_Obj(get);
  BoxPtr *arg_obj = BoxAny_Get_Obj(arg);

  if (!get_obj || BoxPtr_Is_Null(get_obj)) {
    BoxAny_Finish(get);
    BoxAny_Copy(get, arg);
    return BOXTASK_OK;
  } else {
    BoxType *get_type = BoxAny_Get_Type(get),
            *idx_type = BoxAny_Get_Type(arg);
    BoxType *required_type;

    if (!(get_type && idx_type)) {
      BoxVM_Set_Fail_Msg(vm->vm, "Invalid argument to Any (bad type)");
      return BOXTASK_FAILURE;
    }

    required_type = Box_Get_Core_Type(BOXTYPEID_ARRAY);
    if (BoxType_Compare(required_type, get_type) < BOXTYPECMP_EQUAL) {
      BoxVM_Set_Fail_Msg(vm->vm, "Container type does not support Get");
      return BOXTASK_FAILURE;
    }

    required_type = Box_Get_Core_Type(BOXTYPEID_INT);
    if (BoxType_Compare(required_type, idx_type) < BOXTYPECMP_EQUAL) {
      BoxVM_Set_Fail_Msg(vm->vm, "Index must be an integer");
      return BOXTASK_FAILURE;
    }

    if (arg_obj) {
      BoxInt *idx = (BoxInt *) BoxPtr_Get_Target(arg_obj);
      BoxArray *a = (BoxArray *) BoxPtr_Get_Target(get_obj);
      if (idx) {
        BoxAny dst_save = *get;
        BoxAny *item = BoxArr_Get_Item_Ptr(& a->arr, (size_t) *idx + 1);
        BoxAny_Copy(get, item);
        BoxAny_Finish(& dst_save);
        return BOXTASK_OK;
      }
    }

    BoxVM_Set_Fail_Msg(vm->vm, "Empty Any object given as index");
    return BOXTASK_FAILURE;
  }
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
          *get = Box_Get_Core_Type(BOXTYPEID_GET),
          *num = Box_Get_Core_Type(BOXTYPEID_NUM);

  BoxCombDef defs[] =
    {BOXCOMBDEF_I_AT_T(BOXTYPEID_INIT, arr, Box_Runtime_Init_At_Array),
     BOXCOMBDEF_I_AT_T(BOXTYPEID_FINISH, arr, Box_Runtime_Finish_At_Array)};

  size_t num_defs = sizeof(defs)/sizeof(BoxCombDef);
  (void) BoxCombDef_Define(defs, num_defs);

  Bltin_Comb_Def(arr, BOXCOMBTYPE_COPY, arr, My_Array_Copy_Array);
  Bltin_Proc_Def_With_Id(arr, BOXTYPEID_ANY, My_Any_At_Array);

  Bltin_Proc_Def_With_Id(get, BOXTYPEID_ANY, My_Any_At_Get);

  Bltin_Proc_Def_With_Id(num, BOXTYPEID_ARRAY, My_Array_At_Num);
}
