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

/* ps.c - 15 aprile 2004
 *
 * Questo file contiene le funzioni che implementano una finestra di tipo
 * postscript (ps). In questa finestra i comandi grafici vengono immediatamente
 * diretti sul file postscript di output.
 */

#include <stdlib.h>
#include <stdio.h>
/*#include <math.h>*/

#include "error.h"    /* Serve per la segnalazione degli errori */
#include "types.h"
#include "graphic.h"  /* Dichiaro alcune strutture grafiche generali */
#include "fig.h"
#include "autoput.h"
#include "bb.h"
#include "g.h"

static void not_available(void);
static void ps_close_win(void);
static void ps_rreset(void);
static void ps_rinit(void);
static void ps_rdraw(DrawStyle style);
static void ps_rline(Point *a, Point *b);
static void ps_rcong(Point *a, Point *b, Point *c);
static void ps_rcircle(Point *ctr, Point *a, Point *b);
static void ps_rfgcolor(Real r, Real g, Real b);
static void ps_fake_point(Point *p);
#if 0
static void ps_rbgcolor(Real r, Real g, Real b);
#endif
static int ps_save(const char *unused);

static void not_available(void) {
  ERRORMSG("not_available", "Not available for postscript windows.");
}

/* Queste funzioni non sono disponibili per finestre postscript
 */
static void (*ps_lowfn[])() = {
  ps_close_win, not_available, not_available, not_available
};

/* Variabili usate dalle procedure per scrivere il file postscript */
static int beginning_of_line = 1, beginning_of_path = 1;
static long previous_px, previous_py;

/* Le coordinate dei punti passati alle funzioni grafiche in questo file,
 * sono espresse in millimetri: le converto nelle unita' postscript:
 *  1 unita' = 1/72 inch = 0.35277777... millimetri
 */
#define PS_POINT(p, px, py) \
  long px = (p->x * 283.46457), \
       py = (p->y * 283.46457);

static void ps_close_win(void) {fclose((FILE *) grp_win->ptr);}

static void ps_rreset(void) {
  beginning_of_line = 1;
  beginning_of_path = 1;
}

static void ps_rinit(void) {return;}

static void ps_rdraw(DrawStyle style) {
  if ( ! beginning_of_path ) {
    switch(style) {
    case DRAW_FILL:
      fprintf( (FILE *) grp_win->ptr, " fill\n"); break;
    case DRAW_EOFILL:
      fprintf( (FILE *) grp_win->ptr, " eofill\n"); break;
    case DRAW_CLIP:
      fprintf( (FILE *) grp_win->ptr, " clip\n"); break;
    case DRAW_EOCLIP:
      fprintf( (FILE *) grp_win->ptr, " eoclip\n"); break;
    default:
      g_warning("Unsupported drawing style: using even-odd fill algorithm!");
      fprintf( (FILE *) grp_win->ptr, " eofill\n"); break;
    }
  }
}

static void ps_rline(Point *a, Point *b) {
  PS_POINT(a, ax, ay); PS_POINT(b, bx, by);

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

static void ps_rcong(Point *a, Point *b, Point *c) {
  PS_POINT(a, ax, ay); PS_POINT(b, bx, by); PS_POINT(c, cx, cy);
  int a_eq_b = ax == bx && ay == by,
      a_eq_c = ax == cx && ay == cy,
      b_eq_c = bx == cx && by == cy,
      n_eq = a_eq_b + a_eq_c + b_eq_c;
  if (n_eq == 3) return;

  if ( beginning_of_path )
    fprintf( (FILE *) grp_win->ptr, " newpath" );

  fprintf( (FILE *) grp_win->ptr,
   " %ld %ld %ld %ld %ld %ld cong", ax, ay, bx, by, cx, cy );
  previous_px = cx; previous_py = cy;
  beginning_of_line = 0;
  return;
}

static void ps_rcircle(Point *ctr, Point *a, Point *b) {
  PS_POINT(ctr, cx, cy); PS_POINT(a, ax, ay); PS_POINT(b, bx, by);

  if ( beginning_of_path )
    fprintf( (FILE *) grp_win->ptr, " newpath" );

  fprintf( (FILE *) grp_win->ptr,
   " %ld %ld %ld %ld %ld %ld circle", cx, cy, ax, ay, bx, by );
  beginning_of_line = 1;
  beginning_of_path = 0;
  return;
}

static void ps_rfgcolor(Real r, Real g, Real b) {
  fprintf( (FILE *) grp_win->ptr,
   "  %g %g %g setrgbcolor\n", r, g, b );
  return;
}

#if 0
static void ps_rbgcolor(Real r, Real g, Real b) {return;}
#endif

/***************************************************************************************/
/* PROCEDURE DI GESTIONE DELLA FINESTRA GRAFICA */

/** Set the default methods to the ps window */
static void ps_repair(GrpWindow *w) {
  grp_window_block(w);
  w->lowfn = ps_lowfn;
  w->rreset = ps_rreset;
  w->rinit = ps_rinit;
  w->rdraw = ps_rdraw;
  w->rline = ps_rline;
  w->rcong = ps_rcong;
  w->rcircle = ps_rcircle;
  w->rfgcolor = ps_rfgcolor;
  w->fake_point = ps_fake_point;
  w->save = ps_save;
}

/* NOME: ps_open_win
 * DESCRIZIONE: Apre una finestra grafica di tipo postscript.
 *  Tale finestra tradurra' i comandi grafici ricevuti in istruzioni postscript,
 *  che saranno scritte immediatamente su file.
 */
grp_window *ps_open_win(const char *file) {
  grp_window *wd;
  FILE *winstream;

  /* Creo la finestra */
  wd = (grp_window *) malloc( sizeof(grp_window) );
  if ( wd == NULL ) {
    ERRORMSG("ps_open_win", "Memoria esaurita");
    return NULL;
  }

  winstream = (FILE *) malloc( sizeof(FILE) );
  if ( winstream == NULL) {
    ERRORMSG("ps_open_win", "Memoria esaurita");
    free(wd);
    return NULL;
  }

  /* Apro il file su cui verranno scritte le istruzioni postscript */
  winstream = fopen(file, "w");
  if ( winstream == NULL ) {
    ERRORMSG("ps_open_win", "Impossibile aprire il file");
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
   "0.01 0.01 scale\n90 rotate\n1 -1 scale\n0 0 0 setrgbcolor\n"
   , file
  );

  return wd;
}

static int ps_save(const char *unused) {
  fclose((FILE *) grp_win->ptr);
  return 1;
}

int ps_save_fig(const char *file_name, grp_window *figure) {
  Point bb_min, bb_max, translation, center;
  Real sx, sy, rot_angle;
  grp_window *cur_win = grp_win;

  bb_bounding_box(figure, & bb_min, & bb_max);
  printf("Bounding box (%f, %f) - (%f, %f)\n",
         bb_min.x, bb_min.y, bb_max.x, bb_max.y);

  grp_win = ps_open_win(file_name);
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

static void ps_fake_point(Point *p) {return;}
