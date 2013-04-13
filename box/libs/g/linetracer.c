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

/* PROCEDURE DI TRACCIATURA DI LINEE SPESSE */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "types.h"
#include "error.h"
#include "g.h"
#include "graphic.h"
#include "gpath.h"
#include "buffer.h"
#include "i_window.h"
#include "autoput.h"
#include "fig.h"
#include "linetracer.h"

/*#define DEBUG_PROBLEM*/

static int lt_closed_selected = 0;
static long lt_entered_numpnts = 0;
static BoxPoint lt_entered_first_pnt;
static BoxReal lt_entered_s;

/* Procedure contenente gli algoritmi usati per tracciare le linee */
static int lt_draw_opened(BoxGWin *w, LineTracer *lt);
static int lt_draw_closed(LineTracer *lt);
static int lt_put_to_begin_or_end(BoxGWin *win,
                                  LineTracer *lt, BoxPoint *p1, BoxPoint *p2,
                                  BoxReal w, BoxReal fw, void *f, int final);
#if 0
static int lt_put_to_join(LineTracer *lt, BoxPoint *p1, BoxPoint *p2, BoxPoint *p3,
                          BoxReal w1, BoxReal w2, BoxReal fw, void *f, int first);
#endif

static void lt_first_point(LineTracer *lt, BoxPoint *p, BoxReal s);
static void lt_next_point(LineTracer *lt, BoxPoint *p, BoxReal si, BoxReal so);
static void lt_final_point(LineTracer *lt, BoxPoint *p, BoxReal s);
static void lt_closed_begin(LineTracer *lt, BoxPoint *p0, BoxPoint *p1,
                            BoxReal s0o, BoxReal s1i, BoxReal s1o);
static void lt_closed_finish(LineTracer *lt, BoxPoint *p, BoxReal si);
void lt_next_line(LineTracer *lt, double x, double y,
                  double sp1, double sp2, int style);
void lt_first_line(LineTracer *lt, BoxReal x1, BoxReal y1, BoxReal sp1,
                   BoxReal x2, BoxReal y2, BoxReal sp2, BoxReal startlenght,
                   int is_closed);
void lt_last_line(LineTracer *lt, double lastlenght, int is_closed);

LineTracer *lt_new(void) {
  LineTracer *lt = (LineTracer *) malloc(sizeof(LineTracer));
  if (lt == (LineTracer *) NULL) return (LineTracer *) NULL;
  lt->border[0] = gpath_init();
  lt->border[1] = gpath_init();

  if (   lt->border[0] == (GPath *) NULL
      || lt->border[1] == (GPath *) NULL
      || !buff_create(& lt->pieces, sizeof(LinePiece), 10)) {
    gpath_destroy(lt->border[0]);
    gpath_destroy(lt->border[1]);
    buff_free(& lt->pieces);
    free(lt);
    return (LineTracer *) NULL;
  }

  lt->cutting = 8.0;
  return lt;
}

void lt_destroy(LineTracer *lt) {
  gpath_destroy(lt->border[0]);
  gpath_destroy(lt->border[1]);
  buff_free(& lt->pieces);
  free(lt);
}

void lt_add_piece(LineTracer *lt, LinePiece *lp) {
  (void) buff_push(& lt->pieces, lp);
}

void lt_clear(LineTracer *lt) {
  buff_clear(& lt->pieces);
}

BoxInt lt_num_pieces(LineTracer *lt) {
  return buff_numitems(& lt->pieces);
}










/* DESCRIZIONE: Disegna la linea aperta i cui dati sono contenuti in line_desc.
 */
static int lt_draw_opened(BoxGWin *w, LineTracer *lt) {
  long numpnt, m;
  LinePiece *i, *in; /* p --> previous, n --> next */

  /* Una linea e' descritta da almeno 2 punti */
  numpnt = buff_numitem(& lt->pieces);
  if ( numpnt < 2 ) {
    /*g_warning("Line with less than two points");*/
    return 1;
  }

  /* i1 punta all'ultimo punto, i2 e 13 al primo e secondo rispettivamente */
  i = buff_firstitemptr(& lt->pieces, LinePiece);
  in = buff_itemptr(& lt->pieces, LinePiece, 2);

  if ( i->arrow == NULL ) {
    lt_first_point(lt, & (i->point), in->width1 );

  } else {
    /* Attenzione: in->width1 e' lo spessore uscente di i->pnt! */
    if ( ! lt_put_to_begin_or_end(w, lt, & (i->point), & (in->point),
                                  in->width1, in->arrow_scale, i->arrow, 0 ) )
      return 0;
  }

  /* Ripeto per (numpnt - 2) volte */
  for (m = 2; m < numpnt; m++) {
    /* Faccio uno shift dei puntatori, per passare al prossimo punto */
    i = in++;

    lt_join_style_set(lt, & i->style);

    if (i->arrow == NULL) {
      lt_next_point(lt, & (i->point), i->width2, in->width1 );

    } else {
      assert(0);
      /*if ( ! lt_put_to_join(lt, & (ip->pnt), & (i->pnt), & (in->pnt), i->width2, in->width1,
                              1.0 ???, i->arrow, 0) )
        return 0;*/
    }
  }

  /* Ora traccio l'ultima linea */
  lt_join_style_set(lt, & in->style);
  if (in->arrow == NULL) {
    lt_final_point(lt, & (in->point), in->width2 );

  } else {
    if ( ! lt_put_to_begin_or_end(w, lt, & (in->point), & (i->point),
                                  in->width1, in->arrow_scale, in->arrow, 1) )
      return 0;
  }

  return 1;
}

/* DESCRIZIONE: Disegna la linea aperta i cui dati sono contenuti in line_desc.
 */
static int lt_draw_closed(LineTracer *lt) {
  long numpnt, m;
  LinePiece *ip, *i, *in; /* p --> previous, n --> next */
  BoxPoint tp;

  /* Una linea e' descritta da almeno 2 punti */
  numpnt = buff_numitem(& lt->pieces);
  if ( numpnt < 2 ) {
    g_warning("Linea con meno di 2 punti");
    return 1;
  }

  /* i punta all'ultimo punto, ip e in al pen-ultimo e al primo rispett. */
  in = buff_firstitemptr(& lt->pieces, LinePiece);
  i = buff_lastitemptr(& lt->pieces, LinePiece);
  ip = i-1;

  if ( i->arrow == NULL ) {
    lt_closed_begin(lt, & (ip->point), & (i->point),
                    i->width1, i->width2, in->width1);

  } else {
#if 1
    assert(0);
#else
    tp = ip->pnt;
    if ( ! lt_put_to_join(lt, & tp, & (i->pnt), & (in->pnt),
                          i->width2, in->width1, 1.0 ???, i->arrow, 1) )
      return 0;
#endif
  }

  /* Ripeto per (numpnt - 1) volte */
  for ( m = 1; m < numpnt; m++ ) {
    /* Faccio uno shift dei puntatori, per passare al prossimo punto */
    ip = i; i = in++;

    lt_join_style_set(lt, & i->style);

    if ( i->arrow == NULL ) {
      lt_next_point(lt, & (i->point), i->width2, in->width1 );

    } else {
      assert(0);
      /*if ( ! lt_put_to_join(lt, & (ip->pnt), & (i->pnt), & (in->pnt), i->width2, in->width1,
                              1.0 ???, i->arrow, 0) )
        return 0;*/
    }
  }

  /* in punta all'ultimo "punto" della lista, quello da cui siamo partiti! */
  lt_join_style_set(lt, & in->style);
  if ( in->arrow == NULL ) {
    lt_closed_finish(lt, & (in->point), in->width1);

  } else {
    lt_final_point(lt, & tp, in->width1);
  }

  return 1;
}

/** Traccia la linea e pulisce line_desc (i dati relativi). */
int lt_draw(BoxGWin *w, LineTracer *lt, int closed) {
/*    FILE *f;*/

  if (closed) {
    if (!lt_draw_closed(lt)) return 0;
    /*grp_draw_gpath(lt->border[0]); doesn't work in FILL mode!!!
    grp_draw_gpath(lt->border[1]);*/

    gpath_append_gpath(lt->border[0], lt->border[1],
                      GPATH_INVERT | GPATH_JOIN | GPATH_CLOSE);
    BoxGWin_Draw_GPath(w, lt->border[0]);

    /*f = fopen("part0.dat", "w");
    gpath_print_points(lt->border[0], f);
    fclose(f);
    f = fopen("part1.dat", "w");
    gpath_print_points(lt->border[1], f);
    fclose(f);
    gpath_print_points(lt->border[1], stdout);*/

  } else {
    if (!lt_draw_opened(w, lt)) return 0;
    gpath_append_gpath(lt->border[0], lt->border[1],
                      GPATH_INVERT | GPATH_JOIN | GPATH_CLOSE);
    /*f = fopen("part.dat", "w");
    gpath_print_points(lt->border[0], f);
    fclose(f);*/

    BoxGWin_Draw_GPath(w, lt->border[0]);
    /*gpath_print_points(lt->border[0], stdout);*/
/*    printf("---\nBORDER 1+2\n");
    gpath_print(lt->border[0], stdout);*/

  }
  gpath_clear(lt->border[1]);
  gpath_clear(lt->border[0]);
  return 1;
}

/* DESCRIZIONE: Questa funzione serve ad usare una figura come elemento
 *  di (inizio/termine)-linea. La figura verra' disposta sull'estremo p1
 *  della linea p1-p2 (serve a realizzare linee che terminano con una freccia,
 *  ad esempio).
 *  Sulla figura individuiamo 3 punti: f1 (head), f2 (center), f3.
 *  L'algoritmo traslera' f1 su p1, ruotera' la figura usando il vincolo
 *  !near[f2, p2], scalera' la figura (senza deformarla) con s = w/d(f1, f2)
 *  (con w spessore della linea)
 *  NOTA: d(..., ...) e' la distanza fra 2 punti, quindi l'algoritmo scalera'
 *  la figura in modo tale che la distanza f1-f2 diventi uguale a w.
 *  Questa trasformazione rende le dimensioni dell'oggetto poporzionate
 *  allo spessore della linea!).
 *  Infine, una volta che l'oggetto e' stato disposto, viene disegnato
 *  il segmento f3-p2.
 *  Se i punti non sono 3, ma solo 1 (f1), allora verra' assunto f2=f3=(0, 0).
 *  Se i punti sono solo 2, verra' assunto f3=(0, 0).
 */
static int lt_put_to_begin_or_end(BoxGWin *win,
                                  LineTracer *lt, BoxPoint *p1, BoxPoint *p2,
                                  BoxReal w, BoxReal fig_w, void *f, int final) {
  long num_hp;
  BoxReal rot_angle = 0.0, scale_x = 1.0, scale_y = 1.0;
  BoxPoint rot_center, trsl_vect;
  /* Punto finale e iniziale a cui vanno congiunti i segmenti della spezzata */
  BoxPoint pfi, pnt[3];
  Matrix m;
  Window *fw = (Window *) f;

  if (fw == (Window *) NULL)
    return 1;

  else {
    BoxPoint *p;
    p = pointlist_find(& fw->pointlist, "head");
    if (p == (BoxPoint *) NULL) {
      g_error("The figure needs to have at least one hot point with name "
              "\"head\" to be used as an arrow!");
      return 0;
    }
    pnt[0] = *p;
    num_hp = 1;

    p = pointlist_find(& fw->pointlist, "tail");
    if (p != (BoxPoint *) NULL) pnt[num_hp++] = *p;

    p = pointlist_find(& fw->pointlist, "join");
    if (p != (BoxPoint *) NULL) pnt[num_hp++] = *p;
  }

  if (num_hp < 1) {
    g_error("The figure has not any hot points.");
    return 1;
  }

  /* Traslo in modo che pnt[1] vada a finire in p2 */
  rot_center.x = pnt[0].x; rot_center.y = pnt[0].y;
  trsl_vect.x = p1->x - rot_center.x; trsl_vect.y = p1->y - rot_center.y;

  /* Se ho solo un hot-point la trasformazione l'ho gia' individuata!
   * Se ne ho piu' di 1, devo calcolare rotazione e scala!
   */
  if ( num_hp > 1 ) {
    register BoxReal d, dx, dy;

    /* Calcolo la distanza di riferimento per eseguire la scala */
    dx = pnt[1].x - pnt[0].x;
    dy = pnt[1].y - pnt[0].y;
    d = sqrt(dx*dx + dy*dy);

    /* Calcolo il fattore di scala */
    if ( d > 0.0 )
      scale_x = scale_y = fig_w*w/d;

    /* Resta soltanto da eseguire la rotazione */
    {
      int needed = 0;
      BoxReal weight = 1.0;
      BoxPoint near_fig, near_back;

      if ( ! aput_allow("r", & needed ) ) return 0;

      near_back.x = p2->x; near_back.y = p2->y;
      near_fig.x = pnt[1].x; near_fig.y = pnt[1].y;

      /* Setto i parametri prima dei calcoli */
      aput_set( & rot_center, & trsl_vect,
       & rot_angle, & scale_x, & scale_y );

      /* Calcolo i parametri della trasformazione in base ai vincoli */
      if ( ! aput_autoput( & near_fig, & near_back, & weight, 1, needed ) )
        return 0;

      /* Preleva i risultati dei calcoli */
      aput_get( & rot_center, & trsl_vect,
       & rot_angle, & scale_x, & scale_y );
    }
  }

  #ifdef DEBUG
  printf("Vettore di traslazione: T = (%g, %g)\n", trsl_vect.x, trsl_vect.y);
  printf("Centro di rotazione: Q = (%g, %g)\n", rot_center.x, rot_center.y);
  printf("Angolo di rotazione: theta = %g\n", rot_angle);
  printf("Scala x: %g\n", scale_x);
  printf("Scala y: %g\n", scale_y);
  #endif

  /* Calcolo la matrice di trasformazione */
  BoxGMatrix_Set(& m, & trsl_vect, & rot_center, rot_angle, scale_x, scale_y);

  /* Calcolo dove vanno a finire pfi[0] = f3 e pfi[1] = f5,
   * quando trasformo la figura
   */
  if (num_hp < 3)
    pfi = (BoxPoint) {0.0, 0.0};
  else
    pfi = pnt[2];

  BoxGMatrix_Map_Point(& m, & pfi, & pfi);

  /* Disegno l'oggetto */
  BoxGWin_Fig_Draw_Fig_With_Matrix(win, fw->window, & m);

  /* Continuo a disegnare le linee */
  if (final)
    lt_final_point(lt, & pfi, w );
  else
    lt_first_point(lt, & pfi, w );

  return 1;
}

#if 0
/* DESCRIZIONE: Simile alla precedente, ma usa una figura come elemento
 *  di congiunzione fra due linee di una spezzata.
 * NOTA: first specifica se il punto p2 e' il primo, cioe' quello da cui
 *  si inizia a tracciare la linea (che quindi sara' chiusa, altrimenti
 *  sarebbe stata usata la lt_put_to_begin_or_end).
 *  Se first == 1, non viene usata lt_final_point e il primo punto
 *  della figura (f3) viene salvato in *p1 (per uso futuro).
 */
static int lt_put_to_join(LineTracer *lt, BoxPoint *p1, BoxPoint *p2, BoxPoint *p3,
                          BoxReal w1, BoxReal w2, BoxReal fw, void *f, int first) {
  int num_hp;
  BoxReal w = 0.5*(w1 + w2);
  BoxReal rot_angle = 0.0, scale_x = 1.0, scale_y = 1.0;
  BoxPoint rot_center, trsl_vect, *pnt;
  /* Punto finale e iniziale a cui vanno congiunti i segmenti della spezzata */
  BoxPoint pfi[2] = {{0.0, 0.0}, {0.0, 0.0}};
  Matrix m;

  if ( ((obj_header *) f)->child == NULL ) {
    /* La figura da disporre non possiede hot-points: errore! */
    vrmc_liberr("La figura non possiede una lista di hot-points!");
    EXIT_ERR("...->child == NULL!\n");
  }

  /* Prelevo la PList che contiene gli hot-points */
  f_hots = LIST_GET_FROM_STATUS( ((obj_header *) f)->child );

  /* Accedo alla PList */
  pnt = list_access( OBJID_PLIST, f_hots, & num_hp );
  if ( pnt == NULL ) { EXIT_ERR("list_access fallita!\n"); }

  if ( num_hp < 1 ) {
    vrmc_liberr("figura priva di hot-points");
    EXIT_ERR("num_hp = 0!\n");
  }

  /* Traslo in modo che pnt[1] vada a finire in p2 */
  rot_center.x = pnt->x; rot_center.y = pnt->y;
  trsl_vect.x = p2->x - rot_center.x; trsl_vect.y = p2->y - rot_center.y;

  /* Se ho solo un hot-point la trasformazione l'ho gia' individuata!
   * Se ne ho piu' di 1, devo calcolare rotazione e scala!
   */
  if ( num_hp > 1 ) {
    register BoxReal d;

    if ( num_hp >= 3 ) {
      pfi[0] = pnt[2];
      if ( num_hp >= 5 ) {
        pfi[1] = pnt[4];
      }
    }

    /* Calcolo la distanza di riferimento per eseguire la scala */
    if ( num_hp < 4 ) {
      register BoxReal dx, dy;

      dx = pnt[1].x - pnt->x;
      dy = pnt[1].y - pnt->y;
      d = sqrt( dx*dx + dy*dy );

    } else {
      register BoxReal dx, dy;

      dx = pnt[1].x - pnt->x;
      dy = pnt[1].y - pnt->y;
      d = sqrt( dx*dx + dy*dy );
      dx = pnt[3].x - pnt->x;
      dy = pnt[3].y - pnt->y;
      d = ( d + sqrt(dx*dx + dy*dy) ) / 2.0;
    }

    if ( d > 0.0 )
      scale_x = scale_y = w/d;

    /* Resta soltanto da eseguire la rotazione */
    {
      int needed = 0;
      BoxReal weight[2] = {1.0, 1.0};
      BoxPoint near_fig[2], near_back[2];

      if ( ! aput_allow("r", & needed ) ) {
        g_error("aput_allow fallita!");
        return 1;
      }

      near_back[0].x = p1->x; near_back[0].y = p1->y;
      near_back[1].x = p3->x; near_back[1].y = p3->y;

      if ( num_hp < 4 ) {
        near_fig[1].x = near_fig[0].x = pnt[1].x;
        near_fig[1].y = near_fig[0].y = pnt[1].y;
      } else {
        near_fig[0].x = pnt[1].x; near_fig[0].y = pnt[1].y;
        near_fig[1].x = pnt[3].x; near_fig[1].y = pnt[3].y;
      }

      /* Setto i parametri prima dei calcoli */
      aput_set( & rot_center, & trsl_vect,
       & rot_angle, & scale_x, & scale_y );

      /* Calcolo i parametri della trasformazione in base ai vincoli */
      if ( ! aput_autoput( near_fig, near_back, weight, 2, needed ) ) {
        g_error("aput_autoput fallita!");
        return 1;
      }

      /* Preleva i risultati dei calcoli */
      aput_get( & rot_center, & trsl_vect,
       & rot_angle, & scale_x, & scale_y );
    }
  }

  /* Calcolo la matrice di trasformazione */
  BoxGMatrix_Set(& m, & trsl_vect, & rot_center, rot_angle, scale_x, scale_y);

  /* Calcolo dove vanno a finire pfi[0] = f3 e pfi[1] = f5,
   * quando trasformo la figura
   */
  fig_transform_point( pfi, 2 );

  /* Disegno l'oggetto */
  fig_draw_fig( ((obj_header *) f)->info );

  /* Continuo a disegnare le linee */
  lt_first_point( & pfi[1], w2 );

  if ( first ) {
    *p1 = pfi[0];
  } else {
    lt_final_point(lt, & pfi[0], w1 );
  }

  EXIT_OK("figura disegnata!\n");
}
#endif

/* DESCRIZIONE: Specifica il primo punto di una spezzata, col relativo
 *  spessore iniziale.
 */
static void lt_first_point(LineTracer *lt, BoxPoint *p, BoxReal s) {
  if ( lt_entered_numpnts > 0 ) {
    g_warning("Inizio nuova linea, senza aver terminato la linea precedente");
    return;
  }

  lt_entered_first_pnt = *p;
  lt_entered_s = s;

  lt_entered_numpnts = 1;
  return;
}

/* DESCRIZIONE: Specifica il prossimo punto di una spezzata, col relativo
 *  spessore entrante (si) ed uscente (so).
 */
static void lt_next_point(LineTracer *lt, BoxPoint *p, BoxReal si, BoxReal so) {
  if ( lt_entered_numpnts > 1 ) {
    lt_next_line(lt, p->x, p->y, lt_entered_s, si, 1);
    lt_entered_s = so;
    ++lt_entered_numpnts;
    return;

  } else if ( lt_entered_numpnts == 1 ) {
    lt_first_line(lt, lt_entered_first_pnt.x, lt_entered_first_pnt.y,
                  lt_entered_s, p->x, p->y, si, 0.0, 0 );
    lt_entered_s = so;
    ++lt_entered_numpnts;
    return;

  } else {
    g_warning("Secondo punto senza il primo");
    return;
  }

  return;
}

/* DESCRIZIONE: Specifica l'ultimo punto di una spezzata, col relativo
 *  spessore finale.
 */
static void lt_final_point(LineTracer *lt, BoxPoint *p, BoxReal s) {
  if ( lt_entered_numpnts > 1 ) {
    lt_next_line(lt, p->x, p->y, lt_entered_s, s, 1);
    lt_last_line(lt, 0.0, 0);
    lt_entered_numpnts = 0;
    return;

  } else if ( lt_entered_numpnts == 1 ) {
    lt_first_line(lt, lt_entered_first_pnt.x, lt_entered_first_pnt.y,
                  lt_entered_s, p->x, p->y, s, 0.0, 0 );
    lt_last_line(lt, 0.0, 0);
    lt_entered_numpnts = 0;
    return;

  } else {
    g_warning("Ultimo punto senza il primo");
    return;
  }
}

/* DESCRIZIONE: Per tracciare una linea che si richiude su se' stessa, bisogna
 *  usare questa funzione in luogo della lt_first_point e lt_close_finish
 *  alla fine per completarla.
 *  p0 e' l'ultimo punto (serve per dare la giusta forma alla congiuntura
 *  sull'angolo)
 */
static void lt_closed_begin(LineTracer *lt, BoxPoint *p0, BoxPoint *p1,
                            BoxReal s0o, BoxReal s1i, BoxReal s1o) {
  if (lt_entered_numpnts > 0 || lt_closed_selected) {
    g_warning("Inizio nuova linea, senza aver terminato la linea precedente");
    return;
  }

  lt_first_line(lt, p0->x, p0->y, s0o, p1->x, p1->y, s1i, 0.0, 1);

  lt_entered_s = s1o;
  lt_entered_numpnts = 2;
  lt_closed_selected = 1;
}

/* DESCRIZIONE: Completa una linea chiusa. p e' il primo punto, cioe' il p1
 *  della funzione lt_closed_begin.
 */
static void lt_closed_finish(LineTracer *lt, BoxPoint *p, BoxReal si) {
  if ( ! lt_closed_selected ) {
    g_warning("Tentativo di chiudere una linea aperta");
    return;
  }

  if ( lt_entered_numpnts > 1 ) {
    lt_next_line(lt, p->x, p->y, lt_entered_s, si, 1);
    lt_last_line(lt, 0.0, 1);
    lt_entered_numpnts = 0;
    lt_closed_selected = 0;
    return;

  } else if ( lt_entered_numpnts == 1 ) {
    lt_next_line(lt, p->x, p->y, lt_entered_s, si, 1);
    lt_last_line(lt, 0.0, 1);
    lt_entered_numpnts = 0;
    lt_closed_selected = 0;
    return;

  } else {
    g_warning("Meno di 3 punti nella linea chiusa");
    return;
  }
}
















/* Macro utilizzate in seguito */
#define MOD2(x, y)  (x*x + y*y)

/* DESCRIZIONE: Questa funzione calcola l'intersezione fra le seguenti linee:
 *  1) la linea passante per il punto p1 e diretta lungo il vettore d1;
 *  2) la linea passante per p2 e diretta lungo d2.
 *  Se il calcolo riesce restituisce 1, 0 altrimenti (se le linee sono
 *  parallele oppure coincidenti).
 *  La procedura restituisce alpha1, che mi permette di ottenere il punto
 *  d'intersezione nel modo seguente: intersez = p1 + alpha1 * d1
 */
int lt_intersection(BoxPoint *p1, BoxPoint *d1, BoxPoint *p2, BoxPoint *d2,
                    double *alpha1) {
  BoxPoint p2mp1;
  double d1vectd2;

  p2mp1.x = p2->x - p1->x;
  p2mp1.y = p2->y - p1->y;

  /* Primo punto di giunzione */
  d1vectd2 = d1->x * d2->y - d1->y * d2->x;

  if (d1vectd2 == 0.0) return 0;

  *alpha1 = (p2mp1.x * d2->y - p2mp1.y * d2->x) / d1vectd2;

  return 1;
}

/* DESCRIZIONE: Questa funzione calcola l'intersezione fra le seguenti linee:
 *  1) la linea passante per il punto p1 e diretta lungo il vettore d1;
 *  2) la linea passante per p2 e diretta lungo d2.
 *  Se il calcolo riesce restituisce 1, 0 altrimenti (se le linee sono
 *  parallele oppure coincidenti).
 *  La procedura restituisce alpha1, che mi permette di ottenere il punto
 *  d'intersezione nel modo seguente: intersez = p1 + alpha1 * d1
 *  e alpha2 che permette di ottenere lo stesso punto come:
 *  intersez = p2 + alpha2 * d2
 */
int lt_intersection2(BoxPoint *p1, BoxPoint *d1, BoxPoint *p2, BoxPoint *d2,
                     double *alpha1, double *alpha2) {
  BoxPoint p2mp1;
  double d1vectd2;

  p2mp1.x = p2->x - p1->x;
  p2mp1.y = p2->y - p1->y;

  /* Primo punto di giunzione */
  d1vectd2 = d1->x * d2->y - d1->y * d2->x;

  if (d1vectd2 == 0.0) return 0;

  *alpha1 = (p2mp1.x * d2->y - p2mp1.y * d2->x) / d1vectd2;
  *alpha2 = (p2mp1.x * d1->y - p2mp1.y * d1->x) / d1vectd2;

  return 1;
}

/* DESCRIZIONE: Setta lo stile di giunzione. userjs e' un puntatore ad un array
 *  di 4 numeri di tipo float, che descrivono il tipo di giunzione da usare
 *  per collegare le linee fra loro.
 */
void lt_join_style_from_array(LineJoinStyle *ljs,
                              BoxReal ti, BoxReal te, BoxReal ni, BoxReal ne) {
  ljs->ti = ti;
  ljs->te = te;
  ljs->ni = ni;
  ljs->ne = ne;
}

void lt_join_style_set(LineTracer *lt, LineJoinStyle *ljs) {
  lt->join_style = *ljs;
}

/* DESCRIZIONE: Setta il valore di cutting. Quando un angolo della spezzata
 *  e' a gomito molto stretto la congiuntura puo' sporgere troppo.
 *  Allora si puo' pensare di tagliare questa sporgenza.
 *  Il parametro di cutting serve a questo: piu' piccolo e' e piu' grande sara'
 *  il taglio. Esso deve essere positivo ed e' consigliabile sceglierlo
 *  piu' grande di 1.
 * NOTA: Se non c'e' una sporgenza rilevante il taglio non sara' praticato.
 */
void lt_cutting(LineTracer *lt, BoxReal c) {
  if (c > 0.0) lt->cutting = c;
}

/* DESCRIZIONE: Specifica gli estremi della prima linea da tracciare.
 *  Le altre linee che continuano la spezzata devono essere specificate
 *  con lt_next_line
 *  La prima parte della linea pu essere "tagliata".
 *  A tal proposito startlenght specifica la lunghezza della prima parte
 *  della linea che si vuole "omettere".
 *  sp1 e sp2 sono gli spessori della linea in corrispondenza ai due punti
 *  (x1, y1) e (x2, y2).
 */
void lt_first_line(LineTracer *lt, BoxReal x1, BoxReal y1, BoxReal sp1,
                   BoxReal x2, BoxReal y2, BoxReal sp2, BoxReal startlenght,
                   int is_closed) {
  BoxReal sl;
  LineDesc *thsl;

  /* Setto i puntatori alle strutture che contengono i dati sulle linee */
  thsl = lt->thsl = & lt->line1;

  /* Calcolo il vettore relativo alla linea */
  thsl->v.x = (thsl->pnt2.x = x2) - (thsl->pnt1.x = x1);
  thsl->v.y = (thsl->pnt2.y = y2) - (thsl->pnt1.y = y1);
  thsl->mod = sqrt(thsl->mod2 = MOD2(thsl->v.x, thsl->v.y));

  /* Calcolo il versore relativo e il versore ortogonale */
  thsl->o.x = thsl->u.y = thsl->v.y / thsl->mod;
  thsl->o.y = -(thsl->u.x = thsl->v.x / thsl->mod);

  /* Calcolo i due punti laterali relativi alla linea */
  thsl->p[0].x = thsl->pnt1.x + sp1 * thsl->o.x;
  thsl->p[0].y = thsl->pnt1.y + sp1 * thsl->o.y;

  thsl->p[1].x = thsl->pnt1.x - sp1 * thsl->o.x;
  thsl->p[1].y = thsl->pnt1.y - sp1 * thsl->o.y;

  /* Calcolo i vettori dei relativi lati e i loro moduli */
  thsl->vb[0].x = thsl->v.x + (sp2 - sp1) * thsl->o.x;
  thsl->vb[0].y = thsl->v.y + (sp2 - sp1) * thsl->o.y;

  thsl->vb[1].x = thsl->v.x - (sp2 - sp1) * thsl->o.x;
  thsl->vb[1].y = thsl->v.y - (sp2 - sp1) * thsl->o.y;

  /* Determino quanta parte della linea devo "tagliare" */
  sl = startlenght/thsl->mod;

  /* Calcolo i due punti di partenza */
  thsl->vertex[0].x = thsl->p[0].x + sl * thsl->vb[0].x;
  thsl->vertex[0].y = thsl->p[0].y + sl * thsl->vb[0].y;
  thsl->vertex[1].x = thsl->p[1].x + sl * thsl->vb[1].x;
  thsl->vertex[1].y = thsl->p[1].y + sl * thsl->vb[1].y;

  thsl->sp1 = sp1;
  thsl->sp2 = sp2;
  lt->is_closed = is_closed;
  lt->segment = 0;
}

/* DESCRIZIONE: Conclude la tracciatura della spezzata disegnando
 *  la sua ultima linea.
 *  La linea puo' essere accorciata nella sua ultima parte.
 *  A tal proposito lastlenght specifica la lunghezza della parte
 *  che si vuole sia "tagliata". Specificando lastlenght = 0
 *  l'ultima linea verra' tracciata nella sua intera lunghezza.
 */
void lt_last_line(LineTracer *lt, double lastlenght, int is_closed) {
  LineDesc *thsl = lt->thsl;
  if ( is_closed ) {
    /* La linea e' chiusa */
    thsl->vertex[2] = lt->firstline.vertex[2];
    thsl->vertex[3] = lt->firstline.vertex[3];

    /* Traccia l'ultima linea della spezzata */
    gpath_move_to(lt->border[0], & (thsl->vertex[1]));
    gpath_line_to(lt->border[0], & (thsl->vertex[2]));
    gpath_move_to(lt->border[1], & (thsl->vertex[0]));
    gpath_line_to(lt->border[1], & (thsl->vertex[3]));

  } else {
    /* La linea e' aperta */
    double ll = 1 - lastlenght/thsl->mod;

    thsl->vertex[3].x = thsl->p[0].x + ll * thsl->vb[0].x;
    thsl->vertex[3].y = thsl->p[0].y + ll * thsl->vb[0].y;
    thsl->vertex[2].x = thsl->p[1].x + ll * thsl->vb[1].x;
    thsl->vertex[2].y = thsl->p[1].y + ll * thsl->vb[1].y;

    /* Traccia l'ultima linea della spezzata */
    gpath_move_to(lt->border[0], & (thsl->vertex[1]));
    gpath_line_to(lt->border[0], & (thsl->vertex[2]));
    gpath_move_to(lt->border[1], & (thsl->vertex[0]));
    gpath_line_to(lt->border[1], & (thsl->vertex[3]));
  }
}

/* DESCRIZIONE: Continua la tracciatura della spezzata, specificando un altro
 *  suo vertice. sp1 e sp2 sono gli spessori della linea in corrispondenza
 *  ai due punti (xl, yl) e (x, y), dove (xl, yl) e' l'ultimo punto immesso.
 *  style specifica il modo in cui le linee della spezzata devono essere
 *  congiunte.............
 */
void lt_next_line(LineTracer *lt, double x, double y,
                  double sp1, double sp2, int style) {
  LineDesc *thsl = lt->thsl, *nxtl = lt->nxtl;
  double thscongpos[2], nxtcongpos[2];
  int flag, problem[2];

  /* Calcolo il vettore relativo alla seconda linea */
  nxtl->v.x = (nxtl->pnt2.x = x) - (nxtl->pnt1.x = thsl->pnt2.x);
  nxtl->v.y = (nxtl->pnt2.y = y) - (nxtl->pnt1.y = thsl->pnt2.y);
  nxtl->mod = sqrt(nxtl->mod2 = MOD2(nxtl->v.x, nxtl->v.y));

  /* Calcolo il versore relativo e il versore ortogonale */
  nxtl->o.x = nxtl->u.y = nxtl->v.y / nxtl->mod;
  nxtl->o.y = -(nxtl->u.x = nxtl->v.x / nxtl->mod);

  /* Calcolo i due punti laterali relativi alla seconda linea */
  nxtl->p[0].x = nxtl->pnt1.x + sp1 * nxtl->o.x;
  nxtl->p[0].y = nxtl->pnt1.y + sp1 * nxtl->o.y;

  nxtl->p[1].x = nxtl->pnt1.x - sp1 * nxtl->o.x;
  nxtl->p[1].y = nxtl->pnt1.y - sp1 * nxtl->o.y;

  /* Calcolo i vettori dei relativi lati e i loro moduli */
  nxtl->vb[0].x = nxtl->v.x + (sp2 - sp1) * nxtl->o.x;
  nxtl->vb[0].y = nxtl->v.y + (sp2 - sp1) * nxtl->o.y;

  nxtl->vb[1].x = nxtl->v.x - (sp2 - sp1) * nxtl->o.x;
  nxtl->vb[1].y = nxtl->v.y - (sp2 - sp1) * nxtl->o.y;

  nxtl->sp1 = sp1;
  nxtl->sp2 = sp2;

  /* Calcolo i punti di giunzione */
  /* Primo punto di giunzione */
  flag = lt_intersection2(& thsl->p[0], & thsl->vb[0],  /* Prima retta */
                          & nxtl->p[0], & nxtl->vb[0],  /* Seconda retta */
                          & thscongpos[0], & nxtcongpos[0]);
  problem[0] =   (flag == 0)*0x4
               | (thscongpos[0] < 0.0)*0x2
               | (nxtcongpos[0] > 1.0)*0x1;
#ifdef DEBUG_PROBLEM
  if (problem[0] != 0)
    printf("problem[0] = %d\n", problem[0]);
#endif

  if (thscongpos[0] < 0.0) thscongpos[0] = 0.0;
  if (nxtcongpos[0] > 1.0) nxtcongpos[0] = 1.0;

  /* Secondo punto di giunzione */
  flag = lt_intersection2(& thsl->p[1], & thsl->vb[1],  /* Prima retta */
                          & nxtl->p[1], & nxtl->vb[1],  /* Seconda retta */
                          & thscongpos[1], & nxtcongpos[1]);

  problem[1] =   (flag == 0)*0x4
               | (thscongpos[1] < 0.0)*0x2
               | (nxtcongpos[1] > 1.0)*0x1;
#ifdef DEBUG_PROBLEM
  if (problem[1] != 0)
    printf("problem[1] = %d\n", problem[1]);
#endif

  if (thscongpos[1] < 0.0) thscongpos[1] = 0.0;
  if (nxtcongpos[1] > 1.0) nxtcongpos[1] = 1.0;

  /* Completo il calcolo dei punti di giunzione */
  nxtl->cong[0].x = nxtl->p[0].x + nxtcongpos[0] * nxtl->vb[0].x;
  nxtl->cong[0].y = nxtl->p[0].y + nxtcongpos[0] * nxtl->vb[0].y;

  nxtl->cong[1].x = nxtl->p[1].x + nxtcongpos[1] * nxtl->vb[1].x;
  nxtl->cong[1].y = nxtl->p[1].y + nxtcongpos[1] * nxtl->vb[1].y;

  if (style == 0) {
    /* In questo caso le linee si congiungono senza arrotondamenti */
    thsl->vertex[2].x = nxtl->vertex[1].x = nxtl->cong[1].x;
    thsl->vertex[2].y = nxtl->vertex[1].y = nxtl->cong[1].y;
    thsl->vertex[3].x = nxtl->vertex[0].x = nxtl->cong[0].x;
    thsl->vertex[3].y = nxtl->vertex[0].y = nxtl->cong[0].y;

    /* Ora non resta che tracciare la linea */
    if (++lt->segment != 1 || !lt->is_closed) {
      gpath_move_to(lt->border[0], & (thsl->vertex[1]));
      gpath_line_to(lt->border[0], & (thsl->vertex[2]));
      gpath_move_to(lt->border[1], & (thsl->vertex[0]));
      gpath_line_to(lt->border[1], & (thsl->vertex[3]));
    } else {
      lt->firstline = *thsl;
    }

  } else {
    /* In questo caso le linee vengono collegate smussando gli spigoli */
    int inn, ext;
    double thsproj[2], nxtproj[2];
    double thsposzero, nxtposzero;
    double thsipos, thsepos, nxtipos, nxtepos;

    /* Determino quale dei due lati della linea e' interno e quale e' esterno */
    thsproj[0] = (nxtl->cong[0].x - thsl->pnt1.x)*thsl->v.x +
                 (nxtl->cong[0].y - thsl->pnt1.y)*thsl->v.y;

    thsproj[1] = (nxtl->cong[1].x - thsl->pnt1.x)*thsl->v.x +
                 (nxtl->cong[1].y - thsl->pnt1.y)*thsl->v.y;

    nxtproj[0] = (nxtl->cong[0].x - nxtl->pnt1.x)*nxtl->v.x +
                 (nxtl->cong[0].y - nxtl->pnt1.y)*nxtl->v.y;

    nxtproj[1] = (nxtl->cong[1].x - nxtl->pnt1.x)*nxtl->v.x +
                 (nxtl->cong[1].y - nxtl->pnt1.y)*nxtl->v.y;

    if (thsproj[0] < thsproj[1]) {
      inn = 0;
      ext = 1;
      thsposzero = thsproj[0] / thsl->mod2;
    } else {
      inn = 1;
      ext = 0;
      thsposzero = thsproj[1] / thsl->mod2;
    }

    if (nxtproj[0] > nxtproj[1])
      nxtposzero = nxtproj[0] / nxtl->mod2;
    else
      nxtposzero = nxtproj[1] / nxtl->mod2;

    /* We deal with the internal part of the line now */
    if (problem[inn] == 0) {
      /* Trovo punto di terminazione del bordo interno della prima linea */
      if (lt->join_style.ti < 0.0) {
        if (lt->join_style.ti < -1.0)
          thsipos = thscongpos[inn];
        else
          thsipos = thsposzero
                    - lt->join_style.ti*(thscongpos[inn] - thsposzero);

      } else {
        thsipos = thsposzero - lt->join_style.ti * thsl->sp2/thsl->mod;
        if (thsipos < 0.0) thsipos = 0.0;
      }

        /* Trovo punto di terminazione del bordo interno della seconda linea */
      if (lt->join_style.ni < 0.0) {
        if (lt->join_style.ni < -1.0)
          nxtipos = nxtcongpos[inn];
        else
          nxtipos = nxtposzero
                    - lt->join_style.ni*(nxtcongpos[inn] - nxtposzero);

      } else {
        nxtipos = nxtposzero + lt->join_style.ni * nxtl->sp1/nxtl->mod;
        if (nxtipos > 1.0) nxtipos = 1.0;
      }

      /* Completo il calcolo dei punti */
      thsl->vertex[3-inn].x = thsl->p[inn].x + thsipos * thsl->vb[inn].x;
      thsl->vertex[3-inn].y = thsl->p[inn].y + thsipos * thsl->vb[inn].y;

      nxtl->vertex[inn].x = nxtl->p[inn].x + nxtipos * nxtl->vb[inn].x;
      nxtl->vertex[inn].y = nxtl->p[inn].y + nxtipos * nxtl->vb[inn].y;

    } else {
      /* Completo il calcolo dei punti */
      thsl->vertex[3-inn].x = thsl->p[inn].x;
      thsl->vertex[3-inn].y = thsl->p[inn].y;

      nxtl->vertex[inn].x = nxtl->p[inn].x;
      nxtl->vertex[inn].y = nxtl->p[inn].y;
    }

    /* We deal with the internal part of the line now */
    if (problem[ext] == 0) {
      /* Trovo punto di terminazione del bordo esterno della prima linea */
      if (lt->join_style.te < 0.0) {
        if (lt->join_style.te < -1.0)
          thsepos = thscongpos[ext];
        else
          thsepos = thsposzero - lt->join_style.te*(thscongpos[ext] - thsposzero);

      } else {
        thsepos = thsposzero - lt->join_style.te * thsl->sp2/thsl->mod;
        if (thsepos < 0.0) thsepos = 0.0;
      }

        /* Trovo punto di terminazione del bordo esterno della seconda linea */
      if (lt->join_style.ne < 0.0) {
        if (lt->join_style.ne < -1.0)
          nxtepos = nxtcongpos[ext];
        else
          nxtepos = nxtposzero - lt->join_style.ne*(nxtcongpos[ext]-nxtposzero);

      } else {
        nxtepos = nxtposzero + lt->join_style.ne * nxtl->sp1/nxtl->mod;
        if (nxtepos > 1.0) nxtepos = 1.0;
      }

      /*printf("thsipos=%g, nxtipos=%g\n", thsipos, nxtipos);*/

      /* Completo il calcolo dei punti */
      thsl->vertex[3-ext].x = thsl->p[ext].x + thsepos * thsl->vb[ext].x;
      thsl->vertex[3-ext].y = thsl->p[ext].y + thsepos * thsl->vb[ext].y;

      nxtl->vertex[ext].x = nxtl->p[ext].x + nxtepos * nxtl->vb[ext].x;
      nxtl->vertex[ext].y = nxtl->p[ext].y + nxtepos * nxtl->vb[ext].y;

    } else {
      thsl->vertex[3-ext].x = thsl->p[ext].x;
      thsl->vertex[3-ext].y = thsl->p[ext].y;

      nxtl->vertex[ext].x = nxtl->p[ext].x;
      nxtl->vertex[ext].y = nxtl->p[ext].y;
    }

    /* Ora traccio la linea */
    if (++lt->segment != 1 || !lt->is_closed) {
      gpath_move_to(lt->border[0], & (thsl->vertex[1]));
      gpath_line_to(lt->border[0], & (thsl->vertex[2]));
      gpath_move_to(lt->border[1], & (thsl->vertex[0]));
      gpath_line_to(lt->border[1], & (thsl->vertex[3]));

    } else {
      lt->firstline = *thsl;
    }

    /* Ora controllo l'angolo per decidere il tipo di congiuntura
     * da tracciare
     */
    if (lt->cutting > 0.0 && problem[ext] == 0 &&
        (thsl->u.x * nxtl->u.x + thsl->u.y * nxtl->u.y < 0.0) ) {
      /* ANGOLO ACUTO: CURVA A GOMITO: USO UNA CONGIUNTURA SMORZATA */

      double cgwidth, cgrefwidth, alpha;
      BoxPoint cgvertex[5];

      /* La congiuntura ha 4 lati, 2 sono linee, 2 sono curve.
       * Il lato interno e' una curva semplice, mentre quello esterno
       * e' una curva smorzata (costituita da due curve successive)
       * Ora traccio il lato esterno, ma devo fare un sacco di conti!
       */

      /* Trovo la larghezza di riferimento per la congiuntura */
      {
        BoxReal s1, s2, x1, y1, x2, y2, r;

        s1 = thsl->sp2;
        s2 = nxtl->sp1;
        x1 = thsl->mod;
        y1 = s1 - thsl->sp1;
        x2 = s2 - nxtl->sp2;
        y2 = nxtl->mod;

        r = (x2 * s1  + y2 * s2) * x1;
        s2 = (x1 * s1  + y1 * s2) * y2;
        s1 = x1*y2 - x2*y1;
        cgrefwidth = sqrt(r*r + s2*s2) / s1;
      }

      /* Trovo la "larghezza" della congiuntura */
      {
        BoxReal r1, r2;

        cgvertex[2].x = r1 = nxtl->cong[ext].x;
        cgvertex[2].y = r2 = nxtl->cong[ext].y;
        cgvertex[3].x = r1 = nxtl->pnt1.x - r1;
        cgvertex[3].y = r2 = nxtl->pnt1.y - r2;
        cgwidth = sqrt(r1*r1 + r2*r2);
      }

      /* Trovo il punto centrale della curva ( = lato esterno) */
      alpha = 1.0 - lt->cutting * 0.707106781 * cgrefwidth / cgwidth;

      if (alpha > 0 && alpha < 1) {
        /* In questo caso ho cutting! */
        BoxReal beta1, beta2;
        BoxPoint cutdir;

        /* Continuo il calcolo che stavo facendo. */
        cgvertex[2].x += alpha * cgvertex[3].x;

        /* Questi sono i punti iniziale e finale */
        cgvertex[0].x = thsl->vertex[2+inn].x;
        cgvertex[0].y = thsl->vertex[2+inn].y;
        cgvertex[4].x = nxtl->vertex[ext].x;
        cgvertex[4].y = nxtl->vertex[ext].y;

        /* Ora devo trovare i punti 1 e 3 ! */

        /* Trovo la direzione della tangente alla curva nel punto
         * centrale del lato curvo (cioe' cgvertex[2])
         */
        cutdir.x = cgvertex[0].x - cgvertex[4].x;
        cutdir.y = cgvertex[0].y - cgvertex[4].y;

        /* Trovo il punto 1: e' intersezione di 2 rette! */
        if ( ! lt_intersection(
         /* Prima retta: bordo esterno della linea 1 */
         & cgvertex[0], & thsl->vb[ext],
         /* Seconda retta: passante per 2 con direzione cutdir */
         & cgvertex[2], & cutdir, & beta1) ) {goto nxln_err1;}

        /* Trovo il punto 2: e' intersezione di 2 rette! */
        if ( ! lt_intersection(
         /* Prima retta: bordo esterno della linea 2 */
         & cgvertex[4], & nxtl->vb[ext],
         /* Seconda retta: passante per 2 con direzione cutdir */
         & cgvertex[2], & cutdir, & beta2) ) {goto nxln_err1;}

        if (beta1 < 0.0 || beta2 > 0.0) goto nxln_err1;

        cgvertex[1].x = cgvertex[0].x + beta1 * thsl->vb[ext].x;
        cgvertex[1].y = cgvertex[0].y + beta1 * thsl->vb[ext].y;
        cgvertex[3].x = cgvertex[4].x + beta2 * nxtl->vb[ext].x;
        cgvertex[3].y = cgvertex[4].y + beta2 * nxtl->vb[ext].y;
        cgvertex[2].x = 0.5*(cgvertex[1].x + cgvertex[3].x);
        cgvertex[2].y = 0.5*(cgvertex[1].y + cgvertex[3].y);

        /* Finalmente posso tracciare il lato esterno */
        gpath_move_to(lt->border[inn], & (cgvertex[0]));
        gpath_arc_to(lt->border[inn], & (cgvertex[1]), & (cgvertex[2]));
        gpath_arc_to(lt->border[inn], & (cgvertex[3]), & (cgvertex[4]));

        /* Ora traccio il lato interno */
        cgvertex[0].x = thsl->vertex[2+ext].x;
        cgvertex[0].y = thsl->vertex[2+ext].y;
        cgvertex[1].x = nxtl->cong[inn].x;
        cgvertex[1].y = nxtl->cong[inn].y;
        cgvertex[2].x = nxtl->vertex[inn].x;
        cgvertex[2].y = nxtl->vertex[inn].y;
        gpath_move_to(lt->border[ext], & (cgvertex[0]));
        gpath_arc_to(lt->border[ext], & (cgvertex[1]), & (cgvertex[2]));
        goto nxln_exit;
      }

    }

    {
      /* ANGOLO OTTUSO: CURVA DOLCE: USO UNA CONGIUNTURA SEMPLICE! */
      BoxPoint cgvertex[3];

      /* La congiuntura ha 4 lati, 2 sono linee, 2 sono curve.
       * Traccio solo le curve, una in seguito all'altra.
       */

      /* Prima curva */
      if (problem[1] == 0) {
        cgvertex[0].x = thsl->vertex[3].x;
        cgvertex[0].y = thsl->vertex[3].y;
        cgvertex[1].x = nxtl->cong[0].x;
        cgvertex[1].y = nxtl->cong[0].y;
        cgvertex[2].x = nxtl->vertex[0].x;
        cgvertex[2].y = nxtl->vertex[0].y;

      } else {
        cgvertex[0].x = thsl->vertex[3].x;
        cgvertex[0].y = thsl->vertex[3].y;
        cgvertex[1].x = cgvertex[0].x;
        cgvertex[1].y = cgvertex[0].y;
        cgvertex[2].x = nxtl->vertex[0].x;
        cgvertex[2].y = nxtl->vertex[0].y;
      }
      gpath_move_to(lt->border[1], & (cgvertex[0]));
      gpath_arc_to(lt->border[1], & (cgvertex[1]), & (cgvertex[2]));

      /* Seconda curva */
      if (problem[0] == 0) {
        cgvertex[0].x = nxtl->vertex[1].x;
        cgvertex[0].y = nxtl->vertex[1].y;
        cgvertex[1].x = nxtl->cong[1].x;
        cgvertex[1].y = nxtl->cong[1].y;
        cgvertex[2].x = thsl->vertex[2].x;
        cgvertex[2].y = thsl->vertex[2].y;

      } else {
        cgvertex[0].x = nxtl->vertex[1].x;
        cgvertex[0].y = nxtl->vertex[1].y;
        cgvertex[1].x = cgvertex[0].x;
        cgvertex[1].y = cgvertex[0].y;
        cgvertex[2].x = thsl->vertex[2].x;
        cgvertex[2].y = thsl->vertex[2].y;
      }
      gpath_move_to(lt->border[0], & (cgvertex[2]));
      gpath_arc_to(lt->border[0], & (cgvertex[1]), & (cgvertex[0]));
    }
  }

nxln_exit:
  /* Preparo l'elaborazione della prossima linea */
  lt->thsl = nxtl;
  lt->nxtl = thsl;
  return;

nxln_err1:
  printf("Impossibile tracciare lo spigolo! Disattivare cutting!\n");
  goto nxln_exit;
}
