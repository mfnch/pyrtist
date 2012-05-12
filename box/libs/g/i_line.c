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

/* Interface for Line */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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
#include "i_gradient.h"
#include "i_line.h"

#include "error.h"
#include "buffer.h"
#include "fig.h"
#include "autoput.h"

#include "linetracer.h"

/*#define DEBUG*/

Task line_window_init(Window *w) {
  w->line.lt = lt_new();
  if (w->line.lt == (LineTracer *) NULL) {
    g_error("Cannot create the LineTracer object\n");
    return Failed;
  }

  /* We want these settings to be global:
   * for all lines of this window
   */
  LineJoinStyle *ls = & w->line.this_piece.style;
  ls->ti = ls->te = ls->ni = ls->ne = 0.0;

  /* Mando le impostazioni alla libreria grafica */
  lt_join_style_set(w->line.lt, ls);

  w->line.this_piece.width1 = w->line.this_piece.width2 = 1.0;
  w->line.this_piece.arrow = (void *) NULL;
  w->line.this_piece.arrow_scale = 1.0;
  return Success;
}

Task line_color(BoxVM *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  w->line.color = BOX_VM_ARG1(vmp, Color);
  w->line.got.color = 1;
  return Success;
}

Task line_gradient(BoxVM *vmp) {
  return x_gradient(vmp);
}

void line_window_destroy(Window *w) {
  lt_destroy(w->line.lt);
}

Task line_begin(BoxVM *vmp) {
  Window *w = BOX_VM_SUB_PARENT(vmp, WindowPtr);
  IPointListPtr *ipl_ptr = BOX_VM_SUB_CHILD_PTR(vmp, IPointListPtr);

  TASK( ipl_create(ipl_ptr) );

  w->line.state = GOT_NOTHING;

  lt_clear(w->line.lt);
  w->line.num_points = 0;
  w->line.close = 0;
  w->line.got.color = 0;
  w->line.this_piece.arrow_scale = 1.0;

  g_style_new(& w->line.style, & w->style);
  return Success;
}

Task line_end(BoxVM *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  if (lt_num_pieces(w->line.lt) >= 1) {
    if (w->line.got.color)
      BoxGWin_Set_Fg_Color(w->window, & w->line.color);

    lt_draw(w->window, w->line.lt, w->line.close);
    (void) BoxGWin_Draw_With_Style(w->window, & w->line.style,
                                   & w->line.default_style, DRAW_WHEN_END);
  }

  g_style_clear(& w->line.style);
  return Success;
}

Task line_real(BoxVM *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  Real width = 0.5*BOX_VM_ARG1(vmp, Real);

  switch(w->line.state) {
  case GOT_NOTHING:
    w->line.this_piece.width2 = w->line.this_piece.width1 = width;
    w->line.state = GOT_2ND_FLOAT;
    break;

  case GOT_POINT:
    w->line.this_piece.width2 = w->line.this_piece.width1 = width;
    w->line.state = GOT_1ST_FLOAT;
    break;

  case GOT_1ST_FLOAT:
    w->line.this_piece.width2 = width;
    w->line.state = GOT_2ND_FLOAT;
    break;

  case GOT_2ND_FLOAT:
    g_error("Too many width specificators.");
    return Failed;

  default:
    g_error("line_real: unknown line state.");
    break;
  }
  return Success;
}

Task line_point(BoxVM *vmp) {
  Window *w = BOX_VM_SUB_PARENT(vmp, WindowPtr);
  IPointList *ipl = BOX_VM_SUB_CHILD(vmp, IPointListPtr);
  Point *p = BOX_VM_ARG1_PTR(vmp, Point);
  PointList *pl = IPL_POINTLIST(ipl);

  w->line.state = GOT_POINT;
  w->line.this_piece.point = *p;
  lt_add_piece(w->line.lt, & w->line.this_piece);

  ++w->line.num_points;
  w->line.this_piece.width1 = w->line.this_piece.width2;
  w->line.this_piece.arrow = (void *) NULL;
  return pointlist_add(pl, p, (char *) NULL);
}

Task line_pause(BoxVM *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  if (w->line.got.color) {
    BoxGWin_Set_Fg_Color(w->window, & w->line.color);
    w->line.got.color = 0;
  }
  (void) lt_draw(w->window, w->line.lt, w->line.close);
  (void) BoxGWin_Draw_With_Style(w->window, & w->line.style,
                                 & w->line.default_style, DRAW_WHEN_PAUSE);

  w->line.state = GOT_NOTHING;
  w->line.num_points = 0;
  w->line.close = 0;
  lt_clear(w->line.lt);
  return Success;
}

Task line_window(BoxVM *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  WindowPtr *wp = BOX_VM_ARG1_PTR(vmp, WindowPtr);

  w->line.this_piece.arrow = (void *) *wp;
  return Success;
}

Task line_linestyle(BoxVM *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  Real *ls = BOX_VM_ARG1_PTR(vmp, Real);
  lt_join_style_from_array(& w->line.this_piece.style,
                           ls[0], ls[1], ls[2], ls[3]);
  return Success;
}


Task line_style(BoxVM *vmp) {
  IStyle *s = BOX_VM_ARG(vmp, IStylePtr);
  SUBTYPE_OF_WINDOW(vmp, w);
  g_style_copy_selected(& w->line.style, & s->style, s->have);
  return Success;
}

Task window_line_close_begin(BoxVM *vmp) {
  SUBTYPE2_OF_WINDOW(vmp, w);
  w->line.close = 1;
  return Success;
}

Task window_line_close_int(BoxVM *vmp) {
  SUBTYPE2_OF_WINDOW(vmp, w);
  w->line.close = BOX_VM_ARG1(vmp, Int);
  return Success;
}

Task window_line_arrowscale(BoxVM *vmp) {
  SUBTYPE2_OF_WINDOW(vmp, w);
  w->line.this_piece.arrow_scale = BOX_VM_ARG1(vmp, Real);
  return Success;
}
