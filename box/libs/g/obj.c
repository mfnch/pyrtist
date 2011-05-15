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
#include <box/virtmach.h>

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
  BoxGObj *gobj = BoxMem_Safe_Alloc(sizeof(BoxGObj));
  BoxGObj_Init(gobj);
  return gobj;
}

void BoxGObj_Destroy(BoxGObj *gobj) {
  BoxGObj_Finish(gobj);
  BoxMem_Free(gobj);
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

/** Expanding the given BoxGObj adding another element to the list.
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

static void BoxGObj_Merge_X(BoxGObj *gobj, BoxGObjKind kind, void *content) {
  size_t sz = BoxGObjKind_Size(kind);
  BoxGObj *gobj_dest = BoxGObj_Expand(gobj, 1);

  assert(kind != BOXGOBJKIND_COMPOSITE);

  gobj_dest->kind = kind;
  if (content != NULL && sz > 0)
    My_Copy(& gobj_dest->value, content, kind, /*init*/1);
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

/** Add an object gobj_src to another object gobj_dest.
 * This function is equivalent to the Box source gobj_dest[gobj_src],
 * when both gobj_src and gobj_dest are Obj objects.
 */
static void BoxGObj_Add(BoxGObj *gobj_dest, BoxGObj *gobj_src) {
  BoxGObj *gobj_dest_item = BoxGObj_Expand(gobj_dest, 0);
  BoxGObjKind kind = gobj_src->kind;
  if (kind != BOXGOBJKIND_EMPTY && kind != BOXGOBJKIND_COMPOSITE)
    gobj_dest_item = BoxGObj_Expand(gobj_dest_item, 0);
  BoxGObj_Init_From(gobj_dest_item, gobj_src);
}

void BoxGObj_Add_Str(BoxGObj *gobj, const BoxStr *s) {
  BoxGObj_Merge_X(gobj, BOXGOBJKIND_STR, (void *) s);
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
  return (gobj->kind == kind) ? & gobj->value : NULL;
}

/****************************************************************************
 * WRAPPING FUNCTIONS FOR BOX                                               *
 ****************************************************************************/

BoxTask GLib_Init_At_Obj(BoxVM *vm) {
  BoxGObjPtr *gobjptr = BOX_VM_THIS_PTR(vm, BoxGObjPtr);
  *gobjptr = BoxGObj_New();
  return BOXTASK_OK;
}

BoxTask GLib_Finish_At_Obj(BoxVM *vm) {
  BoxGObjPtr *gobjptr = BOX_VM_THIS_PTR(vm, BoxGObjPtr);
  BoxGObj_Destroy(*gobjptr);
  *gobjptr = (BoxGObj *) NULL;
  return BOXTASK_OK;
}

BoxTask GLib_X_At_Obj(BoxVM *vm, BoxGObjKind kind) {
  BoxGObjPtr gobj = BOX_VM_THIS(vm, BoxGObjPtr);
  BoxGObj_Merge_X(gobj, kind, BOX_VM_ARG_PTR(vm, void));
  return BOXTASK_OK;
}

BoxTask GLib_Char_At_Obj(BoxVM *vm) {
  return GLib_X_At_Obj(vm, BOXGOBJKIND_CHAR);
}

BoxTask GLib_Int_At_Obj(BoxVM *vm) {
  return GLib_X_At_Obj(vm, BOXGOBJKIND_INT);
}

BoxTask GLib_Real_At_Obj(BoxVM *vm) {
  return GLib_X_At_Obj(vm, BOXGOBJKIND_REAL);
}

BoxTask GLib_Point_At_Obj(BoxVM *vm) {
  return GLib_X_At_Obj(vm, BOXGOBJKIND_POINT);
}

BoxTask GLib_Str_At_Obj(BoxVM *vm) {
  BoxGObj_Add_Str(BOX_VM_THIS(vm, BoxGObjPtr), BOX_VM_ARG_PTR(vm, BoxStr));
  return BOXTASK_OK;
}

BoxTask GLib_Obj_At_Obj(BoxVM *vm) {
  BoxGObj *gobj_src = BOX_VM_ARG(vm, BoxGObjPtr),
          *gobj_dest = BOX_VM_THIS(vm, BoxGObjPtr);
  BoxGObj_Add(gobj_dest, gobj_src);
  return BOXTASK_OK;
}

static BoxTask GLib_Obj_At_X(BoxVM *vm, BoxGObjKind kind) {
  void *obj_data = BOX_VM_THIS_PTR(vm, void);
  BoxGObjPtr gobj = BOX_VM_ARG(vm, BoxGObjPtr);
  if (gobj->kind == kind) {
    My_Copy(obj_data, & gobj->value, kind, /*init*/0);
    return BOXTASK_OK;

  } else {
    char *msg = Box_SPrintF("Cannot convert Obj to %s. Obj has type %s.",
                            BoxGObjKind_Name(kind),
                            BoxGObjKind_Name(gobj->kind));
    BoxVM_Set_Fail_Msg(vm, msg);
    BoxMem_Free(msg);
    return BOXTASK_FAILURE;
  }
}

BoxTask GLib_Obj_At_Char(BoxVM *vm) {
  return GLib_Obj_At_X(vm, BOXGOBJKIND_CHAR);
}

BoxTask GLib_Obj_At_Int(BoxVM *vm) {
  return GLib_Obj_At_X(vm, BOXGOBJKIND_INT);
}

BoxTask GLib_Obj_At_Real(BoxVM *vm) {
  return GLib_Obj_At_X(vm, BOXGOBJKIND_REAL);
}

BoxTask GLib_Obj_At_Point(BoxVM *vm) {
  return GLib_Obj_At_X(vm, BOXGOBJKIND_POINT);
}

BoxTask GLib_Obj_At_Str(BoxVM *vm) {
  return GLib_Obj_At_X(vm, BOXGOBJKIND_STR);
}

BoxTask GLib_Int_At_Obj_Get(BoxVM *vm) {
  BoxSubtype *obj_get = BOX_VM_THIS_PTR(vm, BoxSubtype);
  BoxGObjPtr gobj_parent = *((BoxGObjPtr *) BOXSUBTYPE_PARENT_PTR(obj_get));
  BoxGObjPtr gobj_child = *((BoxGObjPtr *) BOXSUBTYPE_CHILD_PTR(obj_get));
  BoxInt idx = BOX_VM_ARG(vm, BoxInt);
  BoxGObj *gobj_indexed = BoxGObj_Get(gobj_parent, idx);
  if (gobj_indexed != NULL) {
    BoxGObj_Init_From(gobj_child, gobj_indexed);
    return BOXTASK_OK;

  } else {
    char *msg =
      Box_SPrintF("Obj does not have a sub-object at index %d.", idx);
    BoxVM_Set_Fail_Msg(vm, msg);
    BoxMem_Free(msg);
    return BOXTASK_FAILURE;
  }
}

BoxTask GLib_Obj_At_Length(BoxVM *vm) {
  BoxInt *length = BOX_VM_THIS_PTR(vm, BoxInt);
  BoxGObjPtr gobj = BOX_VM_ARG(vm, BoxGObjPtr);
  *length += (BoxInt) BoxGObj_Get_Length(gobj);
  return BOXTASK_OK;
}

BoxTask GLib_Int_At_Obj_GetType(BoxVM *vm) {
  BoxSubtype *obj_get = BOX_VM_THIS_PTR(vm, BoxSubtype);
  BoxGObjPtr gobj_parent = *((BoxGObjPtr *) BOXSUBTYPE_PARENT_PTR(obj_get));
  BoxInt *gobj_type = (BoxInt *) BOXSUBTYPE_CHILD_PTR(obj_get);
  BoxInt idx = BOX_VM_ARG(vm, BoxInt);
  BoxInt t = BoxGObj_Get_Type(gobj_parent, idx);
  if (t >= 0) {
    *gobj_type = t;
    return BOXTASK_OK;

  } else {
    BoxVM_Set_Fail_Msg(vm, "Cannot get item type. Index out of bounds.");
    return BOXTASK_FAILURE;
  }
}

BoxTask GLib_StrStr_Compare(BoxVM *vm) {
  BoxInt *result = BOX_VM_THIS_PTR(vm, BoxInt);
  BoxStr *s = BOX_VM_ARG_PTR(vm, BoxStr);
  *result = strcmp((s[0].length > 0) ? s[0].ptr : "",
                   (s[1].length > 0) ? s[1].ptr : "");
  return BOXTASK_OK;
}

BoxTask GLib_Obj_At_MergeObjs(BoxVM *vm) {
  BoxSubtype *obj_merge = BOX_VM_THIS_PTR(vm, BoxSubtype);
  BoxGObjPtr gobj_dest = *((BoxGObjPtr *) BOXSUBTYPE_PARENT_PTR(obj_merge));
  BoxGObjPtr gobj_src = BOX_VM_ARG(vm, BoxGObjPtr);
  BoxGObj_Merge(gobj_dest, gobj_src);
  return BOXTASK_OK;
}


