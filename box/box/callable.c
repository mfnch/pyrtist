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

#include <box/types.h>
#include <box/ntypes.h>
#include <box/core.h>
#include <box/obj.h>
#include <box/callable.h>

struct BoxCallable_struct {
  BoxCallableKind kind;
  union {
    BoxCCall1 c_call_1;
    BoxCCall2 c_call_2;
    BoxCCall3 c_call_3;
  }               implem;
};

/* Initialize a callable object from a BoxCCall1 C function. */
void BoxCallable_Init_From_CCall1(BoxCallable *cb, BoxCCall1 call) {
  cb->kind = BOXCALLABLEKIND_C_1;
  cb->implem.c_call_1 = call;
}

/* Initialize a callable object from a BoxCCall2 C function. */
void BoxCallable_Init_From_CCall2(BoxCallable *cb, BoxCCall2 call) {
  cb->kind = BOXCALLABLEKIND_C_2;
  cb->implem.c_call_2 = call;
}

/* Create a callable object from a BoxCCall1 C function. */
BoxCallable *BoxCallable_Create_From_CCall1(BoxCCall1 call) {
  BoxCallable *cb = BoxSPtr_Raw_Alloc(box_core_types.callable_type,
                                      sizeof(BoxCallable));
  BoxCallable_Init_From_CCall1(cb, call);
  return cb;
}

/* Create a callable object from a BoxCCall2 C function. */
BoxCallable *BoxCallable_Create_From_CCall2(BoxCCall2 call) {
  BoxCallable *cb = BoxSPtr_Raw_Alloc(box_core_types.callable_type,
                                      sizeof(BoxCallable));
  BoxCallable_Init_From_CCall2(cb, call);
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
