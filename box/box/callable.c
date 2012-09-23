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
#include <box/obj.h>
#include <box/callable.h>
#include <box/vm.h>

#include <box/core_priv.h>
#include <box/callable_priv.h>


BOXOUT BoxCallable *
BoxCallable_Create_Undefined(BoxType *t_out, BoxType *t_in) {
  BoxType *t_cb = BoxType_Create_Function(t_in, t_out);
  BoxCallable *cb = NULL;
  if (t_cb) {
    cb = BoxSPtr_Raw_Alloc(t_cb, sizeof(BoxCallable));
    if (cb) {
      cb->uid = NULL;
      cb->kind = BOXCALLABLEKIND_UNDEFINED;
      BoxPtr_Init(& cb->context);      
    }

    BoxSPtr_Unlink(t_cb);
  }

  return cb;
}

/* Set the unique identifier for the callable. */
BoxBool
BoxCallable_Set_Uid(BoxCallable *cb, BoxUid *uid) {
  if (!(cb && !cb->uid))
    return BOXBOOL_FALSE;

  cb->uid = uid;
  return BOXBOOL_TRUE;
}

/* Return the unique identifier associated to the given callable. */
BoxUid *
BoxCallable_Get_Uid(BoxCallable *cb) {
  return (cb) ? cb->uid : NULL;
}

/* Initialize a callable object as an undefined callable. */
void BoxCallable_Init_As_Undefined(BoxCallable *cb) {
  cb->uid = NULL;
  cb->kind = BOXCALLABLEKIND_UNDEFINED;
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
  BoxPtr_Init(& cb->context);
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
  BoxPtr_Init(& cb->context);
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
  BoxPtr_Init(& cb->context);
  return cb;
}

/* Initialize a callable object from a BoxCCall3 C function. */
void BoxCallable_Define_From_CCall3(BoxCallable *cb, BoxCCall3 call,
                                    BOXIN BoxPtr *context) {
  assert(cb->kind == BOXCALLABLEKIND_UNDEFINED);
  cb->kind = BOXCALLABLEKIND_C_2;

  if (context)
    cb->context = *context;
  else
    BoxPtr_Init(& cb->context);

  cb->implem.c_call_3 = call;
}

#define BoxVM_Link(x) (x)

/* Define a callable object from a BoxVM procedure. */
BOXOUT BoxCallable *
BoxCallable_Define_From_VM(BOXIN BoxCallable *cb, BOXIN BoxPtr *context,
                           BoxVM *vm, BoxVMCallNum num) {
  if (!cb)
    return NULL;

  if (cb->kind != BOXCALLABLEKIND_UNDEFINED) {
    (void) BoxSPtr_Unlink(cb);
    return NULL;
  }

  cb->kind = BOXCALLABLEKIND_VM;

  if (context)
    cb->context = *context;
  else
    BoxPtr_Init(& cb->context);

  cb->implem.vm_call.vm = BoxVM_Link(vm);
  cb->implem.vm_call.call_num = num;
  return cb;
}


BoxBool
BoxCallable_Get_VM_CallNum(BoxCallable *cb, BoxVM *vm, BoxVMCallNum *cn) {
  if (cb->kind == BOXCALLABLEKIND_VM && vm == cb->implem.vm_call.vm) {
    if (cn)
      *cn = cb->implem.vm_call.call_num;
    return BOXBOOL_TRUE; 
  }

  return BOXBOOL_FALSE;
}

BoxVMCallNum BoxVM_Get_Call_Num(BoxVM *vm, BoxCallable *cb) {
  /* If the callable represent a procedure of `vm', we just return the
   * corresponding call number.
   */
  if (cb->kind == BOXCALLABLEKIND_VM && vm == cb->implem.vm_call.vm)
    return cb->implem.vm_call.call_num;

  return BOXVMCALLNUM_NONE;
}

#if 0
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
        if (BoxVM_Get_Code_Implem(vm, call_num, NULL))
          return BOXBOOL_TRUE;
        if (!BoxVM_Get_Callable_Implem(vm, call_num, & cb))
          return BOXBOOL_FALSE;
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
#endif

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



#if 0

BoxBool BoxCallable_Register_With_VM(BoxCallable *cb, BoxVM *vm, BoxVMCallNum *num, BOXOUT BoxCallable **cb);

#endif
