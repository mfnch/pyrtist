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
BOXEXPORT BoxException *
Box_Runtime_Array_To_Array(BoxPtr *parent, BoxPtr *child) {
  BoxArray *dst = BoxPtr_Get_Target(parent),
           *src = BoxPtr_Get_Target(child);
  size_t num_src_items = BoxArr_Get_Num_Items(& src->arr);
  BoxAny *dst_items, *src_items;

  if (num_src_items < 1) {
    BoxArray_Init(dst);
    return NULL;
  }

  src_items = (BoxAny *) BoxArr_Get_First_Item(& src->arr);
  if (!src_items)
    return BoxException_Create("Failure copying array (1)");

  BoxArray_Init(dst);

  dst_items = (BoxAny *) BoxArr_MPush(& dst->arr, NULL, num_src_items);
  if (dst_items) {
    size_t i;
    for (i = 0; i < num_src_items; i++)
      BoxAny_Copy(& dst_items[i], & src_items[i]);
    return NULL;
  }

  BoxArray_Finish(dst);
  return BoxException_Create("Failure copying array (2)");
}

/* Implementation of Any@Array. */
BOXEXPORT BoxException *
Box_Runtime_Any_At_Array(BoxPtr *parent, BoxPtr *child) {
  BoxArray *a = BoxPtr_Get_Target(parent);
  BoxAny *any = BoxPtr_Get_Target(child);
  BoxAny *new_item = BoxArr_Push(& a->arr, NULL);
  if (new_item) {
    BoxAny_Copy(new_item, any);
    return NULL;
  }
  return BoxException_Create("Cannot append item to Array");
}

/* Implementation of Any@Get. */
BOXEXPORT BoxException *
Box_Runtime_Any_At_Get(BoxPtr *parent, BoxPtr *child) {
  BoxAny *get = BoxPtr_Get_Target(parent);
  BoxAny *arg = BoxPtr_Get_Target(child);
  BoxPtr *get_obj = BoxAny_Get_Obj(get);
  BoxPtr *arg_obj = BoxAny_Get_Obj(arg);

  if (!get_obj || BoxPtr_Is_Null(get_obj)) {
    BoxAny_Finish(get);
    BoxAny_Copy(get, arg);
    return NULL;
  } else {
    BoxType *get_type = BoxAny_Get_Type(get),
            *idx_type = BoxAny_Get_Type(arg);
    BoxType *required_type;

    if (!(get_type && idx_type))
      return BoxException_Create("Invalid argument to Any (bad type)");

    required_type = Box_Get_Core_Type(BOXTYPEID_ARRAY);
    if (BoxType_Compare(required_type, get_type) < BOXTYPECMP_EQUAL)
      return BoxException_Create("Container type does not support Get");

    required_type = Box_Get_Core_Type(BOXTYPEID_INT);
    if (BoxType_Compare(required_type, idx_type) < BOXTYPECMP_EQUAL)
      return BoxException_Create("Index must be an integer");

    if (arg_obj) {
      BoxInt *idx = (BoxInt *) BoxPtr_Get_Target(arg_obj);
      BoxArray *a = (BoxArray *) BoxPtr_Get_Target(get_obj);
      if (idx) {
        BoxAny dst_save = *get;
        BoxAny *item = BoxArr_Get_Item_Ptr(& a->arr, (size_t) *idx + 1);
        BoxAny_Copy(get, item);
        BoxAny_Finish(& dst_save);
        return NULL;
      }
    }

    return BoxException_Create("Empty Any object given as index");
  }
}

/* Implementation of Array@Num. */
BOXEXPORT BoxException *
Box_Runtime_Array_At_Num(BoxPtr *parent, BoxPtr *child) {
  BoxInt *num = BoxPtr_Get_Target(parent);
  BoxArray *a = BoxPtr_Get_Target(child);
  *num += BoxArr_Get_Num_Items(& a->arr);
  return NULL;
}


#if 0
BoxBool
Box_Runtime_Register_Array(BoxCoreTypes *runtime,
                           BoxType *t_ARRAY, BoxType *t_Array) {
  const BoxTypeId id = BOXTYPEID_ARRAY;
  const char *name = "ARRAY";
  BoxType *t, *t1, *t2;

  id = BOXTYPEID_ARRAY;
  t = BoxType_Create_Primary(id, sizeof(BoxArray), __alignof__(BoxArray));
  t1 = BoxType_Create_Ident(t, "ARRAY");
  Box_Runtime_Install_Type(runtime, id, t1);

  id = BOXTYPEID_Array;
  t2 = BoxType_Create_Ident(t1, "Array");
  Box_Runtime_Install_Type(runtime, id, t2);

  if (t_ARRAY)
    *t_ARRAY = t1;
  if (t_Array)
    *t_Array = t2;

  return t1 && t2;
}

BoxBool
BoxCoreTypes_Register_Array(BoxCoreTypes *ct) {
  const BoxTypeId id = BOXTYPEID_ARRAY;
  const char *name = "ARRAY";
  BoxType *t, *t_arr;

  BoxCombDef defs[] =
    {BOXCOMBDEF_I_AT_T(BOXTYPEID_INIT, t_arr, Box_Runtime_Init_At_Array),
     BOXCOMBDEF_I_AT_T(BOXTYPEID_FINISH, t_arr, Box_Runtime_Finish_At_Array),
     BOXCOMBDEF_I_AT_T(BOXTYPEID_ANY, t_arr, Box_Runtime_Any_At_Array),
     BOXCOMBDEF_T_AT_I(t_arr, BOXTYPEID_NUM, Box_Runtime_Array_At_Num),
     BOXCOMBDEF_T_TO_T(t_arr, t_arr, Box_Runtime_Array_To_Array)};
  size_t num_defs = sizeof(defs)/sizeof(BoxCombDef);

  t = BoxType_Create_Primary(id, sizeof(BoxArray), __alignof__(BoxArray));
  t_arr = BoxType_Create_Ident(t, name);
  BoxCoreType_Install_Type(ct, id, t_arr);

  if (!t_arr)
    return BOXBOOL_FALSE;

  return num_defs == BoxCombDef_Define(defs, num_defs);
}

#endif


void BoxArray_Register_Combs(void) {
  BoxType *t_arr = Box_Get_Core_Type(BOXTYPEID_ARRAY);

  BoxCombDef defs[] =
    {BOXCOMBDEF_I_AT_T(BOXTYPEID_INIT, t_arr, Box_Runtime_Init_At_Array),
     BOXCOMBDEF_I_AT_T(BOXTYPEID_FINISH, t_arr, Box_Runtime_Finish_At_Array),
     BOXCOMBDEF_I_AT_T(BOXTYPEID_ANY, t_arr, Box_Runtime_Any_At_Array),
     BOXCOMBDEF_T_AT_I(t_arr, BOXTYPEID_NUM, Box_Runtime_Array_At_Num),
     BOXCOMBDEF_I_AT_I(BOXTYPEID_ANY, BOXTYPEID_GET, Box_Runtime_Any_At_Get),
     BOXCOMBDEF_T_TO_T(t_arr, t_arr, Box_Runtime_Array_To_Array)};

  size_t num_defs = sizeof(defs)/sizeof(BoxCombDef);
  (void) BoxCombDef_Define(defs, num_defs);
}
