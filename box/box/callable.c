/****************************************************************************
 * Copyright (C) 2012 by Matteo Franchin                                    *
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

#include <box/types.h>
#include <box/mem.h>
#include <box/obj.h>
#include <box/callable.h>
#include <box/vm.h>
#include <box/vmproc.h>

#include <box/core_priv.h>
#include <box/callable_priv.h>

#include <box/vmsym.h>
#include <box/vmsymstuff.h>

/* Initialize a callable object as an undefined callable. */
void
BoxCallable_Init_As_Undefined(BoxCallable *cb) {
  cb->uid = NULL;
  cb->kind = BOXCALLABLEKIND_UNDEFINED;
  BoxPtr_Init(& cb->context);
}

/* Finalize a callable object. */
void BoxCallable_Finish(BoxCallable *cb) {
  if (cb->uid)
    BoxMem_Free(cb->uid);
  BoxPtr_Unlink(& cb->context);
}

/* Create an undefined callable */
BOXOUT BoxCallable *
BoxCallable_Create_Undefined(BoxType *t_out, BoxType *t_in) {
  BoxType *t_cb = BoxType_Create_Callable(t_out, t_in);
  BoxCallable *cb = NULL;
  if (t_cb) {
    cb = BoxSPtr_Raw_Alloc(t_cb, sizeof(BoxCallable));
    if (cb)
      BoxCallable_Init_As_Undefined(cb);

    BoxSPtr_Unlink(t_cb);
  }

  return cb;
}

/* Create an undefined callable copying the type from an exising callable. */
BOXOUT BoxCallable *
BoxCallable_Create_Similar(BoxCallable *cb) {
  BoxCallable *new_cb = NULL;
  BoxType *t_cb = BoxSPtr_Get_Type(cb);
  if (t_cb) {
    new_cb = BoxSPtr_Raw_Alloc(t_cb, sizeof(BoxCallable));
    if (new_cb)
      BoxCallable_Init_As_Undefined(new_cb);
  }

  return new_cb;
}

/* Set the unique identifier for the callable. */
BoxBool
BoxCallable_Set_Uid(BoxCallable *cb, BoxUid *uid) {
  if (!(cb && !cb->uid))
    return BOXBOOL_FALSE;

  cb->uid = BoxMem_Strdup(uid);
  return BOXBOOL_TRUE;
}

/* Return the unique identifier associated to the given callable. */
BoxUid *
BoxCallable_Get_Uid(BoxCallable *cb) {
  return (cb) ? cb->uid : NULL;
}

/* Set the callable context (useful for creating closures). */
void
BoxCallable_Set_Context(BoxCallable *cb, BOXIN BoxPtr *context) {
  (void) BoxPtr_Unlink(& cb->context);
  if (context)
    cb->context = *context;
  else
    BoxPtr_Init(& cb->context);
}

/* Initialize a callable object from a BoxCCall1 C function. */
BOXOUT BoxCallable *
BoxCallable_Define_From_CCall1(BOXIN BoxCallable *cb, BoxCCall1 call) {
  if (!cb)
    return NULL;

  assert(cb->kind == BOXCALLABLEKIND_UNDEFINED);
  cb->kind = BOXCALLABLEKIND_C_1;
  cb->implem.c_call_1 = call;
  return cb;
}

/* Initialize a callable object from a BoxCCall2 C function. */
BOXOUT BoxCallable *
BoxCallable_Define_From_CCall2(BOXIN BoxCallable *cb, BoxCCall2 call) {
  if (!cb)
    return NULL;

  assert(cb->kind == BOXCALLABLEKIND_UNDEFINED);
  cb->kind = BOXCALLABLEKIND_C_2;
  cb->implem.c_call_2 = call;
  return cb;
}

/* Initialize a callable object from a BoxCCallOld C function. */
BOXOUT BoxCallable *
BoxCallable_Define_From_CCallOld(BOXIN BoxCallable *cb, BoxCCallOld c_old) {
  if (!cb)
    return NULL;

  assert(cb->kind == BOXCALLABLEKIND_UNDEFINED);
  cb->kind = BOXCALLABLEKIND_C_OLD;
  cb->implem.c_old = c_old;
  return cb;
}

/* Initialize a callable object from a BoxCCall3 C function. */
void BoxCallable_Define_From_CCall3(BoxCallable *cb, BoxCCall3 call) {
  assert(cb->kind == BOXCALLABLEKIND_UNDEFINED);
  cb->kind = BOXCALLABLEKIND_C_2;
  cb->implem.c_call_3 = call;
}

/* Define a callable object from a BoxVM procedure. */
BOXOUT BoxCallable *
BoxCallable_Define_From_VM(BOXIN BoxCallable *cb, BoxVM *vm,
                           BoxVMCallNum num) {
  if (!cb)
    return NULL;

  if (cb->kind != BOXCALLABLEKIND_UNDEFINED) {
    (void) BoxSPtr_Unlink(cb);
    return NULL;
  }

  cb->kind = BOXCALLABLEKIND_VM;
  cb->implem.vm_call.vm = BoxVM_Link(vm);
  cb->implem.vm_call.call_num = num;
  return cb;
}

/* Return the call number for a VM callable. */
BoxBool
BoxCallable_Get_VM_Call_Num(BoxCallable *cb, BoxVM *vm, BoxVMCallNum *cn) {
  if (cb->kind == BOXCALLABLEKIND_VM && vm == cb->implem.vm_call.vm) {
    if (cn)
      *cn = cb->implem.vm_call.call_num;
    return BOXBOOL_TRUE; 
  }

  return BOXBOOL_FALSE;
}

/* Request a call number for a given callable. */
BoxBool
BoxCallable_Request_VM_Call_Num(BoxCallable *cb, BoxVM *vm, BoxVMCallNum *num,
                                BOXOUT BoxCallable **cb_out) {
  BoxVMCallNum new_num = BOXVMCALLNUM_NONE;
  BoxCallable *new_cb = NULL;

  switch (cb->kind) {
  case BOXCALLABLEKIND_UNDEFINED:
    {
      /* Here we allocate a new VM call number and we leave it undefined.
       * We define the undefined callable in cb and return it in cb_out.
       */
      new_num = BoxVM_Allocate_Call_Num(vm);
      if (new_num == BOXVMCALLNUM_NONE)
        break;

      new_cb = BoxCallable_Define_From_VM(BoxCallable_Link(cb), vm, new_num);
      if (!new_cb)
        break;

      /* Alert the virtual machine that this call number is being used and
       * needs therefore to be resolved.
       */
      BoxVMSym_Reference_Proc(vm, new_num, new_cb);

      *num = new_num;
      *cb_out = new_cb;
      return BOXBOOL_TRUE;
    }

  case BOXCALLABLEKIND_C_1:
  case BOXCALLABLEKIND_C_2:
  case BOXCALLABLEKIND_C_3:
  case BOXCALLABLEKIND_C_OLD:
    {
      /* Here we just allocate a new call number, register it so that it points
       * to the C implementation in cb and create a new BoxCallable for the
       * VM call. This is finally what is returned in cb_out.
       */

      /* Allocate a new call number. */
      new_num = BoxVM_Allocate_Call_Num(vm);
      if (new_num == BOXVMCALLNUM_NONE)
        break;

      /* Create a new VM callable and define it. */
      new_cb = BoxCallable_Create_Similar(cb);
      if (!new_cb)
        break;

      new_cb = BoxCallable_Define_From_VM(new_cb, vm, new_num);
      if (!new_cb)
        break;

      /* Register the callable with the VM. */
      if (!BoxVM_Install_Proc_Callable(vm, new_num, cb))
        break;
      
      *num = new_num;
      *cb_out = new_cb;
      return BOXBOOL_TRUE;
    }

  case BOXCALLABLEKIND_VM:
    {
      /* Check whether we do already have a call number for this VM. */
      if (vm == cb->implem.vm_call.vm) {
        /* We do: we can return it. */
        *num = cb->implem.vm_call.call_num;
        *cb_out = NULL;
        return BOXBOOL_TRUE;

      } else {
        /* We don't have a call number for the required VM. */

        /* This is the case where we have more than one VM and one of them
         * may be calling a procedure defined in the other.
         * While we want to support this in the future, for now we just abort.
         */
        abort();
      }
    }
    break;
  }

  /* Failure: release resources and exit. */
  if (new_num != BOXVMCALLNUM_NONE)
    (void) BoxVM_Deallocate_Call_Num(vm, new_num);

  if (!new_cb)
    (void) BoxCallable_Unlink(new_cb);

  return BOXBOOL_FALSE;
}

/* Whether the callable is implemented. */
BoxBool
BoxCallable_Is_Implemented(BoxCallable *cb) {
  /* The loop below may never return, if the BoxCallable is badly formed. */
  while (1) {
    switch (cb->kind) {
    case BOXCALLABLEKIND_UNDEFINED:
      return BOXBOOL_FALSE;
    case BOXCALLABLEKIND_C_1:
    case BOXCALLABLEKIND_C_2:
    case BOXCALLABLEKIND_C_3:
    case BOXCALLABLEKIND_C_OLD:
      return BOXBOOL_TRUE;
    case BOXCALLABLEKIND_VM:
      {
        BoxVM *vm = cb->implem.vm_call.vm;
        BoxVMCallNum call_num = cb->implem.vm_call.call_num;
        switch (BoxVM_Get_Proc_Kind(vm, call_num)) {
        case BOXVMPROCKIND_UNDEFINED:
        case BOXVMPROCKIND_RESERVED:
          return BOXBOOL_FALSE;
        case BOXVMPROCKIND_FOREIGN:
          if (!BoxVM_Get_Callable_Implem(vm, call_num, & cb))
            return BOXBOOL_FALSE;
          break;
        default:
          return BOXBOOL_TRUE;
        }
        break;
      }
    default:
      return BOXBOOL_FALSE;
    }
  }

  /* Does not return. */
  abort();
  return BOXBOOL_FALSE;
}

/* Create a callable object from a BoxCCall2 C function. */
BOXOUT BoxException *
BoxCallable_Call1(BoxCallable *cb, BoxPtr *parent) {
  switch (cb->kind) {
  case BOXCALLABLEKIND_UNDEFINED:
    return BoxException_Create();
  case BOXCALLABLEKIND_C_1:
    return cb->implem.c_call_1(parent);
  case BOXCALLABLEKIND_C_2:
    {
      BoxPtr null;
      BoxPtr_Init(& null);
      return cb->implem.c_call_2(parent, & null);
    }
  case BOXCALLABLEKIND_C_3:
    {
      BoxPtr callable, null;
      BoxPtr_Init(& null);
      BoxPtr_Init_From_SPtr(& callable, cb);
      return cb->implem.c_call_3(& callable, parent, & null);
    }
  case BOXCALLABLEKIND_VM:
    assert(0);
    return NULL;

  }
  return BoxException_Create();
}

/* Create a callable object from a BoxCCall2 C function. */
BOXOUT BoxException *
BoxCallable_Call2(BoxCallable *cb, BoxPtr *parent, BoxPtr *child) {
  switch (cb->kind) {
  case BOXCALLABLEKIND_UNDEFINED:
    return BoxException_Create();
  case BOXCALLABLEKIND_C_1:
    return cb->implem.c_call_1(parent);
  case BOXCALLABLEKIND_C_2:
    return cb->implem.c_call_2(parent, child);
  case BOXCALLABLEKIND_C_3:
    {
      BoxPtr callable;
      BoxPtr_Init_From_SPtr(& callable, cb);
      return cb->implem.c_call_3(& callable, parent, child);
    }
  case BOXCALLABLEKIND_VM:
    /*BoxVM_Module_Execute_With_Args(vmx, cb->call_num, parent, child);*/
    assert(0);
    return NULL;
  }
  return BoxException_Create();
}

#include "messages.h"

BoxTask
BoxCallable_CallOld(BoxCallable *cb, BoxVMX *vmx) {
  if (cb->kind == BOXCALLABLEKIND_C_OLD)
    return cb->implem.c_old(vmx);
  MSG_FATAL("Call to new-style procedure is not supported, yet.");
  return BOXTASK_FAILURE;
}
