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

/* Questo file contiene procedure di grafica generali
 * NOTA: da compilare con error.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "config.h"
#include "error.h"      /* Serve per la segnalazione degli errori */
#include "graphic.h"    /* Dichiaro alcune strutture grafiche generali */
#include "gpath.h"
#include "g.h"

#if HAVE_LIBCAIRO
#  include "wincairo.h"
#endif

/* Costanti moltiplicative usate per convertire: */
/*  - lunghezze in millimetri: */
Real grp_tomm  = 1.0;
/* - angoli in radianti: */
Real grp_torad = grp_radperdeg;
/* - risoluzioni in punti per millimetro: */
Real grp_toppmm = grp_ppmmperdpi;

/********************************************************************/
/*             FUNZIONI GENERALI DI CALCOLO GEOMETRICO              */
/********************************************************************/

/* DESCRIZIONE: Questa procedura serve a cambiare sistema di riferimento:
 *  Il nuovo sistema di riferimento e' specificato dal punto d'origine o
 *  dal vettore v che FISSA LA DIREZIONE dell'asse x (non la scala).
 *  L'ultimo argomento p e' un punto le cui coordinate sono specificate
 *  rispetto a questo nuovo sistema di riferimento.
 *  La funzione restituisce le coordinate di p nel vecchio sistema.
 * NOTA: v viene normalizzato e messo in vx, questo viene ruotato in direzione
 *  oraria (assex -> assey) di 90° per ottenere vy e infine viene restituito
 *  il valore di (p.x * vx + p.y * vy).
 * ATTENZIONE! Ad ogni chiamata la procedura mette il risultato in una
 *  variabile statica e restituisce sempre il relativo puntatore
 *  (che percio' non cambia da una chiamata all'altra!).
 *  Bisogna quindi salvare o comunque utilizzare il risultato prima
 *  di ri-chiamare grp_ref!
 */
Point *grp_ref(Point *o, Point *v, Point *p) {
  Real c, cx, cy;
  static Point result;

  /* Normalizzo v e trovo pertanto vx */
  cx = v->x; cy = v->y;
  c = sqrt(cx*cx + cy*cy);
  if (c == 0.0) {
    ERRORMSG("grp_ref",
    "Punti coincidenti: impossibile costruire il riferimento cartesiano!");
    return NULL;
  }

  cx /= c; cy /= c;

  result.x = o->x + p->x * cx - p->y * cy;
  result.y = o->y + p->x * cy + p->y * cx;

  return & result;
}

/********************************************************************/
/*           GESTIONE DELLE TAVOLAZZE DI COLORI (PALETTE)           */
/********************************************************************/

/* Dichiaro la procedura di hash per trovare velocemente i colori */
static unsigned long color_hash(palette *p, color *c);

/* Questa macro serve a determinare se due colori coincidono o meno
 * (c1 e c2 sono i due puntatori a strutture di tipo color)
 */
#define COLOR_EQUAL(c1, c2) \
 ( ((c1)->r == (c2->r)) && ((c1)->g == (c2->g)) && ((c1)->b == (c2->b)) )


/* DESCRIZIONE: Traduce un colore con componenti RGB date come numeri
 *  compresi tra 0.0 e 1.0, in una struttura di tipo color.
 *  r, g, b sono le componenti iniziali, *c conterra' la struttura finale.
 */
void grp_color_build(Real r, Real g, Real b, color *c)
{
  if ( r < 0.0 ) c->r = 0; else if ( r > 1.0 ) c->r = 255; else c->r = 255*r;
  if ( g < 0.0 ) c->g = 0; else if ( g > 1.0 ) c->g = 255; else c->g = 255*g;
  if ( b < 0.0 ) c->b = 0; else if ( b > 1.0 ) c->b = 255; else c->b = 255*b;
  return;
}

/* DESCRIZIONE: Arrotonda un colore. Il risultato e' simile all'arrotondamento
 *  di un numero reale in un numero intero: usando questa funzione si riduce
 *  il numero complessivo dei colori utilizzati.
 */
void grp_color_reduce(palette *p, color *c)
{
  register unsigned int mask, add, col;
  unsigned int mtable[8] =
   {0777, 0776, 0774, 0770, 0760, 0740, 0700, 0600};
  unsigned int atable[8] =
   {0x0, 0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40};

  mask = mtable[p->reduce];
  add = atable[p->reduce];
  col = ( ((unsigned int) c->r) + add ) & mask;
  c->r = (col >= 255) ? 255 : col;
  col = ( ((unsigned int) c->g) + add ) & mask;
  c->g = (col >= 255) ? 255 : col;
  col = ( ((unsigned int) c->b) + add ) & mask;
  c->b = (col >= 255) ? 255 : col;
  return;
}

/* DESCRIZIONE: Costruisce una tavolazza di colori.
 *  numcol specifica il numero dei colori. La funzione restituisce il puntatore
 *  alla nuova struttura di tipo palette, oppure restituisce NULL, in caso di
 *  insuccesso.
 */
palette *grp_palette_build(long numcol, long hashdim, long hashmul, int reduce)
{
  palette *p;

  if ( (numcol < 1) || (hashmul < 2) ) {
    ERRORMSG("grp_build_palette", "Errore nei parametri");
    return NULL;
  }

  p = (palette *) malloc( sizeof(palette) );
  if ( p == NULL ) {
    ERRORMSG("grp_build_palette", "Memoria esaurita");
    return NULL;
  }

  p->hashdim = hashdim;
  p->hashmul = hashmul;
  p->hashtable = (palitem **) calloc( p->hashdim, sizeof(palitem *) );
  if ( p->hashtable == NULL ) {
    ERRORMSG("grp_build_palette", "Memoria esaurita");
    return NULL;
  }

  p->dim = numcol;
  p->num = 0;

  p->reduce = ( (reduce > 0) && (reduce < 8) ) ?
   reduce : 0;

  return p;
}

/* DESCRIZIONE: Cerca il colore c fra i colori della tavolazza
 */
palitem *grp_color_find(palette *p, color *c)
{
  palitem *pi;

  for ( pi = p->hashtable[color_hash(p, c)];
   pi != (palitem *) NULL; pi = pi->next )
    if ( COLOR_EQUAL( & pi->c, c ) )
      return pi;

  return (palitem *) NULL;
}

/* DESCRIZIONE: Inserisce un nuovo colore, fra quelli gia' presenti nella
 *  tavolazza e restituisce la sua posizione in essa. Se e' gia' presente
 *  lo stesso colore (nei limiti di tolleranza definiti in grp_build_palette)
 *  restituisce la posizione di questo (senza creare nulla!).
 *  Se la tavolazza e' piena, si comporta come stabilito durante la creazione
 *  della tavolazza (vedi grp_build_palette per maggiori dettagli).
 *  Se la richiesta di colore non puo' essere soddisfatta restituisce -1.
 */
palitem *grp_color_request(palette *p, color *c)
{
  color c2;
  palitem *new;

  c2 = *c;
  grp_color_reduce(p, & c2);

  new = grp_color_find(p, & c2);

  if (  new == NULL ) {
    /* Colore non ancora introdotto */
    palitem *pi;
    unsigned long hashval;

    if ( p->num >= p->dim) {
      ERRORMSG("grp_color_request", "Tavolazza piena");
      return NULL;
    }

    pi = (palitem *) malloc( sizeof(palitem) );

    if ( pi == NULL ) {
      ERRORMSG("grp_color_request", "Memoria esaurita");
      return NULL;
    }

    pi->index = p->num++;
    pi->c = c2;

    hashval = color_hash(p, & c2);
    pi->next = p->hashtable[hashval];
    p->hashtable[hashval] = pi;

    return pi;
  }

  return new;
}

/* DESCRIZIONE: Scorre tutta la tavolazza dei colori eseguendo l'operazione
 *  operation per ogni elemento *pi della tavolazza.
 *  L'operazione viene svolta chiamando la funzione operator, la quale
 *  restituisce 1 se va tutto bene (in tal caso la scansione della tabella
 *  continua fino alla fine), oppure restituisce 0 se si e' verificato qualche
 *  errore (in tal caso la scansione ha immediatamente termine e
 *  grp_palette_transform esce restituendo 0).
 */
int grp_palette_transform( palette *p, int (*operation)(palitem *pi) )
{
  int i;
  palitem *pi;

  /* Scorriamo tutta la hash-table in cerca di tutti gli elementi */
  for ( i = 0; i < p->hashdim; i++ ) {

    for ( pi = p->hashtable[i];
     pi != (palitem *) NULL; pi = pi->next ) {

      /* Eseguo l'operazione su ciascun elemento */
      if ( ! operation(pi) ) return 0;
    }

  }

  return 1;
}

/* DESCRIZIONE: Distrugge la palette p, liberando la memoria da essa occupata.
 */
void grp_palette_destroy(palette *p)
{
  int i;
  palitem *pi, *nextpi;

  /* Scorriamo tutta la hash-table in cerca di elementi da cancellare */
  for ( i = 0; i < p->hashdim; i++ ) {

    for ( pi = p->hashtable[i];
     pi != (palitem *) NULL; pi = nextpi ) {

      nextpi = pi->next;
      free( pi );
    }

  }

  /* Elimino la hash-table */
  free(p->hashtable);
  /* Elimino la struttura "palette" */
  free(p);

  return;
}

/* DESCRIZIONE: Funzione che associa ad ogni colore, un indice (della tavola
 *  di hash) in modo "abbastanza casuale". Serve per trovare velocemente i colori,
 *  senza scorrere tutta la tavolazza.
 */
static unsigned long color_hash(palette *p, color *c)
{
  return ( ( (unsigned long ) c->b ) + (c->g + c->r * p->hashmul) * p->hashmul ) % p->hashdim;
}

/* NEW CODE */

static int grp_draw_gpath_iterator(Int index, GPathPiece *piece, void *data) {
  Point *p = & (piece->p[0]);
  switch(piece->kind) {
  case GPATHKIND_LINE:
    grp_rline(& p[0], & p[1]);
    return 0;

  case GPATHKIND_ARC:
    grp_rcong(& p[0], & p[1], & p[2]);
    return 0;

  default:
    return 1;
  }
}

void grp_draw_gpath(GPath *gp) {
  (void) gpath_iter(gp, grp_draw_gpath_iterator, (void *) NULL);
}

/****************************************************************************/
/*  DEFINITION OF A DUMMY WINDOW WHICH DOES NOTHING OR JUST REPORTS ERRORS  */
/****************************************************************************/

/* Creo una finestra priva di qualsiasi funzione: essa dara' un errore
 * per qualsiasi operazione s tenti di eseguire.
 * La finestra cosi' creata sara' la finestra iniziale, che corrisponde
 * allo stato "nessuna finestra ancora aperta" (in modo da evitare
 * accidentali "segmentation fault").
 */

static void dummy_err(GrpWindow *w, const char *method) {
  if (! w->quiet) {
    fprintf(stderr, "%s.%s: method is not implemented.\n",
            w->win_type_str, method);
  }
}

static void dummy_rreset(void) {dummy_err(grp_win, "rreset");}
static void dummy_rinit(void) {dummy_err(grp_win, "rinit");}
static void dummy_rdraw(DrawStyle style) {dummy_err(grp_win, "rdraw");}
static void dummy_rline(Point *a, Point *b) {dummy_err(grp_win, "rline");}
static void dummy_rcong(Point *a, Point *b, Point *c) {
  dummy_err(grp_win, "rcong");
}
static void dummy_rcircle(Point *ctr, Point *a, Point *b) {
  dummy_err(grp_win, "rcircle");
}
static void dummy_rfgcolor(Real r, Real g, Real b) {
  dummy_err(grp_win, "rfgcolor");
}
static void dummy_rbgcolor(Real r, Real g, Real b) {
  dummy_err(grp_win, "rbgcolor");
}
static void dummy_text(Point *p, const char *text) {
  dummy_err(grp_win, "text");
}
static void dummy_font(const char *font, Real size) {
  dummy_err(grp_win, "font");
}
static void dummy_fake_point(Point *p) {dummy_err(grp_win, "fake_point");}
static int dummy_save(const char *file_name) {
  /* If this function is not provided by the specific terminal, then
   * the window is probably a stream window. The best thing to do is then
   * to silently ignore the command.
   */
  return 1;
}

void dummy_close_win(void) {dummy_err(grp_win, "close_win");}
void dummy_set_col(int col) {dummy_err(grp_win, "set_col");}
void dummy_draw_point(Int ptx, Int pty) {dummy_err(grp_win, "draw_point");}
void dummy_hor_line(Int y, Int x1, Int x2) {dummy_err(grp_win, "hor_line");}

void grp_window_block(GrpWindow *w) {
  w->rreset = dummy_rreset;
  w->rinit = dummy_rinit;
  w->rdraw = dummy_rdraw;
  w->rline = dummy_rline;
  w->rcong = dummy_rcong;
  w->rcircle = dummy_rcircle;
  w->rfgcolor = dummy_rfgcolor;
  w->rbgcolor = dummy_rbgcolor;
  w->text = dummy_text;
  w->font = dummy_font;
  w->fake_point = dummy_fake_point;
  w->save = dummy_save;

  w->close_win = dummy_close_win;
  w->set_col = dummy_set_col;
  w->draw_point = dummy_draw_point;
  w->hor_line = dummy_hor_line;
}

GrpWindow grp_dummy_win = {
  "blocked",
  dummy_rreset,
  dummy_rinit,
  dummy_rdraw,
  dummy_rline,
  dummy_rcong,
  dummy_rcircle,
  dummy_rfgcolor,
  dummy_rbgcolor,
  dummy_text,
  dummy_font,
  dummy_fake_point,
  dummy_save,
  0, /* quiet */
  dummy_close_win,
  dummy_set_col,
  dummy_draw_point,
  dummy_hor_line,
  grp_window_block /* repair */
};

/* Descrittore della finestra attualmente in uso: lo faccio puntare
 * ad una finestra che da solo errori (vedi piu' avanti).
 */
grp_window *grp_win = & grp_dummy_win;

/****************************************************************************/

enum {HAVE_TYPE=1, HAVE_ORIGIN=2, HAVE_SIZE=4, HAVE_CORNERS=6,
      HAVE_RESOLUTION=8, HAVE_FILE_NAME=0x10, HAVE_NUM_LAYERS=0x20};

typedef enum {WL_NONE=-1, WL_G=0, WL_CAIRO} WL;

struct win_lib {
  char *name;
  WL win_lib;

} win_libs[] = {
  {          "g", WL_G},
  {      "cairo", WL_CAIRO},
  {(char *) NULL, WL_NONE}
};

static int num_win_terminals = -1;

struct win_type {
  char *type_str;
  WT type_num;
  WL win_lib;
  int must_have;

} win_types[] = {
  /* NOTE: preferred terminals must come later. Example: if we have two
   * EPS terminals (WL_G and WL_CAIRO), the one which appears latter in
   * the list is the one which is chosen by default, i.e. when the prefix
   * "cairo:" (or "g:") is missing.
   */
  {"eps", WT_EPS, WL_CAIRO, HAVE_TYPE + HAVE_FILE_NAME + HAVE_SIZE},
  {"ps",   WT_PS, WL_CAIRO, HAVE_TYPE + HAVE_FILE_NAME + HAVE_SIZE},
  {"a1",   WT_A1, WL_CAIRO, HAVE_TYPE + HAVE_CORNERS + HAVE_RESOLUTION},
  {"a8",   WT_A8, WL_CAIRO, HAVE_TYPE + HAVE_CORNERS + HAVE_RESOLUTION},
  {"rgb24",   WT_RGB24, WL_CAIRO, HAVE_TYPE + HAVE_CORNERS + HAVE_RESOLUTION},
  {"argb32", WT_ARGB32, WL_CAIRO, HAVE_TYPE + HAVE_CORNERS + HAVE_RESOLUTION},
  {"pdf", WT_PDF, WL_CAIRO, HAVE_TYPE + HAVE_FILE_NAME + HAVE_SIZE},
  {"svg", WT_SVG, WL_CAIRO, HAVE_TYPE + HAVE_FILE_NAME + HAVE_SIZE},
  {"bm1", WT_BM1, WL_G, HAVE_TYPE + HAVE_CORNERS + HAVE_RESOLUTION},
  {"bm4", WT_BM4, WL_G, HAVE_TYPE + HAVE_CORNERS + HAVE_RESOLUTION},
  {"bm8", WT_BM8, WL_G, HAVE_TYPE + HAVE_CORNERS + HAVE_RESOLUTION},
  {"fig", WT_FIG, WL_G, HAVE_TYPE},
  {"ps",   WT_PS, WL_G, HAVE_TYPE + HAVE_FILE_NAME},
  {"eps", WT_EPS, WL_G, HAVE_TYPE + HAVE_FILE_NAME + HAVE_SIZE},
  {(char *) NULL, WT_NONE}
};

int grp_window_type_from_string(const char *type_str) {
  char *colon = (char *) NULL;
  struct win_type *this_type;
  WL preferred_lib = WL_NONE;
  int type = -1, i;

  colon = index(type_str, ':');
  if (colon != (char *) NULL) {
    char *lib = strdup(type_str);
    assert(type_str != (char *) NULL);
    lib[colon - type_str] = '\0';
    struct win_lib *this_lib;
    for(this_lib = win_libs; this_lib->name != (char *) NULL; this_lib++) {
      if (strcasecmp(this_lib->name, lib) == 0) {
        preferred_lib = this_lib->win_lib;
        break;
      }
    }

    type_str = colon + 1;
    free(lib);

    if (preferred_lib == WL_NONE)
      g_warning("Preferred window library not found!");
  }

  i = 0;
  for(this_type = win_types;
      this_type->type_str != (char *) NULL;
      this_type++, i++) {
    if (strcasecmp(this_type->type_str, type_str) == 0) {
      if (preferred_lib == this_type->win_lib)
        return i;
      type = i;
    }
  }
  return type;
}

/** Define a function which can create new windows of all
 * the different types.
 */
GrpWindow *grp_window_open(GrpWindowPlan *plan) {
  int must_have = 0;
  WL win_lib;
  WT win_type;

  if ((must_have & HAVE_TYPE) != 0 && !plan->have.type) {
    g_error("Cannot open the window: window type is missing!");
    return (GrpWindow *) NULL;
  }

  if (num_win_terminals < 1) {
    /* Count the number of terminals if it hasn't been done before:
     * This could be stored with a macro, but it require extra sync.
     */
    struct win_type *tt;
    num_win_terminals = 0;
    for(tt = win_types; tt->type_str != (char *) NULL; tt++)
      ++num_win_terminals;
  }

  if (plan->type < 0 || plan->type >= num_win_terminals) {
    g_error("Cannot open the window: unknown window type!");
    return (GrpWindow *) NULL;
  }

  win_type = win_types[plan->type].type_num;
  must_have = win_types[plan->type].must_have;
  win_lib = win_types[plan->type].win_lib;

  if ((must_have & HAVE_ORIGIN) != 0 && !plan->have.origin) {
    g_error("Cannot open the window: origin is missing!");
    return (GrpWindow *) NULL;
  }

  if ((must_have & HAVE_SIZE) != 0 && !plan->have.size) {
    g_error("Cannot open the window: size is missing!");
    return (GrpWindow *) NULL;
  }

  if ((must_have & HAVE_RESOLUTION) != 0 && !plan->have.resolution) {
    g_error("Cannot open the window: window resolution is missing!");
    return (GrpWindow *) NULL;
  }

  if ((must_have & HAVE_FILE_NAME) != 0 && !plan->have.file_name) {
    g_error("Cannot open the window: file name is missing!");
    return (GrpWindow *) NULL;
  }

  if ((must_have & HAVE_NUM_LAYERS) != 0 && !plan->have.num_layers) {
    g_error("Cannot open the window: number of layers is missing!");
    return (GrpWindow *) NULL;
  }

  if (win_lib != WL_G) {
    assert(win_lib == WL_CAIRO);
#if HAVE_LIBCAIRO
    plan->type = win_type;
    return cairo_open_win(plan);
#else
    g_error("The graphic library has been compiled without Cairo support.");
    return (GrpWindow *) NULL;
#endif
  }

  switch(win_type) {
  case WT_BM1:
    return gr1b_open_win(plan->origin.x, plan->origin.y,
                         plan->origin.x + plan->size.x,
                         plan->origin.y + plan->size.y,
                         plan->resolution.x, plan->resolution.y);
  case WT_BM4:
    return gr4b_open_win(plan->origin.x, plan->origin.y,
                         plan->origin.x + plan->size.x,
                         plan->origin.y + plan->size.y,
                         plan->resolution.x, plan->resolution.y);
  case WT_BM8:
    return gr8b_open_win(plan->origin.x, plan->origin.y,
                         plan->origin.x + plan->size.x,
                         plan->origin.y + plan->size.y,
                         plan->resolution.x, plan->resolution.y);
  case WT_FIG:
    return fig_open_win(1);
  case WT_PS:
    return ps_open_win(plan->file_name);
  case WT_EPS:
    return eps_open_win(plan->file_name, plan->size.x, plan->size.y);
  default:
    g_error("Unknown window type!");
    return (GrpWindow *) NULL;
  }
}
