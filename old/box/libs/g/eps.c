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

static int My_EPS_Save_To_File(BoxGWin *w, const char *unused);

/* Variabili usate dalle procedure per scrivere il file postscript */
static int beginning_of_path = 1;
static long previous_px, previous_py;

static BoxReal eps_point_scale = 283.46457;
/* Le coordinate dei punti passati alle funzioni grafiche in questo file,
 * sono espresse in millimetri: le converto nelle unita' postscript:
 *  1 unita' = 1/72 inch = 0.35277777... millimetri
 */
#define EPS_POINT(p, px, py) \
  long px = (p->x * eps_point_scale), \
       py = (p->y * eps_point_scale);

#define EPS_REAL(r) ((r)*eps_point_scale)

static void My_EPS_Finish(BoxGWin *w) {
  FILE *f = (FILE *) w->ptr;
  assert(f != (FILE *) NULL);
  fprintf(f, "\nrestore\nshowpage\n%%%%Trailer\n%%EOF\n");
  fclose(f);
}

static void My_EPS_Create_Path(BoxGWin *w) {
  beginning_of_path = 1;
}

static void My_EPS_Begin_Drawing(BoxGWin *w) {}

static void My_EPS_Draw_Path(BoxGWin *w, DrawStyle *style) {
  if (! beginning_of_path) {
    BoxReal scale = style->scale;
    int do_border = (style->bord_width > 0.0);
    FILE *out = (FILE *) w->ptr;
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
      BoxReal bw = EPS_REAL(scale*style->bord_width);
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
        BoxInt num_dashes = style->bord_num_dashes, i;
        BoxReal dash_offset = EPS_REAL(scale*style->bord_dash_offset);
        char *sep = " [";
        for(i=0; i<num_dashes; i++) {
          fprintf(out, "%s"SReal, sep, EPS_REAL(scale*style->bord_dashes[i]));
          sep = ", ";
        }
        fprintf(out, "] "SReal" setdash", dash_offset);
      }

      if (lj == 0) {
        BoxReal ml = EPS_REAL(scale*style->bord_miter_limit);
        fprintf(out, " %g setmiterlimit stroke grestore\n", ml);
      } else
        fprintf(out, " stroke grestore\n");

    } else {
      fprintf(out, "\n");
    }
  }
}

static void My_EPS_Add_Line_Path(BoxGWin *w, BoxPoint *a, BoxPoint *b) {
  EPS_POINT(a, ax, ay); EPS_POINT(b, bx, by);
  int continuing = (ax == previous_px) && (ay == previous_py),
      length_zero = (ax == bx && ay == by);

  if (continuing && length_zero) return;

  if (beginning_of_path) {
    fprintf((FILE *) w->ptr, " newpath");
    beginning_of_path = 0;
    continuing = 0;
  }

  if (!continuing)
    fprintf((FILE *) w->ptr, " %ld %ld moveto", ax, ay);

  fprintf((FILE *) w->ptr, " %ld %ld lineto", bx, by);
  previous_px = bx; previous_py = by;
}

static void My_EPS_Add_Join_Path(BoxGWin *w, BoxPoint *a, BoxPoint *b, BoxPoint *c) {
  EPS_POINT(a, ax, ay); EPS_POINT(b, bx, by); EPS_POINT(c, cx, cy);

  if (ax == cx && ay == cy) return;

  if (beginning_of_path) {
    fprintf((FILE *) w->ptr, " newpath");
    beginning_of_path = 0;
  }

  fprintf((FILE *) w->ptr,
          " %ld %ld %ld %ld %ld %ld cong", ax, ay, bx, by, cx, cy);
  previous_px = cx; previous_py = cy;
}

static void My_EPS_Close_Path(BoxGWin *w) {
  if (!beginning_of_path)
    fprintf((FILE *) w->ptr, " closepath");
}

static void My_EPS_Add_Circle_Path(BoxGWin *w,
                                   BoxPoint *ctr, BoxPoint *a, BoxPoint *b) {
  EPS_POINT(ctr, cx, cy); EPS_POINT(a, ax, ay); EPS_POINT(b, bx, by);

  if (beginning_of_path)
    fprintf((FILE *) w->ptr, " newpath");

  fprintf((FILE *) w->ptr,
          " %ld %ld %ld %ld %ld %ld circle", cx, cy, ax, ay, bx, by);
  beginning_of_path = 0;
}

static void My_EPS_Set_Fg_Color(BoxGWin *w, Color *c) {
  fprintf((FILE *) w->ptr,
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
    case '(': case ')': case '\\': *(d++) = '\\'; *(d++) = *s; break;
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
  BoxPoint sub_vec, sup_vec;
  BoxReal sub_scale, sup_scale;
} TextPrivate;

static void _Text_Fmt_Draw(BoxGFmtStack *stack) {
  BoxGFmt *fmt = BoxGFmt_Get(stack);
  TextPrivate *private = (TextPrivate *) BoxGFmt_Get_Private(fmt);
  char *escaped_text = Escape_Text(BoxGFmt_Get_Buffer(stack));
  fprintf(private->out, " (%s) textdraw", escaped_text);
  free(escaped_text);
  BoxGFmt_Clear_Buffer(stack);
}

static void _Text_Fmt_Superscript(BoxGFmtStack *stack) {
  BoxGFmt *fmt = BoxGFmt_Get(stack);
  TextPrivate *private = (TextPrivate *) BoxGFmt_Get_Private(fmt);
  fprintf(private->out, " textsup");
}

static void _Text_Fmt_Subscript(BoxGFmtStack *stack) {
  BoxGFmt *fmt = BoxGFmt_Get(stack);
  TextPrivate *private = (TextPrivate *) BoxGFmt_Get_Private(fmt);
  fprintf(private->out, " textsub");
}

static void _Text_Fmt_Save(BoxGFmtStack *stack) {
  BoxGFmt *fmt = BoxGFmt_Get(stack);
  TextPrivate *private = (TextPrivate *) BoxGFmt_Get_Private(fmt);
  fprintf(private->out, " textsave");
}

static void _Text_Fmt_Restore(BoxGFmtStack *stack) {
  BoxGFmt *fmt = BoxGFmt_Get(stack);
  TextPrivate *private = (TextPrivate *) BoxGFmt_Get_Private(fmt);
  fprintf(private->out, " textrestore");
}

static void _Text_Fmt_Newline(BoxGFmtStack *stack) {
  BoxGFmt *fmt = BoxGFmt_Get(stack);
  TextPrivate *private = (TextPrivate *) BoxGFmt_Get_Private(fmt);
  fprintf(private->out, " textnewline");
}

static void _Text_Fmt_Init(BoxGFmt *fmt) {
  BoxGFmt_Init(fmt);
  fmt->draw = _Text_Fmt_Draw;
  fmt->subscript = _Text_Fmt_Subscript;
  fmt->superscript = _Text_Fmt_Superscript;
  fmt->save = _Text_Fmt_Save;
  fmt->restore = _Text_Fmt_Restore;
  fmt->newline = _Text_Fmt_Newline;
}

static void My_EPS_Add_Text_Path(BoxGWin *w, BoxPoint *ctr, BoxPoint *right,
                                 BoxPoint *up, BoxPoint *from,
                                 const char *text) {
  FILE *out = (FILE *) w->ptr;
  TextPrivate private;
  BoxGFmt fmt;
  EPS_POINT(ctr, ctrx, ctry);
  EPS_POINT(right, rx, ry);
  EPS_POINT(up, ux, uy);

  _Text_Fmt_Init(& fmt);
  BoxGFmt_Set_Private(& fmt, & private);
  private.out = out;
  private.sup_vec.x = 0.0;
  private.sup_vec.y = 0.5;
  private.sup_scale = 0.5;
  private.sub_vec.x = 0.0;
  private.sub_vec.y = -0.1;
  private.sub_scale = 0.5;

  fprintf(out,
          "textdict begin\n"
          "  %g %g %g %g %g %g %g %g textparams\n",
          from->x, from->y,
          private.sup_vec.x, private.sup_vec.y, private.sup_scale,
          private.sub_vec.x, private.sub_vec.y, private.sub_scale);

  fprintf(out,
          "  %ld %ld %ld %ld %ld %ld textbegin\n",
          ctrx, ctry, rx, ry, ux, uy);

  BoxGFmt_Draw_Text(& fmt, text);
  fprintf(out, " textcalc\n");
  BoxGFmt_Draw_Text(& fmt, text);
  fprintf(out, "\n  textend\nend\n");
  beginning_of_path = 0;
}

static void My_EPS_Set_Font(BoxGWin *w, const char *font) {
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

  fprintf((FILE *) w->ptr,
          "  /%s findfont setfont\n", full_name);
}

static void My_EPS_Add_Fake_Point(BoxGWin *w, BoxPoint *p) {}

/***************************************************************************************/
/* PROCEDURE DI GESTIONE DELLA FINESTRA GRAFICA */

/** Set the default methods to the eps window */
static void eps_repair(BoxGWin *w) {
  BoxGWin_Block(w);

  w->create_path = My_EPS_Create_Path;
  w->begin_drawing = My_EPS_Begin_Drawing;
  w->draw_path = My_EPS_Draw_Path;
  w->add_line_path = My_EPS_Add_Line_Path;
  w->add_join_path = My_EPS_Add_Join_Path;
  w->close_path = My_EPS_Close_Path;
  w->add_circle_path = My_EPS_Add_Circle_Path;
  w->set_fg_color = My_EPS_Set_Fg_Color;
  w->add_text_path = My_EPS_Add_Text_Path;
  w->set_font = My_EPS_Set_Font;
  w->add_fake_point = My_EPS_Add_Fake_Point;
  w->save_to_file = My_EPS_Save_To_File;

  w->finish = My_EPS_Finish;
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
  "/textdict 18 dict def\n"
  "textdict begin\n"
  "  /supx 0 def /supy 0.5 def /sups 0.5 def\n"
  "  /subx 0 def /suby 0.0 def /subs 0.5 def\n"
  "end\n"
  "/textparams {\n"
  "  /subs exch def /suby exch def /subx exch def\n"
  "  /sups exch def /supy exch def /supx exch def\n"
  "  /fromy exch def /fromx exch def\n"
  "} def\n"
  "/textbegin {\n"
  "  /yb exch def /xb exch def\n"
  "  /ya exch def /xa exch def\n"
  "  /yo exch def /xo exch def\n\n"
  "    matrix currentmatrix\n"
  "    /xu xa xo sub def /yu ya yo sub def\n"
  "    /xv xb xo sub def /yv yb yo sub def\n\n"
  "    [xu yu xv yv xo yo] concat\n"
  "    newpath 0 0 moveto matrix currentmatrix\n"
  "} def\n"
  "/textend {setmatrix} def\n"
  "/textdraw {true charpath} def\n"
  "/textcalc {setmatrix pathbbox newpath 2 index sub fromy mul exch\n"
  "           3 index sub fromx mul 4 1 roll add neg 3 1 roll add neg exch\n"
  "           translate 0 0 moveto} def\n"
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
BoxGWin *BoxGWin_Create_EPS(const char *file, BoxReal size_x, BoxReal size_y) {
  BoxGWin *wd;
  FILE *winstream;
  BoxReal size_x_psunit, size_y_psunit;
  int x_max, y_max;

  /* Should express the size of the window in postscript units 1/72 of inch */
  /* 1 inch = 25.4 mm */
  size_x_psunit = (size_x / grp_mm_per_inch)/grp_inch_per_psunit;
  size_y_psunit = (size_y / grp_mm_per_inch)/grp_inch_per_psunit;
  x_max = (int) size_x_psunit+1;
  y_max = (int) size_y_psunit+1;

  /* Creo la finestra */
  wd = (BoxGWin *) malloc(sizeof(BoxGWin));
  if (wd == NULL) {
    ERRORMSG("BoxGWin_Create_EPS", "Memoria esaurita");
    return NULL;
  }

  /* Apro il file su cui verranno scritte le istruzioni postscript */
  winstream = fopen(file, "w");
  if (winstream == NULL) {
    ERRORMSG("BoxGWin_Create_EPS", "Cannot open the file for writing!");
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

static int My_EPS_Save_To_File(BoxGWin *w, const char *unused) {
  return 1;
}

int eps_save_fig(const char *file_name, BoxGWin *figure) {
  BoxGBBox bbox;
  BoxPoint translation, center, size;
  BoxReal sx, sy, rot_angle;
  BoxGWin *dest;
  Matrix m;

  BoxGBBox_Compute(& bbox, figure);

  size.x = fabs(bbox.max.x - bbox.min.x);
  size.y = fabs(bbox.max.y - bbox.min.y);
  dest = BoxGWin_Create_EPS(file_name, size.x, size.y);
  translation.x = -bbox.min.x;
  translation.y = -bbox.min.y;
  center.y = center.x = 0.0;
  sy = sx = 1.0;
  rot_angle = 0.0;
  BoxGMatrix_Set(& m, & translation, & center, rot_angle, sx, sy);
  BoxGWin_Fig_Draw_Fig_With_Matrix(dest, figure, & m);
  BoxGWin_Destroy(dest);
  return 1;
}





/****************************************************************************/
/* Here we define another window type: PS (postscript), very similar to EPS */

static void My_PS_Close_Win(BoxGWin *w) {
  FILE *f = (FILE *) w->ptr;
  assert(f != (FILE *) NULL);
  /*fprintf(f, "\nrestore\nshowpage\n%%%%Trailer\n%%EOF\n");*/
  fclose(f);
}

/***************************************************************************************/
/* Here we define another window type: PS (postscript), very similar to EPS. */

/** Set the default methods to the ps window */
static void ps_repair(BoxGWin *w) {
  BoxGWin_Block(w);
  w->create_path = My_EPS_Create_Path;
  w->begin_drawing = My_EPS_Begin_Drawing;
  w->draw_path = My_EPS_Draw_Path;
  w->add_line_path = My_EPS_Add_Line_Path;
  w->add_join_path = My_EPS_Add_Join_Path;
  w->close_path = My_EPS_Close_Path;
  w->add_circle_path = My_EPS_Add_Circle_Path;
  w->set_fg_color = My_EPS_Set_Fg_Color;
  w->add_fake_point = My_EPS_Add_Fake_Point;
  w->save_to_file = My_EPS_Save_To_File;

  w->finish = My_PS_Close_Win;
}

/* DESCRIZIONE: Apre una finestra grafica di tipo postscript.
 *  Tale finestra tradurra' i comandi grafici ricevuti in istruzioni postscript,
 *  che saranno scritte immediatamente su file.
 */
BoxGWin *BoxGWin_Create_PS(const char *file) {
  BoxGWin *wd;
  FILE *winstream;

  /* Creo la finestra */
  wd = (BoxGWin *) malloc( sizeof(BoxGWin) );
  if ( wd == NULL ) {
    ERRORMSG("BoxGWin_Create_PS", "Memoria esaurita");
    return NULL;
  }

  /* Apro il file su cui verranno scritte le istruzioni postscript */
  winstream = fopen(file, "w");
  if ( winstream == NULL ) {
    ERRORMSG("BoxGWin_Create_PS", "Impossibile aprire il file");
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

int ps_save_fig(const char *file_name, BoxGWin *figure) {
  BoxGBBox bbox;
  BoxPoint translation, center;
  BoxReal sx, sy, rot_angle;
  BoxGWin *dest;
  Matrix m;

  BoxGBBox_Compute(& bbox, figure);

  dest = BoxGWin_Create_PS(file_name);
  translation.x = -bbox.min.x;
  translation.y = -bbox.min.y;
  center.y = center.x = 0.0;
  sy = sx = 1.0;
  rot_angle = 0.0;
  BoxGMatrix_Set(& m, & translation, & center, rot_angle, sx, sy);
  BoxGWin_Fig_Draw_Fig_With_Matrix(dest, figure, & m);
  BoxGWin_Destroy(dest);
  return 1;
}