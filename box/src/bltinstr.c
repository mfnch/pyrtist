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

#include "types.h"
#include "typesys.h"
#include "compiler.h"
#include "virtmach.h"
#include "builtins.h"
#include "bltinstr.h"

Type type_Str;

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
  TASK( Tym_Def_Explicit_Alias(& type_Str, & NAME("Str"), d) );

  /* Now we register all the methods */
  TASK( Str_Register_All() );
  return Success;
}

void Bltin_Str_Destroy(void) {}

static Task Str_Begin(VMProgram *vmp) {
  return Success;
}

static Task Str_Destroy(VMProgram *vmp) {
  printf("Destroying string!\n");
  return Success;
}

static Task Str_Register_All(void) {
  struct {
    Type child;
    int kind;
    Task (*proc)(VMProgram *);

  } *item, table[] = {
    {TYPE_OPEN, BOX_CREATION, Str_Begin},
    {TYPE_DESTROY, BOX_CREATION | BOX_MODIFICATION, Str_Destroy},
    {TYPE_NONE}
  };

  for(item = & table[0]; item->child != TYPE_NONE; item++) {
    TASK(Cmp_Builtin_Proc_Def(item->child, item->kind, type_Str, item->proc));
  }
  return Success;
}
