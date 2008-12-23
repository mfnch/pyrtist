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
#include "compiler.h"
#include "virtmach.h"
#include "print.h"
#include "builtins.h"
#include "bltinstr.h"

#include "vmproc.h"
#include "vmsymstuff.h"

/****************************************************************************
 * Here we define some generic functions to handle Str objects              *
 ****************************************************************************/

void Str_Init(Str *s) {
  s->ptr = NULL;
  s->length = 0;
  s->buffer_size = 0;
}

Task Str_Large_Enough(Str *s, Int length) {
  Int len;
  assert(s->length >= 0 && length >= 0);

  len = s->length + length + 1;
  len = len + (len+1)/2;
  s->ptr = (char *) BoxMem_Realloc(s->ptr, len);
  s->buffer_size = len;
  return Success;
}

Task Str_Concat(Str *s, const char *ca) {
  Int len = strlen(ca);
  if (len < 1) return Success;
  if (s->buffer_size - s->length - 1 < len) Str_Large_Enough(s, len);
  assert(s->buffer_size - s->length - 1 >= len);
  (void) strcpy(s->ptr + s->length, ca);
  s->length += len;
  return Success;
}

/****************************************************************************
 * Here we interface Str and register it for the compiler.                  *
 ****************************************************************************/

Type type_Str, type_StrSpecies;

static Task Str_Register_All(void);

Task Bltin_Str_Init(void) {
  /* First define: STR = ++(Int length, buffer_size, Ptr ptr) */
  Type s, d;
  TASK( TS_Structure_Begin(cmp->ts, & s) );
  TASK( TS_Structure_Add(cmp->ts, s, type_IntNum, "length") );
  TASK( TS_Structure_Add(cmp->ts, s, type_IntNum, "buffer_size") );
  TASK( TS_Structure_Add(cmp->ts, s, TYPE_PTR, "ptr") );
  TASK( TS_Detached_New(cmp->ts, & d, s) );
  TASK( Tym_Def_Explicit_Alias(& type_Str, & NAME("STR"), d) );

  /* Then define Str = (()Char -> STR) */
  TASK( TS_Species_Begin(cmp->ts, & s) );
  TASK( TS_Species_Add(cmp->ts, s, type_CharArray) );
  TASK( TS_Species_Add(cmp->ts, s, type_Str) );
  TASK( Tym_Def_Explicit_Alias(& type_StrSpecies, & NAME("Str"), s) );

  /* Now we register all the methods */
  TASK( Str_Register_All() );
  return Success;
}

void Bltin_Str_Destroy(void) {}

static Task Str_Begin(VMProgram *vmp) {
  Str *s = BOX_VM_THIS_PTR(vmp, Str);
  Str_Init(s);
  return Success;
}

static Task Str_Destroy(VMProgram *vmp) {
  Str *s = BOX_VM_THIS_PTR(vmp, Str);
  if (s->ptr != (char *) NULL) {
    BoxMem_Free(s->ptr);
    s->ptr = (char *) NULL;
    s->length = 0;
    s->buffer_size = 0;
  }
  return Success;
}

/* Used for the conversion in the species Str = (()Char -> STR)*/
static Task Str_From_CharArray(VMProgram *vmp) {
  Str *s = BOX_VM_THIS_PTR(vmp, Str);
  char *ca = BOX_VM_ARG1_PTR(vmp, char);
  Str_Init(s);
  return Str_Concat(s, ca);
}

static Task Str_Char(VMProgram *vmp) {
  Str *s = BOX_VM_THIS_PTR(vmp, Str);
  char ca[2] = {BOX_VM_ARG1(vmp, char), '\0'};
  return Str_Concat(s, ca);
}

static Task Str_Int(VMProgram *vmp) {
  Str *s = BOX_VM_THIS_PTR(vmp, Str);
  Int i = BOX_VM_ARG1(vmp, Int);
  char *tmp = printdup("%I", i);
  if (tmp != (char *) NULL) {
    TASK( Str_Concat(s, tmp) );
    BoxMem_Free(tmp);
  }
  return Success;
}

static Task Str_Real(VMProgram *vmp) {
  Str *s = BOX_VM_THIS_PTR(vmp, Str);
  Real r = BOX_VM_ARG1(vmp, Real);
  char *tmp = printdup("%R", r);
  if (tmp != (char *) NULL) {
    TASK( Str_Concat(s, tmp) );
    BoxMem_Free(tmp);
  }
  return Success;
}

static Task Str_Point(VMProgram *vmp) {
  Str *s = BOX_VM_THIS_PTR(vmp, Str);
  Point *p = BOX_VM_ARG1_PTR(vmp, Point);
  char *tmp = printdup("(%R, %R)", p->x, p->y);
  if (tmp != (char *) NULL) {
    TASK( Str_Concat(s, tmp) );
    BoxMem_Free(tmp);
  }
  return Success;
}

static Task Str_CharArray(VMProgram *vmp) {
  Str *s = BOX_VM_THIS_PTR(vmp, Str);
  char *ca = BOX_VM_ARG1_PTR(vmp, char);
  return Str_Concat(s, ca);
}

static Task Str_STR(VMProgram *vmp) {
  Str *s = BOX_VM_THIS_PTR(vmp, Str);
  Str *s2 = BOX_VM_ARG1_PTR(vmp, Str);
  if (s2->length > 0)
    return Str_Concat(s, s2->ptr);
  return Success;
}

static Task Str_Pause(VMProgram *vmp) {
  return Str_Concat(BOX_VM_THIS_PTR(vmp, Str), "\n");
}

static Task Print_Str(VMProgram *vmp) {
  Str *s = BOX_VM_ARG1_PTR(vmp, Str);
  if (s->length > 0)
    printf("%s", s->ptr);
  return Success;
}

static Task Str_Register_All(void) {
  struct {
    Type parent;
    Type child;
    int kind;
    Task (*proc)(VMProgram *);

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
