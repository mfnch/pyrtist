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
#include "i_line.h"

#include "debug.h"
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
  LineStyle *ls = & w->line.this_piece.style;
  (*ls)[0] = 0.0; (*ls)[1] = 0.0; (*ls)[2] = 0.0; (*ls)[3] = 0.0;
  {
    grp_window *cur_win = grp_win;
    grp_win = w->window;
    /* Mando le impostazioni alla libreria grafica */
    grp_join_style(& w->line.this_piece.style[0]);
    grp_win = cur_win;
  }

  w->line.this_piece.width1 = w->line.this_piece.width2 = 1.0;
  w->line.this_piece.arrow = (void *) NULL;
  w->line.this_piece.arrow_scale = 1.0;
  return Success;
}

Task line_color(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  w->line.color = BOX_VM_ARG1(vmp, Color);
  w->line.got.color = 1;
  return Success;
}

void line_window_destroy(Window *w) {
  lt_destroy(w->line.lt);
}

Task line_begin(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  w->line.state = GOT_NOTHING;

  lt_clear(w->line.lt);
  w->line.num_points = 0;
  w->line.close = 0;
  w->line.got.color = 0;
  w->line.this_piece.arrow_scale = 1.0;
  return Success;
}

Task line_end(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  if (lt_num_pieces(w->line.lt) < 1)
    return Success;

  else {
    grp_window *cur_win = grp_win;
    grp_win = w->window;
    if (w->line.got.color) {
      Color *c = & w->line.color;
      grp_rfgcolor(c->r, c->g, c->b);
    }
    lt_draw(w->line.lt, w->line.close);
    grp_win = cur_win;
    return Success;
  }
}

Task line_real(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  Real width = BOX_VM_ARG1(vmp, Real);

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

Task line_point(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  Point *p = BOX_VM_ARGPTR1(vmp, Point);

  w->line.state = GOT_POINT;
  w->line.this_piece.point = *p;
  lt_add_piece(w->line.lt, & w->line.this_piece);

  ++w->line.num_points;
  w->line.this_piece.width1 = w->line.this_piece.width2;
  w->line.this_piece.arrow = (void *) NULL;
  return Success;
}

Task line_pause(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  grp_window *cur_win = grp_win;
  grp_win = w->window;
  if (w->line.got.color) {
    Color *c = & w->line.color;
    grp_rfgcolor(c->r, c->g, c->b);
    w->line.got.color = 0;
  }
  (void) lt_draw(w->line.lt, w->line.close);
  grp_win = cur_win;

  w->line.state = GOT_NOTHING;
  w->line.num_points = 0;
  w->line.close = 0;
  lt_clear(w->line.lt);
  return Success;
}

Task line_window(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  WindowPtr *wp = BOX_VM_ARGPTR1(vmp, WindowPtr);

  w->line.this_piece.arrow = (void *) *wp;
  return Success;
}

/* For now the same style is applied to the whole line.
 * I think we can mix styles for the same line. FIXME
 */
Task line_style(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  LineStyle *ls = BOX_VM_ARGPTR1(vmp, LineStyle);
  grp_window *cur_win = grp_win;
  w->line.this_piece.style[0] = (*ls)[0];
  w->line.this_piece.style[1] = (*ls)[1];
  w->line.this_piece.style[2] = (*ls)[2];
  w->line.this_piece.style[3] = (*ls)[3];
  grp_win = w->window;
  grp_join_style(& w->line.this_piece.style[0]);
  grp_win = cur_win;
  return Success;
}

Task window_line_close_begin(VMProgram *vmp) {
  SUBTYPE2_OF_WINDOW(vmp, w);
  w->line.close = 1;
  return Success;
}

Task window_line_close_int(VMProgram *vmp) {
  SUBTYPE2_OF_WINDOW(vmp, w);
  w->line.close = BOX_VM_ARG1(vmp, Int);
  return Success;
}

Task window_line_arrowscale(VMProgram *vmp) {
  SUBTYPE2_OF_WINDOW(vmp, w);
  w->line.this_piece.arrow_scale = BOX_VM_ARG1(vmp, Real);
  return Success;
}
