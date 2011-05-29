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

static void My_Got_Point(BoxGWin *w, Real x, Real y) {
  Point p = {x, y};
  Grp_BB_Must_Contain(& bb_local, & p);
}

static void My_BB_Finish_Drawing(BoxGWin *w) {}

static void My_BB_Draw_Path(BoxGWin *w, DrawStyle *style) {
  int do_border = (style->bord_width > 0.0);
  Real scale = style->scale;
  if (do_border)
    Grp_BB_Margin(& bb_local, 0.5*scale*style->bord_width);

  Grp_BB_Fuse(& bb_global, & bb_local);
  Grp_BB_Init(& bb_local);
}

static void My_BB_Add_Line_Path(BoxGWin *w, Point *a, Point *b) {
#ifdef DEBUG
  printf("line\n");
#endif
  My_Got_Point(w, a->x, a->y); My_Got_Point(w, b->x, b->y);
}

static void My_BB_Add_Join_Path(BoxGWin *w, Point *a, Point *b, Point *c) {
#ifdef DEBUG
  printf("cong\n");
#endif
  My_Got_Point(w, a->x, a->y); My_Got_Point(w, b->x, b->y);
  My_Got_Point(w, c->x, c->y); My_Got_Point(w, a->x + c->x - b->x, a->y + c->y - b->y);
}

static void My_BB_Add_Circle_Path(BoxGWin *w, Point *ctr, Point *a, Point *b) {
  Point va, vb;
#ifdef DEBUG
  printf("circle\n");
#endif
  va.x = a->x - ctr->x; va.y = a->y - ctr->y;
  vb.x = b->x - ctr->x; vb.y = b->y - ctr->y;

  My_Got_Point(w, a->x + vb.x, a->y + vb.y);
  My_Got_Point(w, a->x - vb.x, a->y - vb.y);
  My_Got_Point(w, ctr->x - va.x - vb.x, ctr->y - va.y - vb.y);
  My_Got_Point(w, ctr->x - va.x + vb.x, ctr->y - va.y + vb.y);

}

static void My_BB_Add_Text_Path(BoxGWin *w, BoxPoint *ctr, BoxPoint *left,
                                BoxPoint *up, BoxPoint *from,
                                const char *text) {
  My_Got_Point(w, ctr->x, ctr->y);
  My_Got_Point(w, left->x, left->y);
  My_Got_Point(w, up->x, up->y);
}

static void My_BB_Add_Fake_Point(BoxGWin *w, Point *p) {
  My_Got_Point(w, p->x, p->y);
}

/** Set the default methods to the bb window */
static void bb_repair(BoxGWin *w) {
  BoxGWin_Block(w);
  w->draw_path = My_BB_Draw_Path;
  w->add_line_path = My_BB_Add_Line_Path;
  w->add_join_path = My_BB_Add_Join_Path;
  w->add_circle_path = My_BB_Add_Circle_Path;
  w->add_text_path = My_BB_Add_Text_Path;
  w->add_fake_point = My_BB_Add_Fake_Point;

  w->finish_drawing = My_BB_Finish_Drawing;
}

int bb_bounding_box(BoxGWin *figure, Point *bb_min, Point *bb_max) {
  BoxGWin bb;
  /* Ora do' le procedure per gestire la finestra */
  bb.quiet = 1;
  bb.repair = bb_repair;
  bb.repair(& bb);
  bb.win_type_str = "bb";
  Grp_BB_Init(& bb_global);
  Grp_BB_Init(& bb_local);
  BoxGWin_Fig_Draw_Fig(& bb, figure);
  Grp_BB_Fuse(& bb_global, & bb_local);
  if (bb_min != (Point *) NULL) *bb_min = bb_global.min;
  if (bb_max != (Point *) NULL) *bb_max = bb_global.max;
  return (Grp_BB_Volume(& bb_global) > 0.0);
}

