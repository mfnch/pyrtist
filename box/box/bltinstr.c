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
#include "combs.h"
#include "mem.h"
#include "vm_priv.h"
#include "print.h"
#include "compiler.h"
#include "builtins.h"
#include "str.h"
#include "bltinstr.h"

#include "vmproc.h"

/****************************************************************************
 * Here we interface Str and register it for the compiler.                  *
 ****************************************************************************/

static BoxException *My_Concat_C_String(BoxStr *s, const char *cstr) {
  if (BoxStr_Concat_C_String(s, cstr) == BOXTASK_OK)
    return NULL;
  return BoxException_Create("Failure allocating string");
}

static BoxException *My_Concat_And_Free_C_String(BoxStr *s, char *cstr) {
  if (cstr && BoxStr_Concat_C_String(s, cstr) == BOXTASK_OK) {
    Box_Mem_Free(cstr);
    return NULL;
  }

  Box_Mem_Free(cstr);
  return BoxException_Create("Allocation failure (bltinstr.c)");
}

BOXEXPORT BoxException *
Box_Runtime_Init_At_Str(BoxPtr *parent, BoxPtr *child) {
  BoxStr *s = BoxPtr_Get_Target(parent);
  BoxStr_Init(s);
  return NULL;
}

BOXEXPORT BoxException *
Box_Runtime_Finish_At_Str(BoxPtr *parent, BoxPtr *child) {
  BoxStr *s = BoxPtr_Get_Target(parent);
  BoxStr_Finish(s);
  return NULL;
}

BOXEXPORT BoxException *
Box_Runtime_Pause_At_Str(BoxPtr *parent, BoxPtr *child) {
  BoxStr *s = BoxPtr_Get_Target(parent);
  return My_Concat_C_String(s, "\n");
}

BOXEXPORT BoxException *
Box_Runtime_Char_At_Str(BoxPtr *parent, BoxPtr *child) {
  BoxStr *s = BoxPtr_Get_Target(parent);
  char ca[2] = {*((char *) BoxPtr_Get_Target(child)), '\0'};
  return My_Concat_C_String(s, ca);
}

BOXEXPORT BoxException *
Box_Runtime_INT_At_Str(BoxPtr *parent, BoxPtr *child) {
  BoxStr *s = BoxPtr_Get_Target(parent);
  BoxInt i = *((BoxInt *) BoxPtr_Get_Target(child));
  return My_Concat_And_Free_C_String(s, Box_SPrintF("%I", i));
}

BOXEXPORT BoxException *
Box_Runtime_REAL_At_Str(BoxPtr *parent, BoxPtr *child) {
  BoxStr *s = BoxPtr_Get_Target(parent);
  BoxReal r = *((BoxReal *) BoxPtr_Get_Target(child));
  return My_Concat_And_Free_C_String(s, Box_SPrintF("%R", r));
}

BOXEXPORT BoxException *
Box_Runtime_Point_At_Str(BoxPtr *parent, BoxPtr *child) {
  BoxStr *s = BoxPtr_Get_Target(parent);
  BoxPoint *p = (BoxPoint *) BoxPtr_Get_Target(child);
  return My_Concat_And_Free_C_String(s, Box_SPrintF("(%R, %R)", p->x, p->y));
}

BOXEXPORT BoxException *
Box_Runtime_Ptr_At_Str(BoxPtr *parent, BoxPtr *child) {
  BoxStr *s = BoxPtr_Get_Target(parent);
  BoxPtr *p = BoxPtr_Get_Target(child);
  char *cs = Box_SPrintF("Ptr[.block=%p, .data=%p]", p->block, p->ptr);
  return My_Concat_And_Free_C_String(s, cs);
}

BOXEXPORT BoxException *
Box_Runtime_CPtr_At_Str(BoxPtr *parent, BoxPtr *child) {
  BoxStr *s = BoxPtr_Get_Target(parent);
  BoxCPtr cp = *((BoxCPtr *) BoxPtr_Get_Target(child));
  char *cs = Box_SPrintF("CPtr[%p]", cp);
  return My_Concat_And_Free_C_String(s, cs);
}

BOXEXPORT BoxException *
Box_Runtime_Str_At_Str(BoxPtr *parent, BoxPtr *child) {
  BoxStr *dst = BoxPtr_Get_Target(parent);
  BoxStr *src = BoxPtr_Get_Target(child);
  if (src->length > 0)
    return My_Concat_C_String(dst, src->ptr);
  return NULL;
}

BOXEXPORT BoxException *
Box_Runtime_Obj_At_Str(BoxPtr *parent, BoxPtr *child) {
  BoxStr *s = BoxPtr_Get_Target(parent);
  char *c_s = BoxPtr_Get_Target(child);
  assert(s);
  return My_Concat_C_String(s, c_s);
}

BOXEXPORT BoxException *
Box_Runtime_Str_To_Str(BoxPtr *parent, BoxPtr *child) {
  BoxStr *dst = BoxPtr_Get_Target(parent),
         *src = BoxPtr_Get_Target(child);

  char *src_text = (char *) src->ptr; /* Need this (for case: src == dest) */
  BoxStr_Init(dst);
  if (src->length > 0)
    return My_Concat_C_String(dst, src_text);

  return NULL;
}

BOXEXPORT BoxException *
Box_Runtime_Str_At_Num(BoxPtr *parent, BoxPtr *child) {
  BoxInt *len = BoxPtr_Get_Target(parent);
  BoxStr *s = BoxPtr_Get_Target(child);
  *len += s->length;
  return NULL;
}

static BoxTask My_Compare_Str(BoxVMX *vm) {
  BoxInt *compare = BOX_VM_THIS_PTR(vm, BoxInt);
  struct struc_str_couple {BoxStr left, right;} *str_couple;
  str_couple = BOX_VM_ARG_PTR(vm, struct struc_str_couple);
  *compare = BoxStr_Compare(& str_couple->left, & str_couple->right);
  return BOXTASK_OK;
}

static void My_Register_Compare_Str(BoxCmp *c) {
  BoxType *str = Box_Get_Core_Type(BOXTYPEID_STR);
  BoxType *str_couple = BoxType_Create_Structure();
  BoxType_Add_Member_To_Structure(str_couple, str, NULL);
  BoxType_Add_Member_To_Structure(str_couple, str, NULL);
  Bltin_Proc_Def(Box_Get_Core_Type(BOXTYPEID_COMPARE),
                 str_couple, My_Compare_Str);
  (void) BoxType_Unlink(str_couple);
}

void Bltin_Str_Register_Procs(BoxCmp *c) {
  BoxType *t_str = Box_Get_Core_Type(BOXTYPEID_STR),
          *t_num = Box_Get_Core_Type(BOXTYPEID_NUM);
  BoxCombDef defs[] =
    {BOXCOMBDEF_I_AT_T(BOXTYPEID_INIT, t_str, Box_Runtime_Init_At_Str),
     BOXCOMBDEF_I_AT_T(BOXTYPEID_FINISH, t_str, Box_Runtime_Finish_At_Str),
     BOXCOMBDEF_T_TO_T(t_str, t_str, Box_Runtime_Str_To_Str),
     BOXCOMBDEF_I_AT_T(BOXTYPEID_PAUSE, t_str, Box_Runtime_Pause_At_Str),
     BOXCOMBDEF_I_AT_T(BOXTYPEID_CHAR, t_str, Box_Runtime_Char_At_Str),
     BOXCOMBDEF_I_AT_T(BOXTYPEID_INT, t_str, Box_Runtime_INT_At_Str),
     BOXCOMBDEF_I_AT_T(BOXTYPEID_REAL, t_str, Box_Runtime_REAL_At_Str),
     BOXCOMBDEF_I_AT_T(BOXTYPEID_SPOINT, t_str, Box_Runtime_Point_At_Str),
     BOXCOMBDEF_T_AT_T(t_str, t_str, Box_Runtime_Str_At_Str),
     BOXCOMBDEF_I_AT_T(BOXTYPEID_PTR, t_str, Box_Runtime_Ptr_At_Str),
     BOXCOMBDEF_I_AT_T(BOXTYPEID_CPTR, t_str, Box_Runtime_CPtr_At_Str),
     BOXCOMBDEF_I_AT_T(BOXTYPEID_OBJ, t_str, Box_Runtime_Obj_At_Str),
     BOXCOMBDEF_T_AT_T(t_str, t_num, Box_Runtime_Str_At_Num)};
  size_t num_defs = sizeof(defs)/sizeof(BoxCombDef);
  (void) BoxCombDef_Define(defs, num_defs);

  /* String comparison */
  My_Register_Compare_Str(c);
}
