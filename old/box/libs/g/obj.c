/****************************************************************************
 * Copyright (C) 2010, 2011 by Matteo Franchin                              *
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

#include <box/types.h>
#include <box/mem.h>
#include <box/print.h>
#include <box/array.h>
#include <box/vm_priv.h>

#include "obj.h"

size_t BoxGObjKind_Size(BoxGObjKind kind) {
  switch (kind) {
  case BOXGOBJKIND_EMPTY: return 0;
  case BOXGOBJKIND_VOID: return 0;
  case BOXGOBJKIND_CHAR: return sizeof(BoxChar);
  case BOXGOBJKIND_INT: return sizeof(BoxInt);
  case BOXGOBJKIND_REAL: return sizeof(BoxReal);
  case BOXGOBJKIND_POINT: return sizeof(BoxPoint);
  case BOXGOBJKIND_STR: return sizeof(BoxStr);
  case BOXGOBJKIND_COMPOSITE: return sizeof(BoxArr);
  default:
    assert(0);
    return 0;
  }
}

const char *BoxGObjKind_Name(BoxGObjKind kind) {
  switch (kind) {
  case BOXGOBJKIND_EMPTY: return "Empty";
  case BOXGOBJKIND_VOID: return "Void";
  case BOXGOBJKIND_CHAR: return "Char";
  case BOXGOBJKIND_INT: return "Int";
  case BOXGOBJKIND_REAL: return "Real";
  case BOXGOBJKIND_POINT: return "Point";
  case BOXGOBJKIND_STR: return "Str";
  case BOXGOBJKIND_COMPOSITE: return "Obj";
  default:
    assert(0);
    return "Unknown";
  }
}

static BoxTask My_Copy(void *dest, const void *src,
                       BoxGObjKind kind, int init) {
  switch (kind) {
  case BOXGOBJKIND_STR:
    if (init)
      return BoxStr_Init_From((BoxStr *) dest, (BoxStr *) src);
    else
      return BoxStr_Concat((BoxStr *) dest, (BoxStr *) src);

  default:
    (void) memcpy(dest, src, BoxGObjKind_Size(kind));
    return BOXTASK_OK;
  }
}

void BoxGObj_Init(BoxGObj *gobj) {
  gobj->kind = BOXGOBJKIND_EMPTY;
}

void BoxGObj_Finish(BoxGObj *gobj) {
  switch (gobj->kind) {
  case BOXGOBJKIND_STR:
    BoxStr_Finish(& gobj->value.v_str);
    return;

  case BOXGOBJKIND_COMPOSITE:
    BoxArr_Finish(& gobj->value.v_composite);
    return;

  default:
    return;
  }
}

BoxGObj *BoxGObj_New(void) {
  BoxGObj *gobj = Box_Mem_Safe_Alloc(sizeof(BoxGObj));
  BoxGObj_Init(gobj);
  return gobj;
}

void BoxGObj_Destroy(BoxGObj *gobj) {
  BoxGObj_Finish(gobj);
  Box_Mem_Free(gobj);
}

static void My_GObj_Array_Finalizer(void *item) {
  BoxGObj_Finish((BoxGObj *) item);
}

void BoxGObj_Transform_To_Composite(BoxGObj *gobj) {
  if (gobj->kind != BOXGOBJKIND_COMPOSITE) {
    BoxGObj original_gobj = *gobj;
    BoxArr *a = & gobj->value.v_composite;
    gobj->kind = BOXGOBJKIND_COMPOSITE;
    BoxArr_Init(a, sizeof(BoxGObj), 2);
    BoxArr_Set_Finalizer(a, My_GObj_Array_Finalizer);
    if (original_gobj.kind != BOXGOBJKIND_EMPTY)
      (void) BoxArr_Push(a, & original_gobj);
  }
}

/** Expand the given BoxGObj adding another element to the list.
 * Return the pointer to the new (uninitalized) member of the Obj.
 */
static BoxGObj *BoxGObj_Expand(BoxGObj *gobj, int merge) {
  BoxGObj *new_memb;

  if (gobj->kind == BOXGOBJKIND_EMPTY) {
    if (merge)
      return gobj;
  }

  if (gobj->kind != BOXGOBJKIND_COMPOSITE)
    BoxGObj_Transform_To_Composite(gobj);

  assert(gobj->kind == BOXGOBJKIND_COMPOSITE);
  new_memb = (BoxGObj *) BoxArr_Push(& gobj->value.v_composite, NULL);
  BoxGObj_Init(new_memb);
  return new_memb;
}

void BoxGObj_Append_C_Value(BoxGObj *gobj, BoxGObjKind kind, void *content) {
  size_t sz = BoxGObjKind_Size(kind);
  BoxGObj *gobj_dest = BoxGObj_Expand(gobj, 1);

  assert(kind != BOXGOBJKIND_COMPOSITE);

  gobj_dest->kind = kind;
  if (content != NULL && sz > 0)
    My_Copy(& gobj_dest->value, content, kind, /*init*/1);
}

BoxGObj *BoxGObj_Append_Composite(BoxGObj *gobj, size_t num_items) {
  BoxGObj *sub_gobj = BoxGObj_Expand(gobj, 0);
  BoxArr *a_sub_gobj = & sub_gobj->value.v_composite;
  sub_gobj->kind = BOXGOBJKIND_COMPOSITE;
  assert(num_items > 0);
  BoxArr_Init(a_sub_gobj, sizeof(BoxGObj), num_items);
  return sub_gobj;
}

static void BoxGObj_Init_From_Ptr(BoxGObj *gobj_dest,
                                  BoxGObjKind kind, const void *data) {
  gobj_dest->kind = kind;
  if (kind == BOXGOBJKIND_COMPOSITE) {
    BoxArr *a_src = (BoxArr *) data,
           *a_dest = & gobj_dest->value.v_composite;
    size_t num_items = BoxArr_Num_Items(a_src), i;

    BoxArr_Init(a_dest, sizeof(BoxGObj), num_items);
    (void) BoxArr_MPush(a_dest, NULL, num_items);
    for (i = 1; i <= num_items; i++)
      BoxGObj_Init_From((BoxGObj *) BoxArr_Item_Ptr(a_dest, i),
                        (BoxGObj *) BoxArr_Item_Ptr(a_src, i));
    BoxArr_Set_Finalizer(a_dest, My_GObj_Array_Finalizer);

  } else if (kind != BOXGOBJKIND_EMPTY) {
    if (BoxGObjKind_Size(kind) > 0)
      My_Copy(& gobj_dest->value, data, kind, /*init*/1);
  }
}

/** Create a copy of gobj_src in gobj_dest. gobj_dest should not contain
 * a proper BoxGObj object. In other words it should be either uninitalized
 * or be a BOXGOBJKIND_EMPTY object (just BoxGObj_Init has been called on it).
 */
void BoxGObj_Init_From(BoxGObj *gobj_dest, const BoxGObj *gobj_src) {
  return BoxGObj_Init_From_Ptr(gobj_dest, gobj_src->kind, & gobj_src->value);
}

void BoxGObj_Filter(BoxGObj *gobj_dest, BoxGObj *gobj_src,
                    BoxGObjFilter filter, void *pass) {
  BoxGObjKind kind = gobj_src->kind;
  if (kind == BOXGOBJKIND_COMPOSITE) {
    BoxArr *a_src = & gobj_src->value.v_composite,
           *a_dest = & gobj_dest->value.v_composite;
    size_t num_items = BoxArr_Num_Items(a_src), i;

    gobj_dest->kind = kind;
    BoxArr_Init(a_dest, sizeof(BoxGObj), num_items);
    (void) BoxArr_MPush(a_dest, NULL, num_items);
    for (i = 1; i <= num_items; i++)
      BoxGObj_Filter((BoxGObj *) BoxArr_Item_Ptr(a_dest, i),
                     (BoxGObj *) BoxArr_Item_Ptr(a_src, i),
                     filter, pass);
    BoxArr_Set_Finalizer(a_dest, My_GObj_Array_Finalizer);

  } else {
    assert(filter != NULL);
    filter(gobj_dest, gobj_src, pass);
  }
}

void BoxGObj_Merge_Filtered(BoxGObj *gobj_dest, BoxGObj *gobj_src,
                            BoxGObjFilter filter, void *pass) {
  BoxGObjKind kind = gobj_src->kind;
  if (kind == BOXGOBJKIND_COMPOSITE) {
    BoxArr *a_src = & gobj_src->value.v_composite;
    size_t num_items = BoxArr_Num_Items(a_src), i;

    for (i = 1; i <= num_items; i++) {
      BoxGObj *gobj_src_item = (BoxGObj *) BoxArr_Item_Ptr(a_src, i);
      BoxGObj *gobj_dest_item = BoxGObj_Expand(gobj_dest, 0);
      filter(gobj_dest_item, gobj_src_item, pass);
    }

  } else {
    BoxGObj *gobj_dest_item = BoxGObj_Expand(gobj_dest, 1);
    filter(gobj_dest_item, gobj_src, pass);
  }
}

static void My_BoxGObj_Merge_Filter(BoxGObj *gobj_dest,
                                    const BoxGObj *gobj_src, void *pass) {
  BoxGObj_Init_From(gobj_dest, gobj_src);
}

void BoxGObj_Merge(BoxGObj *gobj_dest, BoxGObj *gobj_src) {
  BoxGObj_Merge_Filtered(gobj_dest, gobj_src, My_BoxGObj_Merge_Filter, NULL);
}

void BoxGObj_Append_Obj(BoxGObj *gobj_dest, BoxGObj *gobj_src) {
  BoxGObj *gobj_dest_item = BoxGObj_Expand(gobj_dest, 0);
  BoxGObjKind kind = gobj_src->kind;
  if (kind != BOXGOBJKIND_EMPTY && kind != BOXGOBJKIND_COMPOSITE)
    gobj_dest_item = BoxGObj_Expand(gobj_dest_item, 0);
  BoxGObj_Init_From(gobj_dest_item, gobj_src);
}

void BoxGObj_Append_Str(BoxGObj *gobj, const BoxStr *s) {
  BoxGObj_Append_C_Value(gobj, BOXGOBJKIND_STR, (void *) s);
}

BoxGObj *BoxGObj_Get(BoxGObj *gobj, BoxInt idx) {
  if (gobj->kind == BOXGOBJKIND_COMPOSITE) {
    BoxArr *a = & gobj->value.v_composite;
    return (idx >= 0 && idx < BoxArr_Num_Items(a)) ?
           BoxArr_Item_Ptr(a, idx + 1) : (BoxGObj *) NULL;

  } else if (gobj->kind == BOXGOBJKIND_EMPTY) {
    return (BoxGObj *) NULL;

  } else
    return (idx == 0) ? gobj : (BoxGObj *) NULL;
}

size_t BoxGObj_Get_Length(BoxGObj *gobj) {
  switch (gobj->kind) {
  case BOXGOBJKIND_EMPTY:
    return 0;
  case BOXGOBJKIND_COMPOSITE:
    return BoxArr_Num_Items(& gobj->value.v_composite);
  default:
    return 1;
  }
}

BoxInt BoxGObj_Get_Type(BoxGObj *gobj, BoxInt idx) {
  if (gobj->kind == BOXGOBJKIND_COMPOSITE) {
    BoxArr *a = (BoxArr *) & gobj->value;
    return (idx >= 0 && idx < BoxArr_Num_Items(a)) ?
           ((BoxGObj *) BoxArr_Item_Ptr(a, idx + 1))->kind : -1;

  } else
    return (idx == 0) ? gobj->kind : -1;
}

void *BoxGObj_To(BoxGObj *gobj, BoxGObjKind kind) {
  if (gobj != NULL)
    return (gobj->kind == kind) ? & gobj->value : NULL;
  return NULL;
}

BoxTask BoxGObj_Extract_Array(BoxGObj *gobj, BoxGObjKind kind,
                              size_t start_idx, size_t num,
                              void **out) {
  size_t i;
  int fail = 0;

  for (i = 0; i < num; i++) {
    size_t idx = start_idx + i;
    BoxGObj *sub_gobj = BoxGObj_Get(gobj, idx);
    if (sub_gobj != NULL || BoxGObj_Get_Type(gobj, idx) == kind)
      out[i] = & sub_gobj->value;
    
    else {
      out[i] = NULL;
      fail = 1;
    }
  }

  return (fail) ? BOXTASK_FAILURE : BOXTASK_OK;
}

BoxTask BoxGObj_Iter(BoxGObj *gobj, size_t start_idx, size_t *num_args,
                     BoxGObjIterator iter, void *pass) {
  size_t max_idx = BoxGObj_Get_Num(gobj),
         dummy,
         *out_num_args = (num_args != NULL) ? num_args : & dummy;

  if (start_idx < max_idx) {
    size_t max_n = max_idx - start_idx,
           n = (   num_args != NULL
                && *num_args > 0
                && *num_args < max_n) ? *num_args : max_n;

    if (gobj->kind == BOXGOBJKIND_COMPOSITE) {
      BoxGObj *sub_obj =
        BoxArr_Item_Ptr(& gobj->value.v_composite, start_idx + 1);
      size_t i;

      for (i = 0; i < n; i++) {
        BoxGObjKind sk = sub_obj->kind;
        BoxTask t = iter(i, sk, sub_obj++, pass);
        if (t != BOXTASK_OK) {
          *out_num_args = i;
          return t;
        }
      }

      *out_num_args = n;
      return BOXTASK_OK;

    } else {
      BoxTask t;
      assert(gobj->kind != BOXGOBJKIND_EMPTY && start_idx == 0);
      t = iter(0, gobj->kind, gobj, pass);
      *out_num_args = (t == BOXTASK_OK) ? 1 : 0;
      return t;
    }

  } else {
    *out_num_args = 0;
    return BOXTASK_OK;
  }
}

/****************************************************************************
 * WRAPPING FUNCTIONS FOR BOX                                               *
 ****************************************************************************/

BoxTask GLib_Init_At_Obj(BoxVMX *vm) {
  BoxGObjPtr *gobjptr = BOX_VM_THIS_PTR(vm, BoxGObjPtr);
  *gobjptr = BoxGObj_New();
  return BOXTASK_OK;
}

BoxTask GLib_Finish_At_Obj(BoxVMX *vm) {
  BoxGObjPtr *gobjptr = BOX_VM_THIS_PTR(vm, BoxGObjPtr);
  BoxGObj_Destroy(*gobjptr);
  *gobjptr = (BoxGObj *) NULL;
  return BOXTASK_OK;
}

BoxTask GLib_Obj_Copy_Obj(BoxVMX *vm) {
  BoxGObjPtr *dest = BOX_VM_THIS_PTR(vm, BoxGObjPtr),
             gobj_src = BOX_VM_ARG(vm, BoxGObjPtr);
  *dest = BoxGObj_New();
  BoxGObj_Init_From(*dest, gobj_src);
  return BOXTASK_OK;
}

BoxTask GLib_X_At_Obj(BoxVMX *vm, BoxGObjKind kind) {
  BoxGObjPtr gobj = BOX_VM_THIS(vm, BoxGObjPtr);
  BoxGObj_Append_C_Value(gobj, kind, BOX_VM_ARG_PTR(vm, void));
  return BOXTASK_OK;
}

BoxTask GLib_Char_At_Obj(BoxVMX *vm) {
  return GLib_X_At_Obj(vm, BOXGOBJKIND_CHAR);
}

BoxTask GLib_Int_At_Obj(BoxVMX *vm) {
  return GLib_X_At_Obj(vm, BOXGOBJKIND_INT);
}

BoxTask GLib_Real_At_Obj(BoxVMX *vm) {
  return GLib_X_At_Obj(vm, BOXGOBJKIND_REAL);
}

BoxTask GLib_Point_At_Obj(BoxVMX *vm) {
  return GLib_X_At_Obj(vm, BOXGOBJKIND_POINT);
}

BoxTask GLib_Str_At_Obj(BoxVMX *vm) {
  BoxGObj_Append_Str(BOX_VM_THIS(vm, BoxGObjPtr), BOX_VM_ARG_PTR(vm, BoxStr));
  return BOXTASK_OK;
}

BoxTask GLib_Obj_At_Obj(BoxVMX *vm) {
  BoxGObj *gobj_src = BOX_VM_ARG(vm, BoxGObjPtr),
          *gobj_dest = BOX_VM_THIS(vm, BoxGObjPtr);
  BoxGObj_Append_Obj(gobj_dest, gobj_src);
  return BOXTASK_OK;
}

static BoxTask GLib_Obj_At_X(BoxVMX *vm, BoxGObjKind kind) {
  void *obj_data = BOX_VM_THIS_PTR(vm, void);
  BoxGObjPtr gobj = BOX_VM_ARG(vm, BoxGObjPtr);
  if (gobj->kind == kind) {
    My_Copy(obj_data, & gobj->value, kind, /*init*/0);
    return BOXTASK_OK;

  } else {
    char *msg = Box_SPrintF("Cannot convert Obj to %s. Obj has type %s.",
                            BoxGObjKind_Name(kind),
                            BoxGObjKind_Name(gobj->kind));
    BoxVM_Set_Fail_Msg(vm->vm, msg);
    Box_Mem_Free(msg);
    return BOXTASK_FAILURE;
  }
}

BoxTask GLib_Obj_At_Char(BoxVMX *vm) {
  return GLib_Obj_At_X(vm, BOXGOBJKIND_CHAR);
}

BoxTask GLib_Obj_At_Int(BoxVMX *vm) {
  return GLib_Obj_At_X(vm, BOXGOBJKIND_INT);
}

BoxTask GLib_Obj_At_Real(BoxVMX *vm) {
  return GLib_Obj_At_X(vm, BOXGOBJKIND_REAL);
}

BoxTask GLib_Obj_At_Point(BoxVMX *vm) {
  return GLib_Obj_At_X(vm, BOXGOBJKIND_POINT);
}

BoxTask GLib_Obj_At_Str(BoxVMX *vm) {
  return GLib_Obj_At_X(vm, BOXGOBJKIND_STR);
}

BoxTask GLib_Int_At_Obj_Get(BoxVMX *vm) {
  BoxSubtype *obj_get = BOX_VM_THIS_PTR(vm, BoxSubtype);
  BoxGObjPtr
    gobj_parent = *((BoxGObjPtr *) BoxSubtype_Get_Parent_Target(obj_get)),
    gobj_child = *((BoxGObjPtr *) BoxSubtype_Get_Child_Target(obj_get));

  BoxInt idx = BOX_VM_ARG(vm, BoxInt);
  BoxGObj *gobj_indexed = BoxGObj_Get(gobj_parent, idx);
  if (gobj_indexed != NULL) {
    BoxGObj_Init_From(gobj_child, gobj_indexed);
    return BOXTASK_OK;

  } else {
    char *msg =
      Box_SPrintF("Obj does not have a sub-object at index %d.", idx);
    BoxVM_Set_Fail_Msg(vm->vm, msg);
    Box_Mem_Free(msg);
    return BOXTASK_FAILURE;
  }
}

BoxTask GLib_Obj_At_Length(BoxVMX *vm) {
  BoxInt *length = BOX_VM_THIS_PTR(vm, BoxInt);
  BoxGObjPtr gobj = BOX_VM_ARG(vm, BoxGObjPtr);
  *length += (BoxInt) BoxGObj_Get_Length(gobj);
  return BOXTASK_OK;
}

BoxTask GLib_Int_At_Obj_GetType(BoxVMX *vm) {
  BoxSubtype *obj_get = BOX_VM_THIS_PTR(vm, BoxSubtype);
  BoxGObjPtr
    gobj_parent = *((BoxGObjPtr *) BoxSubtype_Get_Parent_Target(obj_get));
  BoxInt *gobj_type = (BoxInt *) BoxSubtype_Get_Child_Target(obj_get);
  BoxInt idx = BOX_VM_ARG(vm, BoxInt);
  BoxInt t = BoxGObj_Get_Type(gobj_parent, idx);
  if (t >= 0) {
    *gobj_type = t;
    return BOXTASK_OK;

  } else {
    BoxVM_Set_Fail_Msg(vm->vm, "Cannot get item type. Index out of bounds.");
    return BOXTASK_FAILURE;
  }
}

BoxTask GLib_StrStr_Compare(BoxVMX *vm) {
  BoxInt *result = BOX_VM_THIS_PTR(vm, BoxInt);
  BoxStr *s = BOX_VM_ARG_PTR(vm, BoxStr);
  *result = strcmp((s[0].length > 0) ? s[0].ptr : "",
                   (s[1].length > 0) ? s[1].ptr : "");
  return BOXTASK_OK;
}

BoxTask GLib_Obj_At_MergeObjs(BoxVMX *vm) {
  BoxSubtype *obj_merge = BOX_VM_THIS_PTR(vm, BoxSubtype);
  BoxGObjPtr
    gobj_dest = *((BoxGObjPtr *) BoxSubtype_Get_Parent_Target(obj_merge));
  BoxGObjPtr gobj_src = BOX_VM_ARG(vm, BoxGObjPtr);
  BoxGObj_Merge(gobj_dest, gobj_src);
  return BOXTASK_OK;
}


