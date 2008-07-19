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

/* fig.c
 *
 * Questo file contiene quanto basta per poter disegnare su una "finestra
 * virtuale".
 * I comandi vengono semplicemente salvati in diverse "liste di comandi",
 * che chiamo "layer". I layer servono a ordinare gli oggetti grafici
 * in modo da realizzare la giusta sovrapposizione.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

/* De-commentare per includere i messaggi di debug */
/*#define DEBUG*/

#include "debug.h"
#include "error.h"
#include "buffer.h"
#include "graphic.h"  /* Dichiaro alcune strutture grafiche generali */
#include "g.h"
#include "fig.h"
#include "bb.h"
#include "autoput.h"

/*#define grp_activelayer (((struct fig_header *) grp_win->wrdep)->current)*/

/* Dimensione iniziale di un layer in bytes = dimensione iniziale dello spazio
 * in cui vengono memorizzate le istruzioni di ciascun layer
 */
#define LAYER_INITIAL_SIZE 128

/* DESCRIZIONE: Questa macro permette di usare una indicizzazione "circolare",
 *  secondo cui, data una lista di num_items elementi, l'indice 1 si riferisce
 *  al primo elemento, 2 al secondo, ..., num_items all'ultimo, num_items+1 al primo,
 *  num_items+2 al secondo, ...
 *  Inoltre l'indice 0 si riferisce all'ultimo elemento, -1 al pen'ultimo, ...
 */
#ifdef CIRCULAR_INDEX
#  undef CIRCULAR_INDEX
#endif

#  define CIRCULAR_INDEX(num_items, index) \
    ( (index > 0) ? ( (index - 1) % num_items ) + 1 : \
                    num_items - ( (-index) % num_items ) )

typedef struct {
  int numlayers;  /* Numero dei layer della figura */
  int current;    /* Layer attualmente in uso */
  int top;        /* Primo layer della lista */
  int bottom;     /* Ultimo layer della lista */
  int firstfree;  /* Primo posto libero nella lista dei layer (0 = nessuno) */
  buff layerlist; /* Buffer che gestisce la lista dei layer */
} FigHeader;

typedef struct {
  unsigned long ID;
  int numcmnd;
  int previous;
  int next;
  buff layer;
} LayerHeader;

static char *fig_id_string = "fig";

/****************************************************************************/
/* PROCEDURE CORRISPONDENTI AI COMANDI DA "CATTURARE" */

/* Nei layers le istruzioni vengono memorizzate argomento per argomento.
 * Ogni istruzione e' costituita da un header iniziale, fatto cosi':
 *  Nome Tipo  Descrizione
 *------|-----|-------------------------------------
 *  ID   long  Numero che identifica l'istruzione
 *  dim  long  Dimesione in byte dell'istruzione (header escluso)
 * Questo header e' poi seguito da una struttura contenente i valori
 * degli argomenti del comando specificato.
 */

/* Enumero gli ID di tutti i comandi */
enum ID_type {
  ID_rreset = 1, ID_rinit, ID_rdraw, ID_rline,
  ID_rcong, ID_rclose, ID_rcircle, ID_rfgcolor, ID_rbgcolor,
  ID_rgradient, ID_text, ID_font, ID_fake_point
};

struct cmnd_header {
  long  ID;
  long  size;
};

typedef struct cmnd_header CmndHeader;

typedef struct {
  int arg_data_size;
  void *arg_data;
} CmndArg;

/* This function is used to insert a command in the current layer */
static void _fig_insert_command(int id, CmndArg *args) {
  LayerHeader *lh;
  buff *layer;
  int total_size, i;
  CmndHeader ch;

  /* Calculate total size of arguments */
  total_size = 0;
  for(i=0; args[i].arg_data_size > 0; i++)
    total_size += args[i].arg_data_size;

  /* Compile command header */
  ch.ID = id;
  ch.size = total_size;

  lh = (LayerHeader *) grp_win->ptr;
  assert(lh->ID == 0x7279616c);  /* 0x7279616c = "layr" */
  layer = & lh->layer;

  assert(buff_mpush(layer, & ch, sizeof(CmndHeader)) == 1);
  for (i=0; args[i].arg_data_size > 0; i++)
    assert(buff_mpush(layer, args[i].arg_data, args[i].arg_data_size) == 1);

  ++lh->numcmnd; /* Increase counter for number of commands in the layer */
}

void fig_rreset(void) {
  CmndArg args[] = {{0, (void *) NULL}};
  _fig_insert_command(ID_rreset, args);
}

void fig_rinit(void) {
  CmndArg args[] = {{0, (void *) NULL}};
  _fig_insert_command(ID_rinit, args);
}

void fig_rdraw(DrawStyle *style) {
  CmndArg args[] = {{sizeof(DrawStyle), style},
                    {0, (void *) NULL},
                    {0, (void *) NULL}};
  if (style->bord_num_dashes > 0) {
    args[1].arg_data_size = sizeof(Real)*style->bord_num_dashes;
    args[1].arg_data = style->bord_dashes;
  }
  _fig_insert_command(ID_rdraw, args);
}

void fig_rline(Point *a, Point *b) {
  CmndArg args[] = {{sizeof(Point), a},
                    {sizeof(Point), b},
                    {0, (void *) NULL}};
  _fig_insert_command(ID_rline, args);
}

void fig_rcong(Point *a, Point *b, Point *c) {
  CmndArg args[] = {{sizeof(Point), a},
                    {sizeof(Point), b},
                    {sizeof(Point), c},
                    {0, (void *) NULL}};
  _fig_insert_command(ID_rcong, args);
}

void fig_rclose(void) {
  CmndArg args[] = {{0, (void *) NULL}};
  _fig_insert_command(ID_rclose, args);
}

void fig_rcircle(Point *ctr, Point *a, Point *b) {
  CmndArg args[] = {{sizeof(Point), ctr},
                    {sizeof(Point), a},
                    {sizeof(Point), b},
                    {0, (void *) NULL}};
  _fig_insert_command(ID_rcircle, args);
}

void fig_rfgcolor(Color *c) {
  CmndArg args[] = {{sizeof(Color), c}, {0, (void *) NULL}};
  _fig_insert_command(ID_rfgcolor, args);
}

void fig_rbgcolor(Color *c) {
  CmndArg args[] = {{sizeof(Color), c}, {0, (void *) NULL}};
  _fig_insert_command(ID_rbgcolor, args);
}

static void fig_text(Point *p, const char *text) {
  Int text_size = strlen(text);
  CmndArg args[] = {{sizeof(Point), p},
                    {sizeof(Int), & text_size},
                    {text_size+1, (void *) text},
                    {0, (void *) NULL}};
  _fig_insert_command(ID_text, args);
}

static void fig_font(const char *font, Real size) {
  Int font_size = strlen(font);
  CmndArg args[] = {{sizeof(Real), & size},
                    {sizeof(Int), & font_size},
                    {font_size+1, (void *) font},
                    {0, (void *) NULL}};
  _fig_insert_command(ID_font, args);
}

static void fig_fake_point(Point *p) {
  CmndArg args[] = {{sizeof(Point), p}, {0, (void *) NULL}};
  _fig_insert_command(ID_fake_point, args);
}

static void fig_close_win(void) {
  GrpWindow *w = grp_win;
  FigHeader *fh = (FigHeader *) w->wrdep;
  LayerHeader *lh = buff_firstitemptr(& fh->layerlist, LayerHeader);
  Int i, n = buff_numitem(& fh->layerlist);
  for(i=0; i<n; i++) buff_free(& (lh++)->layer);
  buff_free(& fh->layerlist);
  free(fh);
  free(w);
}

static void fig_rgradient(ColorGrad *cg) {
  CmndArg args[] = {{sizeof(ColorGrad), cg},
                    {0, (void *) NULL},
                    {0, (void *) NULL}};
  if (cg->num_items > 0) {
    args[1].arg_data_size = sizeof(ColorGradItem)*cg->num_items;
    args[1].arg_data = cg->items;
  }
  _fig_insert_command(ID_rgradient, args);
}

static int fig_save(const char *file_name) {
  char *out_type = "eps";
  GrpWindowPlan plan;

  enum {EXT_EPS=0, EXT_BMP, EXT_PNG, EXT_PDF, EXT_PS, EXT_SVG, EXT_NUM};
  char *ext[] = {"eps", "bmp", "png", "pdf", "ps", "svg", (char *) NULL};
  switch(file_extension(ext, file_name)) {
  case EXT_EPS: out_type = "eps"; break;
  case EXT_BMP: out_type = "bm8"; break;
  case EXT_PNG: out_type = "argb32"; break;
  case EXT_PDF: out_type = "pdf"; break;
  case  EXT_PS: out_type = "cairo:ps"; break;
  case EXT_SVG: out_type = "svg"; break;
  default:
    g_warning("Unrecognized extension in file name: using eps file format!");
    out_type = "eps"; break;
  }

  plan.file_name = (char *) file_name;
  plan.have.file_name = 1;
  plan.type = Grp_Window_Type_From_String(out_type);
  plan.have.type = 1;
  assert(plan.type >= 0);
  plan.have.size = 0;
  plan.have.origin = 0;
  plan.resolution.x = plan.resolution.y = 100.0/grp_mmperinch; /* ~ 100 dpi */
  plan.have.resolution = 1;
  plan.have.num_layers = 0;
  return fig_save_fig(grp_win, & plan);
}

/** Set the default methods to the gr1b window */
static void fig_repair(GrpWindow *w) {
  grp_window_block(w);
  w->rreset = fig_rreset;
  w->rinit = fig_rinit;
  w->rdraw = fig_rdraw;
  w->rline = fig_rline;
  w->rcong = fig_rcong;
  w->rclose = fig_rclose;
  w->rcircle = fig_rcircle;
  w->rfgcolor = fig_rfgcolor;
  w->rbgcolor = fig_rbgcolor;
  w->rgradient = fig_rgradient;
  w->text = fig_text;
  w->font = fig_font;
  w->fake_point = fig_fake_point;
  w->save = fig_save;
  w->close_win = fig_close_win;
}

/****************************************************************************/
/* DESCRIZIONE: Apre una finestra di tipo "fig", che consiste essenzialmente
 *  in un "registratore" di comandi grafici. Infatti ogni comando che
 *  la finestra riceve viene memorizzato in diversi "contenitori": i layer.
 *  Questi sono ordinati dal piu' basso, cioe' quello che viene disegnato
 *  per primo (e quindi sara' ricoperto da tutti i successivi), al piu' alto,
 *  che viene disegnato per ultimo (e quindi ricopre tutti gli altri).
 *  numlayers specifica proprio il numero dei layer della figura.
 *  L'ordine dei layer puo' essere modificato (altre procedure di questo file).
 *  Ad ogni layer e' associato un numero (da 1 a numlayers) e questo viene
 *  utilizzato per far riferimento ad esso.
 */
grp_window *fig_open_win(int numlayers) {
  grp_window *wd;
  FigHeader *figh;
  LayerHeader *layh;
  buff *laylist;
  int i;

  PRNMSG("fig_open_win:\n  ");

  if (numlayers < 1) {
    ERRORMSG("fig_open_win", "Figura senza layers");
    return NULL;
  }

  /* Creo gli headers della figura (con tutte le informazioni utili
   * per la gestione dei layers)
   */
  figh = (FigHeader *) malloc( sizeof(FigHeader) );

  if (figh == NULL) {
    ERRORMSG("fig_open_win", "Memoria esaurita");
    return NULL;
  }

  /* Creo la lista dei layers con numlayers elementi */
  laylist = & figh->layerlist;
  if ( ! buff_create(laylist, sizeof(LayerHeader), numlayers) ) {
    ERRORMSG("fig_open_win", "Memoria esaurita");
    return NULL;
  }

  /* buff_create crea una lista vuota e usa malloc solo quando si tenta
   * di riempirla (con buff_push, etc).
   * Quindi ora forzo l'allocazione della lista!
   */
  if ( ! buff_bigenough( laylist, buff_numitem(laylist) = numlayers ) ) {
    ERRORMSG("fig_open_win", "Memoria esaurita");
    return NULL;
  }

  /* Compilo l'header della figura */
  figh->numlayers = numlayers;
  figh->top = 1;
  figh->bottom = numlayers;
  figh->firstfree = 0;  /* Nessun layer libero */

  /* Adesso creo la lista dei layer */
  layh = buff_firstitemptr(laylist, LayerHeader);
  i = 0;
  while (i < numlayers) {
    /* Definisco lo spazio dove verranno memorizzate le istruzioni */
    if ( ! buff_create( & layh->layer, 1, LAYER_INITIAL_SIZE) ) {
      ERRORMSG("fig_open_win", "Memoria esaurita");
      return NULL;
    }

    /* Compilo l'header del layer */
    layh->ID = 0x7279616c;  /* 0x7279616c = "layr" */
    //layh->color = ???;
    layh->numcmnd = 0;
    layh->previous = i++;
    layh->next = (i + 1) % numlayers;

    ++layh;    /* Passo al prossimo layer */
  }

  wd = (grp_window *) malloc( sizeof(grp_window) );
  if ( (wd == NULL) ) {
    ERRORMSG("fig_open_win", "Memoria esaurita");
    return NULL;
  }

  wd->ptr = buff_ptr(laylist);

  wd->wrdep = (void *) figh;

  wd->quiet = 0;
  wd->repair = fig_repair;
  wd->repair(wd);
  wd->win_type_str = fig_id_string;

  PRNMSG("Ok!\n");
  return wd;
}

/* DESCRIZIONE: Elimina un layer con tutto il suo contenuto.
 *  l e' il numero del layer da distruggere.
 */
int fig_destroy_layer(int l) {
  FigHeader *figh;
  LayerHeader *flayh, *llayh, *layh;
  buff *laylist;
  int p, n;

  PRNMSG("fig_destroy_layer:\n  ");

  /* Trovo l'header della figura attualmente attiva */
  figh = (FigHeader *) grp_win->wrdep;

  if (figh->numlayers < 2) {
    ERRORMSG("fig_destroy_layer", "Figura senza layers");
    return 0;
  }

  l = CIRCULAR_INDEX(figh->numlayers, l);

  /* Trovo l'header del layer l */
  laylist = & figh->layerlist;
  flayh = buff_ptr(laylist);
  llayh = flayh + l - 1;

  /* Iniziamo col cancellare i comandi inseriti nel layer */
  buff_free(& llayh->layer);

  /* Togliamo l'header del layer dalla lista */
  p = llayh->previous;
  n = llayh->next;
  if (p == 0) {
    /* l e' il primo elemento della lista, quindi non e' l'ultimo, cioe'
     * n e' diverso da 0. n sara' il nuovo primo layer!
     */
    figh->top = n;
    layh = flayh + n - 1;
    layh->previous = 0;

  } else if (n == 0) {
    /* l e' l'ultimo elemento della lista, quindi non e' il primo, cioe'
     * p e' diverso da 0. p sara' il nuovo ultimo layer!
     */
    figh->bottom = p;
    layh = flayh + p - 1;
    layh->next = 0;

  } else {
    /* l non e' ne' il primo, ne' l'ultimo elemento della lista, cioe'
     * n e p diversi entrambi da 0.
     */
    layh = flayh + p - 1;
    layh->next = n;
    layh = flayh + n - 1;
    layh->previous = p;
  }

  /* Contrassegno l'header come vuoto e lo metto nella catena
   * degli header vuoti
   */
  llayh->ID = 0x65657266; /* 0x65657266 = "free" */
  llayh->next = figh->firstfree;
  figh->firstfree = l;

  /* Aggiorno i dati sulla figura */
  --figh->numlayers;
  if ( (figh->current == l) ) {
    WARNINGMSG("fig_destroy_layer", "Layer attivo distrutto: nuovo layer attivo = 1");
    fig_select_layer(1);
  }

  return 1;
}

/* DESCRIZIONE: Crea un nuovo layer vuoto e ne restituisce il numero
 *  identificativo (> 0) o 0 in caso di errore.
 */
int fig_new_layer(void) {
  FigHeader *figh;
  LayerHeader *flayh, *llayh, *layh;
  buff *laylist;
  int l;

  PRNMSG("fig_new_layer:\n  ");

  /* Trovo l'header della figura attualmente attiva */
  figh = (FigHeader *) grp_win->wrdep;

  laylist = & figh->layerlist;

  if (figh->firstfree == 0) {
    /* Devo ingrandire la lista */
    l = ++buff_numitem(laylist);
    if ( ! buff_bigenough( laylist, l ) ) {
      ERRORMSG("fig_new_layer", "Memoria esaurita");
      return 0;
    }

    /* Trovo l'header del nuovo layer */
    flayh = buff_ptr(laylist);
    llayh = flayh + l - 1;

    /* buff_bigenough puo' cambiare l'indirizzo della lista dei layer */
    fig_select_layer( figh->current );

  } else {
    /* Esiste un posto vuoto: lo occupo! */
    /* Trovo l'header del layer libero (figh->firstfree) */
    l = figh->firstfree;
    flayh = buff_ptr(laylist);
    llayh = flayh + l - 1;

    /* Verifica che si tratta di un header libero */
    if ( llayh->ID != 0x65657266 ) {  /* 0x65657266 = "free" */
      ERRORMSG("fig_new_layer", "Errore interno (bad layer ID, 1)");
      return 0;
    }
    /* Ora posso prendermelo! */
    figh->firstfree = llayh->next;
  }

  /* Definisco lo spazio dove verranno memorizzate le istruzioni */
  if ( ! buff_create( & llayh->layer, 1, LAYER_INITIAL_SIZE) ) {
    ERRORMSG("fig_new_layer", "Memoria esaurita");
    return 0;
  }

  /* Allungo la lista dei layers: il nuovo andra' sopra gli altri! */
  layh = flayh + figh->bottom - 1;
  layh->next = l;

  /* Compilo l'header del layer */
  layh->ID = 0x7279616c;  /* 0x7279616c = "layr" */

  //layh->color = ???;
  llayh->numcmnd = 0;
  llayh->previous = figh->bottom;
  llayh->next = 0;
  figh->bottom = l;
  ++figh->numlayers;

  return l;
}

/* DESCRIZIONE: Seleziona il layer l: fino alla prossima istruzione
 * fig_select_layer, i comandi grafici saranno inviati a quel layer.
 */
void fig_select_layer(int l) {
  buff *laylist;
  FigHeader *figh;
  LayerHeader *layh;

  PRNMSG("fig_select_layer:\n  ");

  /* Trovo l'header della figura attualmente attiva */
  figh = (FigHeader *) grp_win->wrdep;

  /* Setto il layer attivo a l */
  l = CIRCULAR_INDEX(figh->numlayers, l);
  figh->current = l;

  /* Trovo l'header del layer l */
  laylist = & figh->layerlist;
  layh = buff_ptr(laylist) + l - 1;
  /* Per convenzione grp_win->ptr punta a tale header: lo setto! */
  grp_win->ptr = layh;
}

/* DESCRIZIONE: Pulisce il contenuto del layer l.
 */
void fig_clear_layer(int l) {
  buff *laylist;
  FigHeader *figh;
  LayerHeader *layh;

  PRNMSG("fig_clear_layer:\n  ");

  /* Trovo l'header della figura attualmente attiva */
  figh = (FigHeader *) grp_win->wrdep;

  /* Trovo l'header del layer l */
  l = CIRCULAR_INDEX(figh->numlayers, l);
  laylist = & figh->layerlist;
  layh = buff_ptr(laylist) + l - 1;

  /* Cancello il contenuto del layer */
  layh->numcmnd = 0;
  if ( !buff_clear( & layh->layer ) ) {
    ERRORMSG("fig_clear_layer", "Memoria esaurita");
  }

  if ( figh->current == l )
    fig_select_layer(l);
}

/***************************************************************************************/
/* PROCEDURE PER DISEGNARE I LAYER SULLA FINESTRA ATTIVA */

/* Matrice usata nella trasformazione lineare di fig_transform_point */
Real fig_matrix[6] = {1.0, 0.0, 0.0, 1.0, 0.0, 0.0};

/* Apply the transformation in matrix fig_matrix to the n points in p[] */
void fig_transform_point(Point *p, int n) {
  Real m11 = fig_matrix[0], m12 = fig_matrix[1],
       m21 = fig_matrix[2], m22 = fig_matrix[3],
       m13 = fig_matrix[4], m23 = fig_matrix[5];
  Real px;

  for (; n-- > 0; p++) {
    px = p->x;
    p->x = m11 * px + m12 * p->y + m13;
    p->y = m21 * px + m22 * p->y + m23;
  }
}

/* Apply the linear transformation in fig_matrix to the n points in v[] */
void fig_transform_vector(Point *v, int n) {
  Real m11 = fig_matrix[0], m12 = fig_matrix[1],
       m21 = fig_matrix[2], m22 = fig_matrix[3];
  Real vx;

  for (; n-- > 0; v++) {
    vx = v->x;
    v->x = m11 * vx + m12 * v->y;
    v->y = m21 * vx + m22 * v->y;
  }
}

/* Find how the norm of the vector (cos(angle), sin(angle)) changes
 * with the trasformation in fig_matrix
 */
Real fig_transform_factor(Real angle) {
  Point v = {cos(angle), sin(angle)};
  fig_transform_vector(& v, 1);
  return sqrt(v.x*v.x + v.y*v.y);
}

void fig_transform_scalar(Real *r, int n, Real angle) {
  Real factor = fig_transform_factor(angle);
  for (; n-- > 0; r++) *r *= factor;
}

/* DESCRIZIONE: Disegna il layer l della figura source sulla finestra grafica
 *  attualmente in uso. Questo disegno puo' essere "filtrato" e cioe' puo' essere
 *  ruotato, ridimensionato, traslato, ...
 *  A tal scopo occorre impostare la trasformazione con ???????????????????????
 * NOTA: Non e' possibile copiare un layer in se' stesso: infatti l'esecuzione
 *  di comandi sul layer puo' causare un suo ridimensionamento, cioe' una realloc.
 *  In tal caso il fig_draw_layer continuera' a leggere sui vecchi indirizzi,
 *  senza controllare questa eventualita'! (problema   R I M O S S O !))
 */
void fig_draw_layer(grp_window *source, int l) {
  FigHeader *figh;
  LayerHeader *layh;
  buff *laylist, *layer;
  union {
   struct cmnd_header *hdr;
   void *ptr;
  } cmnd;
  long ID, ip, nc;

  PRNMSG("fig_draw_layer:\n  ");

  /* Trovo l'header della figura "source" */
  figh = (FigHeader *) source->wrdep;

  l = CIRCULAR_INDEX(figh->numlayers, l);

  /* Trovo l'header del layer l */
  laylist = & figh->layerlist;
  layh = buff_itemptr(laylist, LayerHeader, l);
  if ( layh->ID != 0x7279616c) {  /* 0x7279616c = "layr" */
    ERRORMSG( "fig_draw_layer", "Errore interno (bad layer ID), 3" );
    return;
  }

  layer = & layh->layer;

  ip = 0;    /* ip tiene traccia della posizione del buffer */
  nc = layh->numcmnd;      /* Numero dei comandi inseriti nel layer */

  PRNMSG("Layer contenente "); PRNINTG(nc); PRNMSG(" comandi.\n");

  /* Continuo fino ad aver eseguito ogni comando */
  while (nc > 0) {
    /* Trovo l'indirizzo dell'istruzione corrente.
     * NOTA: devo ricalcolarmelo ogni volta, dato che durante il ciclo while
     *  il buffer potrebbe essere ri-allocato e buff_ptr(layer) potrebbe
     *  cambiare! Questo accade quando copio un layer in se' stesso,
     *  in tal caso le istruzioni lette dal layer vengono anche scritte
     *  sullo stesso layer, cioe' le dimensioni del layer aumentano
     *  e potrebbe essere necessaria una realloc.
     */
    int cmnd_header_size;
    Point tp[3];
    Real tr;
    ColorGrad cg;

    cmnd.ptr = (void *) buff_ptr(layer) + (long) ip;

    cmnd_header_size = cmnd.hdr->size;
    ID = cmnd.hdr->ID;
    ip += sizeof(CmndHeader) + cmnd_header_size;
    cmnd.ptr += sizeof(CmndHeader);

    switch (ID) {
    case ID_rreset:
      grp_rreset();
      break;

    case ID_rinit:
      grp_rinit();
      break;

    case ID_rdraw:
      ((DrawStyle *) cmnd.ptr)->bord_dashes =
        (Real *) (cmnd.ptr + sizeof(DrawStyle));
      tr = ((DrawStyle *) cmnd.ptr)->scale;
      ((DrawStyle *) cmnd.ptr)->scale *= fig_transform_factor(0.0);
      grp_rdraw((DrawStyle *) cmnd.ptr);
      ((DrawStyle *) cmnd.ptr)->scale = tr;
      break;

    case ID_rline:
      tp[0] = *((Point *) cmnd.ptr);
      tp[1] = *((Point *) (cmnd.ptr + sizeof(Point)));
      fig_transform_point(tp, 2);
      grp_rline(& tp[0], & tp[1]);
      break;

    case ID_rcong:
      tp[0] = *((Point *) cmnd.ptr);
      tp[1] = *((Point *) (cmnd.ptr + sizeof(Point)));
      tp[2] = *((Point *) (cmnd.ptr + 2*sizeof(Point)));
      fig_transform_point(tp, 3);
      grp_rcong(& tp[0], & tp[1], & tp[2]);
      break;

    case ID_rclose:
      grp_rclose();
      break;

    case ID_rcircle:
      tp[0] = *((Point *) cmnd.ptr);
      tp[1] = *((Point *) (cmnd.ptr + sizeof(Point)));
      tp[2] = *((Point *) (cmnd.ptr + 2*sizeof(Point)));
      fig_transform_point(tp, 3);
      grp_rcircle(& tp[0], & tp[1], & tp[2]);
      break;

    case ID_rfgcolor:
      grp_rfgcolor((Color *) cmnd.ptr); break;

    case ID_rbgcolor:
      grp_rbgcolor((Color *) cmnd.ptr); break;

    case ID_rgradient:
      cg = *((ColorGrad *) cmnd.ptr);
      cg.items = (ColorGradItem *) (cmnd.ptr + sizeof(ColorGrad));
      fig_transform_point(& cg.point1, 1);
      fig_transform_point(& cg.point2, 1);
      fig_transform_point(& cg.ref1, 1);
      fig_transform_point(& cg.ref2, 1);
      grp_rgradient(& cg);
      break;

    case ID_text:
      {
        void *ptr = cmnd.ptr;
        Point p = *((Point *) ptr); ptr += sizeof(Point);
        Int str_size = *((Int *) ptr); ptr += sizeof(Int);
        char *str = (char *) ptr; ptr += str_size; /* ptr now points to '\0' */
        if (str_size + sizeof(Point) + sizeof(Int) <= cmnd_header_size) {
          if ( *((char *) ptr) == '\0' ) {
            fig_transform_point(& p, 1);
            grp_text(& p, str);
          } else {
            WARNINGMSG("fig_draw_layer", "Ignoring text command (bad str)!");
          }
        } else {
          WARNINGMSG("fig_draw_layer", "Ignoring text command (bad size)!");
        }
      }
      break;

    case ID_font:
      {
        void *ptr = cmnd.ptr;
        Real r = *((Real *) ptr); ptr += sizeof(Real);
        Int str_size = *((Int *) ptr); ptr += sizeof(Int);
        char *str = (char *) ptr; ptr += str_size; /* ptr now points to '\0' */
        r *= fig_transform_factor(0.0);
        if (str_size + sizeof(Real) + sizeof(Int) <= cmnd_header_size) {
          if ( *((char *) ptr) == '\0' ) {
            grp_font(str, r);
          } else {
            WARNINGMSG("fig_draw_layer", "Ignoring font command (bad str)!");
          }
        } else {
          WARNINGMSG("fig_draw_layer", "Ignoring font command (bad size)!");
        }
      }
      break;

    case ID_fake_point:
      {
        Point p = *((Point *) cmnd.ptr);
        fig_transform_point(& p, 1);
        grp_fake_point(& p);
      }
      break;

    default:
      ERRORMSG("fig_draw_layer", "Comando grafico non riconosciuto");
      return;
      break;
    }

    /* Passo al prossimo comando */
    --nc;
  }

  return;
}

/* DESCRIZIONE: Disegna la figura source sulla finestra grafica attualmente
 *  in uso. I layer vengono disegnati uno dietro l'altro con fig_draw_layer
 *  a partire dal "layer bottom", fino ad arrivare al "layer top".
 */
void fig_draw_fig(grp_window *source) {
  FigHeader *figh;
  LayerHeader *layh;
  buff *laylist;
  long nl, cl;

  PRNMSG("fig_draw_fig:\n  ");
  assert(source->win_type_str == fig_id_string);

  /* Trovo l'header della figura "source" */
  figh = (FigHeader *) source->wrdep;

  laylist = & figh->layerlist;

  /* Parto dall'header che sta sotto tutti gli altri */
  cl = figh->bottom;

  PRNMSG("Inizio a disegnare i "); PRNINTG(figh->numlayers);
  PRNMSG(" layer(s)!\n  ");

  for ( nl = figh->numlayers; nl > 0; nl-- ) {
    /* Disegno il layer cl */
    fig_draw_layer(source, cl);

    /* Ora passo a quello successivo che lo ricopre */
    /* Trovo l'header del layer cl */
    layh = (void *) buff_ptr(laylist) + (long) (cl - 1);
    cl = layh->previous;
  }

  if (cl != 0) {
    ERRORMSG( "fig_draw_fig", "Errore interno (layer chain)" );
    PRNMSG("Catena dei layer danneggiata!\n");
    return;
  }

  PRNMSG("Ok!\n");
}

int fig_save_fig(GrpWindow *figure, GrpWindowPlan *plan) {
  Point translation, center;
  Real sx, sy, rot_angle;
  GrpWindow *cur_win = grp_win;

  if (!plan->have.file_name || plan->file_name == (char *) NULL) {
    g_error("To save the \"fig\" Window you need to provide a filename!");
    return 0;
  }

  if (!(plan->have.size && plan->have.origin)) {
    Point bb_min, bb_max;
    if (!bb_bounding_box(figure, & bb_min, & bb_max)) {
      g_warning("Computed bounding box is degenerate: "
                "cannot save the figure!");
      return 0;
    }
    /*printf("Bounding box (%f, %f) - (%f, %f)\n",
           bb_min.x, bb_min.y, bb_max.x, bb_max.y);*/

    plan->size.x = fabs(bb_max.x - bb_min.x);
    plan->size.y = fabs(bb_max.y - bb_min.y);
    plan->have.size = 1;

    plan->origin.x = bb_min.x;
    plan->origin.y = bb_min.y;
  }

  translation.x = -plan->origin.x;
  translation.y = -plan->origin.y;
  center.y = center.x = 0.0;
  sy = sx = 1.0;
  rot_angle = 0.0;

  plan->origin.x = 0.0;
  plan->origin.y = 0.0;
  plan->have.origin = 1;
  grp_win = Grp_Window_Open(plan);
  if (grp_win != (GrpWindow *) NULL) {
    aput_matrix(& translation, & center, rot_angle, sx, sy, fig_matrix);
    fig_draw_fig(figure);
    grp_save(plan->file_name); /* Some terminals require an explicit save! */
    grp_close_win();
    grp_win = cur_win;
    return 1;
  }

  grp_win = cur_win;
  return 0;
}
