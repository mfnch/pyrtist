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
#include <box/vm_private.h>
#include "graphic.h"
#include "g.h"
#include "i_window.h"
#include "i_style.h"
#include "pointlist.h"
#include "i_pointlist.h"
#include "i_gradient.h"

Task circle_color(BoxVMX *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  w->circle.color = BOX_VM_ARG1(vmp, Color);
  w->circle.got.color = 1;
  return BOXTASK_OK;
}

Task circle_gradient(BoxVMX *vmp) {
  return x_gradient(vmp);
}

Task circle_begin(BoxVMX *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  w->circle.got.radius_a = GOT_NOT;
  w->circle.got.radius_b = GOT_NOT;
  w->circle.got.center = GOT_NOT;
  w->circle.got.color = 0;

  g_style_new(& w->circle.style, & w->style);
  return BOXTASK_OK;
}

static Task _circle_point(Window *w, Point *center) {
  if (w->circle.got.center == GOT_NOW) {
    if (!w->circle.got.radius_a) {
      Real dx = w->circle.center.x - center->x,
           dy = w->circle.center.y - center->y;
      Real radius = sqrt(dx*dx + dy*dy);
      w->circle.radius_b = w->circle.radius_a = radius;
      w->circle.got.radius_a = GOT_NOW;
      return BOXTASK_OK;

    } else {
      g_error("You already specified a center for the circle.");
      return BOXTASK_OK;
    }
  }
  w->circle.center = *center;
  w->circle.got.center = GOT_NOW;
  return BOXTASK_OK;
}

Task circle_point(BoxVMX *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  Point *center = BOX_VM_ARG1_PTR(vmp, Point);
  return _circle_point(w, center);
}

Task circle_real(BoxVMX *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  Real radius = BOX_VM_ARG1(vmp, Real);
  if (w->circle.got.radius_b == GOT_NOW) {
    g_warning("You already specified radius_a and radius_b for the circle!");
    return BOXTASK_OK;

  } else if (w->circle.got.radius_a == GOT_NOW) {
    w->circle.radius_b = radius;
    w->circle.got.radius_b = GOT_NOW;

  } else {
    w->circle.radius_b = w->circle.radius_a = radius;
    w->circle.got.radius_a = GOT_NOW;
  }
  return BOXTASK_OK;
}

Task circle_style(BoxVMX *vmp) {
  IStyle *s = BOX_VM_ARG(vmp, IStylePtr);
  SUBTYPE_OF_WINDOW(vmp, w);
  g_style_copy_selected(& w->circle.style, & s->style, s->have);
  return BOXTASK_OK;
}

static Task _circle_draw(Window *w, DrawWhen when) {
  if (w->circle.got.center == GOT_NOT || w->circle.got.radius_a == GOT_NOT) {
    /*g_warning("To draw a circle you have to specify at least "
                "the center (a point) the radius (a real)");*/
    return BOXTASK_OK;

  } else {
    Point c, a, b;
    c = w->circle.center;
    a.x = c.x + w->circle.radius_a;
    b.y = c.y + w->circle.radius_b;
    a.y = c.y;
    b.x = c.x;

    BoxGWin_Add_Circle_Path(w->window, & c, & a, & b);
    if (w->circle.got.color) {
      BoxGWin_Set_Fg_Color(w->window, & w->circle.color);
      w->circle.got.color = 0;
    }
    (void) BoxGWin_Draw_With_Style(w->window, & w->circle.style,
                                   & w->circle.default_style, when);

    w->circle.got.center = (w->circle.got.center == GOT_NOT)
                           ? GOT_NOT : GOT_BEFORE;
    w->circle.got.radius_a = (w->circle.got.radius_a == GOT_NOT)
                             ? GOT_NOT : GOT_BEFORE;
    w->circle.got.radius_b = (w->circle.got.radius_b == GOT_NOT)
                             ? GOT_NOT : GOT_BEFORE;
    return BOXTASK_OK;
  }
}

Task circle_pause(BoxVMX *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  return _circle_draw(w, DRAW_WHEN_PAUSE);
}

Task circle_end(BoxVMX *vmp) {
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
  BOXTASK( _circle_point(params->w, center) );
  if (index == params->num_points) return BOXTASK_OK;
  return _circle_draw(params->w, DRAW_WHEN_PAUSE);
}

Task circle_pointlist(BoxVMX *vmp) {
  Window *w = BOX_VM_SUB_PARENT(vmp, WindowPtr);
  IPointList *arg_ipl = BOX_VM_ARG1(vmp, IPointListPtr);
  struct params_for_add_from_pl params;
  PointList *arg_pl = IPL_POINTLIST(arg_ipl);
  params.w = w;
  params.num_points = pointlist_num(arg_pl);
  return pointlist_iter(arg_pl, _add_from_pl, & params);
}
