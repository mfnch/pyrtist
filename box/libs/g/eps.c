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

#include "error.h"
#include "types.h"
#include "graphic.h"
#include "fig.h"
#include "autoput.h"
#include "bb.h"

/* This terminal discretizes the points into a grid where the number of
 * points per inch in the x and y directions are discretization_x and
 * discretization_y
 */
static int discretization_x = 1000, discretization_y = 1000;

static void not_available(void);
static void eps_close_win(void);
static void eps_rreset(void);
static void eps_rinit(void);
static void eps_rdraw(void);
static void eps_rline(Point a, Point b);
static void eps_rcong(Point a, Point b, Point c);
static void eps_rcircle(Point ctr, Point a, Point b);
static void eps_rfgcolor(Real r, Real g, Real b);
static void eps_text(Point *p, const char *text);
static void eps_font(const char *font, Real size);
static void eps_fake_point(Point *p);

#if 0
static void eps_rbgcolor(Real r, Real g, Real b);
#endif
static int eps_save(void);

static void not_available(void) {
  ERRORMSG("not_available", "Not available for postscript windows.");
}

/* Queste funzioni non sono disponibili per finestre postscript
 */
static void (*eps_lowfn[])() = {
  eps_close_win, not_available, not_available, not_available
};

/* Lista delle funzioni di rasterizzazione */
static void (*eps_midfn[])() = {
  eps_rreset, eps_rinit, eps_rdraw,
  eps_rline, eps_rcong, not_available,
  eps_rcircle, eps_rfgcolor, not_available,
  not_available, eps_text, eps_font,
  eps_fake_point
};

/* Variabili usate dalle procedure per scrivere il file postscript */
static int beginning_of_line = 1, beginning_of_path = 1;
static long previous_px, previous_py;

static Real eps_point_scale = 283.46457;
/* Le coordinate dei punti passati alle funzioni grafiche in questo file,
 * sono espresse in millimetri: le converto nelle unita' postscript:
 *  1 unita' = 1/72 inch = 0.35277777... millimetri
 */
#define EPS_POINT(p, px, py) \
  long px = (p.x * eps_point_scale), \
       py = (p.y * eps_point_scale);

#define EPS_REAL(r) ((r)*eps_point_scale)

static void eps_close_win(void) {
  FILE *f = (FILE *) grp_win->ptr;
  fprintf(f, "restore\nshowpage\n%%%%Trailer\n%%EOF\n");
  fclose(f);
}

static void eps_rreset(void) {
  beginning_of_line = 1;
  beginning_of_path = 1;
}

static void eps_rinit(void) {return;}

static void eps_rdraw(void) {
  if ( ! beginning_of_path )
    fprintf( (FILE *) grp_win->ptr, " eofill\n");
}

static void eps_rline(Point a, Point b) {
  EPS_POINT(a, ax, ay); EPS_POINT(b, bx, by);


/*   if (ax == bx && ay == by) return; need to think better about it!!! */

  if ( beginning_of_line ) {
    beginning_of_line = 0;
    if ( beginning_of_path ) {
      fprintf( (FILE *) grp_win->ptr,
       " newpath %ld %ld moveto %ld %ld lineto", ax, ay, bx, by );
      previous_px = bx; previous_py = by;
      beginning_of_path = 0;

    } else {
      fprintf( (FILE *) grp_win->ptr,
       " %ld %ld moveto %ld %ld lineto", ax, ay, bx, by );
      previous_px = bx; previous_py = by;
      beginning_of_path = 0;
    }

  } else {
    if ( (ax == previous_px) && (ay == previous_py) ) {
      fprintf( (FILE *) grp_win->ptr,
       " %ld %ld lineto", bx, by );
      previous_px = bx; previous_py = by;

    } else {
      fprintf( (FILE *) grp_win->ptr,
       " %ld %ld moveto %ld %ld lineto", ax, ay, bx, by );
      previous_px = bx; previous_py = by;
    }
  }
}

static void eps_rcong(Point a, Point b, Point c) {
  EPS_POINT(a, ax, ay); EPS_POINT(b, bx, by); EPS_POINT(c, cx, cy);
#if 0
  int a_eq_b = ax == bx && ay == by,
      a_eq_c = ax == cx && ay == cy,
      b_eq_c = bx == cx && by == cy,
      n_eq = a_eq_b + a_eq_c + b_eq_c;
  if (n_eq == 3) return;
#endif
  if (ax == cx && ay == cy) return;

  if ( beginning_of_path )
    fprintf( (FILE *) grp_win->ptr, " newpath" );

  fprintf( (FILE *) grp_win->ptr,
   " %ld %ld %ld %ld %ld %ld cong", ax, ay, bx, by, cx, cy );
  previous_px = cx; previous_py = cy;
  beginning_of_line = 0;
}

static void eps_rcircle(Point ctr, Point a, Point b) {
  EPS_POINT(ctr, cx, cy); EPS_POINT(a, ax, ay); EPS_POINT(b, bx, by);

  if ( beginning_of_path )
    fprintf( (FILE *) grp_win->ptr, " newpath" );

  fprintf( (FILE *) grp_win->ptr,
   " %ld %ld %ld %ld %ld %ld circle", cx, cy, ax, ay, bx, by );
  beginning_of_line = 1;
  beginning_of_path = 0;
}

static void eps_rfgcolor(Real r, Real g, Real b) {
  fprintf( (FILE *) grp_win->ptr,
   "  %g %g %g setrgbcolor\n", r, g, b );
}

static void eps_text(Point *p, const char *text) {
  EPS_POINT((*p), px, py);

  fprintf((FILE *) grp_win->ptr,
          "  %ld %ld moveto (%s) show\n",
          px, py, text);
}

static void eps_font(const char *font, Real size) {
  long s = EPS_REAL(size);

  fprintf((FILE *) grp_win->ptr,
          "  /%s findfont %ld scalefont setfont\n", font, s);
}

static void eps_fake_point(Point *p) {return;}

#if 0
static void eps_rbgcolor(Real r, Real g, Real b) {return;}
#endif

/***************************************************************************************/
/* PROCEDURE DI GESTIONE DELLA FINESTRA GRAFICA */

/* Open a graphic window with type "eps" (encapsulated postscript).
 * The windows opens a file and send all the commands it receives directly
 * to it. The window will have size size_x and size_y (in mm) and will
 * show coordinates from (0, 0) to (size_x, size_y).
 */
grp_window *eps_open_win(char *file, Real size_x, Real size_y) {
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

  /* Creo la finestra */
  wd = (grp_window *) malloc( sizeof(grp_window) );
  if ( wd == NULL ) {
    ERRORMSG("eps_open_win", "Memoria esaurita");
    return NULL;
  }

  winstream = (FILE *) malloc( sizeof(FILE) );
  if ( winstream == NULL) {
    ERRORMSG("eps_open_win", "Memoria esaurita");
    free(wd);
    return NULL;
  }

  /* Apro il file su cui verranno scritte le istruzioni postscript */
  winstream = fopen(file, "w");
  if ( winstream == NULL ) {
    ERRORMSG("eps_open_win", "Impossibile aprire il file");
    free(wd); free(winstream);
    return NULL;
  }

  wd->ptr = (void *) winstream;

  /* Costruisco una tavolazza di colori da associare a questa finestra *
  wd->pal = grp_palette_build(2, 2, 3, 4);
  if ( wd->pal == NULL ) return NULL;

  * La prima richiesta di colore corrisponde all'indice di colore 0,
   * cioe' all'indice dello sfondo della figura.
   * Quindi setto il colore dello sfondo a RGB = {255, 255, 255} = bianco
   *
  wd->bgcol = grp_color_request( wd->pal, & ((color) {255, 255, 255}) );
  if ( wd->bgcol == NULL ) return NULL;

  * Setto il colore di primo piano a RGB = {0, 0, 0} = nero *
  wd->fgcol = grp_color_request( wd->pal, & ((color) {0, 0, 0}) );
  if ( wd->fgcol == NULL ) return NULL;*/

  /* Ora do' le procedure per gestire la finestra */
  wd->save = eps_save;
  wd->lowfn = eps_lowfn;
  wd->midfn = eps_midfn;

  /* Scrivo l'intestazione del file */
  fprintf(winstream, "%%!PS-Adobe-2.0 EPSF-2.0\n"
          "%%%%Title: %s\n%%%%Creator: Box g library\n", file);
  fprintf(winstream, "%%%%BoundingBox: 0 0 %d %d\n", x_max, y_max);
  fprintf(winstream,
   "%%%%Magnification: 1.0000\n%%%%EndComments\n\n"
   "/congdict 8 dict def\n\ncongdict /mtrx matrix put\n"
   "/cong {\ncongdict begin\n  /yc exch def /xc exch def\n"
   "  /yb exch def /xb exch def\n  /ya exch def /xa exch def\n\n"
   "    /xu xb xc sub def /yu yb yc sub def\n"
   "    /xv xb xa sub def /yv yb ya sub def\n"
   "    /xo xa xu sub def /yo ya yu sub def\n\n"
   "    /savematrix mtrx currentmatrix def\n    [xu yu xv yv xo yo] concat\n"
   "    0 0 1 0 90 arc\n    savematrix setmatrix\n\n  end\n} def\n\n"
   "/circledict 8 dict def\n\ncircledict /mtrx matrix put\n"
   "/circle {\ncircledict begin\n  /yb exch def /xb exch def\n"
   "  /ya exch def /xa exch def\n  /yo exch def /xo exch def\n\n"
   "    /xu xa xo sub def /yu ya yo sub def\n"
   "    /xv xb xo sub def /yv yb yo sub def\n\n"
   "    /savematrix mtrx currentmatrix def\n    [xu yu xv yv xo yo] concat\n"
   "    0 0 1 0 360 arc\n    savematrix setmatrix\n\n  end\n} def\n\n"
   "save\n");

  fprintf(winstream, "newpath 0 %d moveto 0 0 lineto %d 0 "
          "lineto %d %d lineto closepath clip newpath\n"
          "0.01 0.01 scale\n0 0 0 setrgbcolor\n",
          y_max, x_max, x_max, y_max);

  return wd;
}

static int eps_save(void) {
  fclose((FILE *) grp_win->ptr);
  return 1;
}

int eps_save_fig(const char *file_name, grp_window *figure) {
  Point bb_min, bb_max, translation, center, size;
  Real sx, sy, rot_angle;
  grp_window *cur_win = grp_win;

  bb_bounding_box(figure, & bb_min, & bb_max);
  printf("Bounding box (%f, %f) - (%f, %f)\n",
         bb_min.x, bb_min.y, bb_max.x, bb_max.y);

  size.x = fabs(bb_max.x - bb_min.x);
  size.y = fabs(bb_max.y - bb_min.y);
  grp_win = eps_open_win(file_name, size.x, size.y);
  translation.x = -bb_min.x;
  translation.y = -bb_min.y;
  center.y = center.x = 0.0;
  sy = sx = 1.0;
  rot_angle = 0.0;
  aput_matrix(& translation, & center, rot_angle, sx, sy, fig_matrix);
  fig_draw_fig(figure);
  grp_close_win();
  grp_win = cur_win;
  return 1;
}
