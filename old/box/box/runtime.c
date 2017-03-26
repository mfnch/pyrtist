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

#include <box/runtime.h>

static BoxTypeId My_Create_Intrinsic(size_t size, size_t algn, char *name) {
  BoxTypeId it = BoxType_Create_Intrinsic(size, algn);
  return BoxType_Create_Ident(it, name);
}

/* Create the root type. */
BoxTypeId Box_Create_Root_Type(void) {
  BoxTypeId t_root, t_char, t_int, t_real, t_point, t_ptr;

  t_root = BoxType_Create_Ident(NULL, "Root");

  t_char = My_Create_Intrinsic(sizeof(BoxChar), __alignof__(BoxChar), "Char");
  t_int = My_Create_Intrinsic(sizeof(BoxInt), __alignof__(BoxInt), "Int");
  t_real = My_Create_Intrinsic(sizeof(BoxReal), __alignof__(BoxReal), "Real");
  t_point = My_Create_Intrinsic(sizeof(BoxPoint),
                                __alignof__(BoxPoint), "Point");
  t_ptr = My_Create_Intrinsic(sizeof(BoxPtr), __alignof__(BoxPtr), "Ptr");

  BoxType_Add_Type(t_root, t_char);
  BoxType_Add_Type(t_root, t_int);
  BoxType_Add_Type(t_root, t_real);
  BoxType_Add_Type(t_root, t_point);
  BoxType_Add_Type(t_root, t_ptr);

  return NULL;
}
