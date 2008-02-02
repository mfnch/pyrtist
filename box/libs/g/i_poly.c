#include <math.h>
#include <assert.h>

#include "types.h"
#include "virtmach.h"
#include "graphic.h"
#include "g.h"
#include "i_window.h"

#define DEBUG

Task poly_window_init(Window *w) {
  g_optcolor_alternative_set(& w->poly.color, & w->fg_color);
  g_optcolor_unset(& w->poly.color);
  return Success;
}

Task poly_color(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  Color *c = BOX_VM_ARGPTR1(vmp, Color);
  return g_optcolor_set(& w->poly.color, c);
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
  return Success;
}

static Task _poly_point(Window *w, Point *p, int omit_line) {
  WindowPoly *wp = & w->poly;
  grp_window *cur_win = grp_win;

  if (wp->num_points < 2)
    wp->first_points[wp->num_points] = *p;

  if (wp->num_points > 0) {
    Point *last = & wp->last_point, lastb, pb;
    Real m1 = wp->margin[0], m2 = wp->margin[1];

    lastb.x = (p->x - last->x) * m1 + last->x;
    lastb.y = (p->y - last->y) * m1 + last->y;
    pb.x = -(p->x - last->x) * m2 + p->x;
    pb.y = -(p->y - last->y) * m2 + p->y;

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
  grp_rdraw();
  grp_rreset();
  grp_win = cur_win;
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
