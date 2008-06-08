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

static BB bb_global, bb_local;

static void got_point(Real x, Real y) {
  Point p = {x, y};
  Grp_BB_Must_Contain(& bb_local, & p);
}

static void bb_close_win(void) {return;}

static void bb_rdraw(DrawStyle *style) {
  int do_border = (style->bord_width > 0.0);
  if (do_border)
    Grp_BB_Margin(& bb_local, style->bord_width);

  Grp_BB_Fuse(& bb_global, & bb_local);
  Grp_BB_Init(& bb_local);
}

static void bb_rline(Point *a, Point *b) {
#ifdef DEBUG
  printf("line\n");
#endif
  got_point(a->x, a->y); got_point(b->x, b->y);
}

static void bb_rcong(Point *a, Point *b, Point *c) {
#ifdef DEBUG
  printf("cong\n");
#endif
  got_point(a->x, a->y); got_point(b->x, b->y);
  got_point(c->x, c->y); got_point(a->x + c->x - b->x, a->y + c->y - b->y);
}

static void bb_rcircle(Point *ctr, Point *a, Point *b) {
  Point va, vb;
#ifdef DEBUG
  printf("circle\n");
#endif
  va.x = a->x - ctr->x; va.y = a->y - ctr->y;
  vb.x = b->x - ctr->x; vb.y = b->y - ctr->y;

  got_point(a->x + vb.x, a->y + vb.y);
  got_point(a->x - vb.x, a->y - vb.y);
  got_point(ctr->x - vb.x + va.x, ctr->y - va.y + vb.y);
  got_point(ctr->x - vb.x - va.x, ctr->y - va.y - vb.y);

}

static void bb_text(Point *p, const char *text) {got_point(p->x, p->y);}

static void bb_fake_point(Point *p) {got_point(p->x, p->y);}

/** Set the default methods to the bb window */
static void bb_repair(GrpWindow *w) {
  grp_window_block(w);
  w->rdraw = bb_rdraw;
  w->rline = bb_rline;
  w->rcong = bb_rcong;
  w->rcircle = bb_rcircle;
  w->text = bb_text;
  w->fake_point = bb_fake_point;

  w->close_win = bb_close_win;
}

int bb_bounding_box(GrpWindow *figure, Point *bb_min, Point *bb_max) {
  GrpWindow *cur_win = grp_win, bb;
  /* Ora do' le procedure per gestire la finestra */
  bb.quiet = 1;
  bb.repair = bb_repair;
  bb.repair(& bb);
  bb.win_type_str = "bb";
  Grp_BB_Init(& bb_global);
  Grp_BB_Init(& bb_local);
  grp_win = & bb;
  aput_identity_matrix(fig_matrix);
  fig_draw_fig(figure);
  grp_win = cur_win;
  if (bb_min != (Point *) NULL) *bb_min = bb_global.min;
  if (bb_max != (Point *) NULL) *bb_max = bb_global.max;
  return (Grp_BB_Volume(& bb_global) > 0.0);
}

