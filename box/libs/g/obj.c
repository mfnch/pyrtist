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

void BoxGObj_Init(BoxGObj *gobj) {
  gobj->kind = BOXGOBJKIND_EMPTY;
}

void BoxGObj_Finish(BoxGObj *gobj) {
  switch (gobj->kind) {
  case BOXGOBJKIND_STR:
    BoxStr_Finish(& gobj->value.v_str);
    return;

  case BOXGOBJKIND_COMPOSITE:
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

void BoxGObj_Transform_To_Composite(BoxGObj *gobj) {
  if (gobj->kind != BOXGOBJKIND_COMPOSITE) {
    BoxGObj original_gobj = *gobj;
    BoxArr *a = & gobj->value.v_array;
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
  new_memb = (BoxGObj *) BoxArr_Push(& gobj->value.v_array, NULL);
  BoxGObj_Init(new_memb);
  return new_memb;
}

static void BoxGObj_Add_Or_Merge_X(BoxGObj *gobj, BoxGObjKind kind,
                                   void *content, int merge) {
  if (kind == BOXGOBJKIND_COMPOSITE) {
    BoxArr *a = (BoxArr *) content;
    size_t num_items = BoxArr_Num_Items(a), i;
    BoxGObj *gobj_dest;

    gobj_dest = BoxGObj_Expand(gobj, merge);

    for(i = 1; i <= num_items; i++) {
      BoxGObj *gobj_src = (BoxGObj *) BoxArr_Item_Ptr(a, i);
      BoxGObj_Add_Or_Merge_X(gobj_dest, gobj_src->kind, & gobj_src->value, 0);
    }

  } else {
    size_t sz = BoxGObjKind_Size(kind);
    BoxGObj *gobj_dest = BoxGObj_Expand(gobj, merge);

    gobj_dest->kind = kind;
    if (content != NULL && sz > 0) {
      switch (kind) {
        case BOXGOBJKIND_STR:
          assert(0);
        default:
          (void) memcpy(& gobj_dest->value, content, sz);
          break;
      }
    }
  }
}

static void BoxGObj_Add_X(BoxGObj *gobj, BoxGObjKind kind, void *content) {
  BoxGObj_Add_Or_Merge_X(gobj, kind, content, 0);
}

static void BoxGObj_Merge_X(BoxGObj *gobj, BoxGObjKind kind, void *content) {
  BoxGObj_Add_Or_Merge_X(gobj, kind, content, 1);
}

void BoxGObj_Add_Str(BoxGObj *gobj, const BoxStr *s) {

}

BoxGObj *BoxGObj_Get(BoxGObj *gobj, BoxInt idx) {
  if (gobj->kind == BOXGOBJKIND_COMPOSITE) {
    BoxArr *a = & gobj->value.v_array;
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
    return BoxArr_Num_Items(& gobj->value.v_array);
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
  BoxGObj *arg = BOX_VM_ARG(vm, BoxGObjPtr);
  BoxGObj_Add_X(BOX_VM_THIS(vm, BoxGObjPtr), arg->kind, & arg->value);
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

BoxTask GLib_Obj_At_Str(BoxVM *vm) {
  return GLib_Obj_At_X(vm, BOXGOBJKIND_STR);
}

BoxTask GLib_Int_At_Obj_Get(BoxVM *vm) {
  BoxSubtype *obj_get = BOX_VM_THIS_PTR(vm, BoxSubtype);
  BoxGObjPtr gobj_parent = *BOXSUBTYPE_PARENT_PTR(obj_get, BoxGObjPtr);
  BoxGObjPtr gobj_child = *BOXSUBTYPE_CHILD_PTR(obj_get, BoxGObjPtr);
  BoxInt idx = BOX_VM_ARG(vm, BoxInt);
  BoxGObj *gobj_indexed = BoxGObj_Get(gobj_parent, idx);
  if (gobj_indexed != NULL) {
    BoxGObj_Merge_X(gobj_child, gobj_indexed->kind, & gobj_indexed->value);
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
  if (t >= 0) {
    *gobj_type = t;
    return BOXTASK_OK;

  } else {
    BoxVM_Set_Fail_Msg(vm, "Cannot get item type. Index out of bounds.");
    return BOXTASK_FAILURE;
  }
}
