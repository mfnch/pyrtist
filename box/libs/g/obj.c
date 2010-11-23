/****************************************************************************
 * Copyright (C) 2010 by Matteo Franchin                                    *
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
  switch(kind) {
  case BOXGOBJKIND_EMPTY: return 0;
  case BOXGOBJKIND_VOID: return 0;
  case BOXGOBJKIND_CHAR: return sizeof(BoxChar);
  case BOXGOBJKIND_INT: return sizeof(BoxInt);
  case BOXGOBJKIND_REAL: return sizeof(BoxReal);
  case BOXGOBJKIND_POINT: return sizeof(BoxPoint);
  case BOXGOBJKIND_ARRAY: return sizeof(BoxArr);
  default:
    assert(0);
    return 0;
  }
}

const char *BoxGObjKind_Name(BoxGObjKind kind) {
  switch(kind) {
  case BOXGOBJKIND_EMPTY: return "Empty";
  case BOXGOBJKIND_VOID: return "Void";
  case BOXGOBJKIND_CHAR: return "Char";
  case BOXGOBJKIND_INT: return "Int";
  case BOXGOBJKIND_REAL: return "Real";
  case BOXGOBJKIND_POINT: return "Point";
  case BOXGOBJKIND_ARRAY: return "Obj";
  default:
    assert(0);
    return "Unknown";
  }
}

void BoxGObj_Init(BoxGObj *gobj) {
  gobj->kind = BOXGOBJKIND_EMPTY;
}

void BoxGObj_Finish(BoxGObj *gobj) {
  switch(gobj->kind) {
  case BOXGOBJKIND_ARRAY:
    BoxArr_Finish(& gobj->value.v_array);
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

void BoxGObj_Transform_To_Array(BoxGObj *gobj) {
  BoxGObj original_gobj = *gobj;
  BoxArr *a = & gobj->value.v_array;
  gobj->kind = BOXGOBJKIND_ARRAY;
  BoxArr_Init(a, sizeof(BoxGObj), 2);
  BoxArr_Set_Finalizer(a, My_GObj_Array_Finalizer);
  (void) BoxArr_Push(a, & original_gobj);
}

/** Expanding the given BoxGObj adding another element to the list.
 * Return the pointer to the new (uninitalized) member of the Obj.
 */
static BoxGObj *BoxGObj_Expand(BoxGObj *gobj) {
  if (gobj->kind == BOXGOBJKIND_EMPTY) {
    return gobj;

  } else {
    if (gobj->kind != BOXGOBJKIND_ARRAY)
      BoxGObj_Transform_To_Array(gobj);

    assert(gobj->kind == BOXGOBJKIND_ARRAY);
    return (BoxGObj *) BoxArr_Push((BoxArr *) & gobj->value, NULL);
  }
}

static void BoxGObj_Add_X(BoxGObj *gobj, BoxGObjKind kind, void *content) {
  BoxGObj *gobj_dest = BoxGObj_Expand(gobj);
  BoxGObj_Init(gobj_dest);

  if (kind == BOXGOBJKIND_ARRAY) {
    BoxArr *a = (BoxArr *) content;
    size_t num_items = BoxArr_Num_Items(a), i;

    for(i = 1; i <= num_items; i++) {
      BoxGObj *gobj_src = (BoxGObj *) BoxArr_Item_Ptr(a, i);
      BoxGObj_Add_X(gobj_dest, gobj_src->kind, & gobj_src->value);
    }

  } else {
    size_t sz = BoxGObjKind_Size(kind);

    gobj_dest->kind = kind;
    if (content != NULL && sz > 0)
      (void) memcpy(& gobj_dest->value, content, sz);
    return;
  }
}

void BoxGObj_Add_Void(BoxGObj *gobj) {
  BoxGObj_Add_X(gobj, BOXGOBJKIND_VOID, NULL);
}

void BoxGObj_Add_Char(BoxGObj *gobj, BoxChar v) {
  BoxGObj_Add_X(gobj, BOXGOBJKIND_CHAR, & v);
}

void BoxGObj_Add_Int(BoxGObj *gobj, BoxInt v) {
  BoxGObj_Add_X(gobj, BOXGOBJKIND_INT, & v);
}

void BoxGObj_Add_Real(BoxGObj *gobj, BoxReal v) {
  BoxGObj_Add_X(gobj, BOXGOBJKIND_REAL, & v);
}

void BoxGObj_Add_Point(BoxGObj *gobj, BoxPoint v) {
  BoxGObj_Add_X(gobj, BOXGOBJKIND_POINT, & v);
}

void BoxGObj_Add_Obj(BoxGObj *gobj, BoxGObj *new_child) {
  BoxGObj_Add_X(gobj, new_child->kind, & new_child->value);
}

BoxGObj *BoxGObj_Get(BoxGObj *gobj, BoxInt idx) {
  if (gobj->kind == BOXGOBJKIND_ARRAY) {
    BoxArr *a = (BoxArr *) & gobj->value;
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
  case BOXGOBJKIND_ARRAY:
    return BoxArr_Num_Items((BoxArr *) & gobj->value);
  default:
    return 1;
  }
}

BoxInt BoxGObj_Get_Type(BoxGObj *gobj, BoxInt idx) {
  if (gobj->kind == BOXGOBJKIND_ARRAY) {
    BoxArr *a = (BoxArr *) & gobj->value;
    return (idx >= 0 && idx < BoxArr_Num_Items(a)) ?
           ((BoxGObj *) BoxArr_Item_Ptr(a, idx + 1))->kind : -1;

  } else
    return (idx == 0) ? gobj->kind : -1;
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
  BoxGObj_Add_X(gobj, kind, BOX_VM_ARG_PTR(vm, void));
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

BoxTask GLib_Obj_At_Obj(BoxVM *vm) {
  BoxGObj_Add_Obj(BOX_VM_THIS(vm, BoxGObjPtr), BOX_VM_ARG(vm, BoxGObjPtr));
  return BOXTASK_OK;
}

static BoxTask GLib_Obj_At_X(BoxVM *vm, BoxGObjKind kind) {
  void *obj_data = BOX_VM_THIS_PTR(vm, void);
  BoxGObjPtr gobj = BOX_VM_ARG(vm, BoxGObjPtr);
  if (gobj->kind == kind) {
    (void) memcpy(obj_data, & gobj->value, BoxGObjKind_Size(kind));
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

BoxTask GLib_Int_At_Obj_Get(BoxVM *vm) {
  BoxSubtype *obj_get = BOX_VM_THIS_PTR(vm, BoxSubtype);
  BoxGObjPtr gobj_parent = *BOXSUBTYPE_PARENT_PTR(obj_get, BoxGObjPtr);
  BoxGObjPtr gobj_child = *BOXSUBTYPE_CHILD_PTR(obj_get, BoxGObjPtr);
  BoxInt idx = BOX_VM_ARG(vm, BoxInt);
  BoxGObj *gobj_indexed = BoxGObj_Get(gobj_parent, idx);
  if (gobj_indexed != NULL) {
    BoxGObj_Add_Obj(gobj_child, gobj_indexed);
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
  BoxGObjPtr gobj_parent = *BOXSUBTYPE_PARENT_PTR(obj_get, BoxGObjPtr);
  BoxInt *gobj_type = BOXSUBTYPE_CHILD_PTR(obj_get, BoxInt);
  BoxInt idx = BOX_VM_ARG(vm, BoxInt);
  BoxInt t = BoxGObj_Get_Type(gobj_parent, idx);
  if (t > 0) {
    *gobj_type = t;
    return BOXTASK_OK;

  } else {
    BoxVM_Set_Fail_Msg(vm, "Cannot get item type. Index out of bounds.");
    return BOXTASK_FAILURE;
  }
}