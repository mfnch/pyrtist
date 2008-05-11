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

#include <stdlib.h>
#include <math.h>

#include <cairo.h>

#include "error.h"
#include "types.h"
#include "graphic.h"
#include "g.h"
#include "wincairo.h"

static void wincairo_close_win(void) {
  cairo_t *cr = (cairo_t *) grp_win->ptr;
  cairo_surface_t *surface = cairo_get_target(cr);
  cairo_destroy(cr);
  cairo_surface_destroy(surface);
}

/* Variabili usate dalle procedure per scrivere il file postscript */
static int beginning_of_line = 1, beginning_of_path = 1;
static Point previous;

static int same_points(Point *a, Point *b) {
  return (fabs(a->x - b->x) < 1e-10 && fabs(a->y - b->y) < 1e-10);
}

static void wincairo_rreset(void) {
  beginning_of_line = 1;
  beginning_of_path = 1;
}

static void wincairo_rinit(void) {}

static void wincairo_rdraw(DrawStyle style) {
  cairo_t *cr = (cairo_t *) grp_win->ptr;

  if ( ! beginning_of_path ) {
    switch(style) {
    case DRAW_FILL:
      cairo_set_fill_rule(cr, CAIRO_FILL_RULE_WINDING);
      cairo_fill(cr);
      break;

    case DRAW_EOFILL:
      cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD);
      cairo_fill(cr);
      break;

    case DRAW_CLIP:
      cairo_set_fill_rule(cr, CAIRO_FILL_RULE_WINDING);
      cairo_clip(cr);
      break;

    case DRAW_EOCLIP:
      cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD);
      cairo_clip(cr);
      break;

    default:
      g_warning("Unsupported drawing style: using even-odd fill algorithm!");
      cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD);
      cairo_fill(cr);
      break;
    }
  }
}

static void wincairo_rfgcolor(Real r, Real g, Real b) {
  cairo_t *cr = (cairo_t *) grp_win->ptr;
  cairo_set_source_rgb(cr, r, g, b);
}

static void wincairo_rline(Point *a, Point *b) {
  cairo_t *cr = (cairo_t *) grp_win->ptr;
  int continuing = same_points(a, & previous),
      length_zero = same_points(a, b);

  if (continuing && length_zero) return;

  if (beginning_of_path) {
    cairo_new_path(cr);
    beginning_of_path = 0;
    continuing = 0;
  }

  if (!continuing) cairo_move_to(cr, a->x, a->y);

  cairo_line_to(cr, b->x, b->y);
  previous = *b;
}

static void wincairo_rcong(Point *a, Point *b, Point *c) {
  cairo_t *cr = (cairo_t *) grp_win->ptr;
#if 0
  int a_eq_b = ax == bx && ay == by,
      a_eq_c = ax == cx && ay == cy,
      b_eq_c = bx == cx && by == cy,
      n_eq = a_eq_b + a_eq_c + b_eq_c;
  if (n_eq == 3) return;
#endif
  if (same_points(a, c)) return;

#if 0
  if (beginning_of_path) {
    cairo_new_path(cr);
    beginning_of_path = 0;
  }

  fprintf( (FILE *) grp_win->ptr,
   " %ld %ld %ld %ld %ld %ld cong", ax, ay, bx, by, cx, cy );

  previous = *c;
  beginning_of_line = 0;
#else
  wincairo_rline(a, b);
  wincairo_rline(b, c);
#endif
}

static void wincairo_rcircle(Point *ctr, Point *a, Point *b) {
  cairo_t *cr = (cairo_t *) grp_win->ptr;

  if (beginning_of_path)
    cairo_new_path(cr);

  /*fprintf( (FILE *) grp_win->ptr,
   " %ld %ld %ld %ld %ld %ld circle", cx, cy, ax, ay, bx, by );*/

  beginning_of_line = 1;
  beginning_of_path = 0;
}

static int wincairo_save(const char *file_name) {
  cairo_t *cr = (cairo_t *) grp_win->ptr;
  cairo_surface_t *surface = cairo_get_target(cr);
  char *exts[] = {"png", "pdf", (char *) NULL};
  enum {EXT_PNG=0};
  cairo_status_t status;

  switch(file_extension(exts, file_name)) {
  default:
    g_warning("Unrecognized extension: using PNG!");
  case EXT_PNG:
#ifdef CAIRO_HAS_PNG_FUNCTIONS
    status = cairo_surface_write_to_png(surface, file_name);
#else
    g_error("Cairo has been compiled without PNG support!");
    return 0;
#endif
  }

  switch(status) {
  case CAIRO_STATUS_SUCCESS:
    return 1;

  default:
    g_error("Cannot save the window!");
    return 0;
  }
}

/** Set the default methods to the cairo windows */
static void wincairo_repair(GrpWindow *w) {
  grp_window_block(w);
  w->save = wincairo_save;
  w->close_win = wincairo_close_win;
  w->rreset = wincairo_rreset;
  w->rinit = wincairo_rinit;
  w->rdraw = wincairo_rdraw;
  w->rfgcolor = wincairo_rfgcolor;
  w->rline = wincairo_rline;
  w->rcong = wincairo_rcong;
  w->rcircle = wincairo_rcircle;
}

GrpWindow *cairo_open_win(GrpWindowPlan *plan) {
  GrpWindow *w;
  cairo_surface_t *surface;
  cairo_t *cr;
  cairo_status_t status;
  int numptx, numpty;

  if ((w = (GrpWindow *) malloc(sizeof(GrpWindow))) == (GrpWindow *) NULL) {
    g_error("cairo_open_win: malloc failed!");
    return (GrpWindow *) NULL;
  }

  if (! (plan->have.size && plan->have.resolution) ) {
    g_error("Cannot create Cairo image surface: size or resolution missing!");
    return (GrpWindow *) NULL;
  }

  numptx = plan->size.x * plan->resolution.x;
  numpty = plan->size.y * plan->resolution.y;

  surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, numptx, numpty);
  status = cairo_surface_status(surface);
  if (status != CAIRO_STATUS_SUCCESS) {
    g_error("Cannot create Cairo image surface:");
    g_error(cairo_status_to_string(status));
    return (GrpWindow *) NULL;
  }
  cr = cairo_create(surface);
  status = cairo_status(cr);
  if (status != CAIRO_STATUS_SUCCESS) {
    g_error("Cannot create Cairo context:");
    g_error(cairo_status_to_string(status));
    return (GrpWindow *) NULL;
  }

  w->ptr = (void *) cr;

  /* Ora do' le procedure per gestire la finestra */
  w->quiet = 0;
  w->repair = wincairo_repair;
  w->repair(w);
  w->win_type_str = "cairo";
  return w;

#if 0
  grp_window *wd;
  FILE *winstream;
  Real size_x_psunit, size_y_psunit;
  int x_max, y_max;

  /* Should express the size of the window in postscript units 1/72 of inch */
  /* 1 inch = 25.4 mm */
  size_x_psunit = (size_x / grp_mm_per_inch)/grp_inch_per_psunit;
  size_y_psunit = (size_y / grp_mm_per_inch)/grp_inch_per_psunit;
  x_max = (int) size_x_psunit+1;
  y_max = (int) size_y_psunit+1;
#endif
  return (GrpWindow *) NULL;
}
