/****************************************************************************
 * Copyright (C) 2008 by Matteo Franchin                                    *
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

#include <string.h>
#include <assert.h>

#include "types.h"
#include "mem.h"
#include "typesys.h"
#include "virtmach.h"
#include "print.h"
#include "compiler.h"
#include "builtins.h"
#include "bltinstr.h"

#include "vmproc.h"
#include "vmsymstuff.h"

/****************************************************************************
 * Here we define some generic functions to handle Str objects              *
 ****************************************************************************/

void BoxStr_Init(BoxStr *s) {
  s->ptr = NULL;
  s->length = 0;
  s->buffer_size = 0;
}

void BoxStr_Finish(BoxStr *s) {
  if (s->ptr != (char *) NULL) {
    BoxMem_Free(s->ptr);
    s->ptr = (char *) NULL;
    s->length = 0;
    s->buffer_size = 0;
  }
}

Task BoxStr_Large_Enough(BoxStr *s, Int length) {
  Int len;
  assert(s->length >= 0 && length >= 0);

  len = s->length + length + 1;
  len = len + (len+1)/2;
  s->ptr = (char *) BoxMem_Realloc(s->ptr, len);
  s->buffer_size = len;
  return Success;
}

Task BoxStr_Concat(BoxStr *s, const char *ca) {
  Int len = strlen(ca);
  if (len < 1) return Success;
  if (s->buffer_size - s->length - 1 < len)
    BoxStr_Large_Enough(s, len);
  assert(s->buffer_size - s->length - 1 >= len);
  (void) strcpy(s->ptr + s->length, ca);
  s->length += len;
  return Success;
}

/****************************************************************************
 * Here we interface Str and register it for the compiler.                  *
 ****************************************************************************/
static Task My_Str_Create(BoxVM *vm) {
  BoxStr *s = BOX_VM_THIS_PTR(vm, BoxStr);
  BoxStr_Init(s);
  return Success;
}

static Task My_Str_Destroy(BoxVM *vm) {
  BoxStr *s = BOX_VM_THIS_PTR(vm, BoxStr);
  BoxStr_Finish(s);
  return Success;
}

static Task My_Str_Char(BoxVM *vm) {
  BoxStr *s = BOX_VM_THIS_PTR(vm, BoxStr);
  char ca[2] = {BOX_VM_ARG(vm, char), '\0'};
  return BoxStr_Concat(s, ca);
}

static Task My_Str_Int(BoxVM *vm) {
  BoxStr *s = BOX_VM_THIS_PTR(vm, BoxStr);
  Int i = BOX_VM_ARG(vm, Int);
  char *tmp = printdup("%I", i);
  if (tmp != (char *) NULL) {
    TASK( BoxStr_Concat(s, tmp) );
    BoxMem_Free(tmp);
  }
  return Success;
}

static Task My_Str_Real(BoxVM *vm) {
  BoxStr *s = BOX_VM_THIS_PTR(vm, BoxStr);
  Real r = BOX_VM_ARG1(vm, Real);
  char *tmp = printdup("%R", r);
  if (tmp != (char *) NULL) {
    TASK( BoxStr_Concat(s, tmp) );
    BoxMem_Free(tmp);
  }
  return Success;
}

static Task My_Str_Point(BoxVM *vm) {
  BoxStr *s = BOX_VM_THIS_PTR(vm, BoxStr);
  Point *p = BOX_VM_ARG_PTR(vm, Point);
  char *tmp = printdup("(%R, %R)", p->x, p->y);
  if (tmp != (char *) NULL) {
    TASK( BoxStr_Concat(s, tmp) );
    BoxMem_Free(tmp);
  }
  return Success;
}

static Task My_Str_Str(BoxVM *vm) {
  BoxStr *s = BOX_VM_THIS_PTR(vm, BoxStr);
  BoxStr *s2 = BOX_VM_ARG_PTR(vm, BoxStr);
  if (s2->length > 0)
    return BoxStr_Concat(s, s2->ptr);
  return Success;
}

static Task My_Str_CString(BoxVM *vm) {
  BoxStr *s = BOX_VM_THIS_PTR(vm, BoxStr);
  char *c_s = BOX_VM_ARG_PTR(vm, char);
  return BoxStr_Concat(s, c_s);
}

static Task My_Str_Copy(BoxVM *vm) {
  BoxStr *dest = BOX_VM_THIS_PTR(vm, BoxStr),
         *src = BOX_VM_ARG_PTR(vm, BoxStr);
  char *src_text = (char *) src->ptr; /* Need this (for case: src == dest) */
  BoxStr_Init(dest);
  if (src->length > 0)
    return BoxStr_Concat(dest, src_text);
  return Success;
}

void Bltin_Str_Register_Procs(BoxCmp *c) {
  Operation *opn;
  BoxVMSymID copy_str;

  Bltin_Proc_Def(c, c->bltin.string,  BOXTYPE_CREATE, My_Str_Create);
  Bltin_Proc_Def(c, c->bltin.string, BOXTYPE_DESTROY, My_Str_Destroy);
  Bltin_Proc_Def(c, c->bltin.string,    BOXTYPE_CHAR, My_Str_Char);
  Bltin_Proc_Def(c, c->bltin.string,     BOXTYPE_INT, My_Str_Int);
  Bltin_Proc_Def(c, c->bltin.string,    BOXTYPE_REAL, My_Str_Real);
  Bltin_Proc_Def(c, c->bltin.string,   BOXTYPE_POINT, My_Str_Point);
  Bltin_Proc_Def(c, c->bltin.string, c->bltin.string, My_Str_Str);
  Bltin_Proc_Def(c, c->bltin.string,     BOXTYPE_OBJ, My_Str_CString);

  /* Copy Str to Str */
  opn = Operator_Add_Opn(& c->convert, c->bltin.string,
                         BOXTYPE_NONE, c->bltin.string);
  copy_str = Bltin_Proc_Add(c, "copy_str", My_Str_Copy);
  Operation_Set_User_Implem(opn, copy_str);
}

#if 0

Type type_Str, type_StrSpecies;

static Task Str_Register_All(void);

Task Bltin_Str_Init(void) {
  /* First define: STR = ++(Int length, buffer_size, Ptr ptr) */
  Type s, d;
  TS_Structure_Begin(cmp->ts, & s);
  TS_Structure_Add(cmp->ts, s, type_IntNum, "length");
  TS_Structure_Add(cmp->ts, s, type_IntNum, "buffer_size");
  TS_Structure_Add(cmp->ts, s, TYPE_PTR, "ptr");
  TS_Detached_New(cmp->ts, & d, s);
  TASK( Tym_Def_Explicit_Alias(& type_Str, & NAME("STR"), d) );

  /* Then define Str = (()Char -> STR) */
  TS_Species_Begin(cmp->ts, & s);
  TS_Species_Add(cmp->ts, s, type_CharArray);
  TS_Species_Add(cmp->ts, s, type_Str);
  TASK( Tym_Def_Explicit_Alias(& type_StrSpecies, & NAME("Str"), s) );

  /* Now we register all the methods */
  TASK( Str_Register_All() );
  return Success;
}

void Bltin_Str_Destroy(void) {}


/* Used for the conversion in the species Str = (()Char -> STR)*/
static Task Str_From_CharArray(BoxVM *vm) {
  Str *s = BOX_VM_THIS_PTR(vm, Str);
  char *ca = BOX_VM_ARG1_PTR(vm, char);
  Str_Init(s);
  return BoxStr_Concat(s, ca);
}

static Task Str_CharArray(BoxVM *vm) {
  Str *s = BOX_VM_THIS_PTR(vm, Str);
  char *ca = BOX_VM_ARG1_PTR(vm, char);
  return BoxStr_Concat(s, ca);
}

static Task Str_Pause(BoxVM *vm) {
  return BoxStr_Concat(BOX_VM_THIS_PTR(vm, Str), "\n");
}

static Task Print_Str(BoxVM *vm) {
  Str *s = BOX_VM_ARG1_PTR(vm, Str);
  if (s->length > 0)
    printf("%s", s->ptr);
  return Success;
}

static Task Str_Register_All(void) {
  struct {
    Type parent;
    Type child;
    int kind;
    Task (*proc)(BoxVM *);

  } *item, table[] = {
    {type_Str, TYPE_OPEN, BOX_CREATION, Str_Begin},
    {type_Str, TYPE_DESTROY, BOX_CREATION | BOX_MODIFICATION, Str_Destroy},
    {type_Str, TYPE_CHAR, BOX_CREATION | BOX_MODIFICATION, Str_Char},
    {type_Str, TYPE_INT, BOX_CREATION | BOX_MODIFICATION, Str_Int},
    {type_Str, TYPE_REAL, BOX_CREATION | BOX_MODIFICATION, Str_Real},
    {type_Str, type_Point, BOX_CREATION | BOX_MODIFICATION, Str_Point},
    {type_Str, type_CharArray, BOX_CREATION | BOX_MODIFICATION, Str_CharArray},
    {type_Str, type_Str, BOX_CREATION | BOX_MODIFICATION, Str_STR},
    {type_Str, TYPE_PAUSE, BOX_CREATION | BOX_MODIFICATION, Str_Pause},
    {type_Print, type_Str, BOX_CREATION, Print_Str},
    {TYPE_NONE}
  };

  for(item = & table[0]; item->parent != TYPE_NONE; item++) {
    TASK(Cmp_Builtin_Proc_Def(item->child, item->kind,
                              item->parent, item->proc));
  }

  /* We also register the conversion ()Char -> STR */
  TASK( Cmp_Builtin_Conv_Def("conv_CharArray_to_STR",
                             type_CharArray, type_Str, Str_From_CharArray) );

  return Success;
}

#endif

