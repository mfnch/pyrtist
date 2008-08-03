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
#include "formatter.h"

/*#define DEBUG*/

#ifdef DEBUG
#  define WHEREAMI \
   fprintf(stderr, "Inside '%s'\n", __func__), fflush(stderr)
#else
#  define WHEREAMI (void) 0
#endif

static Fmt fmt;

static void Init_Fmt(void) {
  Fmt_Init(& fmt);
}

static void Check_Init_Fmt(void) {
  static int already_init = 0;
  if (!already_init) Init_Fmt();
  already_init = 1;
}

/* Invert the cairo matrix 'in' and put the result in 'result'.
 * The determinant of 'in' is returned and is 0.0, if the matrix
 * couldn't be inverted.
 */
static Real invert_cairo_matrix(cairo_matrix_t *result, cairo_matrix_t *in) {
  Real det = in->xx*in->yy - in->xy*in->yx, inv_det;
  if (det == 0.0) return 0.0;
  inv_det = 1.0/det;
  result->xx =  inv_det*in->yy; result->xy = -inv_det*in->xy;
  result->yx = -inv_det*in->yx; result->yy =  inv_det*in->xx;
  result->x0 = -result->xx * in->x0 - result->xy * in->y0;
  result->y0 = -result->yx * in->x0 - result->yy * in->y0;
  return det;
}

static char *wincairo_image_id_string   = "cairo:image";
static char *wincairo_stream_id_string  = "cairo:stream";

static void wincairo_close_win(void) {
  cairo_t *cr = (cairo_t *) grp_win->ptr;
  cairo_surface_t *surface = cairo_get_target(cr);
  WHEREAMI;
  cairo_show_page(cr);
  cairo_destroy(cr);
  cairo_surface_destroy(surface);
}

/* Variabili usate dalle procedure per scrivere il file postscript */
static int beginning_of_line = 1, beginning_of_path = 1;
static Point previous;

static int same_points(Point *a, Point *b) {
  return (fabs(a->x - b->x) < 1e-10 && fabs(a->y - b->y) < 1e-10);
}

static void my_point(Point *out, Point *in) {
  GrpWindow *w = grp_win;
  WHEREAMI;
  out->x = (in->x - w->ltx)*w->resx;
  out->y = (in->y - w->lty)*w->resy;
}

/* Macros used to scale the point coordinates */
#define MY_POINT(a) \
  Point my_a; my_point(& my_a, (a)); (a) = & my_a

#define MY_2POINTS(a, b) \
  Point my_a, my_b; \
  my_point(& my_a, (a)); my_point(& my_b, (b)); \
  (a) = & my_a; (b) = & my_b

#define MY_3POINTS(a, b, c) \
  Point my_a, my_b, my_c; \
  my_point(& my_a, (a)); my_point(& my_b, (b));  my_point(& my_c, (c)); \
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

static void wincairo_rdraw(DrawStyle *style) {
  cairo_t *cr = (cairo_t *) grp_win->ptr;
  int do_fill, do_clip, do_even_odd, do_border = (style->bord_width > 0.0);
  WHEREAMI;

  if ( ! beginning_of_path ) {
    Real scale = style->scale;

    switch(style->fill_style) {
    case FILLSTYLE_VOID:   do_fill = do_clip = do_even_odd = 0; break;
    case FILLSTYLE_PLAIN:  do_fill = 1; do_clip = 0; do_even_odd = 0; break;
    case FILLSTYLE_EO:     do_fill = 1; do_clip = 0; do_even_odd = 1; break;
    case FILLSTYLE_CLIP:   do_fill = 0; do_clip = 1; do_even_odd = 0; break;
    case FILLSTYLE_EOCLIP: do_fill = 0; do_clip = 1; do_even_odd = 1; break;
    default:
      do_fill = 1; do_clip = 0; do_even_odd = 1;
      g_warning("Unsupported drawing style: using even-odd fill algorithm!");
      break;
    }

    cairo_set_fill_rule(cr, do_even_odd ? CAIRO_FILL_RULE_EVEN_ODD
                                        : CAIRO_FILL_RULE_WINDING);
    if (do_border) {
      Color *c = & style->bord_color;
      Real border = MY_REAL(scale*style->bord_width);
      cairo_line_join_t lj;
      cairo_line_cap_t lc;

      switch(style->bord_join_style) {
      case JOIN_STYLE_MITER: lj = CAIRO_LINE_JOIN_MITER; break;
      case JOIN_STYLE_ROUND: lj = CAIRO_LINE_JOIN_ROUND; break;
      case JOIN_STYLE_BEVEL: lj = CAIRO_LINE_JOIN_BEVEL; break;
      default: lj = CAIRO_LINE_JOIN_ROUND; break;
      }

      switch(style->bord_cap) {
      case CAP_STYLE_BUTT:   lc = CAIRO_LINE_CAP_BUTT; break;
      case CAP_STYLE_ROUND:  lc = CAIRO_LINE_CAP_ROUND; break;
      case CAP_STYLE_SQUARE: lc = CAIRO_LINE_CAP_SQUARE; break;
      default: lc = CAIRO_LINE_CAP_BUTT; break;
      }

      if (do_fill) cairo_fill_preserve(cr);
      if (do_clip) cairo_clip_preserve(cr);
      cairo_save(cr);
      cairo_set_source_rgba(cr, c->r, c->g, c->b, c->a);
      cairo_set_line_width(cr, border);
      cairo_set_line_join(cr, lj);
      cairo_set_line_cap(cr, lc);
      if (lj == CAIRO_LINE_JOIN_MITER) {
        Real miter_limit = MY_REAL(scale*style->bord_miter_limit);
        cairo_set_miter_limit(cr, miter_limit);
      }
      if (style->bord_num_dashes > 0) {
        Int num_dashes = style->bord_num_dashes,
            size_dashes = num_dashes*sizeof(Real);
        Real *dashes = (Real *) malloc(size_dashes);
        if (dashes != (Real *) NULL) {
          Int i;
          Real dash_offset = MY_REAL(scale*style->bord_dash_offset);
          for(i=0; i<num_dashes; i++)
            dashes[i] = MY_REAL(scale*style->bord_dashes[i]);
          cairo_set_dash(cr, dashes, num_dashes, dash_offset);
          free(dashes);
        }
      }
      cairo_stroke(cr);
      cairo_restore(cr);

    } else {
      if (do_fill) cairo_fill(cr);
      if (do_clip) cairo_clip(cr);
    }
  }
}

static void wincairo_rfgcolor(Color *c) {
  cairo_t *cr = (cairo_t *) grp_win->ptr;
  WHEREAMI;
  cairo_set_source_rgba(cr, c->r, c->g, c->b, c->a);
}

static void wincairo_rgradient(ColorGrad *cg) {
  cairo_t *cr = (cairo_t *) grp_win->ptr;
  cairo_pattern_t *p;
  cairo_matrix_t m, m_inv;
  Point p1, p2, ref1, ref2;
  Int i;
  WHEREAMI;

  switch(cg->type) {
  case COLOR_GRAD_TYPE_LINEAR:
    my_point(& p1, & cg->point1);
    my_point(& p2, & cg->point2);
    p = cairo_pattern_create_linear(p1.x, p1.y, p2.x, p2.y);
    break;

  case COLOR_GRAD_TYPE_RADIAL:
    my_point(& p1, & cg->point1);
    my_point(& p2, & cg->point2);
    my_point(& ref1, & cg->ref1);
    my_point(& ref2, & cg->ref2);
    ref1.x -= p1.x; ref1.y -= p1.y;
    ref2.x -= p1.x; ref2.y -= p1.y;

    m.xx = ref1.x; m.yx = ref1.y;
    m.xy = ref2.x; m.yy = ref2.y;
    m.x0 = p1.x; m.y0 = p1.y;
    if (invert_cairo_matrix(& m_inv, & m) == 0.0) {
      g_warning("wincairo_rgradient: gradient matrix is non invertible. "
                "Ignoring gradient setting!");
      return;
    }

    p = cairo_pattern_create_radial(0.0, 0.0, cg->radius1,
                                    p2.x - p1.x, p2.y - p1.y, cg->radius2);
    cairo_pattern_set_matrix(p, & m_inv);
    break;

  default:
    g_warning("Unknown color gradient type. Ignoring gradient setting.");
    return;
  }

  cairo_pattern_set_extend(p, cg->extend);
  for(i=0; i<cg->num_items; i++) {
    ColorGradItem *cgi = & cg->items[i];
    Color *c = & cgi->color;
    cairo_pattern_add_color_stop_rgba(p, cgi->position,
                                      c->r, c->g, c->b, c->a);
  }
  cairo_set_source(cr, p);
  cairo_pattern_destroy(p);
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

static void wincairo_rclose(void) {
  cairo_t *cr = (cairo_t *) grp_win->ptr;
  WHEREAMI;
  if (!beginning_of_path) cairo_close_path(cr);
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

static void wincairo_font(const char *font_name) {
  cairo_t *cr = (cairo_t *) grp_win->ptr;
  const char *name;
  FontSlant s;
  FontWeight w;
  cairo_font_slant_t cs;
  cairo_font_weight_t cw;
  cairo_font_face_t *ff;
  cairo_status_t status;
  cairo_matrix_t m;
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
  ff = cairo_get_font_face(cr);
  status = cairo_font_face_status(ff);
  if (status != CAIRO_STATUS_SUCCESS) {
    fprintf(stderr, "Cannot set font: %s.\n", cairo_status_to_string(status));
    cairo_select_font_face(cr, "sans", CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_NORMAL);
  }

  /* Cairo coordinates use the following convention: increasing x goes from
   * the left to the right side of the screen, while increasing y goes from
   * the top to the bottom side. Postscript coordinates use the opposite
   * convention. The Box graphics library also sticks to the Postscript
   * convention, since this is the usual choice in geometry and mathematics.
   * Care has to be put in converting from one to the other convention:
   * we don't want to simply mirror the image: we want to do that only for
   * graphics, while preserving the direction for fonts: mirroring fonts
   * is not really what we want! Therefore we DON'T choose the identity
   * matrix for fonts! We choose diag(1, -1)!
   */
  m.xx = 1.0; m.yy = -1.0;
  m.xy = m.yx = m.x0 = m.y0 = 0.0;
  cairo_set_font_matrix(cr, & m);
}

static void wincairo_text(Point *ctr, Point *right, Point *up, Point *from,
                          const char *text) {
  cairo_t *cr = (cairo_t *) grp_win->ptr;
  cairo_matrix_t previous_m, m;
  cairo_text_extents_t extents;
  MY_3POINTS(ctr, right, up);
  WHEREAMI;

  cairo_get_matrix(cr, & previous_m);
  m.xx = right->x - ctr->x;  m.yx = right->y - ctr->y;
  m.xy = up->x - ctr->x;  m.yy = up->y - ctr->y;
  m.x0 = ctr->x; m.y0 = ctr->y;
  cairo_transform(cr, & m);

  Check_Init_Fmt();
  /* Here we should use the formatter module */

  cairo_move_to(cr, (double) 0, (double) 0);
  cairo_text_extents(cr, text, & extents);
  cairo_move_to(cr, -extents.x_bearing - extents.width*from->x,
                -extents.y_bearing - extents.height*from->y);
  cairo_show_text(cr, text);
  cairo_set_matrix(cr, & previous_m);
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
  w->rgradient = wincairo_rgradient;
  w->rline = wincairo_rline;
  w->rcong = wincairo_rcong;
  w->rclose = wincairo_rclose;
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
  int numptx = 0, numpty = 0;
  int paint_background = 0;
  enum {WC_NONE=-1, WC_IMAGE=1, WC_STREAM} win_class;
  cairo_format_t format=CAIRO_FORMAT_ARGB32; /* Just to avoid warnings! */
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
    paint_background = 1;
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

  if (! plan->have.size ) {
    g_error("Cannot create Cairo image surface: size missing!");
    return (GrpWindow *) NULL;
  }

  w->lx = plan->size.x;
  w->ly = plan->size.y;

  if (plan->have.origin) {
    w->ltx = plan->origin.x;
    w->lty = plan->origin.y;

  } else {
    w->ltx = 0.0;
    w->lty = 0.0;
  }

  w->rdx = w->ltx + plan->size.x;
  w->rdy = w->lty + plan->size.y;

  if (win_class == WC_IMAGE) {
    if (! plan->have.resolution) {
      g_error("Cannot create Cairo image surface: resolution missing!");
      return (GrpWindow *) NULL;
    }

    w->resx = plan->resolution.x * (plan->size.x < 0.0 ? -1.0 : 1.0);
    w->resy = plan->resolution.y * (plan->size.y < 0.0 ? -1.0 : 1.0);

    numptx = fabs(plan->size.x * plan->resolution.x);
    numpty = fabs(plan->size.y * plan->resolution.y);
    surface = cairo_image_surface_create(format, numptx, numpty);
    w->win_type_str = wincairo_image_id_string;

 } else if (win_class == WC_STREAM) {
    double width, height;

    if (! plan->have.file_name) {
      g_error("Cannot create Cairo image surface: file name missing!");
      return (GrpWindow *) NULL;
    }

    /* These quantities are used in the function my_point (macros MY_2POINTS
     * and MY_3POINTS) to scale the coordinates of every point.
     * They express the number of postscript units per mm.
     */
    w->resy = w->resx = 1.0 / (grp_mm_per_inch * grp_inch_per_psunit);

    /* All sizes and coordinates are expressed in mm, we must
     * therefore convert the size of the window in postscript units:
     * 1 postscript unit (also called points) = 1/72 inch
     * 1 inch = 25.4 mm
     */
    width  = plan->size.x * w->resx;
    height = plan->size.y * w->resy;

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

  if (plan->size.y >= 0.0) {
    /* In this case, the origin is placed at the bottom of the screen! */
    w->lty += plan->size.y;
    w->resy = -w->resy;
  }

  if (plan->size.x < 0.0) {
    /* In this case, the origin is placed at the bottom of the screen! */
    w->ltx += plan->size.x;
    w->resx = -w->resx;
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

  /* If we need white background, we paint the window accordingly. */
  if (paint_background) {
    cairo_save(cr);
    cairo_rectangle(cr, 0.0, 0.0, numptx, numpty);
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_fill(cr);
    cairo_restore(cr);
  }

  w->ptr = (void *) cr;

  /* Ora do' le procedure per gestire la finestra */
  w->quiet = 0;
  w->repair = wincairo_repair;
  w->repair(w);
  return w;
}
