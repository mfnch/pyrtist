/****************************************************************************
 * Copyright (C) 2008-2011 by Matteo Franchin                               *
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

#include "types.h"
#include "mem.h"
#include "typesys.h"
#include "vm_priv.h"
#include "print.h"
#include "compiler.h"
#include "builtins.h"
#include "str.h"
#include "bltinstr.h"

#include "vmproc.h"
#include "vmsymstuff.h"

/****************************************************************************
 * Here we interface Str and register it for the compiler.                  *
 ****************************************************************************/
static BoxTask My_Str_Create(BoxVMX *vm) {
  BoxStr *s = BOX_VM_THIS_PTR(vm, BoxStr);
  BoxStr_Init(s);
  return BOXTASK_OK;
}

static BoxTask My_Str_Destroy(BoxVMX *vm) {
  BoxStr *s = BOX_VM_THIS_PTR(vm, BoxStr);
  BoxStr_Finish(s);
  return BOXTASK_OK;
}

static BoxTask My_Str_Pause(BoxVMX *vm) {
  return BoxStr_Concat_C_String(BOX_VM_THIS_PTR(vm, Str), "\n");
}

static BoxTask My_Str_Char(BoxVMX *vm) {
  BoxStr *s = BOX_VM_THIS_PTR(vm, BoxStr);
  char ca[2] = {BOX_VM_ARG(vm, char), '\0'};
  return BoxStr_Concat_C_String(s, ca);
}

static BoxTask My_Str_Int(BoxVMX *vm) {
  BoxStr *s = BOX_VM_THIS_PTR(vm, BoxStr);
  BoxInt i = BOX_VM_ARG(vm, BoxInt);
  char *tmp = Box_SPrintF("%I", i);
  if (tmp != (char *) NULL) {
    if (BoxStr_Concat_C_String(s, tmp) != BOXTASK_OK)
      return BOXTASK_FAILURE;
    BoxMem_Free(tmp);
  }
  return BOXTASK_OK;
}

static BoxTask My_Str_Real(BoxVMX *vm) {
  BoxStr *s = BOX_VM_THIS_PTR(vm, BoxStr);
  BoxReal r = BOX_VM_ARG1(vm, BoxReal);
  char *tmp = Box_SPrintF("%R", r);
  if (tmp != (char *) NULL) {
    if (BoxStr_Concat_C_String(s, tmp) != BOXTASK_OK)
      return BOXTASK_FAILURE;
    BoxMem_Free(tmp);
  }
  return BOXTASK_OK;
}

static BoxTask My_Str_Point(BoxVMX *vm) {
  BoxStr *s = BOX_VM_THIS_PTR(vm, BoxStr);
  BoxPoint *p = BOX_VM_ARG_PTR(vm, BoxPoint);
  char *tmp = Box_SPrintF("(%R, %R)", p->x, p->y);
  if (tmp != (char *) NULL) {
    if (BoxStr_Concat_C_String(s, tmp) != BOXTASK_OK)
      return BOXTASK_FAILURE;
    BoxMem_Free(tmp);
  }
  return BOXTASK_OK;
}

static BoxTask My_Str_Ptr(BoxVMX *vm) {
  BoxStr *s = BOX_VM_THIS_PTR(vm, BoxStr);
  BoxPtr *p = BOX_VM_ARG_PTR(vm, BoxPtr);
  char *tmp = printdup("Ptr[block:%p, data:%p]", p->block, p->ptr);
  if (tmp != (char *) NULL) {
    if (BoxStr_Concat_C_String(s, tmp) != BOXTASK_OK)
      return BOXTASK_FAILURE;
    BoxMem_Free(tmp);
  }
  return BOXTASK_OK;
}

static BoxTask My_Str_CPtr(BoxVMX *vm) {
  BoxStr *s = BOX_VM_THIS_PTR(vm, BoxStr);
  BoxCPtr p = BOX_VM_ARG(vm, BoxCPtr);
  char *tmp = printdup("CPtr[%p]", p);
  if (tmp != (char *) NULL) {
    if (BoxStr_Concat_C_String(s, tmp) != BOXTASK_OK)
      return BOXTASK_FAILURE;
    BoxMem_Free(tmp);
  }
  return BOXTASK_OK;
}

static BoxTask My_Str_Str(BoxVMX *vm) {
  BoxStr *s = BOX_VM_THIS_PTR(vm, BoxStr);
  BoxStr *s2 = BOX_VM_ARG_PTR(vm, BoxStr);
  if (s2->length > 0)
    return BoxStr_Concat_C_String(s, s2->ptr);
  return BOXTASK_OK;
}

static BoxTask My_Str_CString(BoxVMX *vm) {
  BoxStr *s = BOX_VM_THIS_PTR(vm, BoxStr);
  char *c_s = BOX_VM_ARG_PTR(vm, char);
  assert(s != NULL);
  return BoxStr_Concat_C_String(s, c_s);
}

static BoxTask My_Str_Copy(BoxVMX *vm) {
  BoxStr *dest = BOX_VM_THIS_PTR(vm, BoxStr),
         *src = BOX_VM_ARG_PTR(vm, BoxStr);
  char *src_text = (char *) src->ptr; /* Need this (for case: src == dest) */
  BoxStr_Init(dest);
  if (src->length > 0)
    return BoxStr_Concat_C_String(dest, src_text);
  return BOXTASK_OK;
}

static BoxTask My_Length_Str(BoxVMX *vm) {
  BoxInt *len = BOX_VM_THIS_PTR(vm, BoxInt);
  BoxStr *src = BOX_VM_ARG_PTR(vm, BoxStr);
  *len += src->length;
  return BOXTASK_OK;
}

static BoxTask My_Compare_Str(BoxVMX *vm) {
  BoxInt *compare = BOX_VM_THIS_PTR(vm, BoxInt);
  struct struc_str_couple {BoxStr left, right;} *str_couple;
  str_couple = BOX_VM_ARG_PTR(vm, struct struc_str_couple);
  *compare = BoxStr_Compare(& str_couple->left, & str_couple->right);
  return BOXTASK_OK;
}

static void My_Register_Compare_Str(BoxCmp *c) {
  BoxTS *ts = & c->ts;
  BoxTypeId str_couple = BoxTS_Begin_Struct(ts);
  BoxTS_Add_Struct_Member(ts, str_couple, c->bltin.string, NULL);
  BoxTS_Add_Struct_Member(ts, str_couple, c->bltin.string, NULL);
  Bltin_Proc_Def(c, c->bltin.compare, str_couple, My_Compare_Str);
}

void Bltin_Str_Register_Procs(BoxCmp *c) {
  Bltin_Proc_Def(c, c->bltin.string,  BOXTYPE_CREATE, My_Str_Create);
  Bltin_Proc_Def(c, c->bltin.string, BOXTYPE_DESTROY, My_Str_Destroy);
  Bltin_Proc_Def(c, c->bltin.string,   BOXTYPE_PAUSE, My_Str_Pause);
  Bltin_Proc_Def(c, c->bltin.string,    BOXTYPE_CHAR, My_Str_Char);
  Bltin_Proc_Def(c, c->bltin.string,     BOXTYPE_INT, My_Str_Int);
  Bltin_Proc_Def(c, c->bltin.string,    BOXTYPE_REAL, My_Str_Real);
  Bltin_Proc_Def(c, c->bltin.string,   BOXTYPE_POINT, My_Str_Point);
  Bltin_Proc_Def(c, c->bltin.string,     BOXTYPE_PTR, My_Str_Ptr);
  Bltin_Proc_Def(c, c->bltin.string,    BOXTYPE_CPTR, My_Str_CPtr);
  Bltin_Proc_Def(c, c->bltin.string, c->bltin.string, My_Str_Str);
  Bltin_Proc_Def(c, c->bltin.string,     BOXTYPE_OBJ, My_Str_CString);
  Bltin_Proc_Def(c, c->bltin.length, c->bltin.string, My_Length_Str);
  Bltin_Proc_Def(c,    c->bltin.num, c->bltin.string, My_Length_Str);

  /* Copy Str to Str */
  Bltin_Comb_Def(c, c->bltin.string, BOXCOMBTYPE_COPY, c->bltin.string,
                 My_Str_Copy);

  /* String comparison */
  My_Register_Compare_Str(c);
}
