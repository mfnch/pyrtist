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
#include <string.h>
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

#include <box/types.h>
#include <box/mem.h>
#include <box/array.h>

#include "error.h"
#include "graphic.h"
#include "g.h"
#include "wincairo.h"
#include "psfonts.h"
#include "formatter.h"
#include "obj.h"
#include "cmd.h"

/*#define DEBUG*/

/*****************************************************************************
 * Below we provide functionality to save and restore just the line state.
 * This is useful to implement separate border and line settings so that the
 * border for polygons does not affect the way lines are traced (Line command).
 *
 * NOTE ON SEPARATION BETWEEN BORDER LINE STYLE AND Line LINE STYLE:
 * This makes sense, as often Line, Poly, etc are used to draw different parts
 * of the same things, while the border setting is conceptually different.
 */

typedef struct {
  cairo_pattern_t   *pattern;
  double            width,
                    miter_limit;
  cairo_line_cap_t  cap;
  cairo_line_join_t join;
  int               dash_count;
  double            *dashes,
                    dash_offset;

} MyLineState;

/** Always call the Init method before using a MyLineState. */
static void MyLineState_Init(MyLineState *ls) {
  ls->pattern = NULL;
  ls->width = 0.0;
  ls->miter_limit = 10.0;
  ls->cap = CAIRO_LINE_CAP_BUTT;
  ls->join = CAIRO_LINE_JOIN_MITER;
  ls->dash_count = 0;
  ls->dashes = NULL;
}

/** Copy 'src' to 'uninit_dest' assuming 'uninit_dest' points to uninitialized
 * memory which is big enough to hold a MyLineState object.
 */
static void MyLineState_Duplicate(MyLineState *uninit_dest, MyLineState *src) {
  size_t dashes_size = Box_Mem_Safe_AX(sizeof(double), src->dash_count);
  *uninit_dest = *src;
  cairo_pattern_reference(uninit_dest->pattern);
  uninit_dest->dashes = Box_Mem_Safe_Alloc(dashes_size);
  (void) memcpy(uninit_dest->dashes, src->dashes, dashes_size);
}

/** Relocate the MyLineState object at location 'src' to 'uninit_dest'.
 * 'src' will be cleared after the operation (so that you can freely call
 * MyLineState_Finish without compromising the data in 'uninit_dest'.
 * Note that 'uninit_dest' is assumed to be raw uninitialized memory
 * (in other words MyLineState_Finish is not called before overwriting it
 * with 'src').
 */
static void MyLineState_Relocate(MyLineState *uninit_dest, MyLineState *src) {
  *uninit_dest = *src;
  MyLineState_Init(src);
}

/** Always call the Finish method before using a MyLineState. */
static void MyLineState_Finish(MyLineState *ls) {
  cairo_pattern_destroy(ls->pattern);
  Box_Mem_Free(ls->dashes);
  ls->pattern = NULL;
  ls->dashes = NULL;
}

/** Save the line state from the cairo context 'cr' to 'ls'. */
static void MyLineState_Save(MyLineState *ls, cairo_t *cr) {
  MyLineState_Finish(ls);

  ls->width = cairo_get_line_width(cr);
  ls->cap = cairo_get_line_cap(cr);
  ls->join = cairo_get_line_join(cr);
  ls->miter_limit = cairo_get_miter_limit(cr);

  ls->dash_count = cairo_get_dash_count(cr);
  ls->dashes = Box_Mem_Safe_Alloc_Items(sizeof(double), ls->dash_count);
  cairo_get_dash(cr, ls->dashes, & ls->dash_offset);

  ls->pattern = cairo_get_source(cr);
  cairo_pattern_reference(ls->pattern);
}

/** Restore the line state from 'ls' back to the cairo context 'cr'. */
static void MyLineState_Restore(MyLineState *ls, cairo_t *cr) {
  cairo_set_line_width(cr, ls->width);
  cairo_set_line_cap(cr, ls->cap);
  cairo_set_line_join(cr, ls->join);
  cairo_set_miter_limit(cr, ls->miter_limit);

  if (ls->pattern != NULL)
    cairo_set_source(cr, ls->pattern);

  assert(ls->dashes != NULL || ls->dash_count == 0);
  cairo_set_dash(cr, ls->dashes, ls->dash_count, ls->dash_offset);
}

static void MyLineState_Exchange(MyLineState *ls, cairo_t *cr) {
  MyLineState my_ls;
  MyLineState_Init(& my_ls);
  MyLineState_Save(& my_ls, cr);
  MyLineState_Restore(ls, cr);
  MyLineState_Finish(ls);
  *ls = my_ls;
}

/*****************************************************************************/

/** Type of the BoxGWin->data pointer, containing data specific to the Cairo
 * window type
 */
typedef struct {
  cairo_pattern_t *pattern;
  MyLineState     line_state;
  BoxArr          saved_line_states;

} MyCairoWinData;

static void My_Saved_LineState_Finalizer(void *item) {
  MyLineState_Finish((MyLineState *) item);
}

static void MyCairoWinData_Init(MyCairoWinData *wd) {
  wd->pattern = NULL;
  MyLineState_Init(& wd->line_state);
  BoxArr_Init(& wd->saved_line_states, sizeof(MyLineState), 2);
  BoxArr_Set_Finalizer(& wd->saved_line_states, My_Saved_LineState_Finalizer);
}

static void MyCairoWinData_Finish(MyCairoWinData *wd) {
  if (wd->pattern != NULL)
    cairo_pattern_destroy(wd->pattern);
  MyLineState_Finish(& wd->line_state);
  BoxArr_Finish(& wd->saved_line_states);
}

static void MyCairoWinData_LineState_Push_Any(MyCairoWinData *wd,
                                              MyLineState *ls) {
  MyLineState *new_ls = BoxArr_Push(& wd->saved_line_states, NULL);
  assert(new_ls != NULL);
  MyLineState_Duplicate(new_ls, ls);
}

static void MyCairoWinData_LineState_Pop_Any(MyCairoWinData *wd,
                                             MyLineState *ls) {
  MyLineState *top_ls = BoxArr_Last_Item_Ptr(& wd->saved_line_states);
  MyLineState_Finish(ls);
  MyLineState_Relocate(ls, top_ls);
  BoxArr_Pop(& wd->saved_line_states, NULL);
}

static void MyCairoWinData_LineState_Push(MyCairoWinData *wd) {
  MyCairoWinData_LineState_Push_Any(wd, & wd->line_state);
}

static void MyCairoWinData_LineState_Pop(MyCairoWinData *wd) {
  MyCairoWinData_LineState_Pop_Any(wd, & wd->line_state);
}


/** Macro to access Cairo specific window data. */
#define MY_DATA(w) ((MyCairoWinData *) (w)->data)

/*****************************************************************************/

/* Invert the cairo matrix 'in' and put the result in 'result'.
 * The determinant of 'in' is returned and is 0.0, if the matrix
 * couldn't be inverted.
 */
static BoxReal My_Invert_Cairo_Matrix(cairo_matrix_t *result, cairo_matrix_t *in) {
  BoxReal det = in->xx*in->yy - in->xy*in->yx, inv_det;
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

static void My_WinCairo_Finish(BoxGWin *w) {
  cairo_t *cr = (cairo_t *) w->ptr;
  cairo_surface_t *surface = cairo_get_target(cr);
    
  cairo_show_page(cr);
  cairo_destroy(cr);
  cairo_surface_destroy(surface);

  MyCairoWinData_Finish(MY_DATA(w));
  Box_Mem_Free(MY_DATA(w));
}

/* Variabili usate dalle procedure per scrivere il file postscript */
static int beginning_of_path = 1;
static BoxPoint previous;

static int same_points(BoxPoint *a, BoxPoint *b) {
  return (fabs(a->x - b->x) < 1e-10 && fabs(a->y - b->y) < 1e-10);
}

/** This is the function used to map points. */
static void My_Map_Point(BoxGWin *w, BoxPoint *out, BoxPoint *in) {
  out->x = (in->x - w->ltx)*w->resx;
  out->y = (in->y - w->lty)*w->resy;
}

/* Macros used to scale the point coordinates */
#define MY_POINT(w, a) \
  BoxPoint my_a; My_Map_Point((w), & my_a, (a)); (a) = & my_a

#define MY_2POINTS(w, a, b) \
  BoxPoint my_a, my_b; \
  My_Map_Point((w), & my_a, (a)); My_Map_Point((w), & my_b, (b)); \
  (a) = & my_a; (b) = & my_b

#define MY_3POINTS(w, a, b, c) \
  BoxPoint my_a, my_b, my_c; \
  My_Map_Point((w), & my_a, (a)); My_Map_Point((w), & my_b, (b)); \
  My_Map_Point((w), & my_c, (c)); \
  (a) = & my_a; (b) = & my_b; (c) = & my_c

/* This is broken, but is required for fonts (fonts support is broken as well
 * and needs to be better done: font drawing should depend on Points, not on
 * a given Real size!)
 */
#define MY_REAL(w, r) ((r)*(w)->resx)

/* BEGIN OF TEXT FORMATTING IMPLEMENTATION **********************************
 * Here we implement some basic text formatting features, such as           *
 * subscripts, superscripts and multi-line paragraphs. We exploit           *
 * the parser provided by the formatter.c module.                           *
 ****************************************************************************/

typedef struct {
  cairo_t *cr;
  BoxArr  saved_states;
  BoxPoint   sub_vec,
          sup_vec;
  BoxReal    sub_scale,
          sup_scale;
} MyTextPrivate;

typedef struct {
  BoxPoint          cur_pos;
  cairo_matrix_t m;
} MyTextState;

static void My_Text_Fmt_Draw(BoxGFmtStack *stack) {
  BoxGFmt *fmt = BoxGFmt_Get(stack);
  MyTextPrivate *private = BoxGFmt_Get_Private(fmt);
  char *text = BoxGFmt_Get_Buffer(stack);
  cairo_text_path(private->cr, text);
  BoxGFmt_Clear_Buffer(stack);
}

static void My_Text_Fmt_Save(BoxGFmtStack *stack) {
  BoxGFmt *fmt = BoxGFmt_Get(stack);
  MyTextPrivate *private = BoxGFmt_Get_Private(fmt);
  MyTextState ss;
  cairo_get_matrix(private->cr, & ss.m);
  cairo_get_current_point(private->cr, & ss.cur_pos.x, & ss.cur_pos.y);
  (void) BoxArr_Push(& private->saved_states, & ss);
}

static void My_Text_Fmt_Restore(BoxGFmtStack *stack) {
  BoxGFmt *fmt = BoxGFmt_Get(stack);
  MyTextPrivate *private = BoxGFmt_Get_Private(fmt);
  MyTextState *ts = BoxArr_Last_Item_Ptr(& private->saved_states);
  double x, y;
  cairo_set_matrix(private->cr, & ts->m);
  cairo_get_current_point(private->cr, & x, & y);
  cairo_move_to(private->cr, x, ts->cur_pos.y);
  BoxArr_Pop(& private->saved_states, NULL);
}

static void My_Text_Fmt_Change(cairo_t *cr, BoxPoint *vec, BoxReal scale) {
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
  MyTextPrivate *private = BoxGFmt_Get_Private(fmt);
  My_Text_Fmt_Change(private->cr, & private->sup_vec, private->sup_scale);
}

static void My_Text_Fmt_Subscript(BoxGFmtStack *stack) {
  BoxGFmt *fmt = BoxGFmt_Get(stack);
  MyTextPrivate *private = BoxGFmt_Get_Private(fmt);
  My_Text_Fmt_Change(private->cr, & private->sub_vec, private->sub_scale);
}

static void My_Text_Fmt_Newline(BoxGFmtStack *stack) {
  BoxGFmt *fmt = BoxGFmt_Get(stack);
  MyTextPrivate *private = BoxGFmt_Get_Private(fmt);
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

static void My_Cairo_Text_Path(BoxGWin *w, BoxPoint *ctr, BoxPoint *right,
                               BoxPoint *up, BoxPoint *from, const char *text) {
  cairo_t *cr = (cairo_t *) w->ptr;

  /* Here we need to first generate the path, calculate its extent
   * and then position it according to the value of 'from'. The problem:
   * how do we do this without disturbing the path sitting in the main cairo
   * context 'cr'. At the moment, the only option I see is to create a second
   * cairo_t from 'cr' and do the path computation in there.
   * NOTE: it may be worth to store 'my_cr' so that it can be reused in other
   *   calls to this function (mainly to reuse the font face).
   */
  cairo_surface_t *cs = cairo_get_target(cr);
  assert(cairo_surface_status(cs) == CAIRO_STATUS_SUCCESS);
  /* Status should always be successful, unless cr is broken. In this case
   * also cs will be "broken" (see doc for cairo_get_target).
   * The assertion above excludes this case.
   */

  cairo_t *my_cr = cairo_create(cs);
  if (cairo_status(my_cr) == CAIRO_STATUS_SUCCESS) {
    /* We first copy settings from 'cr' to 'my_cr' */
    cairo_font_face_t *ff = cairo_get_font_face(cr);
    if (cairo_font_face_status(ff) == CAIRO_STATUS_SUCCESS) {
      /* Copy the coordinate system, first */
      cairo_matrix_t m, m0;
      cairo_get_matrix(cr, & m0);

      /* Set the matrix to the region defined by 'ctr', 'up' and 'right' */
      m.xx = right->x - ctr->x;  m.yx = right->y - ctr->y;
      m.xy = up->x - ctr->x;  m.yy = up->y - ctr->y;
      m.x0 = ctr->x; m.y0 = ctr->y;
      cairo_transform(cr, & m);

      cairo_get_matrix(cr, & m);
      cairo_set_matrix(my_cr, & m);
      cairo_move_to(my_cr, 0.0, 0.0);

      /* Set the font face and matrix */
      cairo_set_font_face(my_cr, ff);
      m.xx = 1.0; m.yy = -1.0;
      m.xy = m.yx = m.x0 = m.y0 = 0.0;
      cairo_set_font_matrix(my_cr, & m);

      {
        /* We now draw the path to 'my_cr' */
        MyTextPrivate private;
        BoxGFmt fmt;

        My_Text_Fmt_Init(& fmt);
        BoxGFmt_Set_Private(& fmt, & private);
        private.cr = my_cr;
        private.sup_vec.x = 0.0;
        private.sup_vec.y = 0.5;
        private.sup_scale = 0.5;
        private.sub_vec.x = 0.0;
        private.sub_vec.y = -0.1;
        private.sub_scale = 0.5;
        BoxArr_Init(& private.saved_states, sizeof(MyTextState), 8);
        BoxGFmt_Draw_Text(& fmt, text);

        BoxArr_Finish(& private.saved_states);
      }

      {
        cairo_path_t *text_path;
        double x1, y1, x2, y2;

        /* Go back to cr's coordinates */
        cairo_get_matrix(cr, & m);
        cairo_set_matrix(my_cr, & m);

        /* Get extent and path */
        cairo_fill_extents(my_cr, & x1, & y1, & x2, & y2);
        text_path = cairo_copy_path(my_cr);
        assert(text_path->status == CAIRO_STATUS_SUCCESS);

        /* Translate 'cr' as specified by 'from', and draw the stored path */
        cairo_translate(cr,
                        -x1 - (x2 - x1)*from->x,
                        -y1 - (y2 - y1)*from->y);
        cairo_append_path(cr, text_path);
        cairo_path_destroy(text_path);

        /* Restore the coordinate system */
        cairo_set_matrix(cr, & m0);
      }
            
      beginning_of_path = 0;
    }

  } else
    g_warning("My_Cairo_Text_Path: Cannot create cairo context. ");

  cairo_destroy(my_cr);
}

/* END OF TEXT FORMATTING IMPLEMENTATION ************************************/

static void My_Cairo_JoinArc(cairo_t *cr,
                             const BoxPoint *a, const BoxPoint *b, const BoxPoint *c) {
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
                         const BoxPoint *ctr, const BoxPoint *a, const BoxPoint *b,
                         BoxReal angle_begin, BoxReal angle_end) {
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

static cairo_pattern_t *
My_Cairo_Pattern_Create_Radial(BoxPoint *p1, BoxPoint *xone, BoxPoint *yone,
                               BoxPoint *p2, BoxReal rad1, BoxReal rad2) {
  cairo_pattern_t *pattern;
  BoxPoint xaxis, yaxis;
  cairo_matrix_t m, m_inv;
  double p2xt = p2->x, p2yt = p2->y;

  xaxis.x = xone->x - p1->x;
  xaxis.y = xone->y - p1->y;
  yaxis.x = yone->x - p1->x;
  yaxis.y = yone->y - p1->y;

  m.xx = xaxis.x; m.yx = xaxis.y;
  m.xy = yaxis.x; m.yy = yaxis.y;
  m.x0 = p1->x; m.y0 = p1->y;

  if (My_Invert_Cairo_Matrix(& m_inv, & m) == 0.0)
    return NULL;

  cairo_matrix_transform_point(& m_inv, & p2xt, & p2yt);
  pattern = cairo_pattern_create_radial(0.0, 0.0, rad1,
                                        p2xt, p2yt, rad2);
  cairo_pattern_set_matrix(pattern, & m_inv);
  return pattern;
}

static cairo_pattern_t *
My_Cairo_Pattern_Create_Image(cairo_t *cr,
                              const BoxPoint *zero,
                              const BoxPoint *xone, const BoxPoint *yone,
                              const BoxPoint *from,
                              const char *filename)
{
  cairo_pattern_t *pattern;
  cairo_surface_t *image = cairo_image_surface_create_from_png(filename);
  double lx = cairo_image_surface_get_width(image),
         ly = cairo_image_surface_get_height(image);

  cairo_matrix_t m, m_inv;

  m.xx = (xone->x - zero->x)/lx; m.yx = (xone->y - zero->y)/lx;
  m.xy = (zero->x - yone->x)/ly; m.yy = (zero->y - yone->y)/ly;
  m.x0 = yone->x; m.y0 = yone->y;

  if (My_Invert_Cairo_Matrix(& m_inv, & m) == 0.0)
    return NULL;

  m_inv.x0 += from->x*lx;
  m_inv.y0 -= from->y*ly;

  pattern = cairo_pattern_create_for_surface(image);
  if (cairo_pattern_status(pattern) != CAIRO_STATUS_SUCCESS)
    return NULL;

  cairo_pattern_set_matrix(pattern, & m_inv);
  return pattern;
}

static void My_Cairo_Fill_And_Stroke(BoxGWin *w) {
  cairo_t *cr = (cairo_t *) w->ptr;  
  double line_width = MY_DATA(w)->line_state.width;
  if (line_width > 0.0) {
    cairo_fill_preserve(cr);
    MyLineState_Exchange(& MY_DATA(w)->line_state, cr);
    cairo_stroke(cr);
    MyLineState_Exchange(& MY_DATA(w)->line_state, cr);

  } else
    cairo_fill(cr);
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
#if defined(CAIRO_VERSION) && defined(CAIRO_VERSION_ENCODE)
#  if CAIRO_VERSION > CAIRO_VERSION_ENCODE(1, 10, 0)
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
#  endif
#endif
  default: return CAIRO_OPERATOR_OVER;
  }
}

/** Structure used to pass data to the interator function
 * My_WinCairo_Interpret_Iter (which is called by My_WinCairo_Interpret).
 */
typedef struct {
  BoxGWin    *win;
  BoxGWinMap *map;
} MyInterpretData;

static BoxGErr
My_WinCairo_Interpret_Iter(BoxGCmd cmd, BoxGCmdSig sig, int num_args,
                           BoxGCmdArgKind *kinds, void **args,
                           BoxGCmdArg *aux, void *pass) {
  BoxGWin *w = ((MyInterpretData *) pass)->win;
  BoxGWinMap *map = ((MyInterpretData *) pass)->map;
  cairo_t *cr = (cairo_t *) w->ptr;
  size_t i;

  /* We first transform the arguments */
  for (i = 0; i < num_args; i++) {
    void *arg_in = args[i];
    BoxGCmdArg *arg_out = & aux[i];

    switch (kinds[i]) {
    case BOXGCMDARGKIND_POINT:
      BoxGWinMap_Map_Point(map, & arg_out->p, (BoxPoint *) arg_in);
      args[i] = & arg_out->p;
      break;

    case BOXGCMDARGKIND_VECTOR:
      BoxGWinMap_Map_Vector(map, & arg_out->p, (BoxPoint *) arg_in);
      args[i] = & arg_out->p;
      break;

    case BOXGCMDARGKIND_WIDTH:
      BoxGWinMap_Map_Width(map, & arg_out->w, (BoxReal *) arg_in);
      args[i] = & arg_out->w;
      break;

    default:
      break;
    }
  }

  switch (cmd) {
  case BOXGCMD_SAVE:
    cairo_save(cr);
    return BOXGERR_NO_ERR;

  case BOXGCMD_RESTORE:
    cairo_restore(cr);
    return BOXGERR_NO_ERR;

  case BOXGCMD_SET_ANTIALIAS:
    {
      BoxInt *arg1 = args[0];
      cairo_antialias_t v;
      switch(*arg1) {
      default:
      case 0: v = CAIRO_ANTIALIAS_DEFAULT; break;
      case 1: v = CAIRO_ANTIALIAS_NONE; break;
      case 2: v = CAIRO_ANTIALIAS_GRAY; break;
      case 3: v = CAIRO_ANTIALIAS_SUBPIXEL; break;
      }
      cairo_set_antialias(cr, v);
      return BOXGERR_NO_ERR;
    }
    break;

  case BOXGCMD_MOVE_TO:
    {
      BoxPoint *arg1 = args[0];
      cairo_move_to(cr, arg1->x, arg1->y);
      return BOXGERR_NO_ERR;
    }
    break;

  case BOXGCMD_LINE_TO:
    {
      BoxPoint *arg1 = args[0];
      cairo_line_to(cr, arg1->x, arg1->y);
      return BOXGERR_NO_ERR;
    }
    break;

  case BOXGCMD_CURVE_TO:
    {
      BoxPoint *arg1 = args[0], *arg2 = args[1], *arg3 = args[2];
      cairo_curve_to(cr, arg1->x, arg1->y, arg2->x, arg2->y, arg3->x, arg3->y);
      return BOXGERR_NO_ERR;
    }
    break;

  case BOXGCMD_CLOSE_PATH:
    cairo_close_path(cr);
    return BOXGERR_NO_ERR;

  case BOXGCMD_NEW_PATH:
    cairo_new_path(cr);
    return BOXGERR_NO_ERR;

  case BOXGCMD_NEW_SUB_PATH:
    cairo_new_sub_path(cr);
    return BOXGERR_NO_ERR;

  case BOXGCMD_STROKE:
    cairo_stroke(cr);
    return BOXGERR_NO_ERR;

  case BOXGCMD_STROKE_PRESERVE:
    cairo_stroke_preserve(cr);
    return BOXGERR_NO_ERR;

  case BOXGCMD_FILL:
    cairo_fill(cr);
    return BOXGERR_NO_ERR;

  case BOXGCMD_FILL_PRESERVE:
    cairo_fill_preserve(cr);
    return BOXGERR_NO_ERR;

  case BOXGCMD_CLIP:
    cairo_clip(cr);
    return BOXGERR_NO_ERR;

  case BOXGCMD_CLIP_PRESERVE:
    cairo_clip_preserve(cr);
    return BOXGERR_NO_ERR;

  case BOXGCMD_RESET_CLIP:
    cairo_reset_clip(cr);
    return BOXGERR_NO_ERR;

  case BOXGCMD_PUSH_GROUP:
    cairo_push_group(cr);
    return BOXGERR_NO_ERR;

  case BOXGCMD_POP_GROUP_TO_SOURCE:
    cairo_pop_group_to_source(cr);
    return BOXGERR_NO_ERR;

  case BOXGCMD_SET_OPERATOR:
    {
      BoxInt *arg1 = args[0];
      cairo_set_operator(cr, My_Cairo_Operator_Of_Int(*arg1));
      return BOXGERR_NO_ERR;
    }
    break;

  case BOXGCMD_PAINT:
    cairo_paint(cr);
    return BOXGERR_NO_ERR;

  case BOXGCMD_PAINT_WITH_ALPHA:
    {
      BoxReal *arg1 = args[0];
      cairo_paint_with_alpha(cr, *arg1);
      return BOXGERR_NO_ERR;
    }
    return BOXGERR_NO_ERR;

  case BOXGCMD_COPY_PAGE:
    cairo_copy_page(cr);
    return BOXGERR_NO_ERR;

  case BOXGCMD_SHOW_PAGE:
    cairo_show_page(cr);
    return BOXGERR_NO_ERR;

  case BOXGCMD_SET_LINE_WIDTH:
    {
      BoxReal *arg1 = args[0];
      cairo_set_line_width(cr, *arg1);
      return BOXGERR_NO_ERR;
    }
    break;

  case BOXGCMD_SET_LINE_CAP:
    {
      BoxInt *arg1 = args[0];
      cairo_line_cap_t v;
      switch(*arg1) {
      default:
      case 0: v = CAIRO_LINE_CAP_BUTT; break;
      case 1: v = CAIRO_LINE_CAP_ROUND; break;
      case 2: v = CAIRO_LINE_CAP_SQUARE; break;
      }
      cairo_set_line_cap(cr, v);
      return BOXGERR_NO_ERR;
    }
    break;

  case BOXGCMD_SET_LINE_JOIN:
    {
      BoxInt *arg1 = args[0];
      cairo_line_join_t v;
      switch(*arg1) {
      default:
      case 0: v = CAIRO_LINE_JOIN_MITER; break;
      case 1: v = CAIRO_LINE_JOIN_ROUND; break;
      case 2: v = CAIRO_LINE_JOIN_BEVEL; break;
      }
      cairo_set_line_join(cr, v);
      return BOXGERR_NO_ERR;
    }
    break;

  case BOXGCMD_SET_MITER_LIMIT:
    {
      BoxReal *arg1 = args[0];
      cairo_set_miter_limit(cr, *arg1);
      return BOXGERR_NO_ERR;
    }

  case BOXGCMD_SET_DASH:
    {
      int num_dashes = num_args - 1;
      double offset = *((BoxReal *) args[0]),
             *dashes = NULL;

      assert(num_args >= 1);

      if (num_dashes > 0) {
        size_t i;
        dashes = Box_Mem_Safe_Alloc(num_dashes*sizeof(double));
        for (i = 0; i < num_dashes; i++)
          dashes[i] = *((BoxReal *) args[1 + i]);

      }

      cairo_set_dash(cr, dashes, num_dashes, offset);
      Box_Mem_Free(dashes);
      return BOXGERR_NO_ERR;
    }

  case BOXGCMD_SET_FILL_RULE:
    {
      BoxInt *arg1 = args[0];
      cairo_fill_rule_t v;
      switch(*arg1) {
      default:
      case 0: v = CAIRO_FILL_RULE_WINDING; break;
      case 1: v = CAIRO_FILL_RULE_EVEN_ODD; break;
      }
      cairo_set_fill_rule(cr, v);
      return BOXGERR_NO_ERR;
    }
    break;

  case BOXGCMD_SET_SOURCE_RGBA:
    {
      BoxReal *arg1 = args[0], *arg2 = args[1],
              *arg3 = args[2], *arg4 = args[3];
      cairo_set_source_rgba(cr, *arg1, *arg2, *arg3, *arg4);
      return BOXGERR_NO_ERR;
    }
    break;

  case BOXGCMD_TEXT_PATH:
    {
      BoxStr *s = args[0];
      char *text = BoxStr_To_C_String(s);
      if (text != NULL) {
        cairo_text_path(cr, text);
        Box_Mem_Free(text);
        return BOXGERR_NO_ERR;
      }
    }
    break;

  case BOXGCMD_TRANSLATE:
    {
      BoxPoint *arg1 = args[1];
      cairo_translate(cr, arg1->x, arg1->y);
      return BOXGERR_NO_ERR;
    }
    break;

  case BOXGCMD_SCALE:
    {
      BoxPoint *arg1 = args[1];
      cairo_scale(cr, arg1->x, arg1->y);
      return BOXGERR_NO_ERR;
    }
    break;

  case BOXGCMD_ROTATE:
    {
      BoxReal *arg1 = args[0];
      cairo_rotate(cr, *arg1);
      return BOXGERR_NO_ERR;
    }
    break;

  case BOXGCMD_MASK:
    if (MY_DATA(w)->pattern != NULL)
      cairo_mask(cr, MY_DATA(w)->pattern);
    break;

  case BOXGCMD_PATTERN_CREATE_RGB:
    {
      BoxReal *arg1 = args[0], *arg2 = args[1], *arg3 = args[2];
      if (MY_DATA(w)->pattern != NULL)
        cairo_pattern_destroy(MY_DATA(w)->pattern);

      MY_DATA(w)->pattern =
        cairo_pattern_create_rgb(*arg1, *arg2, *arg3);
      return BOXGERR_NO_ERR;
    }
    break;

  case BOXGCMD_PATTERN_CREATE_RGBA:
    {
      BoxReal *arg1 = args[0], *arg2 = args[1],
              *arg3 = args[2], *arg4 = args[3];
      if (MY_DATA(w)->pattern != NULL)
        cairo_pattern_destroy(MY_DATA(w)->pattern);

      MY_DATA(w)->pattern =
        cairo_pattern_create_rgba(*arg1, *arg2, *arg3, *arg4);
      return BOXGERR_NO_ERR;
    }
    break;

  case BOXGCMD_PATTERN_CREATE_LINEAR:
    {
      BoxPoint *arg1 = args[0], *arg2 = args[1];
      if (MY_DATA(w)->pattern != NULL)
        cairo_pattern_destroy(MY_DATA(w)->pattern);

      MY_DATA(w)->pattern =
        cairo_pattern_create_linear(arg1->x, arg1->y, arg2->x, arg2->y);
      return BOXGERR_NO_ERR;
    }
    break;

  case BOXGCMD_PATTERN_CREATE_RADIAL:
    {
      BoxPoint *arg1 = args[0], *arg2 = args[1],
               *arg3 = args[2], *arg4 = args[3];
      BoxReal *arg5 = args[4], *arg6 = args[5];

      if (MY_DATA(w)->pattern != NULL)
        cairo_pattern_destroy(MY_DATA(w)->pattern);

      MY_DATA(w)->pattern =
        My_Cairo_Pattern_Create_Radial(arg1, arg2, arg3, arg4, *arg5, *arg6);
      return (MY_DATA(w)->pattern != NULL) ?
        BOXGERR_NO_ERR : BOXGERR_CAIRO_PATTERN_ERR;
    }
    break;

  case BOXGCMD_PATTERN_CREATE_IMAGE:
    {
      BoxPoint *arg1 = args[0], *arg2 = args[1],
               *arg3 = args[2], *arg4 = args[3];
      BoxStr *arg5 = args[4];
      char *filename = BoxStr_To_C_String(arg5);
      if (filename != NULL) {
        MY_DATA(w)->pattern =
          My_Cairo_Pattern_Create_Image(cr, arg1, arg2, arg3,
                                        arg4, filename);
        Box_Mem_Free(filename);
        return (MY_DATA(w)->pattern != NULL) ?
          BOXGERR_NO_ERR : BOXGERR_CAIRO_PATTERN_ERR;

      } else
        return BOXGERR_CMD_EXEC;
    }
    break;

  case BOXGCMD_PATTERN_ADD_COLOR_STOP_RGB:
    {
      BoxReal *arg1 = args[0], *arg2 = args[1], *arg3 = args[2],
              *arg4 = args[3];
      if (MY_DATA(w)->pattern != NULL)
        cairo_pattern_add_color_stop_rgb(MY_DATA(w)->pattern,
                                         *arg1, *arg2, *arg3, *arg4);
      return BOXGERR_NO_ERR;
    }
    break;

  case BOXGCMD_PATTERN_ADD_COLOR_STOP_RGBA:
    {
      BoxReal *arg1 = args[0], *arg2 = args[1], *arg3 = args[2],
              *arg4 = args[3], *arg5 = args[4];
      if (MY_DATA(w)->pattern != NULL)
        cairo_pattern_add_color_stop_rgba(MY_DATA(w)->pattern, *arg1, *arg2,
                                          *arg3, *arg4, *arg5);
      return BOXGERR_NO_ERR;
    }
    break;

  case BOXGCMD_PATTERN_SET_EXTEND:
    {
      BoxInt *arg1 = args[0];
      cairo_extend_t v;
      switch(*arg1) {
      default:
      case 0: v = CAIRO_EXTEND_NONE; break;
      case 1: v = CAIRO_EXTEND_REPEAT; break;
      case 2: v = CAIRO_EXTEND_REFLECT; break;
      case 3: v = CAIRO_EXTEND_PAD; break;
      }
      cairo_pattern_set_extend(MY_DATA(w)->pattern, v);
      return BOXGERR_NO_ERR;
    }
    break;

  case BOXGCMD_PATTERN_SET_FILTER:
    {
      BoxInt *arg1 = args[0];
      cairo_filter_t v;
      switch(*arg1) {
      default:
      case 0: v = CAIRO_FILTER_FAST; break;
      case 1: v = CAIRO_FILTER_GOOD; break;
      case 2: v = CAIRO_FILTER_BEST; break;
      case 3: v = CAIRO_FILTER_NEAREST; break;
      case 4: v = CAIRO_FILTER_BILINEAR; break;
      case 5: v = CAIRO_FILTER_GAUSSIAN; break;
      }
      cairo_pattern_set_filter(MY_DATA(w)->pattern, v);
      return BOXGERR_NO_ERR;
    }
    break;

  case BOXGCMD_PATTERN_DESTROY:
    if (MY_DATA(w)->pattern != NULL) {
      cairo_pattern_destroy(MY_DATA(w)->pattern);
      MY_DATA(w)->pattern = NULL;
    }
    return BOXGERR_NO_ERR;

  case BOXGCMD_SET_SOURCE:
    if (MY_DATA(w)->pattern != NULL)
      cairo_set_source(cr, MY_DATA(w)->pattern);
    return BOXGERR_NO_ERR;

  case BOXGCMD_EXT_SET_AUTO_BBOX:
  case BOXGCMD_EXT_UNSET_BBOX:
    return BOXGERR_NO_ERR;

  case BOXGCMD_EXT_JOINARC_TO:
    {
      BoxPoint *arg1 = args[0], *arg2 = args[1], *arg3 = args[2];
      My_Cairo_JoinArc(cr, arg1, arg2, arg3);
      return BOXGERR_NO_ERR;
    }
    break;

  case BOXGCMD_EXT_ARC_TO:
    {
      BoxPoint *arg1 = args[0], *arg2 = args[1], *arg3 = args[2];
      BoxReal *arg4 = args[3], *arg5 = args[4];
      My_Cairo_Arc(cr, arg1, arg2, arg3, *arg4, *arg5);
      return BOXGERR_NO_ERR;
    }
    break;

  case BOXGCMD_EXT_SET_FONT:
    {
      BoxStr *arg1 = args[0];
      char *font = BoxStr_To_C_String(arg1);
      if (font != NULL) {
        My_Cairo_Set_Font(w, font);
        Box_Mem_Free(font);
        return BOXGERR_NO_ERR;
      }
    }
    break;

  case BOXGCMD_EXT_TEXT_PATH:
    {
      BoxPoint *arg1 = args[0], *arg2 = args[1],
               *arg3 = args[2], *arg4 = args[3];
      BoxStr *arg5 = args[4];
      char *text = BoxStr_To_C_String(arg5);
      if (text != NULL) {
        My_Cairo_Text_Path(w, arg1, arg2, arg3, arg4, text);
        Box_Mem_Free(text);
        return BOXGERR_NO_ERR;
      }
    }
    break;

  case BOXGCMD_EXT_TRANSFORM:
    {
      /*BoxPoint *arg1 = args[0], *arg2 = args[1], *arg3 = args[2];*/

      /* this command should change the ref frame to the one which
       * has origin in the first point and coordinate (1, 0) at the
       * second point and (0, 1) at the third (not an ortogonal frame)
       * NOT IMPLEMENTED, YET.
       */
      return BOXGERR_NO_ERR;
    }
    break;

  case BOXGCMD_EXT_BORDER_EXCHANGE:
    MyLineState_Exchange(& MY_DATA(w)->line_state, cr);
    return BOXGERR_NO_ERR;

  case BOXGCMD_EXT_BORDER_SAVE:
    MyCairoWinData_LineState_Push(MY_DATA(w));
    return BOXGERR_NO_ERR;

  case BOXGCMD_EXT_BORDER_RESTORE:
    MyCairoWinData_LineState_Pop(MY_DATA(w));
    return BOXGERR_NO_ERR;

  case BOXGCMD_EXT_FILL_AND_STROKE:
    My_Cairo_Fill_And_Stroke(w);
    return BOXGERR_NO_ERR;

  default:
    return BOXGERR_CMD_EXEC;
  }

  return BOXGERR_CMD_EXEC;
}


static BoxTask My_WinCairo_Interpret(BoxGWin *w, BoxGObj *obj, BoxGWinMap *map) {
  BoxGWinMap my_map;
  BoxGMatrix
    *user_mx = BoxGWinMap_Get_Matrix(map),
    *my_mx = BoxGWinMap_Get_Matrix(& my_map);

  MyInterpretData data;
  data.win = w;
  data.map = & my_map;

  my_mx->m11 = w->resx; my_mx->m22 = w->resy;
  my_mx->m12 = my_mx->m21 = 0.0;
  my_mx->m13 = -w->ltx*w->resx;
  my_mx->m23 = -w->lty*w->resy;

  BoxGMatrix_Mul(my_mx, my_mx, user_mx);
  BoxGWinMap_Compute_Width_Transform(& my_map);

  return BoxGCmdIter_Iter(My_WinCairo_Interpret_Iter, obj, & data);
}




static void My_WinCairo_Create_Path(BoxGWin *w) {
  beginning_of_path = 1;
}

static void My_WinCairo_Begin_Drawing(BoxGWin *w) {}

static void My_WinCairo_Draw_Path(BoxGWin *w, DrawStyle *style) {
  cairo_t *cr = (cairo_t *) w->ptr;
  int do_fill, do_clip, do_even_odd, do_border = (style->bord_width > 0.0);

  if ( ! beginning_of_path ) {
    BoxReal scale = style->scale;

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
      BoxReal border = MY_REAL(w, scale*style->bord_width);
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
        BoxReal miter_limit = MY_REAL(w, scale*style->bord_miter_limit);
        cairo_set_miter_limit(cr, miter_limit);
      }
      if (style->bord_num_dashes > 0) {
        BoxInt num_dashes = style->bord_num_dashes,
            size_dashes = num_dashes*sizeof(BoxReal);
        BoxReal *dashes = (BoxReal *) malloc(size_dashes);
        if (dashes != (BoxReal *) NULL) {
          BoxInt i;
          BoxReal dash_offset = MY_REAL(w, scale*style->bord_dash_offset);
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
  BoxPoint p1, p2, ref1, ref2;
  BoxInt i;

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
    if (My_Invert_Cairo_Matrix(& m_inv, & m) == 0.0) {
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

static void My_WinCairo_Add_Line_Path(BoxGWin *w, BoxPoint *a, BoxPoint *b) {
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
                                      BoxPoint *a, BoxPoint *b, BoxPoint *c) {
  cairo_t *cr = (cairo_t *) w->ptr;
  BoxPoint *first = a, *last = c;
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
                                        BoxPoint *ctr, BoxPoint *a, BoxPoint *b) {
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

  switch (file_extension(exts, file_name)) {
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
static void My_WinCairo_Repair(BoxGWin *w) {
  BoxGWin_Block(w);
  w->interpret = My_WinCairo_Interpret;
  w->save_to_file = My_WinCairo_Save_To_File;
  w->finish = My_WinCairo_Finish;
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

BoxGWin *BoxGWin_Create_Cairo(BoxGWinPlan *plan, BoxGErr *err) {
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
  StreamSurfaceCreate stream_surface_create = NULL;

  /* If err == NULL, err points to a dummy BoxGErr, which simplifies coding */
  if (err != NULL)
    *err = BOXGERR_NO_ERR;

  else {
    static BoxGErr dummy_err;
    err = & dummy_err;
  }

  if (!plan->have.type) {
    *err = BOXGERR_MISS_WIN_TYPE;
    return NULL;
  }

  wt = (WT) plan->type;

  /* Allocate space for the BoxGWin object */
  w = BoxGWin_Create_Invalid(err);
  if (*err)
    return NULL;
  assert(w != NULL);

  /* Allocate WinCairo-specific data */
  assert(MY_DATA(w) == NULL);
  MyCairoWinData *wd = Box_Mem_Alloc(sizeof(MyCairoWinData)); 
  if (wd == NULL) {
    BoxGWin_Destroy(w);
    return NULL;
  }
  MyCairoWinData_Init(wd);
  w->data = wd;

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
    *err = BOXGWIN_CAIRO_MISSES_PS;
    return NULL;
#endif

  case WT_EPS:
#ifdef CAIRO_HAS_PS_SURFACE
    stream_surface_create = cairo_ps_surface_create;
    win_class = WC_STREAM;
    break;
#else
    *err = BOXGWIN_CAIRO_MISSES_PS;
    return NULL;
#endif

  case WT_PDF:
#ifdef CAIRO_HAS_PDF_SURFACE
    stream_surface_create = cairo_pdf_surface_create;
    win_class = WC_STREAM;
    break;
#else
    *err = BOXGWIN_CAIRO_MISSES_PDF;
    return NULL;
#endif

  case WT_SVG:
#ifdef CAIRO_HAS_SVG_SURFACE
    stream_surface_create = cairo_svg_surface_create;
    win_class = WC_STREAM;
    break;
#else
    *err = BOXGWIN_CAIRO_MISSES_PDF;
    return NULL;
#endif

  default:
    *err = BOXGERR_UNKNOWN_WIN_TYPE;
    return NULL;
  }

  if (!plan->have.size ) {
    *err = BOXGERR_WIN_SIZE_MISSING;
    return NULL;
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
    if (!plan->have.resolution) {
      *err = BOXGERR_WIN_RES_MISSING;
      return NULL;
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
      *err = BOXGERR_WIN_FILENAME_MISSING;
      return NULL;
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

    if (stream_surface_create == NULL)
      return NULL;

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
    *err = BOXGERR_UNEXPECTED;
    return NULL;
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
    *err = BOXGERR_CAIRO_SURFACE_ERR;
    return NULL;
  }

  cr = cairo_create(surface);
  status = cairo_status(cr);
  if (status != CAIRO_STATUS_SUCCESS) {
    *err = BOXGERR_CAIRO_CONTEXT_ERR;
    return NULL;
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

  /* We now set the Cairo-specific methods to handle the window */
  w->quiet = 0;
  w->repair = My_WinCairo_Repair;
  w->repair(w);

  /* We now allocate Cairo specific data in the Window */
  
  return w;
}
