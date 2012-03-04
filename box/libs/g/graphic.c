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
#include "winmap.h"
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
  case BOXGERR_CAIRO_PATTERN_ERR: return "Error while creating the pattern";
  case BOXGERR_CMD_BAD_ARGS:
    return "Error parsing command arguments (wrong type)";
  case BOXGERR_CMD_MISSING_ARGS: return "Not enough arguments for command";
  case BOXGERR_CMD_UNEXPECTED_ARGS:
    return "Unexpected extra arguments to non-variadic command";
  case BOXGERR_CMD_EXEC: return "Error while executing the command";
  case BOXGERR_CMD_BAD: return "Expecting composite command object";
  case BOXGERR_CMD_EMPTY: return "Empty command object";
  case BOXGERR_CMD_BAD_INDEX:
    return "Cannot find command index (first item should be Int)";
  }
  return "Unknown error";
}

const char *BoxGWinMethod_To_String(BoxGWinMethod m) {
  switch(m) {
  case BOXGWIN_METHOD_CREATE_PATH: return "CREATE_PATH";
  case BOXGWIN_METHOD_BEGIN_DRAWING: return "BEGIN_DRAWING";
  case BOXGWIN_METHOD_DRAW_PATH: return "DRAW_PATH";
  case BOXGWIN_METHOD_LINE_PATH: return "LINE_PATH";
  case BOXGWIN_METHOD_ADD_LINE_PATH: return "ADD_LINE_PATH";
  case BOXGWIN_METHOD_ADD_JOIN_PATH: return "ADD_JOIN_PATH";
  case BOXGWIN_METHOD_ADD_CIRCLE_PATH: return "ADD_CIRCLE_PATH";
  case BOXGWIN_METHOD_ADD_TEXT_PATH: return "ADD_TEXT_PATH";
  case BOXGWIN_METHOD_ADD_FAKE_POINT: return "ADD_FAKE_POINT";
  case BOXGWIN_METHOD_CLOSE_PATH: return "CLOSE_PATH";
  case BOXGWIN_METHOD_SET_FG_COLOR: return "SET_FG_COLOR";
  case BOXGWIN_METHOD_SET_BG_COLOR: return "SET_BG_COLOR";
  case BOXGWIN_METHOD_SET_GRADIENT: return "SET_GRADIENT";
  case BOXGWIN_METHOD_SET_FONT: return "SET_FONT";
  case BOXGWIN_METHOD_SAVE_TO_FILE: return "SAVE_TO_FILE";
  case BOXGWIN_METHOD_INTERPRET: return "INTERPRET";
  case BOXGWIN_METHOD_FINISH: return "FINISH";
  case BOXGWIN_METHOD_SET_COLOR: return "SET_COLOR";
  case BOXGWIN_METHOD_DRAW_POINT: return "DRAW_POINT";
  case BOXGWIN_METHOD_DRAW_HOR_LINE: return "DRAW_HOR_LINE";
  case BOXGWIN_METHOD_UNBLOCK: return "UNBLOCK";
  case BOXGWIN_METHOD_NOTIFY_NOT_IMPLEMENTED: return "NOTIFY_NOT_IMPLEMENTED";
  }

  return "Unknown method";
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

/* Below we define a window that just calls a report error function whenever
 * one of its methods is used. This is the base for other windows types,
 * which should override the methods defined below.
 */

/* The default notification ignores calls to Finish (for obvious reasons)
 * and to BoxGWin_Save_To_File. The latter is convenient as many stream
 * or memory mapped windows do not allow to save the windows this way
 * (typically they take a file name when they are constructed).
 * One can always intercept the notification and act differently, if needed.
 */
static BoxGWin *My_Dummy_Notify_Not_Implemented(BoxGWin *w, BoxGWinMethod m) {
  if (!w->quiet && m != BOXGWIN_METHOD_FINISH
      && m != BOXGWIN_METHOD_SAVE_TO_FILE)
    fprintf(stderr, "%s: method '%s' is not implemented.\n",
            w->win_type_str, BoxGWinMethod_To_String(m));
  return NULL;
}

/* This macros is used to define the bodies of the dummy window methods. */
#define MY_DUMMY_METHOD(w, method, method_name, ...) \
  do { \
    if (w->notify_not_implemented) { \
      BoxGWin *fallback = w->notify_not_implemented((w), (method)); \
      if (fallback) \
        BoxGWin_ ## method_name(__VA_ARGS__); \
    } \
  } while(0)

/* Similar to MY_DUMMY_METHOD, but used in functions that return something.
 * ret is the return value, when the function fails.
 */
#define MY_DUMMY_METHOD_RET(w, method, method_name, ret, ...)    \
  do { \
    if (w->notify_not_implemented) {                                  \
      BoxGWin *fallback = w->notify_not_implemented((w), (method)); \
      if (fallback) \
        return BoxGWin_ ## method_name(__VA_ARGS__); \
    } \
    return (ret); \
  } while(0)

static void My_Dummy_Create_Path(BoxGWin *w) {
  MY_DUMMY_METHOD(w, BOXGWIN_METHOD_CREATE_PATH, Create_Path, w);
}

static void My_Dummy_Begin_Drawing(BoxGWin *w) {
  MY_DUMMY_METHOD(w, BOXGWIN_METHOD_BEGIN_DRAWING, Begin_Drawing, w);
}

static void My_Dummy_Draw_Path(BoxGWin *w, DrawStyle *style) {
  MY_DUMMY_METHOD(w, BOXGWIN_METHOD_DRAW_PATH, Draw_Path, w, style);
}

static void My_Dummy_Add_Line_Path(BoxGWin *w, Point *a, Point *b) {
  MY_DUMMY_METHOD(w, BOXGWIN_METHOD_ADD_LINE_PATH, Add_Line_Path, w, a, b);
}

static void My_Dummy_Add_Join_Path(BoxGWin *w, Point *a, Point *b, Point *c) {
  MY_DUMMY_METHOD(w, BOXGWIN_METHOD_ADD_JOIN_PATH, Add_Join_Path, w, a, b, c);
}

static void My_Dummy_Close_Path(BoxGWin *w) {
  MY_DUMMY_METHOD(w, BOXGWIN_METHOD_CLOSE_PATH, Close_Path, w);
}

static void My_Dummy_Add_Circle_Path(BoxGWin *w,
                                     Point *ctr, Point *a, Point *b) {
  MY_DUMMY_METHOD(w, BOXGWIN_METHOD_ADD_CIRCLE_PATH,
                  Add_Circle_Path, w, ctr, a, b);
}

static void My_Dummy_Set_Fg_Color(BoxGWin *w, Color *c) {
  MY_DUMMY_METHOD(w, BOXGWIN_METHOD_SET_FG_COLOR, Set_Fg_Color, w, c);
}

static void My_Dummy_Set_Bg_Color(BoxGWin *w, Color *c) {
  MY_DUMMY_METHOD(w, BOXGWIN_METHOD_SET_BG_COLOR, Set_Bg_Color, w, c);
}

static void My_Dummy_Set_Gradient(BoxGWin *w, ColorGrad *cg) {
  MY_DUMMY_METHOD(w, BOXGWIN_METHOD_SET_GRADIENT, Set_Gradient, w, cg);
}

static void My_Dummy_Add_Text_Path(BoxGWin *w, BoxPoint *ctr, BoxPoint *right,
                                   BoxPoint *up, BoxPoint *from,
                                   const char *text) {
  MY_DUMMY_METHOD(w, BOXGWIN_METHOD_ADD_TEXT_PATH, Add_Text_Path, w, ctr,
                  right, up, from, text);
}

static void My_Dummy_Set_Font(BoxGWin *w, const char *font) {
  MY_DUMMY_METHOD(w, BOXGWIN_METHOD_SET_FONT, Set_Font, w, font);
}

static void My_Dummy_Add_Fake_Point(BoxGWin *w, Point *p) {
  //MY_DUMMY_METHOD(w, BOXGWIN_METHOD_ADD_FAKE_POINT, Add_Fake_Point, w, p);
}

static int My_Dummy_Save_To_File(BoxGWin *w, const char *file_name) {
  MY_DUMMY_METHOD_RET(w, BOXGWIN_METHOD_SAVE_TO_FILE, Save_To_File,
                      1, w, file_name);
}

static BoxTask My_Dummy_Interpret(BoxGWin *w, BoxGObj *obj, BoxGWinMap *map) {
  MY_DUMMY_METHOD_RET(w, BOXGWIN_METHOD_INTERPRET, Interpret_Obj,
                      BOXTASK_FAILURE, w, obj, map);
}

void My_Dummy_Finish(BoxGWin *w) {
  MY_DUMMY_METHOD(w, BOXGWIN_METHOD_FINISH, Finish, w);
}

void My_Dummy_Set_Color(BoxGWin *w, int col) {
  MY_DUMMY_METHOD(w, BOXGWIN_METHOD_SET_COLOR, Set_Color, w, col);
}

void My_Dummy_Draw_Point(BoxGWin *w, Int ptx, Int pty) {
  MY_DUMMY_METHOD(w, BOXGWIN_METHOD_DRAW_POINT, Draw_Point, w, ptx, pty);
}

void My_Dummy_Draw_Hor_Line(BoxGWin *w, Int y, Int x1, Int x2) {
  MY_DUMMY_METHOD(w, BOXGWIN_METHOD_DRAW_HOR_LINE, Draw_Hor_Line, w, y, x1, x2);
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
  w->interpret = My_Dummy_Interpret;

  w->finish = My_Dummy_Finish;
  w->set_color = My_Dummy_Set_Color;
  w->draw_point = My_Dummy_Draw_Point;
  w->draw_hor_line = My_Dummy_Draw_Hor_Line;

  w->notify_not_implemented = My_Dummy_Notify_Not_Implemented;
}

void BoxGWin_Block_With(BoxGWin *w, BoxGOnError on_error) {
  BoxGWin_Block(w);
  w->notify_not_implemented =
    (on_error != NULL) ? on_error : My_Dummy_Notify_Not_Implemented;
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
}

static BoxGWin *My_Window_Error_Report(BoxGWin *w, BoxGWinMethod m) {
  assert(w->win_type_str == err_id_string);
  GrpWindowErrData *d = (GrpWindowErrData *) w->ptr;
  if (!w->quiet) {
    fprintf(d->out, "%s\n", d->msg);
    w->quiet = 1;
  }
  return NULL;
}

static int My_Window_Error_Save(BoxGWin *w, const char *file_name) {
  w->notify_not_implemented(w, BOXGWIN_METHOD_SAVE_TO_FILE);
  return 0;
}

void BoxGWin_Finish(BoxGWin *w) {
  w->finish(w);
  w->win_type_str = NULL;
}

void BoxGWin_Destroy(BoxGWin *w) {
  BoxGWin_Finish(w);
  BoxMem_Free(w);
}

BoxGWin *BoxGWin_Create_Invalid(BoxGErr *err) {
  BoxGWin *w = BoxMem_Alloc(sizeof(BoxGWin));
  if (w != NULL) {
    BoxGWin_Block(w);
    w->win_type_str = err_id_string;
    w->quiet = 0;
    w->ptr = NULL;
    w->data = NULL;
    if (err != NULL)
      *err = BOXGERR_NO_ERR;
    return w;

  } else {
    if (err != NULL)
      *err = BOXGERR_NO_MEMORY;
    return NULL;
  }
}

/** Create a Window which displays the given error message, when someone
 * tries to use it. The error message is fprinted to the given stream.
 */
BoxGWin *BoxGWin_Create_Faulty(FILE *out, const char *msg) {
  BoxGWin *w = BoxMem_Safe_Alloc(sizeof(BoxGWin));
  GrpWindowErrData *d = BoxMem_Safe_Alloc(sizeof(GrpWindowErrData));
  BoxGWin_Block(w);
  w->win_type_str = err_id_string;
  w->save_to_file = My_Window_Error_Save;
  w->finish = My_Window_Error_Close;
  w->notify_not_implemented = My_Window_Error_Report;
  w->quiet = 0;

  d->msg = BoxMem_Strdup(msg);
  d->out = out;
  w->ptr = d;
  return w;
}

int BoxGWin_Is_Faulty(BoxGWin *w) {
 return w->win_type_str == err_id_string;
}

/* When the first method is used, convert the window to a recording window. */
static BoxGWin *My_Default_Notify_Not_Implemented(BoxGWin *w,
                                                  BoxGWinMethod m) {
  if (m != BOXGWIN_METHOD_FINISH) {
    BoxGWin_Finish(w);
    BoxTask t = BoxGWin_Init_Fig(w, 1);

    if (t == BOXTASK_OK)
      return w; /* so that the method gets really executed with the new window */
  }

  return NULL;
}

void My_Repair_Default(BoxGWin *w) {
  BoxGWin_Block(w);
  w->notify_not_implemented = My_Default_Notify_Not_Implemented;
  w->repair = My_Repair_Default;
}

BoxGWin *BoxGWin_Create_Default(void) {
  BoxGErr err;
  BoxGWin *w = BoxGWin_Create_Invalid(& err);
  if (err != BOXGERR_NO_ERR)
    return NULL;

  else {
    My_Repair_Default(w);
    return w;
  }
}

/****************************************************************************
 * Generic functions to open a Window using a BoxGWinPlan.                  *
 ****************************************************************************/

enum {
  HAVE_TYPE       = 0x01,
  HAVE_ORIGIN     = 0x02, 
  HAVE_SIZE       = 0x04, 
  HAVE_CORNERS    = 0x06,
  HAVE_RESOLUTION = 0x08,
  HAVE_FILE_NAME  = 0x10,
  HAVE_NUM_LAYERS = 0x20
};

typedef enum {
  WL_NONE  = -1,
  WL_G     =  0,
  WL_CAIRO =  1

} WL;

struct win_lib {
  char *name;
  WL win_lib;

} win_libs[] = {
  {          "g", WL_G},
  {      "cairo", WL_CAIRO},
  {(char *) NULL, WL_NONE}
};

#define MY_NUM_WIN_TERMINALS 14

struct win_type {
  char *type_str;
  WT type_num;
  WL win_lib;
  int must_have;

} win_types[MY_NUM_WIN_TERMINALS + 1] = {
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


#define MY_OPEN_FAILED(msg) \
  BoxGWin_Create_Faulty(stderr, "Trying to use an uninitialized window. " \
    "The initialization failed for the following reason: "msg".")


/** Define a function which can create new windows of all
 * the different types.
 */
BoxGWin *BoxGWin_Create(BoxGWinPlan *plan) {
  int must_have = 0;
  WL win_lib;
  WT win_type;

  if ((must_have & HAVE_TYPE) != 0 && !plan->have.type)
    return MY_OPEN_FAILED("window type is missing");

  if (plan->type < 0 || plan->type >= MY_NUM_WIN_TERMINALS)
    return MY_OPEN_FAILED("unknown window type");

  win_type = win_types[plan->type].type_num;
  must_have = win_types[plan->type].must_have;
  win_lib = win_types[plan->type].win_lib;

  if ((must_have & HAVE_ORIGIN) != 0 && !plan->have.origin)
    return MY_OPEN_FAILED("origin is missing");

  if ((must_have & HAVE_SIZE) != 0 && !plan->have.size)
    return MY_OPEN_FAILED("size is missing");

  if ((must_have & HAVE_RESOLUTION) != 0 && !plan->have.resolution)
    return MY_OPEN_FAILED("window resolution is missing");

  if ((must_have & HAVE_FILE_NAME) != 0 && !plan->have.file_name)
    return MY_OPEN_FAILED("file name is missing");

  if ((must_have & HAVE_NUM_LAYERS) != 0 && !plan->have.num_layers)
    return MY_OPEN_FAILED("number of layers is missing");

  if (win_lib != WL_G) {
#if HAVE_LIBCAIRO
    BoxGErr err;
    BoxGWin *w;
    assert(win_lib == WL_CAIRO);
    plan->type = win_type;
    w = BoxGWin_Create_Cairo(plan, & err);
    if (err == BOXGERR_NO_ERR)
      return w;
    return BoxGWin_Create_Faulty(stderr, BoxGErr_To_Str(err));
#else
    return MY_OPEN_FAILED("window type not available, because "
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
    return MY_OPEN_FAILED("unknown window type");
  }
}
