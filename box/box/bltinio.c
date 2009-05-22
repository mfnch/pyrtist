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

#include <stdlib.h>

#include "types.h"
#include "mem.h"
#include "typesys.h"
#include "virtmach.h"
#include "compiler.h"
#include "builtins.h"
#include "bltinstr.h"
#include "bltinio.h"

#if 0
typedef struct {
  unsigned int opened : 1;
  char *name, *mode;
  FILE *file;
} File;

Type type_File;

static Task IO_Register_All(void);

Task Bltin_Io_Init(void) {
  TASK( Tym_Def_Intrinsic(& type_File, & NAME("File"), sizeof(File)) );

  TASK( IO_Register_All() );
  return Success;
}

void Bltin_Io_Destroy(void) {}

static Task C_File_Open(VMProgram *vmp) {
  File *f = BOX_VM_THIS_PTR(vmp, File);
  f->opened = 0;
  f->name = (char *) NULL;
  f->mode = (char *) NULL;
  return Success;
}

static Task File_Destroy(VMProgram *vmp) {
  File *f = BOX_VM_THIS_PTR(vmp, File);
  if (f->opened) fclose(f->file);
  BoxMem_Free(f->name);
  BoxMem_Free(f->mode);
  return Success;
}

static Task C_File_String(VMProgram *vmp) {
  Str *s = BOX_VM_ARG1_PTR(vmp, Str);
  File *f = BOX_VM_THIS_PTR(vmp, File);
  if (f->name == (char *) NULL)
    f->name = BoxMem_Strdup(Str_Get_CStr(s));
  else if (f->mode == (char *) NULL) 
    f->mode = BoxMem_Strdup(Str_Get_CStr(s));
  else {
    fprintf(stderr, "Too many arguments to File object\n");
    return Failed;
  }
  return Success;
}

static Task C_File_Close(VMProgram *vmp) {
  File *f = BOX_VM_THIS_PTR(vmp, File);

  if (f->name == (char *) NULL) {
    fprintf(stderr, "Missing file name for File object.\n");
    return Failed;
  }

  if (f->mode == (char *) NULL)
    f->file = fopen(f->name, "rt");
  else
    f->file = fopen(f->name, f->mode);

  BoxMem_Free(f->name);
  BoxMem_Free(f->mode);

  f->name = (char *) NULL;
  f->mode = (char *) NULL;
  if (f->file == (FILE *) NULL) {
    fprintf(stderr, "Error opening the file.\n");
    return Failed;
  }
  f->opened = 1;
  return Success;
}

static Task M_File_String(VMProgram *vmp) {
  Str *s = BOX_VM_ARG1_PTR(vmp, Str);
  File *f = BOX_VM_THIS_PTR(vmp, File);
  if (! f->opened) {
    fprintf(stderr, "Error: writing to a not opened file. Exiting!\n");
    return Failed;
  }
  fprintf(f->file, "%s", Str_Get_CStr(s)); /* should use fwrite instead */
  return Success;
}

static Task IO_Register_All(void) {
  struct {
    Type parent;
    Type child;
    int kind;
    Task (*proc)(VMProgram *);

  } *item, table[] = {
    {type_File, TYPE_OPEN, BOX_CREATION, C_File_Open},
    {type_File, TYPE_CLOSE, BOX_CREATION, C_File_Close},
    {type_File, TYPE_DESTROY, BOX_CREATION | BOX_MODIFICATION, File_Destroy},
    {type_File, type_StrSpecies, BOX_CREATION, C_File_String},
    {type_File, type_StrSpecies, BOX_MODIFICATION, M_File_String},
    {TYPE_NONE}
  };

  for(item = & table[0]; item->parent != TYPE_NONE; item++) {
    TASK(Cmp_Builtin_Proc_Def(item->child, item->kind,
                              item->parent, item->proc));
  }
  return Success;
}
#endif

