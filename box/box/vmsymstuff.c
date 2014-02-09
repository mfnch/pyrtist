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

#include <assert.h>

#include "types.h"
#include "mem.h"
#include "vm_priv.h"
#include "vmsym.h"
#include "vmsymstuff.h"
#include "container.h"
#include "callable.h"

/*** callables **************************************************************/

/* Temporary, very ugly stuff... */

/**
 * @brief Data necessary to define a procedure.
 */
typedef struct {
  BoxCallable *callable;
  BoxCCallOld fn_ptr;
} MyProcDef;

BoxBool
BoxVMSym_Proc_Is_Implemented(BoxVM *vm, BoxVMSymID sym_id,
                             const char **c_name) {
  MyProcDef *proc_def = BoxVMSym_Get_Definition(vm, sym_id);
  assert(proc_def);
  *c_name = BoxCallable_Get_Uid(proc_def->callable);
  return BoxCallable_Is_Implemented(proc_def->callable);
}

BoxBool
BoxVMSym_Define_Proc(BoxVM *vm, BoxVMSymID sym_id, BoxCCallOld fn_ptr) {
  MyProcDef *proc_def = BoxVMSym_Get_Definition(vm, sym_id);
  assert(proc_def);

  if (fn_ptr)
    proc_def->fn_ptr = fn_ptr;

  if (BoxVMSym_Define(vm, sym_id, NULL) != BOXTASK_OK)
    return BOXBOOL_FALSE;
  return BOXBOOL_TRUE;
}

static BoxTask
My_Resolve_Proc_Ref(BoxVM *vm, BoxVMSymID sym_id, BoxUInt sym_type,
                    int defined, void *def, size_t def_size,
                    void *ref, size_t ref_size) {
  MyProcDef *proc_def = def;
  BoxCallable *cb;
  BoxTask success;

  assert(sym_type == BOXVMSYMTYPE_PROC && proc_def && defined);

  if (BoxCallable_Is_Implemented(proc_def->callable))
    return BOXTASK_OK;

  cb = BoxCallable_Link(proc_def->callable);
  cb = BoxCallable_Define_From_CCallOld(cb, proc_def->fn_ptr);
  success = (cb) ? BOXTASK_OK : BOXTASK_FAILURE;
  (void) BoxCallable_Unlink(cb);
  return success;
}

/*
 */
BoxBool
BoxVMSym_Reference_Proc(BoxVM *vm, BoxCallable *cb) {
  MyProcDef proc_def;
  const char *name = BoxCallable_Get_Uid(cb);

  assert(vm);

  //if (BoxCallable_Is_Implemented(cb))
  //  return BOXBOOL_TRUE;

  proc_def.callable = BoxCallable_Link(cb);

  /* Create a new PROC symbol. */
  BoxVMSymID sym_id = BoxVMSym_Create(vm, BOXVMSYMTYPE_PROC,
                                      & proc_def, sizeof(MyProcDef));
  assert(sym_id != BOXVMSYMID_NONE);

  if (name)
    BoxVMSym_Set_Name(vm, sym_id, name);

  /* Create a new reference to the symbol. */
  BoxVMSym_Ref(vm, sym_id, My_Resolve_Proc_Ref, NULL, 0, BOXVMSYM_AUTO);
  return BOXBOOL_TRUE;
}
