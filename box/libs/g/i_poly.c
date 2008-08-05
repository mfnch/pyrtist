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
#include "i_pointlist.h"
#include "i_gradient.h"

/*#define DEBUG*/

#if 0
/** See VMCall */
typedef Task (*VMCallable)(VMProgram *vmp);

/** This should become part of the VM library. For now we put it there. */
Task VM_Call_C(VMProgram *vmp, VMCallable f, void *obj, void *arg);

Task VM_Call_C(VMProgram *vmp, VMCallable f, void *obj, void *arg) {
  void *save_obj, *save_arg;
  Task t;
  save_obj = *vmp->box_vm_current;
  save_arg = *vmp->box_vm_arg1;
  *vmp->box_vm_current = obj;
  *vmp->box_vm_arg1 = arg;
  t = f(vmp);
  *vmp->box_vm_current = save_obj;
  *vmp->box_vm_arg1 = save_arg;
  return t;
}
#endif

Task poly_color(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  w->poly.color = BOX_VM_ARG1(vmp, Color);
  w->poly.got.color = 1;
  return Success;
}

Task poly_gradient(VMProgram *vmp) {
  return x_gradient(vmp);
}

Task poly_begin(VMProgram *vmp) {
  Window *w = BOX_VM_SUB_PARENT(vmp, WindowPtr);
  IPointListPtr *ipl_ptr = BOX_VM_SUB_CHILD_PTR(vmp, IPointListPtr);

  TASK( ipl_create(ipl_ptr) );

  GrpWindow *cur_win = grp_win;
  grp_win = w->window;
  grp_rreset();
  grp_win = cur_win;

  /* Stato dell'istruzione = iniziale */
  w->poly.state = POLY_GOT_NOTHING;
  w->poly.close = 0;
  w->poly.margin[0] = 0.0;
  w->poly.margin[1] = 0.0;
  w->poly.num_points = 0;
  w->poly.num_margins = 0;
  w->poly.got.color = 0;

  g_style_new(& w->poly.style, & w->style);
  return Success;
}

static Task _poly_point_draw_only(Window *w, Point *p, int omit_line) {
  WindowPoly *wp = & w->poly;
  GrpWindow *cur_win = grp_win;
  Real m1 = wp->margin[0], m2 = wp->margin[1];

  if (wp->num_points < 2) {
    wp->first_points[wp->num_points] = *p;
    if (wp->num_points == 1) {
      wp->first_margins[0] = m1;
      wp->first_margins[1] = m2;
    }
  }

  if (wp->num_points > 0) {
    Point *last = & wp->last_point, lastb, pb;
    Real dx = p->x - last->x, dy = p->y - last->y, d = sqrt(dx*dx + dy*dy), m;

    if (d > 0.0) {
      if (m1 < 0.0) m1 = -m1/d;
      if (m2 < 0.0) m2 = -m2/d;
    } else {
      if (m1 < 0.0) m1 = 0.0;
      if (m2 < 0.0) m2 = 0.0;
    }
    m = m1 + m2;
    if (m > 1.0) {
      g_warning("Margins for Poly segment exceed the length "
                "of the whole segment");
      m1 /= m;
      m2 /= m;
    }

    lastb.x = dx * m1 + last->x;
    lastb.y = dy * m1 + last->y;
    pb.x = -dx * m2 + p->x;
    pb.y = -dy * m2 + p->y;

    grp_win = w->window;
    if (wp->num_points > 1) {
#ifdef DEBUG
      printf("Corner (%g, %g) (%g, %g) (%g, %g)\n",
             wp->lastb.x, wp->lastb.y, last->x, last->y, lastb.x, lastb.y);
#endif
      grp_rcong(& wp->lastb, last, & lastb);
    }

    if (!omit_line) {
#ifdef DEBUG
      printf("Line (%g, %g) (%g, %g)\n",
             lastb.x, lastb.y, pb.x, pb.y);
#endif
      grp_rline(& lastb, & pb);
    }
    grp_win = cur_win;

    wp->lastb = pb;
  }

  wp->last_point = *p;
  ++wp->num_points;
  w->poly.num_margins = 0;
  wp->margin[0] = m2;
  wp->margin[1] = m1;
  return Success;
}

static Task _poly_point(Window *w, IPointList *ipl, Point *p) {
  PointList *pl = IPL_POINTLIST(ipl);
  TASK( pointlist_add(pl, p, (char *) NULL) );
  return _poly_point_draw_only(w, p, 0);
}

Task poly_point(VMProgram *vmp) {
  Window *w = BOX_VM_SUB_PARENT(vmp, WindowPtr);
  IPointList *ipl = BOX_VM_SUB_CHILD(vmp, IPointListPtr);
  Point *p = BOX_VM_ARGPTR1(vmp, Point);
  return _poly_point(w, ipl, p);
}

Task poly_style(VMProgram *vmp) {
  IStyle *s = BOX_VM_ARG(vmp, IStylePtr);
  SUBTYPE_OF_WINDOW(vmp, w);
  g_style_copy_selected(& w->poly.style, & s->style, s->have);
  return Success;
}

static Task _poly_draw(Window *w, DrawWhen dw) {
  GrpWindow *cur_win = grp_win;
  WindowPoly *wp = & w->poly;
  FillStyle *fs;
  int close = wp->close;

  fs = g_style_get_fill_style(& w->poly.style, & w->poly.default_style);
  if (fs != (FillStyle *) NULL)
    if (*fs != FILLSTYLE_VOID) close = 1;

  grp_win = w->window;
  if (close) {
    TASK( _poly_point_draw_only(w, & w->poly.first_points[0], 0) );
    wp->margin[0] = wp->first_margins[0];
    wp->margin[1] = wp->first_margins[1];
    TASK( _poly_point_draw_only(w, & w->poly.first_points[1], 1) );
    grp_rclose();
  }

  if (w->poly.got.color) {
    Color *c = & w->poly.color;
    grp_rfgcolor(c);
    w->poly.got.color = 0;
  }

  (void) g_rdraw(& w->poly.style, & w->poly.default_style, dw);
  grp_win = cur_win;
  return Success;
}

Task poly_end(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  TASK(_poly_draw(w, DRAW_WHEN_END));
  g_style_clear(& w->poly.style);
  return Success;
}

Task poly_pause(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  TASK(_poly_draw(w, DRAW_WHEN_PAUSE));

  /* Restore the status to the initial status */
  w->poly.state = POLY_GOT_NOTHING;
  w->poly.num_points = 0;
  w->poly.num_margins = 0;
  w->poly.got.color = 0;
  return Success;
}

Task poly_real(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  Real margin = BOX_VM_ARG1(vmp, Real), max_margin;

  switch(w->poly.num_margins++) {
  case 0:
    if (margin < 0.0) margin = 0.0;
    if (margin > 1.0) margin = 1.0;
    w->poly.margin[0] = margin;
    w->poly.margin[1] = (margin < 0.5) ? margin : (1.0 - margin);
    break;

  case 1:
    max_margin = 1.0 - w->poly.margin[0];
    if (margin > max_margin) margin = max_margin;
    if (margin < 0.0) margin = 0.0;
    w->poly.margin[1] = margin;
    break;

  default:
    g_warning("Enough margins: ignoring Real value.");
    return Success;
  }
  return Success;
}

struct add_from_pl_params {
  Window *w;
  IPointList *dest_ipl;
};

static Task _add_from_pl(Int index, char *name, void *object, void *data) {
  struct add_from_pl_params *params = (struct add_from_pl_params *) data;
  Point *p = (Point *) object;
  return _poly_point(params->w, params->dest_ipl, p);
}

Task poly_pointlist(VMProgram *vmp) {
  Window *w = BOX_VM_SUB_PARENT(vmp, WindowPtr);
  IPointList *my_ipl = BOX_VM_SUB_CHILD(vmp, IPointListPtr);
  IPointList *arg_ipl = BOX_VM_ARG1(vmp, IPointListPtr);
  PointList *arg_pl = IPL_POINTLIST(arg_ipl);
  struct add_from_pl_params params;
  if (my_ipl == arg_ipl) {
    g_error("can't add a PointList object to itself.");
    return Failed;
  }
  params.w = w;
  params.dest_ipl = my_ipl;
  return pointlist_iter(arg_pl, _add_from_pl, & params);
}

Task window_poly_close_begin(VMProgram *vmp) {
  Window *w = BOX_VM_SUB2_PARENT(vmp, WindowPtr);
  w->poly.close = 1;
  return Success;
}

Task window_poly_close_int(VMProgram *vmp) {
  Window *w = BOX_VM_SUB2_PARENT(vmp, WindowPtr);
  w->poly.close = BOX_VM_ARG1(vmp, Int);
  return Success;
}
