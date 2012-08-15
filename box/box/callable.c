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

#include <box/ntypes.h>
#include <box/core_priv.h>
#include <box/obj.h>
#include <box/callable.h>

struct BoxCallable_struct {
  char            *uid;     /**< Unique identifier for the function. */
  BoxCallableKind kind;     /**< How the callable is internally implemented. */
  BoxPtr          context;  /**< Pointer to callable private data. */
  union {
    BoxCCall1     c_call_1;
    BoxCCall2     c_call_2;
    BoxCCall3     c_call_3;
  }               implem;   /**< Implementation data. */
};

/* Initialize a callable object from a BoxCCall1 C function. */
void BoxCallable_Init_From_CCall1(BoxCallable *cb, BoxCCall1 call) {
  cb->uid = NULL;
  cb->kind = BOXCALLABLEKIND_C_1;
  cb->implem.c_call_1 = call;
  BoxPtr_Init(& cb->context);
}

/* Initialize a callable object from a BoxCCall2 C function. */
void BoxCallable_Init_From_CCall2(BoxCallable *cb, BoxCCall2 call) {
  cb->uid = NULL;
  cb->kind = BOXCALLABLEKIND_C_2;
  cb->implem.c_call_2 = call;
  BoxPtr_Init(& cb->context);
}

/* Initialize a callable object from a BoxCCall3 C function. */
void BoxCallable_Init_From_CCall3(BoxCallable *cb, BoxCCall3 call,
                                  BOXIN BoxPtr *context) {
  cb->uid = NULL;
  cb->kind = BOXCALLABLEKIND_C_2;

  if (context)
    cb->context = *context;
  else
    BoxPtr_Init(& cb->context);

  cb->implem.c_call_3 = call;
}

#if 0

/* Initialize a callable object from a BoxCCall3 C function. */
void BoxCallable_Init_From_CCall3(BoxCallable *cb, BoxCCall3 call,
                                  BOXIN BoxPtr *context) {
  cb->uid = NULL;
  cb->kind = BOXCALLABLEKIND_C_2;

  if (context)
    cb->context = *context;
  else
    BoxPtr_Init(& cb->context);

  cb->implem.vm_call.vm = (BoxVM *) thing;
  cb->implem.vm_call.call_num = (BoxVMCallNum) thing;
}

#endif

/* Create a callable object from a BoxCCall1 C function. */
BOXOUT BoxCallable *
BoxCallable_Create_From_CCall1(BoxXXXX *t_out, BoxXXXX *t_in, BoxCCall1 call) {
  BoxXXXX *t_cb = BoxType_Create_Function(t_in, t_out);
  BoxCallable *cb = BoxSPtr_Raw_Alloc(t_cb, sizeof(BoxCallable));
  BoxCallable_Init_From_CCall1(cb, call);
  BoxSPtr_Unlink(t_cb);
  return cb;
}

/* Create a callable object from a BoxCCall2 C function. */
BOXOUT BoxCallable *
BoxCallable_Create_From_CCall2(BoxXXXX *t_out, BoxXXXX *t_in, BoxCCall2 call) {
  BoxXXXX *t_cb = BoxType_Create_Function(t_in, t_out);
  BoxCallable *cb = BoxSPtr_Raw_Alloc(t_cb, sizeof(BoxCallable));
  BoxCallable_Init_From_CCall2(cb, call);
  BoxSPtr_Unlink(t_cb);
  return cb;
}

/* Create a callable object from a BoxCCall3 C function. */
BOXOUT BoxCallable *
BoxCallable_Create_From_CCall3(BoxXXXX *t_out, BoxXXXX *t_in,
                               BOXIN BoxPtr *context, BoxCCall3 call) {
  BoxXXXX *t_cb = BoxType_Create_Function(t_in, t_out);
  BoxCallable *cb = BoxSPtr_Raw_Alloc(t_cb, sizeof(BoxCallable));
  BoxCallable_Init_From_CCall3(cb, call, context);
  BoxSPtr_Unlink(t_cb);
  return cb;
}










/* Create a callable object from a BoxCCall2 C function. */
BoxException *
BoxCallable_Call1(BoxCallable *cb, BoxPtr *parent) {
  switch (cb->kind) {
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
  }
  return BoxException_Create();
}

/* Create a callable object from a BoxCCall2 C function. */
BoxException *
BoxCallable_Call2(BoxCallable *cb, BoxPtr *parent, BoxPtr *child) {
  switch (cb->kind) {
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
  }
  return BoxException_Create();
}
