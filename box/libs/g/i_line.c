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

/* Procedure contenente gli algoritmi usati per tracciare le linee */
static int line_draw(int closed, buff *line_desc);
static int line_draw_opened(buff *line_desc);
static int line_draw_closed(buff *line_desc);
static int line_put_to_begin_or_end(
 Point *p1, Point *p2, Real w, void *f, int final );
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
  w->line.this_piece.width1 = w->line.this_piece.width2 = 1.0;
  w->line.this_piece.fig = (void *) NULL;
  w->line.default_color = (Color *) NULL;
  return Success;
}

void line_window_destroy(Window *w) {
  buff_free(& w->line.pieces);
}

Task line_begin(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  w->line.state = GOT_NOTHING;

  if ( ! buff_clear(& w->line.pieces) )
    g_error("buff_clear failed!\n");
  w->line.num_points = 0;
  w->line.close = 0;

  if (w->line.default_color != (Color *) NULL)
    w->line.color = *w->line.default_color;
  else
    w->line.color = w->fg_color;

  {
    grp_window *cur_win = grp_win;
    grp_win = w->window;
    /* Mando le impostazioni alla libreria grafica */
    grp_join_style(& w->line.this_piece.style[0]);
    grp_win = cur_win;
  }

  return Success;
}

Task line_end(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  grp_window *cur_win = grp_win;
  grp_win = w->window;
  grp_rfgcolor(w->line.color.r, w->line.color.g, w->line.color.b);
  (void) line_draw(w->line.close, & w->line.pieces);
  grp_win = cur_win;
  return Success;
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
  WindowLinePiece piece;

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
  grp_window *cur_win = grp_win;
  grp_win = w->window;
  grp_rfgcolor(w->line.color.r, w->line.color.g, w->line.color.b);
  (void) line_draw(w->line.close, & w->line.pieces);
  grp_win = cur_win;

  w->line.state = GOT_NOTHING;
  w->line.num_points = 0;
  w->line.close = 0;

  if ( ! buff_clear(& w->line.pieces) )
    g_error("buff_clear failed!\n");

  return Success;
}

Task line_color(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  Color *c = BOX_VM_ARGPTR1(vmp, Color);
  w->line.color = *c;
  w->line.default_color = & w->line.color;
  grp_window *cur_win = grp_win;
  return Success;
}



#if 0

/* Struttura che descrive tutti i dati relativi ad un punto della linea */
typedef struct {
  Point pnt;     /* Punto della spezzata*/
  Real sp1, sp2;  /* Spessori entrante ed uscente */
  line_style style;  /* Stile della linea */
  void *fig;      /* Oggetto da usare come congiuntura(NULL se non c'e') */
} line_item;

/* Variabili correnti */
static line_style style;
void *fig;
#endif

/* Contenitore dei dati che descrivono la linea */
/*static buff *line_desc = NULL;*/

#if 0
static int line_begin(void) {
  /* Stato dell'istruzione = iniziale */
  status = WAIT_1ST;
  entered.closed = 0;
  fig = NULL;

  /* Inizializzo le impostazioni se ce n'e' bisogno */
  if ( ! entered.init ) {
    style[3] = style[2]
     = style[1] = style[0] = 0.0;
    line_color[2] = line_color[1] = line_color[0] = 1.0;
    sp2 = sp1 = 1.0;

    grp_rfgcolor( (real) 0.0, (real) 0.0, (real) 0.0 );

    entered.init = 1;
  }

  /* Setto le impostazioni di stile congiunzione della linea */
  if (entered.style) {
    (void) memcpy(
     /*dest*/ & style,
     /*src*/  settings->line.style,
     /*size*/ sizeof(line_style)
    );
  }

  /* Setto le impostazioni di colore della linea */
  if (entered.color) {
    (void) memcpy(
     /*dest*/ & line_color,
     /*src*/  settings->line.color,
     /*size*/ sizeof(line_color)
    );
  }

  /* Mando le impostazioni alla libreria grafica */
  grp_join_style( & style[0] );


  if ( line_desc == NULL ) {
    /* In tal caso il buffer che descrive la linea non e' stato ancora creato:
     * lo faccio ora!
     */
    static buff b;
    line_desc = & b;

    if ( ! buff_create( line_desc, sizeof(line_item), PLIST_INITIAL_SIZE ) ) {
      EXIT_ERR("buff_create fallita!\n");
    }

  } else {
    if ( ! buff_clear( line_desc ) ) {
      EXIT_ERR("buff_create fallita!\n");
    }
  }

  if ( ! plist_begin() ) {EXIT_ERR("fallito!\n");}

  EXIT_OK("Ok!\n");
}
#endif

#if 0
static int line_endwithobj(void)
{
  /* Traccia la linea e pulisce il buffer line_desc */
  if ( ! line_draw(entered.closed) )
    return 0;

  return plist_endwithobj();
}

static int line_again(void)
{
  /* Traccia la linea e pulisce il buffer line_desc */
  if ( ! line_draw(entered.closed) )
    return 0;

  status = WAIT_1ST;
  entered.closed = 0;
  EXIT_OK("Ok!\n");
}

static int line_float(void)
{
  if (!vrmc_chkargs("n")) {
    vrmc_liberr("Argomenti errati1");
    return 0;
  }

  switch (status) {
   case WAIT_1ST:
    sp2 = sp1 = vrmc_getfa(1);
    status = WAIT_2ND;
    break;
   case WAIT_2ND:
    sp2 = vrmc_getfa(1);
    status = NO_FLOAT;
    break;
   case NO_FLOAT:
    g_warning("Argomento in eccesso");
    break;
  }

  return 1;
}

static int line_name(void)
{
  return plist_name();
}

static int line_point(void)
{
  static line_item current;

  if ( !plist_point() ) {EXIT_ERR("Fallito!\n");}

  /* Aggiungo il punto corrente al buffer che descrive la linea */
  current.pnt = *vrmc_getpa(1);
  current.sp1 = sp1;
  current.sp2 = sp2;
  //current.style = style;
  current.fig = fig;
  if ( ! buff_push(line_desc, & current) ) {
    EXIT_ERR("buff_push fallito!\n");
  }

  sp1 = sp2;
  fig = NULL;
  status = WAIT_1ST;
  EXIT_OK("Ok!\n");
}

static int line_object(void)
{
  obj_header *obj;

  if (!vrmc_chkargs("o")) {
    vrmc_liberr("Argomenti errati2");
    EXIT_ERR("fallito(1)!\n");
  }

  obj = vrmc_getoa(1);
  if (obj == NULL) {EXIT_ERR("fallito(2)!\n");}

  switch (obj->type) {
   case OBJID_FLIST: {
    /* L'oggetto passato come argomento e' di tipo FList */
    long num;
    Real *ptr;

    /* Accedo all'oggetto */
    ptr = list_access(OBJID_FLIST, obj, & num);
    if (ptr == NULL) {
      vrmc_liberr("Oggetto non valido");
      EXIT_ERR("fallito(3)!\n");
    }

    /* Ho due casi a seconda del numero di float contenuti nella FList */
    switch (num) {
     case 3:
      /* 3 numeri: quindi assumo che la FList contenga un colore (R,G,B) */
      (void) memcpy( & line_color, ptr, sizeof(line_color) );
      grp_rfgcolor(ptr[0], ptr[1], ptr[2]);
      break;

     case 4:
      /* 4 numeri: assumo che la FList contenga uno stile congiuntura */
      (void) memcpy( & style, ptr, sizeof(line_style) );
      break;

     default:
      vrmc_liberr("Oggetto non valido");
      EXIT_ERR("fallito(3)!\n");
    }

    break;
    }

    case OBJID_WINDOW:
    fig = obj;
    break;

   default:
    /* Altri tipi di oggetto non sono ammessi! */
    vrmc_liberr("Tipo di oggetto non ammesso");
    EXIT_ERR("Tipo di oggetto errato!\n");
  }

  EXIT_OK("ok!\n");
}

static int line_closed(void)
{
  entered.closed = 1;
  return 1;
}
#endif

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
    g_warning("Linea con meno di 2 punti");
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
#if 1
    assert(0);
#else
    if ( ! line_put_to_begin_or_end(
     & (i->pnt), & (in->pnt), in->width1, i->fig, 0 ) )
      return 0;
#endif
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
#if 1
    assert(0);
#else
    if ( ! line_put_to_begin_or_end(
     & (in->pnt), & (i->point), in->width1, in->fig, 1 ) )
      return 0;
#endif
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

#if 0
/* DESCRIZIONE: Questa funzione serve ad usare una figura come elemento
 *  di (inizio/termine)-linea. La figura verra' disposta sull'estremo p1
 *  della linea p1-p2 (serve a realizzare linee che terminano con una freccia,
 *  ad esempio).
 *  Sulla figura individuiamo 3 punti: f1, f2, f3.
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
static int line_put_to_begin_or_end(
 Point *p1, Point *p2, Real w, void *f, int final )
{
  void *f_hots;
  long numhp;
  Real rot_angle = 0.0, scale_x = 1.0, scale_y = 1.0;
  Point rot_center, trsl_vect, *pnt;
  /* Punto finale e iniziale a cui vanno congiunti i segmenti della spezzata */
  Point pfi;

  if ( ((obj_header *) f)->child == NULL ) {
    /* La figura da disporre non possiede hot-points: errore! */
    vrmc_liberr("La figura non possiede una lista di hot-points!");
    EXIT_ERR("...->child == NULL!\n");
  }

  /* Prelevo la PList che contiene gli hot-points */
  f_hots = LIST_GET_FROM_STATUS( ((obj_header *) f)->child );

  /* Accedo alla PList */
  pnt = list_access( OBJID_PLIST, f_hots, & numhp );
  if ( pnt == NULL ) { EXIT_ERR("list_access fallita!\n"); }

  if ( numhp < 1 ) {
    vrmc_liberr("figura priva di hot-points");
    EXIT_ERR("numhp = 0!\n");
  }

  /* Traslo in modo che pnt[1] vada a finire in p2 */
  rot_center.x = pnt->x; rot_center.y = pnt->y;
  trsl_vect.x = p1->x - rot_center.x; trsl_vect.y = p1->y - rot_center.y;

  /* Se ho solo un hot-point la trasformazione l'ho gia' individuata!
   * Se ne ho piu' di 1, devo calcolare rotazione e scala!
   */
  if ( numhp > 1 ) {
    register Real d, dx, dy;

    /* Calcolo la distanza di riferimento per eseguire la scala */
    dx = pnt[1].x - pnt->x;
    dy = pnt[1].y - pnt->y;
    d = sqrt( dx*dx + dy*dy );

    /* Calcolo il fattore di scala */
    if ( d > 0.0 )
      scale_x = scale_y = w/d;

    /* Resta soltanto da eseguire la rotazione */
    {
      int needed = 0;
      Real weight = 1.0;
      Point near_fig, near_back;

      if ( ! aput_allow("r", & needed ) ) {
        EXIT_ERR("aput_allow fallita!\n");
      }

      near_back.x = p2->x; near_back.y = p2->y;
      near_fig.x = pnt[1].x; near_fig.y = pnt[1].y;

      /* Setto i parametri prima dei calcoli */
      aput_set( & rot_center, & trsl_vect,
       & rot_angle, & scale_x, & scale_y );

      /* Calcolo i parametri della trasformazione in base ai vincoli */
      if ( ! aput_autoput( & near_fig, & near_back, & weight, 1, needed ) ) {
        EXIT_ERR("aput_autoput fallita!\n");
      }

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
  if ( numhp < 3 )
    pfi = (Point) {0.0, 0.0};
  else
    pfi = pnt[2];

  fig_ltransform( & pfi, 1 );

  /* Disegno l'oggetto */
  fig_draw_fig( ((obj_header *) f)->info );

  /* Continuo a disegnare le linee */
  if ( final )
    line_final_point( & pfi, w );
  else
    line_first_point( & pfi, w );

  EXIT_OK("figura disegnata!\n");
}

/* DESCRIZIONE: Simile alla precedente, ma usa una figura come elemento
 *  di congiunzione fra due linee di una spezzata.
 * NOTA: first specifica se il punto p2 e' il primo, cioe' quello da cui
 *  si inizia a tracciare la linea (che quindi sara' chiusa, altrimenti
 *  sarebbe stata usata la line_put_to_begin_or_end).
 *  Se first == 1, non viene usata line_final_point e il primo punto
 *  della figura (f3) viene salvato in *p1 (per uso futuro).
 */
static int line_put_to_join( Point *p1, Point *p2, Point *p3,
 Real w1, Real w2, void *f, int first )
{
  void *f_hots;
  long numhp;
  Real w = ( w1 + w2 ) / 2.0;
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
  pnt = list_access( OBJID_PLIST, f_hots, & numhp );
  if ( pnt == NULL ) { EXIT_ERR("list_access fallita!\n"); }

  if ( numhp < 1 ) {
    vrmc_liberr("figura priva di hot-points");
    EXIT_ERR("numhp = 0!\n");
  }

  /* Traslo in modo che pnt[1] vada a finire in p2 */
  rot_center.x = pnt->x; rot_center.y = pnt->y;
  trsl_vect.x = p2->x - rot_center.x; trsl_vect.y = p2->y - rot_center.y;

  /* Se ho solo un hot-point la trasformazione l'ho gia' individuata!
   * Se ne ho piu' di 1, devo calcolare rotazione e scala!
   */
  if ( numhp > 1 ) {
    register Real d;

    if ( numhp >= 3 ) {
      pfi[0] = pnt[2];
      if ( numhp >= 5 ) {
        pfi[1] = pnt[4];
      }
    }

    /* Calcolo la distanza di riferimento per eseguire la scala */
    if ( numhp < 4 ) {
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
        EXIT_ERR("aput_allow fallita!\n");
      }

      near_back[0].x = p1->x; near_back[0].y = p1->y;
      near_back[1].x = p3->x; near_back[1].y = p3->y;

      if ( numhp < 4 ) {
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
        EXIT_ERR("aput_autoput fallita!\n");
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
