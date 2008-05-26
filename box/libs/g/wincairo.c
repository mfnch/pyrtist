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
#include <assert.h>

#include <cairo.h>
#ifdef CAIRO_HAS_PS_SURFACE
#  include <cairo-ps.h>
#endif
#ifdef CAIRO_HAS_PDF_SURFACE
#  include <cairo-pdf.h>
#endif
#ifdef CAIRO_HAS_SVG_SURFACE
#  include <cairo-svg.h>
#endif

#include "error.h"
#include "types.h"
#include "graphic.h"
#include "g.h"
#include "wincairo.h"
#include "psfonts.h"

/*#define DEBUG*/

#ifdef DEBUG
#  define WHEREAMI \
   fprintf(stderr, "Inside '%s'\n", __func__), fflush(stderr)
#else
#  define WHEREAMI (void) 0
#endif

static char *wincairo_image_id_string   = "cairo:image";
static char *wincairo_stream_id_string  = "cairo:stream";

static void wincairo_close_win(void) {
  cairo_t *cr = (cairo_t *) grp_win->ptr;
  cairo_surface_t *surface = cairo_get_target(cr);
  WHEREAMI;
  cairo_destroy(cr);
  cairo_surface_destroy(surface);
}

/* Variabili usate dalle procedure per scrivere il file postscript */
static int beginning_of_line = 1, beginning_of_path = 1;
static Point previous;

static int same_points(Point *a, Point *b) {
  return (fabs(a->x - b->x) < 1e-10 && fabs(a->y - b->y) < 1e-10);
}

static void my_point(Point *in, Point *out) {
  GrpWindow *w = grp_win;
  WHEREAMI;
  out->x = (in->x - w->ltx)*w->resx;
  out->y = (in->y - w->lty)*w->resy;
}

/* Macros used to scale the point coordinates */
#define MY_POINT(a) \
  Point my_a; my_point((a), & my_a); (a) = & my_a

#define MY_2POINTS(a, b) \
  Point my_a, my_b; \
  my_point((a), & my_a); my_point((b), & my_b); \
  (a) = & my_a; (b) = & my_b

#define MY_3POINTS(a, b, c) \
  Point my_a, my_b, my_c; \
  my_point((a), & my_a); my_point((b), & my_b);  my_point((c), & my_c); \
  (a) = & my_a; (b) = & my_b; (c) = & my_c

/* This is broken, but is required for fonts (fonts support is broken as well
 * and needs to be better done: font drawing should depend on Points, not on
 * a given Real size!)
 */
#define MY_REAL(r) ((r)*grp_win->resx) \

static void wincairo_rreset(void) {
  WHEREAMI;
  beginning_of_line = 1;
  beginning_of_path = 1;
}

static void wincairo_rinit(void) {WHEREAMI;}

static void wincairo_rdraw(DrawStyle style) {
  cairo_t *cr = (cairo_t *) grp_win->ptr;
  WHEREAMI;

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

static void wincairo_rfgcolor(Color *c) {
  cairo_t *cr = (cairo_t *) grp_win->ptr;
  WHEREAMI;
  cairo_set_source_rgba(cr, c->r, c->g, c->b, c->a);
}

static void wincairo_rline(Point *a, Point *b) {
  cairo_t *cr = (cairo_t *) grp_win->ptr;
  MY_2POINTS(a, b);
  WHEREAMI;

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
  WHEREAMI;
#if 0
  int a_eq_b = ax == bx && ay == by,
      a_eq_c = ax == cx && ay == cy,
      b_eq_c = bx == cx && by == cy,
      n_eq = a_eq_b + a_eq_c + b_eq_c;
  if (n_eq == 3) return;
#endif
  cairo_t *cr = (cairo_t *) grp_win->ptr;
  Point *first = a, *last = c;
  MY_3POINTS(a, b, c);

  if (same_points(a, c))
    return;

  else if (same_points(a, b) || same_points(b, c)) {
    wincairo_rline(first, last);
    return;

  } else {
    cairo_matrix_t previous_m, m;

    if (beginning_of_path) {
      cairo_new_path(cr);
      beginning_of_path = 0;
    }

    cairo_get_matrix(cr, & previous_m);
    m.xx = b->x - c->x;  m.yx = b->y - c->y;
    m.xy = b->x - a->x;  m.yy = b->y - a->y;
    m.x0 = a->x - m.xx; m.y0 = a->y - m.yx;
    cairo_transform(cr, & m);

    cairo_arc(cr,
              (double) 0, (double) 0, /* center */
              (double) 1, /* radius */
              (double) 0, (double) M_PI/2.0 /* angle begin and end */);

    cairo_set_matrix(cr, & previous_m);

    previous = *c;
    beginning_of_line = 0;
  }
}

static void wincairo_rcircle(Point *ctr, Point *a, Point *b) {
  cairo_t *cr = (cairo_t *) grp_win->ptr;
  cairo_matrix_t previous_m, m;
  MY_3POINTS(ctr, a, b);
  WHEREAMI;

  if (beginning_of_path)
    cairo_new_path(cr);

  cairo_get_matrix(cr, & previous_m);
  m.xx = a->x - ctr->x;  m.yx = a->y - ctr->y;
  m.xy = b->x - ctr->x;  m.yy = b->y - ctr->y;
  m.x0 = ctr->x; m.y0 = ctr->y;
  cairo_transform(cr, & m);

  cairo_move_to(cr, (double) 1, (double) 0);
  cairo_arc(cr,
            (double) 0, (double) 0, /* center */
            (double) 1, /* radius */
            (double) 0, (double) 2.0*M_PI /* angle begin and end */);

  cairo_set_matrix(cr, & previous_m);

  beginning_of_line = 1;
  beginning_of_path = 0;
}

static void wincairo_font(const char *font_name, Real size) {
  cairo_t *cr = (cairo_t *) grp_win->ptr;
  const char *name;
  FontSlant s;
  FontWeight w;
  cairo_font_slant_t cs;
  cairo_font_weight_t cw;
  WHEREAMI;

  if (ps_font_get_info(font_name, & name, & s, & w)) {
    switch(s) {
    case FONT_SLANT_NORMAL: cs = CAIRO_FONT_SLANT_NORMAL; break;
    case FONT_SLANT_ITALIC: cs = CAIRO_FONT_SLANT_ITALIC; break;
    case FONT_SLANT_OBLIQUE: cs = CAIRO_FONT_SLANT_OBLIQUE; break;
    default: cs = CAIRO_FONT_SLANT_NORMAL; break;
    }

    switch(w) {
    case FONT_WEIGHT_NORMAL: cw = CAIRO_FONT_WEIGHT_NORMAL; break;
    case FONT_WEIGHT_BOLD: cw = CAIRO_FONT_WEIGHT_BOLD; break;
    default: cw = CAIRO_FONT_WEIGHT_NORMAL; break;
    }

  } else {
    name = font_name;
    cs = CAIRO_FONT_SLANT_NORMAL;
    cw = CAIRO_FONT_WEIGHT_NORMAL;
  }

  cairo_select_font_face(cr, name, cs, cw);
  cairo_set_font_size(cr, MY_REAL(size));
}

static void wincairo_text(Point *p, const char *text) {
  cairo_t *cr = (cairo_t *) grp_win->ptr;
  MY_POINT(p);
  WHEREAMI;

  cairo_move_to(cr, p->x, p->y);
  cairo_show_text(cr, text);
}

static int wincairo_save(const char *file_name) {
  cairo_t *cr = (cairo_t *) grp_win->ptr;
  cairo_surface_t *surface = cairo_get_target(cr);
  char *exts[] = {"png", "pdf", (char *) NULL};
  enum {EXT_PNG=0};
  cairo_status_t status;
  WHEREAMI;

  if (grp_win->win_type_str != wincairo_image_id_string)
    return 1;

  switch(file_extension(exts, file_name)) {
  default:
    g_warning("Unrecognized extension: using PNG!");
  case EXT_PNG:
#ifdef CAIRO_HAS_PNG_FUNCTIONS
    status = cairo_surface_write_to_png(surface, file_name);
#else
    g_error("Cairo was not compiled with PNG support!");
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
  WHEREAMI;
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
  w->font = wincairo_font;
  w->text = wincairo_text;
}

GrpWindow *cairo_open_win(GrpWindowPlan *plan) {
  GrpWindow *w;
  cairo_surface_t *surface;
  cairo_t *cr;
  cairo_status_t status;
  WT wt;
  int numptx, numpty;
  enum {WC_NONE=-1, WC_IMAGE=1, WC_STREAM} win_class;
  cairo_format_t format;
  typedef cairo_surface_t *(*StreamSurfaceCreate)(const char *filename,
                                                  double width_in_points,
                                                  double height_in_points);
  StreamSurfaceCreate stream_surface_create = (StreamSurfaceCreate) NULL;
  WHEREAMI;

  if (!plan->have.type) {
    g_error("cairo_open_win: missing window type!");
    return (GrpWindow *) NULL;
  }

  wt = (WT) plan->type;

  if ((w = (GrpWindow *) malloc(sizeof(GrpWindow))) == (GrpWindow *) NULL) {
    g_error("cairo_open_win: malloc failed!");
    return (GrpWindow *) NULL;
  }

  switch(wt) {
  case WT_A1:
    win_class = WC_IMAGE;
    format = CAIRO_FORMAT_A1;
    break;

  case WT_A8:
    win_class = WC_IMAGE;
    format = CAIRO_FORMAT_A8;
    break;

  case WT_RGB24:
    win_class = WC_IMAGE;
    format = CAIRO_FORMAT_RGB24;
    break;

  case WT_ARGB32:
    win_class = WC_IMAGE;
    format = CAIRO_FORMAT_ARGB32;
    break;

  case WT_PS:
#ifdef CAIRO_HAS_PS_SURFACE
    stream_surface_create = cairo_ps_surface_create;
    win_class = WC_STREAM;
    break;
#else
    g_error("cairo_open_win: Cairo was not compiled "
            "with support for the PostScript backend!");
    return (GrpWindow *) NULL;
#endif

  case WT_EPS:
#ifdef CAIRO_HAS_PS_SURFACE
    stream_surface_create = cairo_ps_surface_create;
    win_class = WC_STREAM;
    break;
#else
    g_error("cairo_open_win: Cairo was not compiled "
            "with support for the PostScript backend!");
    return (GrpWindow *) NULL;
#endif

  case WT_PDF:
#ifdef CAIRO_HAS_PDF_SURFACE
    stream_surface_create = cairo_pdf_surface_create;
    win_class = WC_STREAM;
    break;
#else
    g_error("cairo_open_win: Cairo was not compiled "
            "with support for the PDF backend!");
    return (GrpWindow *) NULL;
#endif

  case WT_SVG:
#ifdef CAIRO_HAS_SVG_SURFACE
    stream_surface_create = cairo_svg_surface_create;
    win_class = WC_STREAM;
    break;
#else
    g_error("cairo_open_win: Cairo was not compiled "
            "with support for the SVG backend!");
    return (GrpWindow *) NULL;
#endif

  default:
    g_error("cairo_open_win: unknown window type!");
    return (GrpWindow *) NULL;
  }

  if (win_class == WC_IMAGE) {
    if (! (plan->have.size && plan->have.resolution) ) {
      g_error("Cannot create Cairo image surface: "
              "size or resolution missing!");
      return (GrpWindow *) NULL;
    }

    if (plan->have.origin) {
      w->ltx = plan->origin.x;
      w->lty = plan->origin.y;

    } else {
      w->ltx = 0.0;
      w->lty = 0.0;
    }

    w->lx = plan->size.x;
    w->ly = plan->size.y;
    w->resx = plan->resolution.x * (plan->size.x < 0.0 ? -1.0 : 1.0);
    w->resy = plan->resolution.y * (plan->size.y < 0.0 ? -1.0 : 1.0);

    w->rdx = w->ltx + plan->size.x;
    w->rdy = w->lty + plan->size.y;

    numptx = fabs(plan->size.x * plan->resolution.x);
    numpty = fabs(plan->size.y * plan->resolution.y);
    surface = cairo_image_surface_create(format, numptx, numpty);
    w->win_type_str = wincairo_image_id_string;

 } else if (win_class == WC_STREAM) {
    double width, height;

    if (! (plan->have.file_name && plan->have.size) ) {
      g_error("Cannot create Cairo image surface: "
              "file name or size missing!");
      return (GrpWindow *) NULL;
    }

    /* All sizes and coordinates are expressed in mm, we must
     * therefore convert the size of the window in postscript units:
     * 1 postscript unit (also called point) = 1/72 inch
     * 1 inch = 25.4 mm
     */
    width  = (plan->size.x / grp_mm_per_inch) / grp_inch_per_psunit;
    height = (plan->size.y / grp_mm_per_inch) / grp_inch_per_psunit;

    /* These quantities are used in the function my_point (macros MY_2POINTS
     * and MY_3POINTS) to scale the coordinates of every point.
     */
    w->resy = w->resx =
      (1.0 / grp_mm_per_inch) / grp_inch_per_psunit; /* mm --> psunits */

    if (plan->have.origin) {
      w->ltx = plan->origin.x;
      w->lty = plan->origin.y;

    } else {
      w->ltx = 0.0;
      w->lty = 0.0;
    }

    if (stream_surface_create == (StreamSurfaceCreate) NULL)
      return (GrpWindow *) NULL;

    surface = stream_surface_create(plan->file_name, width, height);
    w->win_type_str = wincairo_stream_id_string;

    if (wt == WT_EPS) {
#ifdef HAVE_CAIRO_EPS
      cairo_ps_surface_set_eps(surface, (cairo_bool_t) 1);
#else
      g_warning("This version of Cairo does not support EPS format: "
                "using PS.");
#endif
    }

  } else {
    g_error("cairo_open_win: shouldn't happen!");
    return (GrpWindow *) NULL;
  }

  status = cairo_surface_status(surface);
  if (status != CAIRO_STATUS_SUCCESS) {
    g_error("Cannot create Cairo surface:");
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
  return w;
}
