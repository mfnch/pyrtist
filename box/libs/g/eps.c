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
#include <stdio.h>
#include <math.h>
#include <assert.h>

#include "error.h"
#include "types.h"
#include "graphic.h"
#include "fig.h"
#include "autoput.h"
#include "bb.h"
#include "g.h"
#include "psfonts.h"
#include "formatter.h"

#if 0
/* This terminal discretizes the points into a grid where the number of
 * points per inch in the x and y directions are discretization_x and
 * discretization_y
 */
static int discretization_x = 1000, discretization_y = 1000;
#endif

#if 0
static void eps_rbgcolor(Color *c);
#endif
static int eps_save(const char *unused);

/* Variabili usate dalle procedure per scrivere il file postscript */
static int beginning_of_line = 1, beginning_of_path = 1;
static long previous_px, previous_py;

static Real eps_point_scale = 283.46457;
/* Le coordinate dei punti passati alle funzioni grafiche in questo file,
 * sono espresse in millimetri: le converto nelle unita' postscript:
 *  1 unita' = 1/72 inch = 0.35277777... millimetri
 */
#define EPS_POINT(p, px, py) \
  long px = (p->x * eps_point_scale), \
       py = (p->y * eps_point_scale);

#define EPS_REAL(r) ((r)*eps_point_scale)

static void eps_close_win(void) {
  FILE *f = (FILE *) grp_win->ptr;
  assert(f != (FILE *) NULL);
  fprintf(f, "\nrestore\nshowpage\n%%%%Trailer\n%%EOF\n");
  fclose(f);
}

static void eps_rreset(void) {
  beginning_of_line = 1;
  beginning_of_path = 1;
}

static void eps_rinit(void) {return;}

static void eps_rdraw(DrawStyle *style) {
  if ( ! beginning_of_path ) {
    Real scale = style->scale;
    int do_border = (style->bord_width > 0.0);
    FILE *out = (FILE *) grp_win->ptr;
    char *fill;

    switch(style->fill_style) {
    case FILLSTYLE_VOID:   fill = (char *) NULL; break;
    case FILLSTYLE_PLAIN:  fill = " fill"; break;
    case FILLSTYLE_EO:     fill = " eofill"; break;
    case FILLSTYLE_CLIP:   fill = " clip"; break;
    case FILLSTYLE_EOCLIP: fill = " eoclip"; break;
    default:
      g_warning("Unsupported drawing style: using even-odd fill algorithm!");
      fill = " eoclip"; break;
    }

    if (fill != (char *) NULL) {
      if (do_border)
        fprintf(out, " gsave%s grestore", fill);
      else
        fprintf(out, " %s", fill);
    }

    if (do_border) {
      Color *c = & style->bord_color;
      Real bw = EPS_REAL(scale*style->bord_width);
      int lj, lc;

      switch(style->bord_join_style) {
      case JOIN_STYLE_MITER: lj = 0; break;
      case JOIN_STYLE_ROUND: lj = 1; break;
      case JOIN_STYLE_BEVEL: lj = 2; break;
      default: lj = 1; break;
      }

      switch(style->bord_join_style) {
      case CAP_STYLE_BUTT:   lc = 0; break;
      case CAP_STYLE_ROUND:  lc = 1; break;
      case CAP_STYLE_SQUARE: lc = 2; break;
      default: lc = 0; break;
      }

      fprintf(out, " gsave %g %g %g setrgbcolor"
                   " %g setlinewidth %d setlinejoin %d setlinecap",
                   c->r, c->g, c->b, bw, lj, lc);

      if (style->bord_num_dashes > 0) {
        Int num_dashes = style->bord_num_dashes, i;
        Real dash_offset = EPS_REAL(scale*style->bord_dash_offset);
        char *sep = " [";
        for(i=0; i<num_dashes; i++) {
          fprintf(out, "%s"SReal, sep, EPS_REAL(scale*style->bord_dashes[i]));
          sep = ", ";
        }
        fprintf(out, "] "SReal" setdash", dash_offset);
      }

      if (lj == 0) {
        Real ml = EPS_REAL(scale*style->bord_miter_limit);
        fprintf(out, " %g setmiterlimit stroke grestore\n", ml);
      } else
        fprintf(out, " stroke grestore\n");

    } else {
      fprintf(out, "\n");
    }
  }
}

static void eps_rline(Point *a, Point *b) {
  EPS_POINT(a, ax, ay); EPS_POINT(b, bx, by);
  int continuing = (ax == previous_px) && (ay == previous_py),
      length_zero = (ax == bx && ay == by);

  if (continuing && length_zero) return;

  if (beginning_of_path) {
    fprintf((FILE *) grp_win->ptr, " newpath");
    beginning_of_path = 0;
    continuing = 0;
  }

  if (!continuing)
    fprintf((FILE *) grp_win->ptr, " %ld %ld moveto", ax, ay);

  fprintf((FILE *) grp_win->ptr, " %ld %ld lineto", bx, by);
  previous_px = bx; previous_py = by;
}

static void eps_rcong(Point *a, Point *b, Point *c) {
  EPS_POINT(a, ax, ay); EPS_POINT(b, bx, by); EPS_POINT(c, cx, cy);
#if 0
  int a_eq_b = ax == bx && ay == by,
      a_eq_c = ax == cx && ay == cy,
      b_eq_c = bx == cx && by == cy,
      n_eq = a_eq_b + a_eq_c + b_eq_c;
  if (n_eq == 3) return;
#endif
  if (ax == cx && ay == cy) return;

  if (beginning_of_path) {
    fprintf( (FILE *) grp_win->ptr, " newpath" );
    beginning_of_path = 0;
  }

  fprintf( (FILE *) grp_win->ptr,
   " %ld %ld %ld %ld %ld %ld cong", ax, ay, bx, by, cx, cy );
  previous_px = cx; previous_py = cy;
  beginning_of_line = 0;
}

static void eps_rclose(void) {
  FILE *out = (FILE *) grp_win->ptr;
  if (!beginning_of_path)
    fprintf(out, " closepath");
}

static void eps_rcircle(Point *ctr, Point *a, Point *b) {
  EPS_POINT(ctr, cx, cy); EPS_POINT(a, ax, ay); EPS_POINT(b, bx, by);

  if ( beginning_of_path )
    fprintf( (FILE *) grp_win->ptr, " newpath" );

  fprintf( (FILE *) grp_win->ptr,
   " %ld %ld %ld %ld %ld %ld circle", cx, cy, ax, ay, bx, by );
  beginning_of_line = 1;
  beginning_of_path = 0;
}

static void eps_rfgcolor(Color *c) {
  fprintf( (FILE *) grp_win->ptr,
   "  %g %g %g setrgbcolor\n", c->r, c->g, c->b );
}

static char *Escape_Text(const char *src) {
  const char *s;
  char *d, *dest;
  int n = 0;
  /* Count the number of parenthesis */
  for(s = src; *s != '\0'; s++)
    n += (*s == '(' || *s == ')') ? 2 : 1;
  d = dest = (char *) malloc(n + 1);
  for(s = src; *s != '\0'; s++) {
    switch(*s) {
    case '(': case ')': *(d++) = '\\'; *(d++) = *s; break;
    default: *(d++) = *s; break;
    }
  }
  *d = '\0';
  /*printf("'%s' --> '%s'\n", src, dest);*/
  return dest;
}

/* BEGIN OF TEXT FORMATTING IMPLEMENTATION **********************************
 * Here we implement some basic text formatting features, such as           *
 * subscripts, superscripts and multi-line paragraphs. We exploit           *
 * the parser provided by the formatter.c module.                           *
 ****************************************************************************/

typedef struct {
  FILE *out;
  Point sub_vec, sup_vec;
  Real sub_scale, sup_scale;
} TextPrivate;

static void _Text_Fmt_Draw(FmtStack *stack) {
  Fmt *fmt = Fmt_Get(stack);
  TextPrivate *private = (TextPrivate *) Fmt_Private_Get(fmt);
  char *escaped_text = Escape_Text(Fmt_Buffer_Get(stack));
  fprintf(private->out, "  (%s) true charpath\n", escaped_text);
  free(escaped_text);
  Fmt_Buffer_Clear(stack);
}

static void _Text_Fmt_Superscript(FmtStack *stack) {
  Fmt *fmt = Fmt_Get(stack);
  TextPrivate *private = (TextPrivate *) Fmt_Private_Get(fmt);
  Real scale = private->sup_scale;
  fprintf(private->out,
          "  currentpoint translate %g %g translate %g %g scale 0 0 moveto\n",
          private->sup_vec.x, private->sup_vec.y, scale, scale);
}

static void _Text_Fmt_Subscript(FmtStack *stack) {
  Fmt *fmt = Fmt_Get(stack);
  TextPrivate *private = (TextPrivate *) Fmt_Private_Get(fmt);
  Real scale = private->sub_scale;
  fprintf(private->out,
          "  currentpoint translate %g %g translate %g %g scale 0 0 moveto\n",
          private->sub_vec.x, private->sub_vec.y, scale, scale);
}

static void _Text_Fmt_Save(FmtStack *stack) {
  Fmt *fmt = Fmt_Get(stack);
  TextPrivate *private = (TextPrivate *) Fmt_Private_Get(fmt);
  fprintf(private->out, " textsave");
}

static void _Text_Fmt_Restore(FmtStack *stack) {
  Fmt *fmt = Fmt_Get(stack);
  TextPrivate *private = (TextPrivate *) Fmt_Private_Get(fmt);
  fprintf(private->out, " textrestore");
}

static void _Text_Fmt_Newline(FmtStack *stack) {
  Fmt *fmt = Fmt_Get(stack);
  TextPrivate *private = (TextPrivate *) Fmt_Private_Get(fmt);
  fprintf(private->out, " textnewline");
}

static void _Text_Fmt_Init(Fmt *fmt) {
  Fmt_Init(fmt);
  fmt->draw = _Text_Fmt_Draw;
  fmt->subscript = _Text_Fmt_Subscript;
  fmt->superscript = _Text_Fmt_Superscript;
  fmt->save = _Text_Fmt_Save;
  fmt->restore = _Text_Fmt_Restore;
  fmt->newline = _Text_Fmt_Newline;
}

static void eps_text(Point *ctr, Point *right, Point *up, Point *from,
                     const char *text) {
  TextPrivate private;
  Fmt fmt;
  EPS_POINT(ctr, ctrx, ctry);
  EPS_POINT(right, rx, ry);
  EPS_POINT(up, ux, uy);

  _Text_Fmt_Init(& fmt);
  Fmt_Private_Set(& fmt, & private);
  private.out = (FILE *) grp_win->ptr;
  private.sup_vec.x = 0.0;
  private.sup_vec.y = 0.5;
  private.sup_scale = 0.5;
  private.sub_vec.x = 0.0;
  private.sub_vec.y = 0.0;
  private.sub_scale = 0.5;

  fprintf((FILE *) grp_win->ptr,
          "  gsave %ld %ld %ld %ld %ld %ld newref newpath 0 0 moveto"
          " matrix currentmatrix\n",
          ctrx, ctry, rx, ry, ux, uy);

  Fmt_Text(& fmt, text);

  fprintf((FILE *) grp_win->ptr,
          "  setmatrix pathbbox newpath"
          " 2 index sub %g mul exch 3 index sub %g mul 4 1 roll add neg"
          " 3 1 roll add neg exch translate 0 0 moveto\n",
          from->y, from->x);

  Fmt_Text(& fmt, text);

  fprintf((FILE *) grp_win->ptr, "  fill grestore\n");
}

static void eps_font(const char *font) {
  const char *full_name;

  full_name = ps_font_get_full_name(font,
                                    FONT_SLANT_NORMAL,
                                    FONT_WEIGHT_NORMAL);

  if (full_name == (const char *) NULL) {
    fprintf(stderr, "Font not found. Available fonts are:\n");
    ps_print_available_fonts(stderr);
    full_name = ps_default_font_full_name();
    fprintf(stderr, "Using default font '%s'\n", full_name);
  }

  fprintf((FILE *) grp_win->ptr,
          "  /%s findfont setfont\n", full_name);
}

static void eps_fake_point(Point *p) {return;}

#if 0
static void eps_rbgcolor(Color *c) {return;}
#endif

/***************************************************************************************/
/* PROCEDURE DI GESTIONE DELLA FINESTRA GRAFICA */

/** Set the default methods to the eps window */
static void eps_repair(GrpWindow *w) {
  grp_window_block(w);

  w->rreset = eps_rreset;
  w->rinit = eps_rinit;
  w->rdraw = eps_rdraw;
  w->rline = eps_rline;
  w->rcong = eps_rcong;
  w->rclose = eps_rclose;
  w->rcircle = eps_rcircle;
  w->rfgcolor = eps_rfgcolor;
  w->text = eps_text;
  w->font = eps_font;
  w->fake_point = eps_fake_point;
  w->save = eps_save;

  w->close_win = eps_close_win;
}

static const char *ps_std_defs =
  "/congdict 14 dict def\n\ncongdict /mtrx matrix put\n"
  "/cong {\ncongdict begin\n  /yc exch def /xc exch def\n"
  "  /yb exch def /xb exch def\n  /ya exch def /xa exch def\n\n"
  "    /xu xb xc sub def /yu yb yc sub def\n"
  "    /xv xb xa sub def /yv yb ya sub def\n"
  "    /xo xa xu sub def /yo ya yu sub def\n\n"
  "    /savematrix mtrx currentmatrix def\n    [xu yu xv yv xo yo] concat\n"
  "    0 0 1 0 90 arc\n    savematrix setmatrix\n\n  end\n} def\n\n"
  "/circledict 12 dict def\n\ncircledict /mtrx matrix put\n"
  "/circle {\ncircledict begin\n  /yb exch def /xb exch def\n"
  "  /ya exch def /xa exch def\n  /yo exch def /xo exch def\n\n"
  "    /xu xa xo sub def /yu ya yo sub def\n"
  "    /xv xb xo sub def /yv yb yo sub def\n\n"
  "    /savematrix mtrx currentmatrix def\n    [xu yu xv yv xo yo] concat\n"
  "    1 0 moveto 0 0 1 0 360 arc\n"
  "    savematrix setmatrix\n\n  end\n} def\n\n"
  "/newrefdict 15 dict def\n\nnewrefdict /mtrx matrix put\n"
  "/newref {\nnewrefdict begin\n"
  "  /yb exch def /xb exch def\n"
  "  /ya exch def /xa exch def\n"
  "  /yo exch def /xo exch def\n\n"
  "    /xu xa xo sub def /yu ya yo sub def\n"
  "    /xv xb xo sub def /yv yb yo sub def\n\n"
  "    [xu yu xv yv xo yo] concat\n"
  "  end\n} def\n\n"
  "/textdict 6 dict def\n"
  "textdict begin\n"
  "  /supx 0 def /supy 0.5 def /sups 0.5 def\n"
  "  /subx 0 def /suby 0.0 def /subs 0.5 def\n"
  "end\n"
  "/textsave {currentpoint matrix currentmatrix} def\n"
  "/textrestore {setmatrix exch pop currentpoint pop exch moveto} def\n"
  "/textsub {currentpoint translate subx suby translate subs dup scale"
  " 0 0 moveto} def\n"
  "/textsup {currentpoint translate supx supy translate sups dup scale"
  " 0 0 moveto} def\n"
  "/textnewline {0 -1 translate 0 0 moveto} def\n\n";

/* Open a graphic window with type "eps" (encapsulated postscript).
 * The windows opens a file and send all the commands it receives directly
 * to it. The window will have size size_x and size_y (in mm) and will
 * show coordinates from (0, 0) to (size_x, size_y).
 */
GrpWindow *eps_open_win(const char *file, Real size_x, Real size_y) {
  GrpWindow *wd;
  FILE *winstream;
  Real size_x_psunit, size_y_psunit;
  int x_max, y_max;

  /* Should express the size of the window in postscript units 1/72 of inch */
  /* 1 inch = 25.4 mm */
  size_x_psunit = (size_x / grp_mm_per_inch)/grp_inch_per_psunit;
  size_y_psunit = (size_y / grp_mm_per_inch)/grp_inch_per_psunit;
  x_max = (int) size_x_psunit+1;
  y_max = (int) size_y_psunit+1;

  /* Creo la finestra */
  wd = (GrpWindow *) malloc( sizeof(GrpWindow) );
  if (wd == NULL) {
    ERRORMSG("eps_open_win", "Memoria esaurita");
    return NULL;
  }

  /* Apro il file su cui verranno scritte le istruzioni postscript */
  winstream = fopen(file, "w");
  if (winstream == NULL) {
    ERRORMSG("eps_open_win", "Cannot open the file for writing!");
    free(wd);
    return NULL;
  }

  wd->ptr = (void *) winstream;

  /* Ora do' le procedure per gestire la finestra */
  wd->quiet = 0;
  wd->repair = eps_repair;
  wd->repair(wd);
  wd->win_type_str = "eps";

  /* Scrivo l'intestazione del file */
  fprintf(winstream, "%%!PS-Adobe-2.0 EPSF-2.0\n"
          "%%%%Title: %s\n%%%%Creator: Box g library\n", file);
  fprintf(winstream, "%%%%BoundingBox: 0 0 %d %d\n", x_max, y_max);
  fprintf(winstream,
   "%%%%Magnification: 1.0000\n%%%%EndComments\n\n"
   "%s"
   "save\n", ps_std_defs);

  fprintf(winstream, "newpath 0 %d moveto 0 0 lineto %d 0 "
          "lineto %d %d lineto closepath clip newpath\n"
          "0.01 0.01 scale\n0 0 0 setrgbcolor\n",
          y_max, x_max, x_max, y_max);

  return wd;
}

static int eps_save(const char *unused) {
  return 1;
}

int eps_save_fig(const char *file_name, GrpWindow *figure) {
  Point bb_min, bb_max, translation, center, size;
  Real sx, sy, rot_angle;
  GrpWindow *cur_win = grp_win;
  Matrix m;

  bb_bounding_box(figure, & bb_min, & bb_max);
  /*printf("Bounding box (%f, %f) - (%f, %f)\n",
         bb_min.x, bb_min.y, bb_max.x, bb_max.y);*/

  size.x = fabs(bb_max.x - bb_min.x);
  size.y = fabs(bb_max.y - bb_min.y);
  grp_win = eps_open_win(file_name, size.x, size.y);
  translation.x = -bb_min.x;
  translation.y = -bb_min.y;
  center.y = center.x = 0.0;
  sy = sx = 1.0;
  rot_angle = 0.0;
  Grp_Matrix_Set(& m, & translation, & center, rot_angle, sx, sy);
  Fig_Draw_Fig_With_Matrix(figure, & m);
  grp_close_win();
  grp_win = cur_win;
  return 1;
}





/****************************************************************************/
/* Here we define another window type: PS (postscript), very similar to EPS */

static void ps_close_win(void) {
  FILE *f = (FILE *) grp_win->ptr;
  assert(f != (FILE *) NULL);
  /*fprintf(f, "\nrestore\nshowpage\n%%%%Trailer\n%%EOF\n");*/
  fclose(f);
}

/***************************************************************************************/
/* Here we define another window type: PS (postscript), very similar to EPS. */

/** Set the default methods to the ps window */
static void ps_repair(GrpWindow *w) {
  grp_window_block(w);
  w->rreset = eps_rreset;
  w->rinit = eps_rinit;
  w->rdraw = eps_rdraw;
  w->rline = eps_rline;
  w->rcong = eps_rcong;
  w->rclose = eps_rclose;
  w->rcircle = eps_rcircle;
  w->rfgcolor = eps_rfgcolor;
  w->fake_point = eps_fake_point;
  w->save = eps_save;

  w->close_win = ps_close_win;
}

/* NOME: ps_open_win
 * DESCRIZIONE: Apre una finestra grafica di tipo postscript.
 *  Tale finestra tradurra' i comandi grafici ricevuti in istruzioni postscript,
 *  che saranno scritte immediatamente su file.
 */
GrpWindow *ps_open_win(const char *file) {
  GrpWindow *wd;
  FILE *winstream;

  /* Creo la finestra */
  wd = (GrpWindow *) malloc( sizeof(GrpWindow) );
  if ( wd == NULL ) {
    ERRORMSG("ps_open_win", "Memoria esaurita");
    return NULL;
  }

  /* Apro il file su cui verranno scritte le istruzioni postscript */
  winstream = fopen(file, "w");
  if ( winstream == NULL ) {
    ERRORMSG("ps_open_win", "Impossibile aprire il file");
    free(wd);
    return NULL;
  }

  wd->ptr = (void *) winstream;

  /* Ora do' le procedure per gestire la finestra */
  wd->quiet = 0;
  wd->repair = ps_repair;
  wd->repair(wd);
  wd->win_type_str = "ps";

  /* Scrivo l'intestazione del file */
  fprintf(winstream,
    "%%!PS-Adobe-2.0\n%%%%Title: %s\n%%%%Creator: Box g library\n"
    "%%%%Orientation: Landscape\n%%%%Pages: 1\n%%%%BoundingBox: 0 0 612 792\n"
    "%%%%BeginSetup\n%%%%IncludeFeature: *PageSize Letter\n%%%%EndSetup\n"
    "%%%%Magnification: 1.0000\n%%%%EndComments\n\n"
    "%s"
    "0.01 0.01 scale\n90 rotate\n1 -1 scale\n0 0 0 setrgbcolor\n",
    ps_std_defs, file);

  return wd;
}

int ps_save_fig(const char *file_name, GrpWindow *figure) {
  Point bb_min, bb_max, translation, center;
  Real sx, sy, rot_angle;
  GrpWindow *cur_win = grp_win;
  Matrix m;

  bb_bounding_box(figure, & bb_min, & bb_max);
  printf("Bounding box (%f, %f) - (%f, %f)\n",
         bb_min.x, bb_min.y, bb_max.x, bb_max.y);

  grp_win = ps_open_win(file_name);
  translation.x = -bb_min.x;
  translation.y = -bb_min.y;
  center.y = center.x = 0.0;
  sy = sx = 1.0;
  rot_angle = 0.0;
  Grp_Matrix_Set(& m, & translation, & center, rot_angle, sx, sy);
  Fig_Draw_Fig_With_Matrix(figure, & m);
  grp_close_win();
  grp_win = cur_win;
  return 1;
}
