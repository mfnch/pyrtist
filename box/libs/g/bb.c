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

/* This window is a fake one, just to calculate the bounding box */

/* #define DEBUG */

#include <stdlib.h>
#include <stdio.h>

#include "error.h"
#include "types.h"
#include "graphic.h"
#include "fig.h"
#include "autoput.h"
#include "bb.h"

static Int bb_num_points;
static Real bb_min_x, bb_min_y, bb_max_x, bb_max_y;

static void got_point(Real x, Real y) {
#ifdef DEBUG
  printf("(%g, %g): from min=(%g, %g), max=(%g, %g); ", x, y, bb_min_x, bb_min_y, bb_max_x, bb_max_y);
#endif
  if (bb_num_points < 1) {
    bb_min_x = bb_max_x = x;
    bb_min_y = bb_max_y = y;

  } else {
    if (x < bb_min_x) bb_min_x = x;
    if (y < bb_min_y) bb_min_y = y;
    if (x > bb_max_x) bb_max_x = x;
    if (y > bb_max_y) bb_max_y = y;
  }
#ifdef DEBUG
  printf("to min=(%g, %g), max=(%g, %g)\n", bb_min_x, bb_min_y, bb_max_x, bb_max_y);
#endif
  ++bb_num_points;
}

static void not_available(void) {
  ERRORMSG("not_available", "Not available for bb windows.");
}

static void bb_close_win(void) {return;}

static void bb_rreset(void) {return;}

static void bb_rinit(void) {return;}

static void bb_rdraw(void) {return;}

static void bb_rline(Point a, Point b) {
#ifdef DEBUG
  printf("line\n");
#endif
  got_point(a.x, a.y); got_point(b.x, b.y);
}

static void bb_rcong(Point a, Point b, Point c) {
#ifdef DEBUG
  printf("cong\n");
#endif
  got_point(a.x, a.y); got_point(b.x, b.y);
  got_point(c.x, c.y); got_point(a.x + c.x - b.x, a.y + c.y - b.y);
}

static void bb_rcircle(Point ctr, Point a, Point b) {
  Point va, vb;
#ifdef DEBUG
  printf("circle\n");
#endif
  va.x = a.x - ctr.x; va.y = a.y - ctr.y;
  vb.x = b.x - ctr.x; vb.y = b.y - ctr.y;

  got_point(a.x + vb.x, a.y + vb.y);
  got_point(a.x - vb.x, a.y - vb.y);
  got_point(ctr.x - vb.x + va.x, ctr.y - va.y + vb.y);
  got_point(ctr.x - vb.x - va.x, ctr.y - va.y - vb.y);

}

static void bb_rfgcolor(Real r, Real g, Real b) {return;}

static void bb_rbgcolor(Real r, Real g, Real b) {return;}

static void bb_text(Point *p, const char *text) {got_point(p->x, p->y);}

static void bb_font(Point *p, const char *text) {return;}

static void bb_fake_point(Point *p) {got_point(p->x, p->y);}

static int bb_save(void) {return 1;}

/* Queste funzioni non sono disponibili per finestre postscript
 */
static void (*bb_lowfn[])() = {
  bb_close_win, not_available, not_available, not_available
};

/* Lista delle funzioni di rasterizzazione */
static void (*bb_midfn[])() = {
  bb_rreset, bb_rinit, bb_rdraw,
  bb_rline, bb_rcong, not_available,
  bb_rcircle, bb_rfgcolor, bb_rbgcolor,
  not_available, bb_text, bb_font,
  bb_fake_point
};

void bb_bounding_box(grp_window *figure, Point *bb_min, Point *bb_max) {
  grp_window *cur_win = grp_win, bb;
  bb.save = bb_save;
  bb.lowfn = bb_lowfn;
  bb.midfn = bb_midfn;
  bb_num_points = 0;
  grp_win = & bb;
  aput_identity_matrix(fig_matrix);
  fig_draw_fig(figure);
  grp_win = cur_win;
  if (bb_min != (Point *) NULL) {bb_min->x = bb_min_x; bb_min->y = bb_min_y;}
  if (bb_max != (Point *) NULL) {bb_max->x = bb_max_x; bb_max->y = bb_max_y;}
}

