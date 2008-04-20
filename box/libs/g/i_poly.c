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

/*#define DEBUG*/

Task poly_color(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  w->poly.color = BOX_VM_ARG1(vmp, Color);
  w->poly.got.color = 1;
  return Success;
}

Task poly_begin(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  grp_window *cur_win = grp_win;
  grp_win = w->window;
  grp_rreset();
  grp_win = cur_win;

  /* Stato dell'istruzione = iniziale */
  w->poly.state = POLY_GOT_NOTHING;
  w->poly.margin[0] = 0.0;
  w->poly.margin[1] = 0.0;
  w->poly.num_points = 0;
  w->poly.num_margins = 0;
  w->poly.got.color = 0;
  return Success;
}

static Task _poly_point(Window *w, Point *p, int omit_line) {
  WindowPoly *wp = & w->poly;
  grp_window *cur_win = grp_win;

  if (wp->num_points < 2)
    wp->first_points[wp->num_points] = *p;

  if (wp->num_points > 0) {
    Point *last = & wp->last_point, lastb, pb;
    Real dx = p->x - last->x, dy = p->y - last->y, d = sqrt(dx*dx + dy*dy),
         m1 = wp->margin[0], m2 = wp->margin[1], m;

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
      grp_rcong(wp->lastb, *last, lastb);
    }

    if (!omit_line) {
#ifdef DEBUG
      printf("Line (%g, %g) (%g, %g)\n",
             lastb.x, lastb.y, pb.x, pb.y);
#endif
      grp_rline(lastb, pb);
    }
    grp_win = cur_win;

    wp->lastb = pb;
  }

  wp->last_point = *p;
  ++wp->num_points;
  w->poly.num_margins = 0;
  return Success;
}

Task poly_point(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  Point *p = BOX_VM_ARGPTR1(vmp, Point);
  return _poly_point(w, p, 0);
}

Task poly_end(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  grp_window *cur_win = grp_win;

  TASK( _poly_point(w, & w->poly.first_points[0], 0) );
  TASK( _poly_point(w, & w->poly.first_points[1], 1) );

  grp_win = w->window;
  if (w->poly.got.color) {
    Color *c = & w->poly.color;
    grp_rfgcolor(c->r, c->g, c->b);
    w->poly.got.color = 0;
  }
  grp_rdraw(DRAW_EOFILL);
  grp_rreset();
  grp_win = cur_win;
  return Success;
}

Task poly_pause(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  grp_window *cur_win = grp_win;

  TASK( _poly_point(w, & w->poly.first_points[0], 0) );
  TASK( _poly_point(w, & w->poly.first_points[1], 1) );

  grp_win = w->window;
  if (w->poly.got.color) {
    Color *c = & w->poly.color;
    grp_rfgcolor(c->r, c->g, c->b);
    w->poly.got.color = 0;
  }
  grp_rdraw(DRAW_EOFILL);
  grp_rreset();
  grp_win = cur_win;

  /* Stato dell'istruzione = iniziale */
  w->poly.state = POLY_GOT_NOTHING;
  w->poly.num_points = 0;
  w->poly.num_margins = 0;
  w->poly.got.color = 0;
  return Success;
}

Task poly_real(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  Real margin = BOX_VM_ARG1(vmp, Real);

  if (w->poly.num_margins > 1) {
    g_warning("Enough margins: ignoring Real value.");
    return Success;
  }

  w->poly.margin[w->poly.num_margins++] = margin;
  return Success;
}
