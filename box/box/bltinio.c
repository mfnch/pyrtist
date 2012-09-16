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
#include <assert.h>

#include "types.h"
#include "mem.h"
#include "typesys.h"
#include "messages.h"
#include "vm_private.h"
#include "builtins.h"
#include "bltinstr.h"
#include "bltinio.h"
#include "compiler.h"
#include "str.h"

typedef struct {
  unsigned int opened : 1;
  char *name, *mode;
  FILE *file;
} File;

static Task My_File_Create(BoxVMX *vm) {
  File *f = BOX_VM_THIS_PTR(vm, File);
  f->opened = 0;
  f->name = (char *) NULL;
  f->mode = (char *) NULL;
  return BOXTASK_OK;
}

static Task My_File_Destroy(BoxVMX *vm) {
  File *f = BOX_VM_THIS_PTR(vm, File);
  if (f->opened) {
    fclose(f->file);
    f->opened = 0;
  }
  BoxMem_Free(f->name);
  BoxMem_Free(f->mode);
  f->name = (char *) NULL;
  f->mode = (char *) NULL;
  return BOXTASK_OK;
}

static Task My_File_Close(BoxVMX *vm) {
  File *f = BOX_VM_THIS_PTR(vm, File);
  if (!f->opened && f->name != NULL) {
    const char *open_mode = (f->mode == NULL) ? "rt" : f->mode;
    f->file = fopen(f->name, open_mode);
    f->opened = (f->file != NULL);
    if (!f->opened)
      MSG_ERROR("Error opening the file \"%s\" (mode=\"%s\").",
                f->name, open_mode);

    BoxMem_Free(f->name);
    BoxMem_Free(f->mode);
    f->name = (char *) NULL;
    f->mode = (char *) NULL;
    return (f->opened) ? BOXTASK_OK : BOXTASK_FAILURE;

  } else
    return BOXTASK_OK;
}

static Task My_File_Str(BoxVMX *vm) {
  File *f = BOX_VM_THIS_PTR(vm, File);
  BoxStr *s = BOX_VM_ARG_PTR(vm, BoxStr);

  if (f->opened) {
    /* write string to disk */
    const char *s_ptr = BoxStr_Get_Ptr(s);
    size_t s_len = BoxStr_Get_Size(s);
    if (s_ptr != NULL && s_len > 0)
      fwrite(s_ptr, s_len, sizeof(char), f->file);

  } else {
    if (f->name == NULL) {
      assert(f->mode == NULL);
      f->name = BoxStr_To_C_String(s);

    } else if (f->mode == NULL) {
      assert(f->mode == NULL);
      f->mode = BoxStr_To_C_String(s);

    } else {
      MSG_ERROR("File just takes only two string arguments: "
                "File[filename, mode].");
      return BOXTASK_FAILURE;
    }
  }

  return BOXTASK_OK;
}

void Bltin_IO_Register(BoxCmp *c) {
  /* Register the new type File */
  BoxTypeId t_file = BLTIN_NEW_TYPE(c, "File", File);
  Bltin_Proc_Def(c, t_file,  BOXTYPE_CREATE, My_File_Create);
  Bltin_Proc_Def(c, t_file,  BOXTYPE_DESTROY, My_File_Destroy);
  Bltin_Proc_Def(c, t_file,  BOXTYPE_END, My_File_Close);
  Bltin_Proc_Def(c, t_file, c->bltin.string, My_File_Str);
}

void Bltin_IO_Unregister(BoxCmp *c) {
}
