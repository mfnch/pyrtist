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

#include <box/mem.h>

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

/********************************************************************
 *                   GENERIC LIBRARY FUNCTIONS                      *
 ********************************************************************/

const char *BoxGErr_To_Str(BoxGErr err) {
  switch (err) {
  case BOXGERR_NO_ERR: return NULL;
  case BOXGERR_UNEXPECTED: return "Unexpected error";
  case BOXGERR_NO_MEMORY: return "Cannot allocate memory";
  case BOXGERR_MISS_WIN_TYPE: return "Missing window type";
  case BOXGERR_CAIRO_MISSES_PS:
    return "Cairo was not compiled with support for the PostScript backend";
  case BOXGERR_CAIRO_MISSES_PDF:
    return "Cairo was not compiled with support for the PDF backend";
  case BOXGERR_CAIRO_MISSES_SVG:
    return "Cairo was not compiled with support for the SVG backend";
  case BOXGERR_UNKNOWN_WIN_TYPE: return "Unknown window type";
  case BOXGERR_WIN_SIZE_MISSING:
    return "Cannot create window: window size is missing";
  case BOXGERR_WIN_RES_MISSING:
    return "Cannot create window: resolution is missing";
  case BOXGERR_WIN_FILENAME_MISSING:
    return "Cannot create window: file name is missing!";
  case BOXGERR_CAIRO_SURFACE_ERR: return "Error in Cairo surface";
  case BOXGERR_CAIRO_CONTEXT_ERR: return "Error in Cairo context";
  }
  return "Unknown error";
}

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
 *  oraria (assex -> assey) di 90� per ottenere vy e infine viene restituito
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
    ERRORMSG("grp_ref", "Punti coincidenti: impossibile costruire "
             "il riferimento cartesiano!");
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

void Color_Trunc(Color *c) {
  if (c->r < 0.0) c->r = 0.0;
  if (c->r > 1.0) c->r = 1.0;
  if (c->g < 0.0) c->g = 0.0;
  if (c->g > 1.0) c->g = 1.0;
  if (c->b < 0.0) c->b = 0.0;
  if (c->b > 1.0) c->b = 1.0;
}


/* Questa macro serve a determinare se due colori coincidono o meno
 * (c1 e c2 sono i due puntatori a strutture di tipo color)
 */
#define COLOR_EQUAL(c1, c2) \
 ( ((c1)->r == (c2->r)) && ((c1)->g == (c2->g)) && ((c1)->b == (c2->b)) )

/* DESCRIZIONE: Traduce un colore con componenti RGB date come numeri
 *  compresi tra 0.0 e 1.0, in una struttura di tipo color.
 *  r, g, b sono le componenti iniziali, *c conterra' la struttura finale.
 */
void grp_color_build(Color *c, ColorBytes *cb) {
#define BYTE_COLOR(x) ((int) 255.0*((x) > 1.0 ? 1.0 : ((x) < 0.0 ? 0.0 : (x))))
  cb->r = BYTE_COLOR(c->r);
  cb->g = BYTE_COLOR(c->g);
  cb->b = BYTE_COLOR(c->b);
#undef BYTE_COLOR
}

/* DESCRIZIONE: Arrotonda un colore. Il risultato e' simile all'arrotondamento
 *  di un numero reale in un numero intero: usando questa funzione si riduce
 *  il numero complessivo dei colori utilizzati.
 */
void grp_color_reduce(palette *p, color *c) {
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
palette *grp_palette_build(long numcol, long hashdim,
                           long hashmul, int reduce) {
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
palitem *grp_color_find(palette *p, color *c) {
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
palitem *grp_color_request(palette *p, color *c) {
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
int grp_palette_transform(palette *p, int (*operation)(palitem *pi)) {
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
void grp_palette_destroy(palette *p) {
  int i;
  palitem *pi, *nextpi;

  /* Scorriamo tutta la hash-table in cerca di elementi da cancellare */
  for ( i = 0; i < p->hashdim; i++ ) {

    for ( pi = p->hashtable[i];
     pi != (palitem *) NULL; pi = nextpi ) {

      nextpi = pi->next;
      free(pi);
    }

  }

  /* Elimino la hash-table */
  free(p->hashtable);
  /* Elimino la struttura "palette" */
  free(p);
}

/* DESCRIZIONE: Funzione che associa ad ogni colore, un indice (della tavola
 *  di hash) in modo "abbastanza casuale". Serve per trovare velocemente i colori,
 *  senza scorrere tutta la tavolazza.
 */
static unsigned long color_hash(palette *p, color *c) {
  return ( ( (unsigned long ) c->b ) +
           (c->g + c->r * p->hashmul) * p->hashmul ) % p->hashdim;
}

/* NEW CODE */

static int My_Draw_GPath_Iterator(Int index, GPathPiece *piece, void *data) {
  BoxGWin *w = (BoxGWin *) data;
  Point *p = & (piece->p[0]);
  switch(piece->kind) {
  case GPATHKIND_LINE:
    BoxGWin_Add_Line_Path(w, & p[0], & p[1]);
    return 0;

  case GPATHKIND_ARC:
    BoxGWin_Add_Join_Path(w, & p[0], & p[1], & p[2]);
    return 0;

  default:
    return 1;
  }
}

void BoxGWin_Draw_GPath(BoxGWin *w, GPath *gp) {
  (void) gpath_iter(gp, My_Draw_GPath_Iterator, w);
}

/****************************************************************************
 *  DEFINITION OF A DUMMY WINDOW WHICH DOES NOTHING OR JUST REPORTS ERRORS  *
 ****************************************************************************/

/* Creo una finestra priva di qualsiasi funzione: essa dara' un errore
 * per qualsiasi operazione s tenti di eseguire.
 * La finestra cosi' creata sara' la finestra iniziale, che corrisponde
 * allo stato "nessuna finestra ancora aperta" (in modo da evitare
 * accidentali "segmentation fault").
 */

static void My_Dummy_Report_Err(BoxGWin *w, const char *method) {
  if (! w->quiet) {
    fprintf(stderr, "%s.%s: method is not implemented.\n",
            w->win_type_str, method);
  }
}

static void My_Dummy_Create_Path(BoxGWin *w) {
  w->_report_error(w, "rreset");
}

static void My_Dummy_Begin_Drawing(BoxGWin *w) {
  w->_report_error(w, "rinit");
}

static void My_Dummy_Draw_Path(BoxGWin *w, DrawStyle *style) {
  w->_report_error(w, "rdraw");
}

static void My_Dummy_Add_Line_Path(BoxGWin *w, Point *a, Point *b) {
  w->_report_error(w, "rline");
}

static void My_Dummy_Add_Join_Path(BoxGWin *w, Point *a, Point *b, Point *c) {
  w->_report_error(w, "rcong");
}

static void My_Dummy_Close_Path(BoxGWin *w) {
  w->_report_error(w, "rclose");
}

static void My_Dummy_Add_Circle_Path(BoxGWin *w,
                                     Point *ctr, Point *a, Point *b) {
  w->_report_error(w, "rcircle");
}

static void My_Dummy_Set_Fg_Color(BoxGWin *w, Color *c) {
  w->_report_error(w, "rfgcolor");
}

static void My_Dummy_Set_Bg_Color(BoxGWin *w, Color *c) {
  w->_report_error(w, "rbgcolor");
}

static void My_Dummy_Set_Gradient(BoxGWin *w, ColorGrad *cg) {
  w->_report_error(w, "rgradient");
}

static void My_Dummy_Add_Text_Path(BoxGWin *w, BoxPoint *ctr, BoxPoint *right,
                                   BoxPoint *up, BoxPoint *from,
                                   const char *text) {
  w->_report_error(w, "text");
}

static void My_Dummy_Set_Font(BoxGWin *w, const char *font) {
  w->_report_error(w, "font");
}

static void My_Dummy_Add_Fake_Point(BoxGWin *w, Point *p) {}

static int My_Dummy_Save_To_File(BoxGWin *w, const char *file_name) {
  /* If this function is not provided by the specific terminal, then
   * the window is probably a stream window. The best thing to do is then
   * to silently ignore the command.
   */
  return 1;
}

static BoxTask My_NotImplem_Interpret(BoxGWin *w, BoxGObj *obj) {
  BoxGWin_Fail("BoxGWin_Interpret_Obj",
               "not implemented for this window type.");
  return BOXTASK_FAILURE;
}

void My_Dummy_Finish_Drawing(BoxGWin *w) {
  w->_report_error(w, "close_win");
}

void My_Dummy_Set_Color(BoxGWin *w, int col) {
  w->_report_error(w, "set_col");
}

void My_Dummy_Draw_Point(BoxGWin *w, Int ptx, Int pty) {
  w->_report_error(w, "draw_point");
}

void My_Dummy_Draw_Hor_Line(BoxGWin *w, Int y, Int x1, Int x2) {
  w->_report_error(w, "hor_line");
}

void BoxGWin_Block(BoxGWin *w) {
  w->create_path = My_Dummy_Create_Path;
  w->begin_drawing = My_Dummy_Begin_Drawing;
  w->draw_path = My_Dummy_Draw_Path;
  w->add_line_path = My_Dummy_Add_Line_Path;
  w->add_join_path = My_Dummy_Add_Join_Path;
  w->close_path = My_Dummy_Close_Path;
  w->add_circle_path = My_Dummy_Add_Circle_Path;
  w->set_fg_color = My_Dummy_Set_Fg_Color;
  w->set_bg_color = My_Dummy_Set_Bg_Color;
  w->set_gradient = My_Dummy_Set_Gradient;
  w->add_text_path = My_Dummy_Add_Text_Path;
  w->set_font = My_Dummy_Set_Font;
  w->add_fake_point = My_Dummy_Add_Fake_Point;
  w->save_to_file = My_Dummy_Save_To_File;
  w->interpret = My_NotImplem_Interpret;

  w->finish_drawing = My_Dummy_Finish_Drawing;
  w->set_color = My_Dummy_Set_Color;
  w->draw_point = My_Dummy_Draw_Point;
  w->draw_hor_line = My_Dummy_Draw_Hor_Line;

  w->_report_error = My_Dummy_Report_Err;
}

void BoxGWin_Block_With(BoxGWin *w, BoxGOnError on_error) {
  BoxGWin_Block(w);
  w->_report_error = (on_error != NULL) ? on_error : My_Dummy_Report_Err;
}

/*** Error window ***********************************************************/
static char *err_id_string = "err";

typedef struct {
  FILE *out;
  char *msg;

} GrpWindowErrData;

static void My_Window_Error_Close(BoxGWin *w) {
  assert(w->win_type_str == err_id_string);
  BoxMem_Free(w->ptr);
  BoxMem_Free(w);
}

static void My_Window_Error_Report(BoxGWin *w, const char *location) {
  assert(w->win_type_str == err_id_string);
  GrpWindowErrData *d = (GrpWindowErrData *) w->ptr;
  if (! w->quiet) {
    fprintf(d->out, "%s\n", d->msg);
    w->quiet = 1;
  }
}

static int My_Window_Error_Save(BoxGWin *w, const char *file_name) {
  w->_report_error(w, "save");
  return 0;
}

BoxGWin *BoxGWin_Create_Invalid(BoxGErr *err) {
  BoxGWin *w = BoxMem_Alloc(sizeof(BoxGWin));
  if (w != NULL) {
    BoxGWin_Block(w);
    w->win_type_str = err_id_string;
    w->pattern = NULL;
    w->quiet = 0;
    w->ptr = NULL;
    if (err != NULL)
      *err = BOXGERR_NO_ERR;
    return w;

  } else {
    if (err != NULL)
      *err = BOXGERR_NO_MEMORY;
    return NULL;
  }
}

void BoxGWin_Destroy(BoxGWin *w) {
  w->win_type_str = NULL;
  assert(w->pattern == NULL);
  BoxMem_Free(w);
}

/** Create a Window which displays the given error message, when someone
 * tries to use it. The error message is fprinted to the given stream.
 */
BoxGWin *Grp_Window_Error(FILE *out, const char *msg) {
  BoxGWin *w = BoxMem_Alloc(sizeof(BoxGWin));
  GrpWindowErrData *d = BoxMem_Alloc(sizeof(GrpWindowErrData));
  BoxGWin_Block(w);
  w->win_type_str = err_id_string;
  w->save_to_file = My_Window_Error_Save;
  w->finish_drawing = My_Window_Error_Close;
  w->_report_error = My_Window_Error_Report;
  w->quiet = 0;

  d->msg = BoxMem_Strdup(msg);
  d->out = out;
  w->ptr = d;
  return w;
}

int Grp_Window_Is_Error(BoxGWin *w) {
 return w->win_type_str == err_id_string;
}

/****************************************************************************/

BoxGWin grp_dummy_win = {
  "blocked",
  My_Dummy_Create_Path,
  My_Dummy_Begin_Drawing,
  My_Dummy_Draw_Path,
  My_Dummy_Add_Line_Path,
  My_Dummy_Add_Join_Path,
  My_Dummy_Close_Path,
  My_Dummy_Add_Circle_Path,
  My_Dummy_Set_Fg_Color,
  My_Dummy_Set_Bg_Color,
  My_Dummy_Set_Gradient,
  My_Dummy_Set_Font,
  My_Dummy_Add_Text_Path,
  My_Dummy_Add_Fake_Point,
  My_Dummy_Save_To_File,
  My_NotImplem_Interpret,
  0, /* quiet */
  My_Dummy_Finish_Drawing,
  My_Dummy_Set_Color,
  My_Dummy_Draw_Point,
  My_Dummy_Draw_Hor_Line,
  BoxGWin_Block /* repair */
};

void Grp_Window_Make_Dummy(BoxGWin *w) {
  *w = grp_dummy_win;
}

/****************************************************************************
 * Generic functions to open a Window using a BoxGWinPlan.                  *
 ****************************************************************************/

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

int BoxGWin_Type_From_String(const char *type_str) {
  char *colon = NULL;
  struct win_type *this_type;
  WL preferred_lib = WL_NONE;
  int type = -1, i;

  colon = strchr(type_str, ':');
  if (colon != NULL) {
    char *lib = BoxMem_Strdup(type_str);
    assert(type_str != NULL);
    lib[colon - type_str] = '\0';
    struct win_lib *this_lib;
    for (this_lib = win_libs; this_lib->name != NULL; this_lib++) {
      if (strcasecmp(this_lib->name, lib) == 0) {
        preferred_lib = this_lib->win_lib;
        break;
      }
    }

    type_str = colon + 1;
    BoxMem_Free(lib);

    if (preferred_lib == WL_NONE)
      g_warning("Preferred window library not found!");
  }

  for (this_type = win_types, i = 0;
       this_type->type_str != NULL;
       this_type++, i++) {
    if (strcasecmp(this_type->type_str, type_str) == 0) {
      if (preferred_lib == this_type->win_lib)
        return i;
      type = i;
    }
  }
  return type;
}


#define OPEN_FAILED(out, msg) \
  Grp_Window_Error((out), "Trying to use an uninitialized window. " \
  "The initialization failed for the following reason: "msg".")


/** Define a function which can create new windows of all
 * the different types.
 */
BoxGWin *BoxGWin_Create(BoxGWinPlan *plan) {
  int must_have = 0;
  WL win_lib;
  WT win_type;

  if ((must_have & HAVE_TYPE) != 0 && !plan->have.type)
    return OPEN_FAILED(stderr, "window type is missing");

  if (num_win_terminals < 1) {
    /* Count the number of terminals if it hasn't been done before:
     * This could be stored with a macro, but it require extra sync.
     */
    struct win_type *tt;
    num_win_terminals = 0;
    for(tt = win_types; tt->type_str != (char *) NULL; tt++)
      ++num_win_terminals;
  }

  if (plan->type < 0 || plan->type >= num_win_terminals)
    return OPEN_FAILED(stderr, "unknown window type");

  win_type = win_types[plan->type].type_num;
  must_have = win_types[plan->type].must_have;
  win_lib = win_types[plan->type].win_lib;

  if ((must_have & HAVE_ORIGIN) != 0 && !plan->have.origin)
    return OPEN_FAILED(stderr, "origin is missing");

  if ((must_have & HAVE_SIZE) != 0 && !plan->have.size)
    return OPEN_FAILED(stderr, "size is missing");

  if ((must_have & HAVE_RESOLUTION) != 0 && !plan->have.resolution)
    return OPEN_FAILED(stderr, "window resolution is missing");

  if ((must_have & HAVE_FILE_NAME) != 0 && !plan->have.file_name)
    return OPEN_FAILED(stderr, "file name is missing");

  if ((must_have & HAVE_NUM_LAYERS) != 0 && !plan->have.num_layers)
    return OPEN_FAILED(stderr, "number of layers is missing");

  if (win_lib != WL_G) {
#if HAVE_LIBCAIRO
    BoxGErr err;
    BoxGWin *w;
    assert(win_lib == WL_CAIRO);
    plan->type = win_type;
    w = BoxGWin_Create_Cairo(plan, & err);
    if (err == BOXGERR_NO_ERR)
      return w;
    return Grp_Window_Error(stderr, BoxGErr_To_Str(err));
#else
    return OPEN_FAILED(stderr, "window type not available, because "
                       "the graphic library was compiled without "
                       "Cairo support");
#endif
  }

  switch(win_type) {
  case WT_BM1:
    return BoxGWin_Create_BM1(plan->origin.x, plan->origin.y,
                              plan->origin.x + plan->size.x,
                              plan->origin.y + plan->size.y,
                              plan->resolution.x, plan->resolution.y);
  case WT_BM4:
    return BoxGWin_Create_BM4(plan->origin.x, plan->origin.y,
                              plan->origin.x + plan->size.x,
                              plan->origin.y + plan->size.y,
                              plan->resolution.x, plan->resolution.y);
  case WT_BM8:
    return BoxGWin_Create_BM8(plan->origin.x, plan->origin.y,
                              plan->origin.x + plan->size.x,
                              plan->origin.y + plan->size.y,
                              plan->resolution.x, plan->resolution.y);
  case WT_FIG:
    return BoxGWin_Create_Fig(1);
  case WT_PS:
    return BoxGWin_Create_PS(plan->file_name);
  case WT_EPS:
    return BoxGWin_Create_EPS(plan->file_name, plan->size.x, plan->size.y);
  default:
    return OPEN_FAILED(stderr, "unknown window type");
  }
}

/****************************************************************************
 *                        BOUNDING BOX (BB) OBJECT                          *
 ****************************************************************************/

void Grp_BB_Init(BB *bb) {
  bb->num = 0;
  bb->min.x = bb->min.y = bb->max.x = bb->max.y = 0.0;
}

void Grp_BB_Must_Contain(BB *bb, Point *p) {
  if (bb->num < 1) {
    assert(bb->num == 0);
    bb->min.x = bb->max.x = p->x;
    bb->min.y = bb->max.y = p->y;

  } else {
    if (p->x < bb->min.x) bb->min.x = p->x;
    if (p->y < bb->min.y) bb->min.y = p->y;
    if (p->x > bb->max.x) bb->max.x = p->x;
    if (p->y > bb->max.y) bb->max.y = p->y;
  }
  ++bb->num;
}

void Grp_BB_Fuse(BB *dst, BB *src) {
  if (src->num != 0) {
    assert(src->num > 0);
    Grp_BB_Must_Contain(dst, & src->min);
    Grp_BB_Must_Contain(dst, & src->max);
    dst->num += src->num - 2;
  }
}

Real Grp_BB_Volume(BB *bb) {
  return (bb->max.x - bb->min.x)*(bb->max.y - bb->min.y);
}

void Grp_BB_Margins(BB *bb, Point *margin_min, Point *margin_max) {
  bb->min.x -= margin_min->x;
  bb->min.y -= margin_min->y;
  bb->max.x += margin_max->x;
  bb->max.y += margin_max->y;
}

void Grp_BB_Margin(BB *bb, Real margin) {
  bb->min.x -= margin;
  bb->min.y -= margin;
  bb->max.x += margin;
  bb->max.y += margin;
}

/****************************************************************************
 *                              Matrix OBJECT                               *
 ****************************************************************************/

void BoxGMatrix_Set(Matrix *m,
                    Point *t, Point *rcntr, Real rang, Real sx, Real sy) {
  Real c, s, m11, m12, m21, m22;

  c = cos(rang); s = sin(rang);
  m11 = sx * c; m12 = -(sy * s);
  m21 = sx * s; m22 = sy * c;

  m->m11 = m11; m->m12 = m12;
  m->m21 = m21; m->m22 = m22;
  m->m13 = (1.0 - m11) * rcntr->x - m12 * rcntr->y + t->x;
  m->m23 = (1.0 - m22) * rcntr->y - m21 * rcntr->x + t->y;
}

void BoxGMatrix_Set_Identity(Matrix *m) {
  m->m11 = m->m22 = 1.0;
  m->m12 = m->m21 = m->m13 = m->m23 = 0.0;
}

void Grp_Matrix_Mul_Point(Matrix *m, Point *pts, int num_pts) {
  Real m11 = m->m11, m12 = m->m12,
       m21 = m->m21, m22 = m->m22,
       m13 = m->m13, m23 = m->m23;
  Real px;

  for (; num_pts-- > 0; pts++) {
    px = pts->x;
    pts->x = m11 * px + m12 * pts->y + m13;
    pts->y = m21 * px + m22 * pts->y + m23;
  }
}

void Grp_Matrix_Mul_Vector(Matrix *m, Point *vecs, int num_vecs) {
  Real m11 = m->m11, m12 = m->m12,
       m21 = m->m21, m22 = m->m22;
  Real vx;

  for (; num_vecs-- > 0; vecs++) {
    vx = vecs->x;
    vecs->x = m11 * vx + m12 * vecs->y;
    vecs->y = m21 * vx + m22 * vecs->y;
  }
}

/*void Grp_Matrix_Mul_Matrix(Matrix *result, Matrix *left, Matrix *right) {

}*/
