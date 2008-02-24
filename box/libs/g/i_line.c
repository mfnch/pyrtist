/* Interface for Line */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "types.h"
#include "virtmach.h"
#include "graphic.h"
#include "g.h"
#include "i_window.h"
#include "i_line.h"

#include "debug.h"
#include "error.h"
#include "buffer.h"
#include "fig.h"
#include "autoput.h"

/*#define DEBUG*/

/* Procedure contenente gli algoritmi usati per tracciare le linee */
static int line_draw(int closed, buff *line_desc);
static int line_draw_opened(buff *line_desc);
static int line_draw_closed(buff *line_desc);
static int line_put_to_begin_or_end(Point *p1, Point *p2, Real w,
                                    void *f, int final);
static int line_put_to_join( Point *p1, Point *p2, Point *p3,
 Real w1, Real w2, void *f, int first );
static void line_first_point(Point *p, Real s);
static void line_next_point(Point *p, Real si, Real so);
static void line_final_point(Point *p, Real s);
static void line_closed_begin(
 Point *p0, Point *p1, Real s0o, Real s1i, Real s1o );
static void line_closed_finish(Point *p, Real si);

Task line_window_init(Window *w) {
  if ( ! buff_create(& w->line.pieces, sizeof(WindowLinePiece), 16) ) {
    g_error("buff_create failed!\n");
    return Failed;
  }

  /* We want these settings to be global:
   * for all lines of this window
   */
  LineStyle *ls = & w->line.this_piece.style;
  (*ls)[0] = 0.0; (*ls)[1] = 0.0; (*ls)[2] = 0.0; (*ls)[3] = 0.0;
  {
    grp_window *cur_win = grp_win;
    grp_win = w->window;
    /* Mando le impostazioni alla libreria grafica */
    grp_join_style(& w->line.this_piece.style[0]);
    grp_win = cur_win;
  }

  w->line.this_piece.width1 = w->line.this_piece.width2 = 1.0;
  w->line.this_piece.fig = (void *) NULL;
  g_optcolor_alternative_set(& w->line.color, & w->fg_color);
  g_optcolor_unset(& w->line.color);
  return Success;
}

Task line_color(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  Color *c = BOX_VM_ARGPTR1(vmp, Color);
  return g_optcolor_set(& w->line.color, c);
}

void line_window_destroy(Window *w) {
  buff_free(& w->line.pieces);
}

Task line_begin(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  w->line.state = GOT_NOTHING;

  if ( ! buff_clear(& w->line.pieces) ) {
    g_error("buff_clear failed!\n");
    return Failed;
  }
  w->line.num_points = 0;
  w->line.close = 0;

  return Success;
}

Task line_end(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  if (buff_numitem(& w->line.pieces) < 1)
    return Success;

  else {
    grp_window *cur_win = grp_win;
    Color *c = g_optcolor_get(& w->line.color);
    grp_win = w->window;
    grp_rfgcolor(c->r, c->g, c->b);
    (void) line_draw(w->line.close, & w->line.pieces);
    grp_win = cur_win;
    return Success;
  }
}

Task line_real(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  Real width = BOX_VM_ARG1(vmp, Real);

  switch(w->line.state) {
  case GOT_NOTHING:
    w->line.this_piece.width2 = width;
    w->line.state = GOT_2ND_FLOAT;
    break;

  case GOT_POINT:
    w->line.this_piece.width1 = width;
    w->line.state = GOT_1ST_FLOAT;
    break;

  case GOT_1ST_FLOAT:
    w->line.this_piece.width2 = width;
    w->line.state = GOT_2ND_FLOAT;
    break;

  case GOT_2ND_FLOAT:
    g_error("Too many width specificators.");
    return Failed;

  default:
    g_error("line_real: unknown line state.");
    break;
  }
  return Success;
}

Task line_point(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  Point *p = BOX_VM_ARGPTR1(vmp, Point);

  w->line.state = GOT_POINT;
  w->line.this_piece.point = *p;
  if (! buff_push(& w->line.pieces, & w->line.this_piece) )
    g_error("buff_push failed!\n");

  ++w->line.num_points;
  w->line.this_piece.width1 = w->line.this_piece.width2;
  w->line.this_piece.fig = (void *) NULL;
  return Success;
}

Task line_pause(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  Color *c = g_optcolor_get(& w->line.color);
  grp_window *cur_win = grp_win;
  grp_win = w->window;
  grp_rfgcolor(c->r, c->g, c->b);
  (void) line_draw(w->line.close, & w->line.pieces);
  grp_win = cur_win;

  w->line.state = GOT_NOTHING;
  w->line.num_points = 0;
  w->line.close = 0;

  if ( ! buff_clear(& w->line.pieces) )
    g_error("buff_clear failed!");

  return Success;
}

Task line_window(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  WindowPtr *wp = BOX_VM_ARGPTR1(vmp, WindowPtr);

  w->line.this_piece.fig = (void *) *wp;
  return Success;
}

/* For now the same style is applied to the whole line.
 * I think we can mix styles for the same line. FIXME
 */
Task line_style(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  LineStyle *ls = BOX_VM_ARGPTR1(vmp, LineStyle);
  grp_window *cur_win = grp_win;
  w->line.this_piece.style[0] = (*ls)[0];
  w->line.this_piece.style[1] = (*ls)[1];
  w->line.this_piece.style[2] = (*ls)[2];
  w->line.this_piece.style[3] = (*ls)[3];
  grp_win = w->window;
  grp_join_style(& w->line.this_piece.style[0]);
  grp_win = cur_win;
  return Success;
}

Task window_line_close_begin(VMProgram *vmp) {
  SUBTYPE2_OF_WINDOW(vmp, w);
  w->line.close = 1;
  return Success;
}

Task window_line_close_int(VMProgram *vmp) {
  SUBTYPE2_OF_WINDOW(vmp, w);
  w->line.close = BOX_VM_ARG1(vmp, Int);
  return Success;
}

/******************************************************************************
 * F U N Z I O N I    D I   D I S E G N O    P E R    L I N E E   S P E S S E *
 ******************************************************************************/

/* DESCRIZIONE: Traccia la linea e pulisce line_desc (i dati relativi).
 */
static int line_draw(int closed, buff *line_desc) {
  return closed ? line_draw_closed(line_desc) : line_draw_opened(line_desc);
}

/* Questa macro viene utilizzata nelle funzioni per disegnare gli angoli
 * di una linea spezzata nelle funzioni line_draw_opened e line_draw_closed.
 */
#define LINE_DRAW_NEXT_CORNER \
  if ( i->fig == NULL ) { \
    line_next_point( & (i->point), i->width2, in->width1 ); \
\
  } else { assert(0); }

#if 0
    if ( ! line_put_to_join(
     & (ip->pnt), & (i->pnt), & (in->pnt), i->width2, in->width1, i->fig, 0) )
      return 0;
  }
#endif


/* DESCRIZIONE: Disegna la linea aperta i cui dati sono contenuti in line_desc.
 */
static int line_draw_opened(buff *line_desc) {
  long numpnt, m;
  WindowLinePiece *ip, *i, *in; /* p --> previous, n --> next */

  /* Una linea e' descritta da almeno 2 punti */
  numpnt = buff_numitem(line_desc);
  if ( numpnt < 2 ) {
    g_warning("Line with less than two points");
    return 1;
  }

  /* i1 punta all'ultimo punto, i2 e 13 al primo e secondo rispettivamente */
  ip = buff_lastitemptr(line_desc, WindowLinePiece);
  i = buff_firstitemptr(line_desc, WindowLinePiece);
  in = buff_itemptr(line_desc, WindowLinePiece, 2);

  if ( i->fig == NULL ) {
    line_first_point( & (i->point), in->width1 );

  } else {
    /* Attenzione: in->width1 e' lo spessore uscente di i->pnt! */
    if ( ! line_put_to_begin_or_end(& (i->point), & (in->point),
                                    in->width1, i->fig, 0 ) )
      return 0;
  }

  /* Ripeto per (numpnt - 2) volte */
  for ( m = 2; m < numpnt; m++ ) {
    /* Faccio uno shift dei puntatori, per passare al prossimo punto */
    ip = i; i = in++;

    LINE_DRAW_NEXT_CORNER;
  }

  /* Ora traccio l'ultima linea */
  if ( in->fig == NULL ) {
    line_final_point( & (in->point), in->width2 );

  } else {
    if ( ! line_put_to_begin_or_end(& (in->point), & (i->point),
                                    in->width1, in->fig, 1) )
      return 0;
  }

  return 1;
}

/* DESCRIZIONE: Disegna la linea aperta i cui dati sono contenuti in line_desc.
 */
static int line_draw_closed(buff *line_desc) {
  long numpnt, m;
  WindowLinePiece *ip, *i, *in; /* p --> previous, n --> next */
  Point tp;

  /* Una linea e' descritta da almeno 2 punti */
  numpnt = buff_numitem(line_desc);
  if ( numpnt < 2 ) {
    g_warning("Linea con meno di 2 punti");
    return 1;
  }

  /* i punta all'ultimo punto, ip e in al pen-ultimo e al primo rispett. */
  in = buff_firstitemptr(line_desc, WindowLinePiece);
  i = buff_lastitemptr(line_desc, WindowLinePiece);
  ip = i-1;

  if ( i->fig == NULL ) {
    line_closed_begin( & (ip->point), & (i->point),
                      i->width1, i->width2, in->width1);

  } else {
#if 1
    assert(0);
#else
    tp = ip->pnt;
    if ( ! line_put_to_join(
     & tp, & (i->pnt), & (in->pnt), i->width2, in->width1, i->fig, 1) )
      return 0;
#endif
  }

  /* Ripeto per (numpnt - 1) volte */
  for ( m = 1; m < numpnt; m++ ) {
    /* Faccio uno shift dei puntatori, per passare al prossimo punto */
    ip = i; i = in++;

    LINE_DRAW_NEXT_CORNER;
  }

  /* in punta all'ultimo "punto" della lista, quello da cui siamo partiti! */
  if ( in->fig == NULL ) {
    line_closed_finish(& (in->point), in->width1);

  } else {
    line_final_point(& tp, in->width1);
  }

  return 1;
}

/* DESCRIZIONE: Questa funzione serve ad usare una figura come elemento
 *  di (inizio/termine)-linea. La figura verra' disposta sull'estremo p1
 *  della linea p1-p2 (serve a realizzare linee che terminano con una freccia,
 *  ad esempio).
 *  Sulla figura individuiamo 3 punti: f1 (head), f2 (center), f3.
 *  L'algoritmo traslera' f1 su p1, ruotera' la figura usando il vincolo
 *  !near[f2, p2], scalera' la figura (senza deformarla) con s = d(f1, f2)/w
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
static int line_put_to_begin_or_end(Point *p1, Point *p2, Real w,
                                    void *f, int final) {
  long num_hp;
  Real rot_angle = 0.0, scale_x = 1.0, scale_y = 1.0;
  Point rot_center, trsl_vect;
  /* Punto finale e iniziale a cui vanno congiunti i segmenti della spezzata */
  Point pfi, pnt[3];
  Window *fw = (Window *) f;

  if (fw == (Window *) NULL)
    return 1;

  else {
    Point *p;
    p = pointlist_find(& fw->pointlist, "head");
    if (p == (Point *) NULL) {
      g_error("The figure needs to have at least one hot point with name "
              "\"head\" to be used as an arrow!");
      return 0;
    }
    pnt[0] = *p;
    num_hp = 1;

    p = pointlist_find(& fw->pointlist, "tail");
    if (p != (Point *) NULL) pnt[num_hp++] = *p;

    p = pointlist_find(& fw->pointlist, "join");
    if (p != (Point *) NULL) pnt[num_hp++] = *p;
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
    register Real d, dx, dy;

    /* Calcolo la distanza di riferimento per eseguire la scala */
    dx = pnt[1].x - pnt[0].x;
    dy = pnt[1].y - pnt[0].y;
    d = sqrt( dx*dx + dy*dy );

    /* Calcolo il fattore di scala */
    if ( d > 0.0 )
      scale_x = scale_y = w/d;

    /* Resta soltanto da eseguire la rotazione */
    {
      int needed = 0;
      Real weight = 1.0;
      Point near_fig, near_back;

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
  aput_matrix(
   & trsl_vect,
   & rot_center, rot_angle,
   scale_x, scale_y,
   fig_matrix
  );

  /* Calcolo dove vanno a finire pfi[0] = f3 e pfi[1] = f5,
   * quando trasformo la figura
   */
  if ( num_hp < 3 )
    pfi = (Point) {0.0, 0.0};
  else
    pfi = pnt[2];

  fig_ltransform( & pfi, 1 );

  /* Disegno l'oggetto */
  fig_draw_fig(fw->window);

  /* Continuo a disegnare le linee */
  if ( final )
    line_final_point( & pfi, w );
  else
    line_first_point( & pfi, w );

  return 1;
}

#if 0
/* DESCRIZIONE: Simile alla precedente, ma usa una figura come elemento
 *  di congiunzione fra due linee di una spezzata.
 * NOTA: first specifica se il punto p2 e' il primo, cioe' quello da cui
 *  si inizia a tracciare la linea (che quindi sara' chiusa, altrimenti
 *  sarebbe stata usata la line_put_to_begin_or_end).
 *  Se first == 1, non viene usata line_final_point e il primo punto
 *  della figura (f3) viene salvato in *p1 (per uso futuro).
 */
static int line_put_to_join( Point *p1, Point *p2, Point *p3,
 Real w1, Real w2, void *f, int first ) {
  int num_hp;
  Real w = 0.5*(w1 + w2);
  Real rot_angle = 0.0, scale_x = 1.0, scale_y = 1.0;
  Point rot_center, trsl_vect, *pnt;
  /* Punto finale e iniziale a cui vanno congiunti i segmenti della spezzata */
  Point pfi[2] = {{0.0, 0.0}, {0.0, 0.0}};

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
    register Real d;

    if ( num_hp >= 3 ) {
      pfi[0] = pnt[2];
      if ( num_hp >= 5 ) {
        pfi[1] = pnt[4];
      }
    }

    /* Calcolo la distanza di riferimento per eseguire la scala */
    if ( num_hp < 4 ) {
      register Real dx, dy;

      dx = pnt[1].x - pnt->x;
      dy = pnt[1].y - pnt->y;
      d = sqrt( dx*dx + dy*dy );

    } else {
      register Real dx, dy;

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
      Real weight[2] = {1.0, 1.0};
      Point near_fig[2], near_back[2];

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
  aput_matrix(
   & trsl_vect,
   & rot_center, rot_angle,
   scale_x, scale_y,
   fig_matrix
  );

  /* Calcolo dove vanno a finire pfi[0] = f3 e pfi[1] = f5,
   * quando trasformo la figura
   */
  fig_ltransform( pfi, 2 );

  /* Disegno l'oggetto */
  fig_draw_fig( ((obj_header *) f)->info );

  /* Continuo a disegnare le linee */
  line_first_point( & pfi[1], w2 );

  if ( first ) {
    *p1 = pfi[0];
  } else {
    line_final_point( & pfi[0], w1 );
  }

  EXIT_OK("figura disegnata!\n");
}
#endif

static int line_closed_selected = 0;
static long line_entered_numpnts = 0;
static Point line_entered_first_pnt;
static Real line_entered_s;

/* DESCRIZIONE: Specifica il primo punto di una spezzata, col relativo
 *  spessore iniziale.
 */
static void line_first_point(Point *p, Real s)
{
  if ( line_entered_numpnts > 0 ) {
    g_warning("Inizio nuova linea, senza aver terminato la linea precedente");
    return;
  }

  line_entered_first_pnt = *p;
  line_entered_s = s;

  line_entered_numpnts = 1;
  return;
}

/* DESCRIZIONE: Specifica il prossimo punto di una spezzata, col relativo
 *  spessore entrante (si) ed uscente (so).
 */
static void line_next_point(Point *p, Real si, Real so)
{
  if ( line_entered_numpnts > 1 ) {
    grp_next_line(p->x, p->y, line_entered_s, si, 1);
    grp_rdraw();
    grp_rreset();
    line_entered_s = so;
    ++line_entered_numpnts;
    return;

  } else if ( line_entered_numpnts == 1 ) {
    grp_first_line(
     line_entered_first_pnt.x, line_entered_first_pnt.y, line_entered_s,
     p->x, p->y, si, 0.0, 0 );
    line_entered_s = so;
    ++line_entered_numpnts;
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
static void line_final_point(Point *p, Real s)
{
  if ( line_entered_numpnts > 1 ) {
    grp_next_line(p->x, p->y, line_entered_s, s, 1);
    grp_rdraw();
    grp_rreset();
    grp_last_line(0.0, 0);
    grp_rdraw();
    grp_rreset();
    line_entered_numpnts = 0;
    return;

  } else if ( line_entered_numpnts == 1 ) {
    grp_first_line(
     line_entered_first_pnt.x, line_entered_first_pnt.y, line_entered_s,
     p->x, p->y, s, 0.0, 0 );
    grp_last_line(0.0, 0);
    grp_rdraw();
    grp_rreset();
    line_entered_numpnts = 0;
    return;

  } else {
    g_warning("Ultimo punto senza il primo");
    return;
  }

  return;
}

/* DESCRIZIONE: Per tracciare una linea che si richiude su se' stessa, bisogna
 *  usare questa funzione in luogo della line_first_point e line_close_finish
 *  alla fine per completarla.
 *  p0 e' l'ultimo punto (serve per dare la giusta forma alla congiuntura
 *  sull'angolo)
 */
static void line_closed_begin(
 Point *p0, Point *p1, Real s0o, Real s1i, Real s1o )
{
  if ( (line_entered_numpnts > 0) || (line_closed_selected) ) {
    g_warning(
     "Inizio nuova linea, senza aver terminato la linea precedente");
    return;
  }

  grp_first_line( p0->x, p0->y, s0o, p1->x, p1->y, s1i, 0.0, 1 );

  line_entered_s = s1o;
  line_entered_numpnts = 2;
  line_closed_selected = 1;
  return;
}

/* DESCRIZIONE: Completa una linea chiusa. p e' il primo punto, cioe' il p1
 *  della funzione line_closed_begin.
 */
static void line_closed_finish(Point *p, Real si)
{
  if ( ! line_closed_selected ) {
    g_warning("Tentativo di chiudere una linea aperta");
    return;
  }

  if ( line_entered_numpnts > 1 ) {
    grp_next_line(p->x, p->y, line_entered_s, si, 1);
    grp_rdraw();
    grp_rreset();
    grp_last_line(0.0, 1);
    grp_rdraw();
    grp_rreset();
    line_entered_numpnts = 0;
    line_closed_selected = 0;
    return;

  } else if ( line_entered_numpnts == 1 ) {
    grp_next_line(p->x, p->y, line_entered_s, si, 1);
    grp_rdraw();
    grp_rreset();
    grp_last_line(0.0, 1);
    grp_rdraw();
    grp_rreset();
    line_entered_numpnts = 0;
    line_closed_selected = 0;
    return;

  } else {
    g_warning("Meno di 3 punti nella linea chiusa");
    return;
  }

  return;
}
