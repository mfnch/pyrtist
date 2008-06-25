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
#include "virtmach.h"
#include "i_gradient.h"
#include "graphic.h"
#include "g.h"

Task gradient_begin(VMProgram *vmp) {
  GradientPtr *g_ptr = BOX_VM_THIS_PTR(vmp, GradientPtr);
  Gradient *g;
  g = *g_ptr = (Gradient *) malloc(sizeof(Gradient));
  if (g == (Gradient *) NULL) return Failed;
  if (!buff_create(& g->items, sizeof(ColorGradItem), 8))
    return Failed;
  g->got.type = 0;
  g->got.point1 = 0;
  g->got.point2 = 0;
  g->got.pause = 0;
  return Success;
}

Task gradient_destroy(VMProgram *vmp) {
  Gradient *g = BOX_VM_THIS(vmp, GradientPtr);
  buff_free(& g->items);
  free(g);
  return Success;
}

static Task set_gradient_type(Gradient *g, ColorGradType t) {
  if (g->got.type && g->gradient.type != t) {
    g_error("Cannot change the gradient type!");
    return Failed;
  }
  g->got.type = 1;
  g->gradient.type = t;
  return Success;
}

Task gradient_line_point(VMProgram *vmp) {
  Gradient *g = BOX_VM_SUB_PARENT(vmp, GradientPtr);
  Point *p = BOX_VM_ARG1_PTR(vmp, Point);
  set_gradient_type(g, COLOR_GRAD_TYPE_LINEAR);
  if (!g->got.point1) {
    g->gradient.point1 = *p;
    g->got.point1 = 1;
    return Success;

  } else if (!g->got.point2) {
    g->gradient.point2 = *p;
    g->got.point2 = 1;
    return Success;

  } else {
    g_warning("Gradient.Line takes just two points!");
    return Success;
  }
}

Task gradient_circle_point(VMProgram *vmp) {
  Gradient *g = BOX_VM_SUB_PARENT(vmp, GradientPtr);
  Point *p = BOX_VM_ARG1_PTR(vmp, Point);
  set_gradient_type(g, COLOR_GRAD_TYPE_RADIAL);
  if (!g->got.pause) {
    if (g->got.point1) {
      g_warning("Already got the center for the first circle: "
                "ignoring this other value!");
      return Success;
    }
    g->gradient.point1 = *p;
    g->got.point1 = 1;

  } else {
    if (g->got.point2) {
      g_warning("Already got the center for the second circle: "
                "ignoring this other value!");
      return Success;
    }
    g->gradient.point2 = *p;
    g->got.point2 = 1;
  }
  return Success;
}

Task gradient_circle_real(VMProgram *vmp) {
  Gradient *g = BOX_VM_SUB_PARENT(vmp, GradientPtr);
  set_gradient_type(g, COLOR_GRAD_TYPE_RADIAL);
  return Success;
}

Task gradient_real(VMProgram *vmp) {
  Gradient *g = BOX_VM_SUB_PARENT(vmp, GradientPtr);
  g->got.pause = 1;
  return Success;
}

Task gradient_color(VMProgram *vmp) {
  return Success;
}

