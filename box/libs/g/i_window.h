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

#ifndef _I_WINDOW_H
#  define _I_WINDOW_H
#  include "types.h"
#  include "g.h"
#  include "buffer.h"
#  include "pointlist.h"

#  define _DEF_WINDOW_SUBOBJECTS
#  include "i_line.h"
#  include "i_circle.h"
#  include "i_poly.h"
#  include "i_put.h"
#  include "i_text.h"
#  undef _DEF_WINDOW_SUBOBJECTS

typedef struct {
  struct {
    int type : 1;
    int size : 1;
  } have;

  enum {BM1, BM4, BM8, FIG, PS} type;
  Point size, origin, res;

  grp_window *window;

  PointList pointlist;
  OptColor fg_color;

  WindowLine line;
  WindowCircle circle;
  WindowPoly poly;
  WindowPut put;
  WindowText text;

  struct {
    struct {
      int point : 1;
      int name : 1;
    } got;
    char *name;
  } hot;

  char *save_file_name;
} Window;

typedef void *WindowPtr;


#  define SUBTYPE_OF_WINDOW(vmp, w) \
    Window *w = *( (Window **) \
      SUBTYPE_PARENT_PTR(BOX_VM_CURRENTPTR(vmp, Subtype), WindowPtr) );

#  define SUBTYPE2_OF_WINDOW(vmp, w) \
    Window *w = *( (Window **) \
      SUBTYPE_PARENT_PTR( \
        SUBTYPE_PARENT_PTR(BOX_VM_CURRENTPTR(vmp, Subtype), Subtype), \
        WindowPtr) );

#  define PROC_OF_WINDOW_SUBTYPE(vmp, w, child, child_type) \
    Window *w = *( (Window **) \
      SUBTYPE_PARENT_PTR(BOX_VM_CURRENTPTR(vmp, Subtype), Window *) ); \
    child_type *child = ( (child_type *) \
      SUBTYPE_CHILD_PTR(BOX_VM_CURRENTPTR(vmp, Subtype), child_type) )

#endif
