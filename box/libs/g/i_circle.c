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
#include "i_style.h"
#include "pointlist.h"
#include "i_pointlist.h"

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

  g_style_new(& w->circle.style, & w->style);
  return Success;
}

static Task _circle_point(Window *w, Point *center) {
  if (w->circle.got.center == GOT_NOW) {
    g_error("You already specified a center for the circle.");
    return Failed;
  }
  w->circle.center = *center;
  w->circle.got.center = GOT_NOW;
  return Success;
}

Task circle_point(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  Point *center = BOX_VM_ARGPTR1(vmp, Point);
  return _circle_point(w, center);
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

Task circle_style(VMProgram *vmp) {
  IStyle *s = BOX_VM_ARG(vmp, IStylePtr);
  SUBTYPE_OF_WINDOW(vmp, w);
  g_style_copy_selected(& w->circle.style, & s->style, s->have);
  return Success;
}

static Task _circle_draw(Window *w, DrawWhen when) {
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
    grp_rcircle(& c, & a, & b);
    if (w->circle.got.color) {
      Color *color = & w->circle.color;
      grp_rfgcolor(color);
      w->circle.got.color = 0;
    }
    (void) g_rdraw(& w->circle.style, & w->circle.default_style, when);
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

Task circle_pause(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  return _circle_draw(w, DRAW_WHEN_PAUSE);
}

Task circle_end(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  Task t = _circle_draw(w, DRAW_WHEN_END);
  g_style_clear(& w->circle.style);
  return t;
}

struct params_for_add_from_pl {
  Int num_points;
  Window *w;
};

static Task _add_from_pl(Int index, char *name, void *object, void *data) {
  struct params_for_add_from_pl *params = (struct params_for_add_from_pl *) data;
  Point *center = (Point *) object;
  TASK( _circle_point(params->w, center) );
  if (index == params->num_points) return Success;
  return _circle_draw(params->w, DRAW_WHEN_PAUSE);
}

Task circle_pointlist(VMProgram *vmp) {
  Window *w = BOX_VM_SUB_PARENT(vmp, WindowPtr);
  IPointList *arg_ipl = BOX_VM_ARG1(vmp, IPointListPtr);
  struct params_for_add_from_pl params;
  PointList *arg_pl = IPL_POINTLIST(arg_ipl);
  params.w = w;
  params.num_points = pointlist_num(arg_pl);
  return pointlist_iter(arg_pl, _add_from_pl, & params);
}
