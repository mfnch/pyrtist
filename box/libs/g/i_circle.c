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

#include <math.h>
#include <assert.h>

#include "types.h"
#include "virtmach.h"
#include "graphic.h"
#include "g.h"
#include "i_window.h"

Task circle_color(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  w->circle.color = BOX_VM_ARG1(vmp, Color);
  w->circle.got.color = 1;
  return Success;
}

Task circle_begin(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  w->circle.got.radius_a = GOT_NOT;
  w->circle.got.radius_b = GOT_NOT;
  w->circle.got.center = GOT_NOT;
  w->circle.got.color = 0;
  return Success;
}

Task circle_point(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  Point *center = BOX_VM_ARGPTR1(vmp, Point);
  if (w->circle.got.center == GOT_NOW) {
    g_error("You already specified a center for the circle.");
    return Failed;
  }
  w->circle.center = *center;
  w->circle.got.center = GOT_NOW;
  return Success;
}

Task circle_real(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  Real radius = BOX_VM_ARG1(vmp, Real);
  if (w->circle.got.radius_b == GOT_NOW) {
    g_error("You already specified radius_a and radius_b for the circle!");
    return Failed;

  } else if (w->circle.got.radius_a == GOT_NOW) {
    w->circle.radius_b = radius;
    w->circle.got.radius_b = GOT_NOW;

  } else {
    w->circle.radius_b = w->circle.radius_a = radius;
    w->circle.got.radius_a = GOT_NOW;
  }
  return Success;
}

Task circle_pause(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);

  if (w->circle.got.center == GOT_NOT || w->circle.got.radius_a == GOT_NOT) {
    g_error("To draw a circle you have to specify at least "
            "the center (a point) the radius (a real)");
    return Failed;

  } else {
    Point c, a, b;
    c = w->circle.center;
    a.x = c.x + w->circle.radius_a;
    b.y = c.y + w->circle.radius_b;
    a.y = c.y;
    b.x = c.x;

    grp_window *cur_win = grp_win;
    grp_win = w->window;
    grp_rreset();
    grp_rcircle(& c, & a, & b);
    if (w->circle.got.color) {
      Color *color = & w->circle.color;
      grp_rfgcolor(color->r, color->g, color->b);
      w->circle.got.color = 0;
    }
    grp_rdraw(DRAW_EOFILL);
    grp_rreset();
    grp_win = cur_win;

    w->circle.got.center = (w->circle.got.center == GOT_NOT)
                           ? GOT_NOT : GOT_BEFORE;
    w->circle.got.radius_a = (w->circle.got.radius_a == GOT_NOT)
                             ? GOT_NOT : GOT_BEFORE;
    w->circle.got.radius_b = (w->circle.got.radius_b == GOT_NOT)
                             ? GOT_NOT : GOT_BEFORE;
    return Success;
  }
}

Task circle_end(VMProgram *vmp) {
  return circle_pause(vmp);
}
