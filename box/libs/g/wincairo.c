/****************************************************************************
 * Copyright (C) 2008-2011 by Matteo Franchin                               *
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
#include <stdarg.h>

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

#include <box/types.h>
#include <box/mem.h>

#include "error.h"
#include "graphic.h"
#include "g.h"
#include "wincairo.h"
#include "psfonts.h"
#include "formatter.h"
#include "buffer.h"
#include "obj.h"

/*#define DEBUG*/

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

static void My_Wincairo_Finish_Drawing(BoxGWin *w) {
  cairo_t *cr = (cairo_t *) w->ptr;
  cairo_surface_t *surface = cairo_get_target(cr);

  cairo_show_page(cr);
  cairo_destroy(cr);
  cairo_surface_destroy(surface);
}

/* Variabili usate dalle procedure per scrivere il file postscript */
static int beginning_of_path = 1;
static Point previous;

static int same_points(Point *a, Point *b) {
  return (fabs(a->x - b->x) < 1e-10 && fabs(a->y - b->y) < 1e-10);
}

/** This is the function used to map points. */
static void My_Map_Point(BoxGWin *w, Point *out, Point *in) {
  out->x = (in->x - w->ltx)*w->resx;
  out->y = (in->y - w->lty)*w->resy;
}

/** This is the function used to map applied vectors. */
static void My_Map_Vector(BoxGWin *w, Point *out, Point *in) {
  out->x = in->x * w->resx;
  out->y = in->y * w->resy;
}

/** This is the function used to map widths. */
static void My_Map_Width(BoxGWin *w, Real *out, Real *in) {
  *out = *in * w->resx;
}

/* Macros used to scale the point coordinates */
#define MY_POINT(w, a) \
  Point my_a; My_Map_Point((w), & my_a, (a)); (a) = & my_a

#define MY_2POINTS(w, a, b) \
  Point my_a, my_b; \
  My_Map_Point((w), & my_a, (a)); My_Map_Point((w), & my_b, (b)); \
  (a) = & my_a; (b) = & my_b

#define MY_3POINTS(w, a, b, c) \
  Point my_a, my_b, my_c; \
  My_Map_Point((w), & my_a, (a)); My_Map_Point((w), & my_b, (b)); \
  My_Map_Point((w), & my_c, (c)); \
  (a) = & my_a; (b) = & my_b; (c) = & my_c

/* This is broken, but is required for fonts (fonts support is broken as well
 * and needs to be better done: font drawing should depend on Points, not on
 * a given Real size!)
 */
#define MY_REAL(w, r) ((r)*(w)->resx) \

/* BEGIN OF TEXT FORMATTING IMPLEMENTATION **********************************
 * Here we implement some basic text formatting features, such as           *
 * subscripts, superscripts and multi-line paragraphs. We exploit           *
 * the parser provided by the formatter.c module.                           *
 ****************************************************************************/

typedef struct {
  cairo_t *cr;
  buff    saved_states;
  Point   sub_vec,
          sup_vec;
  Real    sub_scale,
          sup_scale;
} TextPrivate;

typedef struct {
  Point          cur_pos;
  cairo_matrix_t m;
} TextState;

static void My_Text_Fmt_Draw(BoxGFmtStack *stack) {
  BoxGFmt *fmt = BoxGFmt_Get(stack);
  TextPrivate *private = (TextPrivate *) BoxGFmt_Private_Get(fmt);
  char *text = BoxGFmt_Buffer_Get(stack);
  cairo_text_path(private->cr, text);
  BoxGFmt_Buffer_Clear(stack);
}

static void My_Text_Fmt_Save(BoxGFmtStack *stack) {
  BoxGFmt *fmt = BoxGFmt_Get(stack);
  TextPrivate *private = (TextPrivate *) BoxGFmt_Private_Get(fmt);
  TextState ss;
  cairo_get_matrix(private->cr, & ss.m);
  cairo_get_current_point(private->cr, & ss.cur_pos.x, & ss.cur_pos.y);
  (void) buff_push(& private->saved_states, & ss);
}

static void My_Text_Fmt_Restore(BoxGFmtStack *stack) {
  BoxGFmt *fmt = BoxGFmt_Get(stack);
  TextPrivate *private = (TextPrivate *) BoxGFmt_Private_Get(fmt);
  TextState *ts = buff_lastitemptr(& private->saved_states, TextState);
  double x, y;
  cairo_set_matrix(private->cr, & ts->m);
  cairo_get_current_point(private->cr, & x, & y);
  cairo_move_to(private->cr, x, ts->cur_pos.y);
  buff_dec(& private->saved_states);
}

static void My_Text_Fmt_Change(cairo_t *cr, Point *vec, Real scale) {
  cairo_matrix_t m;
  cairo_get_current_point(cr, & m.x0, & m.y0);
  m.x0 += vec->x;
  m.y0 += vec->y;
  m.xx = m.yy = scale;
  m.xy = m.yx = 0.0;
  cairo_transform(cr, & m);
  cairo_move_to(cr, (double) 0.0, (double) 0.0);
}

static void My_Text_Fmt_Superscript(BoxGFmtStack *stack) {
  BoxGFmt *fmt = BoxGFmt_Get(stack);
  TextPrivate *private = (TextPrivate *) BoxGFmt_Private_Get(fmt);
  My_Text_Fmt_Change(private->cr, & private->sup_vec, private->sup_scale);
}

static void My_Text_Fmt_Subscript(BoxGFmtStack *stack) {
  BoxGFmt *fmt = BoxGFmt_Get(stack);
  TextPrivate *private = (TextPrivate *) BoxGFmt_Private_Get(fmt);
  My_Text_Fmt_Change(private->cr, & private->sub_vec, private->sub_scale);
}

static void My_Text_Fmt_Newline(BoxGFmtStack *stack) {
  BoxGFmt *fmt = BoxGFmt_Get(stack);
  TextPrivate *private = (TextPrivate *) BoxGFmt_Private_Get(fmt);
  cairo_translate(private->cr, 0.0, -1.0);
  cairo_move_to(private->cr, 0.0, 0.0);
}

static void My_Text_Fmt_Init(BoxGFmt *fmt) {
  BoxGFmt_Init(fmt);
  fmt->draw = My_Text_Fmt_Draw;
  fmt->subscript = My_Text_Fmt_Subscript;
  fmt->superscript = My_Text_Fmt_Superscript;
  fmt->newline = My_Text_Fmt_Newline;
  fmt->save = My_Text_Fmt_Save;
  fmt->restore = My_Text_Fmt_Restore;
}

static void My_Cairo_Text_Path(BoxGWin *w, Point *ctr, Point *right,
                               Point *up, Point *from, const char *text) {

  cairo_t *cr = (cairo_t *) w->ptr;
  cairo_matrix_t m;
  double x1, y1, x2, y2;
  TextPrivate private;
  BoxGFmt fmt;

  m.xx = right->x - ctr->x;  m.yx = right->y - ctr->y;
  m.xy = up->x - ctr->x;  m.yy = up->y - ctr->y;
  m.x0 = ctr->x; m.y0 = ctr->y;
  cairo_save(cr);
  cairo_transform(cr, & m);

  My_Text_Fmt_Init(& fmt);
  BoxGFmt_Private_Set(& fmt, & private);
  private.cr = cr;
  private.sup_vec.x = 0.0;
  private.sup_vec.y = 0.5;
  private.sup_scale = 0.5;
  private.sub_vec.x = 0.0;
  private.sub_vec.y = -0.1;
  private.sub_scale = 0.5;

  int state = buff_create(& private.saved_states, sizeof(TextState), 8);
  assert(state == 1);

  cairo_save(cr);
  cairo_new_path(cr);
  cairo_move_to(cr, (double) 0, (double) 0);
  BoxGFmt_Text(& fmt, text);
  cairo_restore(cr);
  cairo_fill_extents(cr, & x1, & y1, & x2, & y2);

  cairo_new_path(cr);
  cairo_translate(cr, -x1 - (x2 - x1)*from->x, -y1 - (y2 - y1)*from->y);
  BoxGFmt_Text(& fmt, text);
  cairo_restore(cr);

  buff_free(& private.saved_states);
  beginning_of_path = 0;
}

/* END OF TEXT FORMATTING IMPLEMENTATION ************************************/

static void My_Cairo_JoinArc(cairo_t *cr,
                             const Point *a, const Point *b, const Point *c) {
  cairo_matrix_t previous_m, m;

  cairo_get_matrix(cr, & previous_m);
  m.xx = b->x - c->x; m.yx = b->y - c->y;
  m.xy = b->x - a->x; m.yy = b->y - a->y;
  m.x0 = a->x - m.xx; m.y0 = a->y - m.yx;
  cairo_transform(cr, & m);

  cairo_arc(cr,
            (double) 0.0, (double) 0.0,      /* center */
            (double) 1.0,                    /* radius */
            (double) 0.0, (double) M_PI/2.0); /* angle begin and end */

  cairo_set_matrix(cr, & previous_m);
}

static void My_Cairo_Arc(cairo_t *cr,
                         const Point *ctr, const Point *a, const Point *b,
                         Real angle_begin, Real angle_end) {
  cairo_matrix_t previous_m, m;

  cairo_get_matrix(cr, & previous_m);
  m.xx = a->x - ctr->x;  m.yx = a->y - ctr->y;
  m.xy = b->x - ctr->x;  m.yy = b->y - ctr->y;
  m.x0 = ctr->x; m.y0 = ctr->y;
  cairo_transform(cr, & m);

  cairo_arc(cr,
            (double) 0, (double) 0,  /* center */
            (double) 1,              /* radius */
            angle_begin, angle_end); /* angle begin and end */

  cairo_set_matrix(cr, & previous_m);
}

static void My_Cairo_Set_Font(BoxGWin *w, const char *font_name) {
  cairo_t *cr = (cairo_t *) w->ptr;
  const char *name;
  FontSlant fs;
  FontWeight fw;
  cairo_font_slant_t cs;
  cairo_font_weight_t cw;
  cairo_font_face_t *ff;
  cairo_status_t status;
  cairo_matrix_t m;

  if (ps_font_get_info(font_name, & name, & fs, & fw)) {
    switch(fs) {
    case FONT_SLANT_NORMAL: cs = CAIRO_FONT_SLANT_NORMAL; break;
    case FONT_SLANT_ITALIC: cs = CAIRO_FONT_SLANT_ITALIC; break;
    case FONT_SLANT_OBLIQUE: cs = CAIRO_FONT_SLANT_OBLIQUE; break;
    default: cs = CAIRO_FONT_SLANT_NORMAL; break;
    }

    switch(fw) {
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

static BoxGObjKind My_BoxGObjKind_Of_Arg(BoxGArgKind t) {
  switch (t) {
  case BOXGARG_INT: return BOXGOBJKIND_INT;
  case BOXGARG_REAL: return BOXGOBJKIND_REAL;
  case BOXGARG_STR: return BOXGOBJKIND_STR;
  case BOXGARG_POINT: case BOXGARG_VECTOR: case BOXGARG_REALP:
    return BOXGOBJKIND_POINT;
  case BOXGARG_WIDTH: return BOXGOBJKIND_REAL;
  default: return BOXGOBJKIND_EMPTY;
  }
}

void My_Extract_Arg(BoxGWin *w,
                    void *out, void *in, BoxGArgKind required_kind) {
  switch (required_kind) {
  case BOXGARG_POINT:
    My_Map_Point(w, (BoxPoint *) out, (BoxPoint *) in);
    break;

  case BOXGARG_VECTOR:
    My_Map_Vector(w, (BoxPoint *) out, (BoxPoint *) in);
    break;

  case BOXGARG_WIDTH:
    My_Map_Width(w, (BoxReal *) out, (BoxReal *) in);
    break;

  default:
    *((void **) out) = in;
    break;
  }
}

#define ITP_MAX_NUM_ARGS 5

static int My_Args_From_Obj(BoxGWin *w, BoxGArg *args, BoxGObj *args_obj,
                            size_t num_args, ...) {
  size_t i;
  int success = 0;
  va_list ap;
  va_start(ap, num_args);

  assert(num_args <= ITP_MAX_NUM_ARGS);

  if (BoxGObj_Get_Length(args_obj) >= num_args + 1) {
    success = 1;
    for (i = 1; i <= num_args; i++) {
      int required_kind = va_arg(ap, int);
      BoxGObj *arg_obj = BoxGObj_Get(args_obj, i);
      void *val;
      assert(arg_obj != NULL);
      val = BoxGObj_To(arg_obj, My_BoxGObjKind_Of_Arg(required_kind));
      assert(val != NULL);

      My_Extract_Arg(w, args++, val, required_kind);
      success &= (val != NULL);
    }
  }

  va_end(ap);
  return success;
}

/**< Data structure to be passed through BoxObj_Iter to
 * My_Arg_Array_From_Obj_Iter.
 */
typedef struct {
  BoxGWin     *w;                   /**< The destination window object */
  BoxGArgKind arg_kind;             /**< Type of argument of the raw graphical
                                         command */
  BoxGObjKind required_subobj_kind; /**< Corresponding type of Obj */
  size_t      item_size,            /**< Size of the destination array item */
              num_items;            /**< Num. of items in the dest. array */
  void        *array;               /**< The destination array */

} MyArgArrayFromObjData;

static BoxTask My_Arg_Array_From_Obj_Iter(size_t idx, BoxGObjKind k,
                                          BoxGObj *sub, void *pass) {
  MyArgArrayFromObjData *data = (MyArgArrayFromObjData *) pass;

  if (k == data->required_subobj_kind) {
    void *val = BoxGObj_To(sub, k),
         *item_ptr = data->array + idx*data->item_size;
    assert(idx < data->num_items);
    My_Extract_Arg(data->w, item_ptr, val, data->arg_kind);
    return BOXTASK_OK;

  } else
    return BOXTASK_FAILURE;
}

static void *My_Arg_Array_From_Obj(BoxGWin *w, BoxGObj *args_obj,
                                   BoxGArgKind arg_kind,
                                   size_t start_idx, size_t *num_args) {
  MyArgArrayFromObjData data;
  size_t max_idx = BoxGObj_Get_Num(args_obj);

  assert(num_args != NULL);

  if (start_idx < max_idx) {
    size_t max_n = max_idx - start_idx,
           n = (*num_args > 0 && *num_args < max_n) ? *num_args : max_n,
           item_size;

    switch (arg_kind) {
    case BOXGARG_WIDTH: item_size = sizeof(BoxReal); break;
    default:
      *num_args = 0;
      return NULL;
    }

    data.w = w;
    data.arg_kind = arg_kind;
    data.required_subobj_kind = My_BoxGObjKind_Of_Arg(arg_kind);
    data.num_items = n;
    data.item_size = item_size;
    data.array = BoxMem_Safe_Alloc(item_size*n);
    (void) BoxGObj_Iter(args_obj, start_idx, num_args,
                        My_Arg_Array_From_Obj_Iter, & data);
    return data.array;

  } else {
    *num_args = 0;
    return NULL;
  }
}

static cairo_operator_t My_Cairo_Operator_Of_Int(BoxInt v) {
  switch (v) {
  case  0: return CAIRO_OPERATOR_CLEAR;
  case  1: return CAIRO_OPERATOR_SOURCE;
  case  2: return CAIRO_OPERATOR_OVER;
  case  3: return CAIRO_OPERATOR_IN;
  case  4: return CAIRO_OPERATOR_OUT;
  case  5: return CAIRO_OPERATOR_ATOP;
  case  6: return CAIRO_OPERATOR_DEST;
  case  7: return CAIRO_OPERATOR_DEST_OVER;
  case  8: return CAIRO_OPERATOR_DEST_IN;
  case  9: return CAIRO_OPERATOR_DEST_OUT;
  case 10: return CAIRO_OPERATOR_DEST_ATOP;
  case 11: return CAIRO_OPERATOR_XOR;
  case 12: return CAIRO_OPERATOR_ADD;
  case 13: return CAIRO_OPERATOR_SATURATE;
  case 14: return CAIRO_OPERATOR_MULTIPLY;
  case 15: return CAIRO_OPERATOR_SCREEN;
  case 16: return CAIRO_OPERATOR_OVERLAY;
  case 17: return CAIRO_OPERATOR_DARKEN;
  case 18: return CAIRO_OPERATOR_LIGHTEN;
  case 19: return CAIRO_OPERATOR_COLOR_DODGE;
  case 20: return CAIRO_OPERATOR_COLOR_BURN;
  case 21: return CAIRO_OPERATOR_HARD_LIGHT;
  case 22: return CAIRO_OPERATOR_SOFT_LIGHT;
  case 23: return CAIRO_OPERATOR_DIFFERENCE;
  case 24: return CAIRO_OPERATOR_EXCLUSION;
  case 25: return CAIRO_OPERATOR_HSL_HUE;
  case 26: return CAIRO_OPERATOR_HSL_SATURATION;
  case 27: return CAIRO_OPERATOR_HSL_COLOR;
  case 28: return CAIRO_OPERATOR_HSL_LUMINOSITY;
  default: return CAIRO_OPERATOR_OVER;
  }
}

static BoxTask My_WinCairo_Interpret_One(BoxGWin *w,
                                         BoxInt cmnd_id, BoxGObj *args_obj) {
  cairo_t *cr = (cairo_t *) w->ptr;
  BoxGArg args[ITP_MAX_NUM_ARGS];

  switch (cmnd_id) {
  case BOXG_CMD_SAVE:
    cairo_save(cr);
    return BOXTASK_OK;

  case BOXG_CMD_RESTORE:
    cairo_restore(cr);
    return BOXTASK_OK;

  case BOXG_CMD_SET_ANTIALIAS:
    if (My_Args_From_Obj(w, args, args_obj, 1, BOXGARG_INT)) {
      cairo_antialias_t v;
      switch(*args[0].i) {
      default:
      case 0: v = CAIRO_ANTIALIAS_DEFAULT; break;
      case 1: v = CAIRO_ANTIALIAS_NONE; break;
      case 2: v = CAIRO_ANTIALIAS_GRAY; break;
      case 3: v = CAIRO_ANTIALIAS_SUBPIXEL; break;
      }
      cairo_set_antialias(cr, v);
      return BOXTASK_OK;
    }
    break;

  case BOXG_CMD_MOVE_TO:
    if (My_Args_From_Obj(w, args, args_obj, 1, BOXGARG_POINT)) {
      cairo_move_to(cr, args[0].p.x, args[0].p.y);
      return BOXTASK_OK;
    }
    break;

  case BOXG_CMD_LINE_TO:
    if (My_Args_From_Obj(w, args, args_obj, 1, BOXGARG_POINT)) {
      cairo_line_to(cr, args[0].p.x, args[0].p.y);
      return BOXTASK_OK;
    }
    break;

  case BOXG_CMD_CURVE_TO:
    if (My_Args_From_Obj(w, args, args_obj, 3, BOXGARG_POINT,
                         BOXGARG_POINT, BOXGARG_POINT)) {
      cairo_curve_to(cr,
                     args[0].p.x, args[0].p.y,
                     args[1].p.x, args[1].p.y,
                     args[2].p.x, args[2].p.y);
      return BOXTASK_OK;
    }
    break;

  case BOXG_CMD_CLOSE_PATH:
    cairo_close_path(cr);
    return BOXTASK_OK;

  case BOXG_CMD_NEW_PATH:
    cairo_new_path(cr);
    return BOXTASK_OK;

  case BOXG_CMD_NEW_SUB_PATH:
    cairo_new_sub_path(cr);
    return BOXTASK_OK;

  case BOXG_CMD_STROKE:
    cairo_stroke(cr);
    return BOXTASK_OK;

  case BOXG_CMD_STROKE_PRESERVE:
    cairo_stroke_preserve(cr);
    return BOXTASK_OK;

  case BOXG_CMD_FILL:
    cairo_fill(cr);
    return BOXTASK_OK;

  case BOXG_CMD_FILL_PRESERVE:
    cairo_fill_preserve(cr);
    return BOXTASK_OK;

  case BOXG_CMD_CLIP:
    cairo_clip(cr);
    return BOXTASK_OK;

  case BOXG_CMD_CLIP_PRESERVE:
    cairo_clip_preserve(cr);
    return BOXTASK_OK;

  case BOXG_CMD_RESET_CLIP:
    cairo_reset_clip(cr);
    return BOXTASK_OK;

  case BOXG_CMD_PUSH_GROUP:
    cairo_push_group(cr);
    return BOXTASK_OK;

  case BOXG_CMD_POP_GROUP_TO_SOURCE:
    cairo_pop_group_to_source(cr);
    return BOXTASK_OK;

  case BOXG_CMD_SET_OPERATOR:
    if (My_Args_From_Obj(w, args, args_obj, 1, BOXGARG_INT)) {
      cairo_set_operator(cr, My_Cairo_Operator_Of_Int(*args[0].i));
      return BOXTASK_OK;
    }
    break;

  case BOXG_CMD_PAINT:
    cairo_paint(cr);
    return BOXTASK_OK;

  case BOXG_CMD_PAINT_WITH_ALPHA:
    if (My_Args_From_Obj(w, args, args_obj, 1, BOXGARG_REAL)) {
      cairo_paint_with_alpha(cr, *args[0].r);
      return BOXTASK_OK;
    }
    return BOXTASK_OK;

  case BOXG_CMD_COPY_PAGE:
    cairo_copy_page(cr);
    return BOXTASK_OK;

  case BOXG_CMD_SHOW_PAGE:
    cairo_show_page(cr);
    return BOXTASK_OK;

  case BOXG_CMD_SET_LINE_WIDTH:
    if (My_Args_From_Obj(w, args, args_obj, 1, BOXGARG_WIDTH)) {
      cairo_set_line_width(cr, args[0].w);
      return BOXTASK_OK;
    }
    break;

  case BOXG_CMD_SET_LINE_CAP:
    if (My_Args_From_Obj(w, args, args_obj, 1, BOXGARG_INT)) {
      cairo_line_cap_t v;
      switch(*args[0].i) {
      default:
      case 0: v = CAIRO_LINE_CAP_BUTT; break;
      case 1: v = CAIRO_LINE_CAP_ROUND; break;
      case 2: v = CAIRO_LINE_CAP_SQUARE; break;
      }
      cairo_set_line_cap(cr, v);
      return BOXTASK_OK;
    }
    break;

  case BOXG_CMD_SET_LINE_JOIN:
    if (My_Args_From_Obj(w, args, args_obj, 1, BOXGARG_INT)) {
      cairo_line_join_t v;
      switch(*args[0].i) {
      default:
      case 0: v = CAIRO_LINE_JOIN_MITER; break;
      case 1: v = CAIRO_LINE_JOIN_ROUND; break;
      case 2: v = CAIRO_LINE_JOIN_BEVEL; break;
      }
      cairo_set_line_join(cr, v);
      return BOXTASK_OK;
    }
    break;

  case BOXG_CMD_SET_MITER_LIMIT:
    if (My_Args_From_Obj(w, args, args_obj, 1, BOXGARG_WIDTH)) {
      cairo_set_miter_limit(cr, args[0].w);
      return BOXTASK_OK;
    }
    break;

  case BOXG_CMD_SET_DASH:
    {
      size_t num_args = 0;
      BoxReal *rs = My_Arg_Array_From_Obj(w, args_obj, BOXGARG_WIDTH,
                                          1, & num_args);
      if (rs != NULL && num_args > 1) {
        BoxReal offset = rs[0],
                *dashes = rs + 1;
        size_t num_dashes = num_args - 1;
        cairo_set_dash(cr, dashes, num_dashes, offset);
        BoxMem_Free(rs);
        return BOXTASK_OK;
      }
      break;
    }

  case BOXG_CMD_SET_FILL_RULE:
    if (My_Args_From_Obj(w, args, args_obj, 1, BOXGARG_INT)) {
      cairo_fill_rule_t v;
      switch(*args[0].i) {
      default:
      case 0: v = CAIRO_FILL_RULE_WINDING; break;
      case 1: v = CAIRO_FILL_RULE_EVEN_ODD; break;
      }
      cairo_set_fill_rule(cr, v);
      return BOXTASK_OK;
    }
    break;

  case BOXG_CMD_SET_SOURCE_RGBA:
    if (My_Args_From_Obj(w, args, args_obj, 4, BOXGARG_REAL,
                         BOXGARG_REAL, BOXGARG_REAL,
                         BOXGARG_REAL)) {
      cairo_set_source_rgba(cr, *args[0].r, *args[1].r, *args[2].r,
                            *args[3].r);
      return BOXTASK_OK;
    }
    break;

  case BOXG_CMD_TEXT_PATH:
    if (My_Args_From_Obj(w, args, args_obj, 1, BOXGARG_STR)) {
      char *text = BoxStr_To_C_String(args[4].s);
      if (text != NULL) {
        cairo_text_path(cr, text);
        BoxMem_Free(text);
        return BOXTASK_OK;
      }
    }
    break;

  case BOXG_CMD_TRANSLATE:
    if (My_Args_From_Obj(w, args, args_obj, 1, BOXGARG_VECTOR)) {
      cairo_translate(cr, args[0].v.x, args[0].v.y);
      return BOXTASK_OK;
    }
    break;

  case BOXG_CMD_SCALE:
    if (My_Args_From_Obj(w, args, args_obj, 1, BOXGARG_VECTOR)) {
      cairo_scale(cr, args[0].v.x, args[0].v.y);
      return BOXTASK_OK;
    }
    break;

  case BOXG_CMD_ROTATE:
    if (My_Args_From_Obj(w, args, args_obj, 1, BOXGARG_REAL)) {
      cairo_rotate(cr, *args[0].r);
      return BOXTASK_OK;
    }
    break;

  case BOXG_CMD_EXT_JOINARC_TO:
    if (My_Args_From_Obj(w, args, args_obj, 3, BOXGARG_POINT,
                         BOXGARG_POINT, BOXGARG_POINT)) {
      My_Cairo_JoinArc(cr, & args[0].p, & args[1].p, & args[2].p);
      return BOXTASK_OK;
    }
    break;

  case BOXG_CMD_EXT_ARC_TO:
    if (My_Args_From_Obj(w, args, args_obj, 5, BOXGARG_POINT,
                         BOXGARG_POINT, BOXGARG_POINT,
                         BOXGARG_REAL, BOXGARG_REAL)) {
      My_Cairo_Arc(cr, & args[0].p, & args[1].p, & args[2].p,
                   *args[3].r, *args[4].r);
      return BOXTASK_OK;
    }
    break;

  case BOXG_CMD_EXT_SET_FONT:
    if (My_Args_From_Obj(w, args, args_obj, 1, BOXGARG_STR)) {
      char *font = BoxStr_To_C_String(args[0].s);
      if (font != NULL) {
        My_Cairo_Set_Font(w, font);
        BoxMem_Free(font);
        return BOXTASK_OK;
      }
    }
    break;

  case BOXG_CMD_EXT_TEXT_PATH:
    if (My_Args_From_Obj(w, args, args_obj, 5, BOXGARG_POINT,
                         BOXGARG_POINT, BOXGARG_POINT,
                         BOXGARG_REALP, BOXGARG_STR)) {
      char *text = BoxStr_To_C_String(args[4].s);
      if (text != NULL) {
        My_Cairo_Text_Path(w, & args[0].p, & args[1].p, & args[2].p,
                           & args[3].p, text);
        BoxMem_Free(text);
        return BOXTASK_OK;
      }
    }
    break;

  case BOXG_CMD_EXT_TRANSFORM:
    if (My_Args_From_Obj(w, args, args_obj, 3, BOXGARG_POINT,
                         BOXGARG_POINT, BOXGARG_POINT)) {
      
      /* this command should change the ref frame to the one which
       * has origin in the first point and coordinate (1, 0) at the
       * second point and (0, 1) at the third (not an ortogonal frame)
       * NOT IMPLEMENTED, YET.
       */
      return BOXTASK_OK;
    }
    break;

  default:
    return BOXTASK_FAILURE;
  }

  return BOXTASK_FAILURE;
}

static BoxTask My_WinCairo_Interpret(BoxGWin *w, BoxGObj *obj) {
  size_t i, n = BoxGObj_Get_Length(obj);
  const char *msg = NULL;
  for (i = 0; i < n; i++) {
    BoxInt sub_type = BoxGObj_Get_Type(obj, i);
    if (sub_type == BOXGOBJKIND_COMPOSITE) {
      BoxGObj *obj_sub = BoxGObj_Get(obj, i);
      size_t n_args = BoxGObj_Get_Length(obj_sub);
      if (n_args >= 1) {
        BoxInt *cmnd_id =
          (BoxInt *) BoxGObj_To(BoxGObj_Get(obj_sub, 0), BOXGOBJKIND_INT);
        if (cmnd_id != NULL) {
          if (!My_WinCairo_Interpret_One(w, *cmnd_id, obj_sub) == BOXTASK_OK) {
            msg = "Command failed";
          }

        } else
          msg = "Expecting command number (type Int)";

      } else
        msg = "Empty command object";

    } else
      msg = "Expecting composite command object";

    if (msg) {
      printf("Error in command Obj: %s.\n", msg);
      return BOXTASK_FAILURE;
    }
  }

  return BOXTASK_OK;
}

static void My_WinCairo_Create_Path(BoxGWin *w) {
  beginning_of_path = 1;
}

static void My_WinCairo_Begin_Drawing(BoxGWin *w) {}

static void My_WinCairo_Draw_Path(BoxGWin *w, DrawStyle *style) {
  cairo_t *cr = (cairo_t *) w->ptr;
  int do_fill, do_clip, do_even_odd, do_border = (style->bord_width > 0.0);

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
      Real border = MY_REAL(w, scale*style->bord_width);
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
        Real miter_limit = MY_REAL(w, scale*style->bord_miter_limit);
        cairo_set_miter_limit(cr, miter_limit);
      }
      if (style->bord_num_dashes > 0) {
        Int num_dashes = style->bord_num_dashes,
            size_dashes = num_dashes*sizeof(Real);
        Real *dashes = (Real *) malloc(size_dashes);
        if (dashes != (Real *) NULL) {
          Int i;
          Real dash_offset = MY_REAL(w, scale*style->bord_dash_offset);
          for(i=0; i<num_dashes; i++)
            dashes[i] = MY_REAL(w, scale*style->bord_dashes[i]);
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

static void My_WinCairo_Set_Fg_Color(BoxGWin *w, Color *c) {
  cairo_t *cr = (cairo_t *) w->ptr;
  cairo_set_source_rgba(cr, c->r, c->g, c->b, c->a);
}

static void My_WinCairo_Set_Gradient(BoxGWin *w, ColorGrad *cg) {
  cairo_t *cr = (cairo_t *) w->ptr;
  cairo_pattern_t *p;
  cairo_matrix_t m, m_inv;
  Point p1, p2, ref1, ref2;
  Int i;

  switch(cg->type) {
  case COLOR_GRAD_TYPE_LINEAR:
    My_Map_Point(w, & p1, & cg->point1);
    My_Map_Point(w, & p2, & cg->point2);
    p = cairo_pattern_create_linear(p1.x, p1.y, p2.x, p2.y);
    break;

  case COLOR_GRAD_TYPE_RADIAL:
    My_Map_Point(w, & p1, & cg->point1);
    My_Map_Point(w, & p2, & cg->point2);
    My_Map_Point(w, & ref1, & cg->ref1);
    My_Map_Point(w, & ref2, & cg->ref2);
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
  for (i = 0; i < cg->num_items; i++) {
    ColorGradItem *cgi = & cg->items[i];
    Color *c = & cgi->color;
    cairo_pattern_add_color_stop_rgba(p, cgi->position,
                                      c->r, c->g, c->b, c->a);
  }
  cairo_set_source(cr, p);
  cairo_pattern_destroy(p);
}

static void My_WinCairo_Add_Line_Path(BoxGWin *w, Point *a, Point *b) {
  cairo_t *cr = (cairo_t *) w->ptr;
  MY_2POINTS(w, a, b);

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

static void My_WinCairo_Add_Join_Path(BoxGWin *w,
                                      Point *a, Point *b, Point *c) {
  cairo_t *cr = (cairo_t *) w->ptr;
  Point *first = a, *last = c;
  MY_3POINTS(w, a, b, c);

  if (same_points(a, c))
    return;

  else if (same_points(a, b) || same_points(b, c)) {
    My_WinCairo_Add_Line_Path(w, first, last);
    return;

  } else {
    if (beginning_of_path) {
      cairo_new_path(cr);
      beginning_of_path = 0;
    }

    My_Cairo_JoinArc(cr, a, b, c);
    previous = *c;
  }
}

static void My_WinCairo_Close_Path(BoxGWin *w) {
  cairo_t *cr = (cairo_t *) w->ptr;
  if (!beginning_of_path)
    cairo_close_path(cr);
}

static void My_WinCairo_Add_Circle_Path(BoxGWin *w,
                                        Point *ctr, Point *a, Point *b) {
  cairo_t *cr = (cairo_t *) w->ptr;
  MY_3POINTS(w, ctr, a, b);

  if (beginning_of_path)
    cairo_new_path(cr);

  cairo_move_to(cr, a->x, a->y);
  My_Cairo_Arc(cr, ctr, a, b, 0.0, 2.0*M_PI);
  beginning_of_path = 0;
}

static void My_WinCairo_Text_Path(BoxGWin *w, BoxPoint *ctr, BoxPoint *right,
                                  BoxPoint *up, BoxPoint *from,
                                  const char *text) {
  MY_3POINTS(w, ctr, right, up);
  My_Cairo_Text_Path(w, ctr, right, up, from, text);
}

static int My_WinCairo_Save_To_File(BoxGWin *w, const char *file_name) {
  cairo_t *cr = (cairo_t *) w->ptr;
  cairo_surface_t *surface = cairo_get_target(cr);
  char *exts[] = {"png", "pdf", (char *) NULL};
  enum {EXT_PNG=0};
  cairo_status_t status;

  if (w->win_type_str != wincairo_image_id_string)
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
static void wincairo_repair(BoxGWin *w) {
  BoxGWin_Block(w);
  w->interpret = My_WinCairo_Interpret;
  w->save_to_file = My_WinCairo_Save_To_File;
  w->finish_drawing = My_Wincairo_Finish_Drawing;
  w->create_path = My_WinCairo_Create_Path;
  w->begin_drawing = My_WinCairo_Begin_Drawing;
  w->draw_path = My_WinCairo_Draw_Path;
  w->set_fg_color = My_WinCairo_Set_Fg_Color;
  w->set_gradient = My_WinCairo_Set_Gradient;
  w->add_line_path = My_WinCairo_Add_Line_Path;
  w->add_join_path = My_WinCairo_Add_Join_Path;
  w->close_path = My_WinCairo_Close_Path;
  w->add_circle_path = My_WinCairo_Add_Circle_Path;
  w->set_font = My_Cairo_Set_Font;
  w->add_text_path = My_WinCairo_Text_Path;
}

BoxGWin *cairo_open_win(GrpWindowPlan *plan) {
  BoxGWin *w;
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

    /* These quantities are used in the function My_Map_Point (macros
     * MY_2POINTS and MY_3POINTS) to scale the coordinates of every point.
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
  if (paint_background && numptx > 0 && numpty > 0) {
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
