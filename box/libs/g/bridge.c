/****************************************************************************
 * Copyright (C) 2011 by Matteo Franchin                                    *
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

/* The way we interface Box and C is by assuming Box creates C consistent
 * datastructures. For example, a Box declaration
 *
 *   MyType = (Real a, b, Str c, Int d)
 *
 * Is assumed to be binary compatible with the C structure:
 *
 *   struct {BoxReal a, b; BoxStr c; BoxInt d;}
 *
 * It is a task of the Box compiler implementation to behave this way.  The
 * problem now is that we may end up definining the same object two times: once
 * in Box code and once in C code. What happens when one changes only one of
 * the two definitions (and forgets about the other). Easy: a segfault! It is
 * then important to provide a way to check consistency between the interface
 * and this is what we do here. In particular, we organize things like this:
 * bridge.h contains all the C definitions of the Box objects defined in the G
 * library which need to be passed directly to C code. bridge.c (this file)
 * provides functions to check the consistency of the C and Box definitions.
 */

#include <box/types.h>
#include <box/print.h>
#include <box/virtmach.h>

#include "bridge.h"

static BoxTask My_Error(BoxVM *vm, char *object, BoxTask t) {
  if (t != BOXTASK_FAILURE)
    return t;

  else {
    char *msg = Box_SPrintF("Found inconsistency between C and Box "
                            "definitions of of the object '%s'.", object);
    BoxVM_Set_Fail_Msg(vm, msg);
    BoxMem_Free(msg);
    return BOXTASK_FAILURE;
  }
}

static BoxTask My_Is_Expected_Matrix(BoxGMatrix *mx) {
  BoxGMatrix emx;
  emx.m11 = 1.00;
  emx.m12 = 2.25;
  emx.m13 = 3.50;
  emx.m21 = 4.75;
  emx.m22 = 6.00;
  emx.m23 = 6.25;

  return ((mx->m11 == emx.m11 && mx->m12 == emx.m12 && mx->m13 == emx.m13 &&
           mx->m21 == emx.m21 && mx->m22 == emx.m22 && mx->m23 == emx.m23) ?
          BOXTASK_OK : BOXTASK_FAILURE);
}

BoxTask Box_G_Lib_Bridge_Test_Matrix(BoxVM *vm) {
  BoxGMatrix *mx = BoxVM_Get_Child_Target(vm);
  return My_Error(vm, "Matrix", My_Is_Expected_Matrix(mx));
}

BoxTask Box_G_Lib_Bridge_Test_SimplePut(BoxVM *vm) {
  BoxGSimplePut *sp = BoxVM_Get_Child_Target(vm);
  return My_Error(vm, "SimplePut", My_Is_Expected_Matrix(& sp->matrix));
}

BoxTask Box_G_Lib_Bridge_Window_At_TestWindow(BoxVM *vm) {
  BoxCPtr *cptr = BoxVM_Get_Parent_Target(vm);
  BoxGWindow *w = BoxVM_Get_Child_Target(vm);
  *cptr = *w;
  return BOXTASK_OK;
}

BoxTask Box_G_Lib_Bridge_SimplePut_At_TestWindow(BoxVM *vm) {
  BoxCPtr *cptr = BoxVM_Get_Parent_Target(vm);
  BoxGSimplePut *sp = BoxVM_Get_Child_Target(vm);
  return My_Error(vm, "SimplePut[.src]",
                  (*cptr == sp->src) ? BOXTASK_OK : BOXTASK_FAILURE);
}
