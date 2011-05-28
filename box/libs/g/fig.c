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

#include "error.h"
#include "graphic.h"  /* Dichiaro alcune strutture grafiche generali */
#include "g.h"
#include "fig.h"
#include "bb.h"
#include "autoput.h"
#include <box/mem.h>
#include <box/array.h>
#include "obj.h"

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
  int numlayers,    /**< Number of layers in the figure */
      current,      /**< Layer which is currently being used */
      top,          /**< Top layer */
      bottom,       /**< Bottom layer */
      firstfree;    /**< First free layer (0 = none) */
  BoxArr
      layerlist;    /**< Buffer che gestisce la lista dei layer */
} FigHeader;

typedef struct {
  unsigned long
         ID;        /**< ID for consistency checks (debugging) */
  int    numcmnd,   /**< Number of commands in this layer */
         previous,  /**< Previous layer */
         next;      /**< Next layer */
  BoxArr layer;     /**< Layer data */
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
  ID_rgradient, ID_text, ID_font, ID_fake_point, ID_interpret
};

#define FIGLAYER_ID_ACTIVE 0x7279616c /* "layr" */
#define FIGLAYER_ID_FREE 0x65657266 /* "free" */

typedef struct {
  long  ID;
  long  size;
} FigCmndHeader;

typedef BoxTask (*FigCmndIter)(FigCmndHeader *cmnd_header, void *cmnd_data,
                               void *pass);

typedef struct {
  int arg_data_size;
  void *arg_data;
} CmndArg;


static BoxTask My_Fig_Iterate_Over_Layer(LayerHeader *layh,
                                         FigCmndIter iter, void *pass) {
  BoxArr *layer;
  long ip, nc;

  assert(layh->ID == FIGLAYER_ID_ACTIVE);

  layer = & layh->layer;
  nc = layh->numcmnd;    /* Numero dei comandi inseriti nel layer */
  ip = 1;                /* ip tiene traccia della posizione del buffer */

  /* Read all the commands in the layer */
  while (nc > 0) {
    /* Fint the address of the current instruction.
     * NOTE: we need to recompute the address each time, as during the while
     * loop the BoxArr may change (be reallocated). This happens when one
     * layer is being written into itself.
     */
    FigCmndHeader *cmnd_header = (FigCmndHeader *) BoxArr_Item_Ptr(layer, ip);
    size_t cmnd_header_size = cmnd_header->size;
    void *cmnd_data = (void *) cmnd_header + sizeof(FigCmndHeader);
    BoxTask task = iter(cmnd_header, cmnd_data, pass);

    if (task != BOXTASK_OK)
      return task;

    /* Continue to next command */
    ip += sizeof(FigCmndHeader) + cmnd_header_size;
    --nc;
  }

  return BOXTASK_OK;
}

BoxTask BoxGWin_Fig_Iterate_Over_Layer(BoxGWin *source, int nr_layer,
                                       FigCmndIter iter, void *pass) {
  FigHeader *figh = (FigHeader *) source->wrdep;
  int l = CIRCULAR_INDEX(figh->numlayers, nr_layer);
  LayerHeader *lh = (LayerHeader *) BoxArr_Item_Ptr(& figh->layerlist, l);
  return My_Fig_Iterate_Over_Layer(lh, iter, pass);
}

static BoxTask My_Layer_Finish_Iter(FigCmndHeader *cmnd_header,
                                    void *cmnd_data, void *pass) {
  switch (cmnd_header->ID) {
  case ID_interpret:
    BoxGObj_Finish((BoxGObj *) cmnd_data);
    return BOXTASK_OK;

  default:
    return BOXTASK_OK;
  }
}

static void My_Layer_Finish(LayerHeader *lh) {
  (void) My_Fig_Iterate_Over_Layer(lh, My_Layer_Finish_Iter, NULL);
  BoxArr_Finish(& lh->layer);
}

/* This function is used to insert a command in the current layer */
static void My_Fig_Push_Commands(BoxGWin *w, int id, CmndArg *args) {
  LayerHeader *lh = (LayerHeader *) w->ptr;
  BoxArr *layer;
  int total_size, i;
  FigCmndHeader ch;

  assert(lh->ID == FIGLAYER_ID_ACTIVE);
  layer = & lh->layer;

  /* Calculate total size of arguments */
  total_size = 0;
  for(i=0; args[i].arg_data_size > 0; i++)
    total_size += args[i].arg_data_size;

  /* Compile command header */
  ch.ID = id;
  ch.size = total_size;

  BoxArr_MPush(layer, & ch, sizeof(FigCmndHeader));
  for (i = 0; args[i].arg_data_size > 0; i++)
    BoxArr_MPush(layer, args[i].arg_data, args[i].arg_data_size);

  ++lh->numcmnd; /* Increase counter for number of commands in the layer */
}

static BoxTask My_Fig_Interpret(BoxGWin *w, BoxGObj *obj) {
  BoxGObj copy;
  CmndArg args[] = {{sizeof(BoxGObj), & copy},
                    {0, (void *) NULL}};

  assert(obj != NULL && w != NULL);

  BoxGObj_Init_From(& copy, obj);
  /* Note that here we are assuming that we can safely relocate BoxGObj
   * objects in memory, i.e. memcopy them and completely forget about the
   * originals (which means we do not call BoxGObj_Finish for them).
   * We then treat the copies as if they were effectively equivalent to the
   * originals. Typically this can be done for objects that are not being
   * referenced by other objects. Here we can do this as we are creating
   * a new copy of obj.
   */
  My_Fig_Push_Commands(w, ID_interpret, args);
  return BOXTASK_OK;
}

void fig_rreset(void) {
  CmndArg args[] = {{0, (void *) NULL}};
  My_Fig_Push_Commands(grp_win, ID_rreset, args);
}

void fig_rinit(void) {
  CmndArg args[] = {{0, (void *) NULL}};
  My_Fig_Push_Commands(grp_win, ID_rinit, args);
}

void fig_rdraw(DrawStyle *style) {
  CmndArg args[] = {{sizeof(DrawStyle), style},
                    {0, (void *) NULL},
                    {0, (void *) NULL}};
  if (style->bord_num_dashes > 0) {
    args[1].arg_data_size = sizeof(Real)*style->bord_num_dashes;
    args[1].arg_data = style->bord_dashes;
  }
  My_Fig_Push_Commands(grp_win, ID_rdraw, args);
}

void fig_rline(Point *a, Point *b) {
  CmndArg args[] = {{sizeof(Point), a},
                    {sizeof(Point), b},
                    {0, (void *) NULL}};
  My_Fig_Push_Commands(grp_win, ID_rline, args);
}

void fig_rcong(Point *a, Point *b, Point *c) {
  CmndArg args[] = {{sizeof(Point), a},
                    {sizeof(Point), b},
                    {sizeof(Point), c},
                    {0, (void *) NULL}};
  My_Fig_Push_Commands(grp_win, ID_rcong, args);
}

void fig_rclose(void) {
  CmndArg args[] = {{0, (void *) NULL}};
  My_Fig_Push_Commands(grp_win, ID_rclose, args);
}

void fig_rcircle(Point *ctr, Point *a, Point *b) {
  CmndArg args[] = {{sizeof(Point), ctr},
                    {sizeof(Point), a},
                    {sizeof(Point), b},
                    {0, (void *) NULL}};
  My_Fig_Push_Commands(grp_win, ID_rcircle, args);
}

void My_Fig_Set_Fg_Color(BoxGWin *w, Color *c) {
  CmndArg args[] = {{sizeof(Color), c}, {0, (void *) NULL}};
  My_Fig_Push_Commands(w, ID_rfgcolor, args);
}

void My_Fig_Set_Bg_Color(BoxGWin *w, Color *c) {
  CmndArg args[] = {{sizeof(Color), c}, {0, (void *) NULL}};
  My_Fig_Push_Commands(w, ID_rbgcolor, args);
}

typedef struct {
  Point ctr, right, up, from;
  Int text_size;
} Arg4Text;

static void My_Fig_Gen_Text_Path(BoxGWin *w,
                                 Point *ctr, Point *right, Point *up,
                                 Point *from, const char *text) {
  Int text_size = strlen(text);
  Arg4Text arg;
  CmndArg args[] = {{sizeof(Arg4Text), & arg},
                    {text_size+1, (void *) text},
                    {0, (void *) NULL}};
  arg.ctr = *ctr; arg.right = *right; arg.up = *up; arg.from = *from;
  arg.text_size = text_size;
  My_Fig_Push_Commands(w, ID_text, args);
}

static void My_Fig_Set_Font(BoxGWin *w, const char *font) {
  Int font_size = strlen(font);
  CmndArg args[] = {{sizeof(Int), & font_size},
                    {font_size+1, (void *) font},
                    {0, (void *) NULL}};
  My_Fig_Push_Commands(w, ID_font, args);
}

static void My_Fig_Fake_Point(BoxGWin *w, Point *p) {
  CmndArg args[] = {{sizeof(Point), p}, {0, (void *) NULL}};
  My_Fig_Push_Commands(w, ID_fake_point, args);
}

static void My_Fig_Close_Win(void) {
  BoxGWin *w = grp_win;
  FigHeader *fh = (FigHeader *) w->wrdep;
  LayerHeader *lh = (LayerHeader *) BoxArr_First_Item_Ptr(& fh->layerlist);
  size_t i, n = BoxArr_Num_Items(& fh->layerlist);

  for (i = 0; i < n; i++)
    My_Layer_Finish(& lh[i]);

  BoxArr_Finish(& fh->layerlist);
  BoxMem_Free(fh);
  BoxMem_Free(w);
}

static void My_Fig_Set_Gradient(BoxGWin *w, ColorGrad *cg) {
  CmndArg args[] = {{sizeof(ColorGrad), cg},
                    {0, (void *) NULL},
                    {0, (void *) NULL}};
  if (cg->num_items > 0) {
    args[1].arg_data_size = sizeof(ColorGradItem)*cg->num_items;
    args[1].arg_data = cg->items;
  }
  My_Fig_Push_Commands(w, ID_rgradient, args);
}

static int My_Fig_Save(const char *file_name) {
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
static void My_Fig_Repair(GrpWindow *w) {
  BoxGWin_Block(w);
  w->rreset = fig_rreset;
  w->rinit = fig_rinit;
  w->rdraw = fig_rdraw;
  w->rline = fig_rline;
  w->rcong = fig_rcong;
  w->rclose = fig_rclose;
  w->rcircle = fig_rcircle;
  w->set_fg_color = My_Fig_Set_Fg_Color;
  w->set_bg_color = My_Fig_Set_Bg_Color;
  w->set_gradient = My_Fig_Set_Gradient;
  w->gen_text_path = My_Fig_Gen_Text_Path;
  w->set_font = My_Fig_Set_Font;
  w->add_fake_point = My_Fig_Fake_Point;
  w->save = My_Fig_Save;
  w->interpret = My_Fig_Interpret;
  w->close_win = My_Fig_Close_Win;
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
BoxGWin *fig_open_win(int numlayers) {
  BoxGWin *wd;
  FigHeader *figh;
  LayerHeader *layh;
  BoxArr *laylist;
  int i;

  if (numlayers < 1) {
    ERRORMSG("fig_open_win", "Figura senza layers");
    return NULL;
  }

  /* Creo gli headers della figura (con tutte le informazioni utili
   * per la gestione dei layers)
   */
  figh = (FigHeader *) BoxMem_Alloc(sizeof(FigHeader));

  if (figh == NULL) {
    ERRORMSG("fig_open_win", "Out of memory");
    return NULL;
  }

  /* Creo la lista dei layers con numlayers elementi */
  laylist = & figh->layerlist;
  BoxArr_Init(& figh->layerlist, sizeof(LayerHeader), numlayers);

  /* Compilo l'header della figura */
  figh->numlayers = numlayers;
  figh->top = 1;
  figh->bottom = numlayers;
  figh->firstfree = 0;  /* Nessun layer libero */

  /* Adesso creo la lista dei layer */
  layh = (LayerHeader *) BoxArr_MPush(& figh->layerlist, NULL, numlayers);
  for (i = 0; i < numlayers; i++, layh++) {
    /* Intialise the data space for each layer */
    BoxArr_Init(& layh->layer, 1, LAYER_INITIAL_SIZE);

    /* Fill the layer header */
    layh->ID = FIGLAYER_ID_ACTIVE;
    layh->numcmnd = 0;
    layh->previous = (i > 0) ? i - 1 : 0;
    layh->next = (i + 1) % numlayers;
  }

  wd = (BoxGWin *) BoxMem_Alloc(sizeof(BoxGWin));
  if (wd == NULL) {
    ERRORMSG("fig_open_win", "Memoria esaurita");
    return NULL;
  }

  /* Window dependent data */
  wd->wrdep = figh;

  /* Pointer to current layer */
  wd->ptr = BoxArr_First_Item_Ptr(laylist);

  wd->quiet = 0;
  wd->repair = My_Fig_Repair;
  wd->repair(wd);
  wd->win_type_str = fig_id_string;
  return wd;
}

/* DESCRIZIONE: Elimina un layer con tutto il suo contenuto.
 *  l e' il numero del layer da distruggere.
 */
int fig_destroy_layer(int l) {
  FigHeader *figh = (FigHeader *) grp_win->wrdep;
  BoxArr *laylist = & figh->layerlist;
  LayerHeader *flayh = BoxArr_First_Item_Ptr(laylist),
              *llayh, *layh;
  int p, n;

  if (figh->numlayers < 2) {
    ERRORMSG("fig_destroy_layer", "Figura senza layers");
    return 0;
  }

  l = CIRCULAR_INDEX(figh->numlayers, l);

  /* Trovo l'header del layer l */
  llayh = flayh + l - 1;
  p = llayh->previous;
  n = llayh->next;

  /* Mark the layer as free and put it in the chain of free layers */
  llayh->ID = FIGLAYER_ID_FREE;
  llayh->next = figh->firstfree;
  figh->firstfree = l;
  My_Layer_Finish(llayh); /* Finalise the layer data */

  /* Unchain the removed layer */
  if (p == 0) {
    /* l is the first layer in the list: n will be the new first layer. */
    assert(n > 0);
    figh->top = n;
    layh = flayh + n - 1;
    layh->previous = 0;

  } else if (n == 0) {
    /* l is the last layer in the list: p will be the new last layer */
    assert(p > 0);
    figh->bottom = p;
    layh = flayh + p - 1;
    layh->next = 0;

  } else {
    /* l is in the middle: n and p are both nonzero. */
    assert(n > 0 && p > 0);
    layh = flayh + p - 1;
    layh->next = n;
    layh = flayh + n - 1;
    layh->previous = p;
  }

  /* Aggiorno i dati sulla figura */
  --figh->numlayers;
  if (figh->current == l) {
    WARNINGMSG("fig_destroy_layer",
               "Layer attivo distrutto: nuovo layer attivo = 1");
    fig_select_layer(1);
  }

  return 1;
}

/* DESCRIZIONE: Crea un nuovo layer vuoto e ne restituisce il numero
 *  identificativo (> 0) o 0 in caso di errore.
 */
int fig_new_layer(void) {
  FigHeader *figh = (FigHeader *) grp_win->wrdep;
  LayerHeader *flayh, *llayh, *layh;
  BoxArr *laylist = & figh->layerlist;
  int l;

  if (figh->firstfree == 0) {
    /* There are no spare free layers: we must create ea new one */
    l = BoxArr_Num_Items(laylist) + 1;
    llayh = (LayerHeader *) BoxArr_Push(laylist, NULL);
    flayh = BoxArr_First_Item_Ptr(laylist);

    /* BoxArr_Push may change the pointer to the list of layer: need
     * to update the pointer to the current layer.
     */
    fig_select_layer(figh->current);

  } else {
    /* Esiste un posto vuoto: lo occupo! */
    /* Trovo l'header del layer libero (figh->firstfree) */
    l = figh->firstfree;
    flayh = BoxArr_First_Item_Ptr(laylist);
    llayh = flayh + l - 1;

    /* Verifica che si tratta di un header libero */
    if (llayh->ID != FIGLAYER_ID_FREE) {
      ERRORMSG("fig_new_layer", "Errore interno (bad layer ID, 1)");
      return 0;
    }
    /* Ora posso prendermelo! */
    figh->firstfree = llayh->next;
  }

  /* Definisco lo spazio dove verranno memorizzate le istruzioni */
  BoxArr_Init(& llayh->layer, 1, LAYER_INITIAL_SIZE);

  /* Allungo la lista dei layers: il nuovo andra' sopra gli altri! */
  layh = flayh + figh->bottom - 1;
  layh->next = l;

  /* Compilo l'header del layer */
  layh->ID = FIGLAYER_ID_ACTIVE;
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
  BoxArr *laylist;
  FigHeader *figh;
  LayerHeader *layh;

  /* Trovo l'header della figura attualmente attiva */
  figh = (FigHeader *) grp_win->wrdep;

  /* Setto il layer attivo a l */
  l = CIRCULAR_INDEX(figh->numlayers, l);
  figh->current = l;

  /* Trovo l'header del layer l */
  laylist = & figh->layerlist;
  layh = BoxArr_First_Item_Ptr(laylist) + l - 1;
  /* Per convenzione grp_win->ptr punta a tale header: lo setto! */
  grp_win->ptr = layh;
}

/* DESCRIZIONE: Pulisce il contenuto del layer l.
 */
void fig_clear_layer(int l) {
  BoxArr *laylist;
  FigHeader *figh;
  LayerHeader *layh;

  /* Trovo l'header della figura attualmente attiva */
  figh = (FigHeader *) grp_win->wrdep;

  /* Trovo l'header del layer l */
  l = CIRCULAR_INDEX(figh->numlayers, l);
  laylist = & figh->layerlist;
  layh = BoxArr_First_Item_Ptr(laylist) + l - 1;

  /* Cancello il contenuto del layer */
  layh->numcmnd = 0;
  BoxArr_Clear(& layh->layer);

  if ( figh->current == l )
    fig_select_layer(l);
}

/***************************************************************************************/
/* PROCEDURE PER DISEGNARE I LAYER SULLA FINESTRA ATTIVA */

/* Matrice usata nella trasformazione lineare di Fig_Transform_Point */
static Matrix fig_matrix = {1.0, 0.0, 0.0, 0.0, 1.0, 0.0};


void Fig_Matrix_Apply(Matrix *m) {
/*  m11 = $.m11*$$.m11 + $.m12*$$.m21, m12 = $.m11*$$.m12 + $.m12*$$.m22
  m21 = $.m21*$$.m11 + $.m22*$$.m21, m22 = $.m21*$$.m12 + $.m22*$$.m22
  $$.m13 = $.m13 + $.m11*$$.m13 + $.m12*$$.m23
  $$.m23 = $.m23 + $.m21*$$.m13 + $.m22*$$.m23
  $$.m11 = m11, $$.m12 = m12, $$.m21 = m21, $$.m22 = m22*/

}

/* Apply the transformation in matrix fig_matrix to the n points in p[] */
static void Fig_Transform_Point(Point *p, int n) {
  Grp_Matrix_Mul_Point(& fig_matrix, p, n);
}

/* Apply the linear transformation in fig_matrix to the n points in v[] */
static void Fig_Transform_Vector(Point *v, int n) {
  Grp_Matrix_Mul_Vector(& fig_matrix, v, n);
}

/* Find how the norm of the vector (cos(angle), sin(angle)) changes
 * with the trasformation in fig_matrix
 */
static Real Fig_Transform_Factor(Real angle) {
  Point v = {cos(angle), sin(angle)};
  Fig_Transform_Vector(& v, 1);
  return sqrt(v.x*v.x + v.y*v.y);
}

#if 0
/* Commented out: it is not used! */
static void Fig_Transform_Scalar(Real *r, int n, Real angle) {
  Real factor = Fig_Transform_Factor(angle);
  for (; n-- > 0; r++) *r *= factor;
}
#endif


static BoxTask My_Fig_Draw_Layer_Iter(FigCmndHeader *cmnd_header,
                                      void *cmnd_data, void *pass) {
  BoxGWin *w = (BoxGWin *) pass;
  Point tp[3];
  Real tr;
  ColorGrad cg;

  switch (cmnd_header->ID) {
  case ID_rreset:
    grp_rreset();
    return BOXTASK_OK;

  case ID_rinit:
    grp_rinit();
    return BOXTASK_OK;

  case ID_rdraw:
    ((DrawStyle *) cmnd_data)->bord_dashes =
      (Real *) (cmnd_data + sizeof(DrawStyle));
    tr = ((DrawStyle *) cmnd_data)->scale;
    ((DrawStyle *) cmnd_data)->scale *= Fig_Transform_Factor(0.0);
    grp_rdraw((DrawStyle *) cmnd_data);
    ((DrawStyle *) cmnd_data)->scale = tr;
    return BOXTASK_OK;

  case ID_rline:
    tp[0] = *((Point *) cmnd_data);
    tp[1] = *((Point *) (cmnd_data + sizeof(Point)));
    Fig_Transform_Point(tp, 2);
    grp_rline(& tp[0], & tp[1]);
    return BOXTASK_OK;

  case ID_rcong:
    tp[0] = *((Point *) cmnd_data);
    tp[1] = *((Point *) (cmnd_data + sizeof(Point)));
    tp[2] = *((Point *) (cmnd_data + 2*sizeof(Point)));
    Fig_Transform_Point(tp, 3);
    grp_rcong(& tp[0], & tp[1], & tp[2]);
    return BOXTASK_OK;

  case ID_rclose:
    grp_rclose();
    return BOXTASK_OK;

  case ID_rcircle:
    tp[0] = *((Point *) cmnd_data);
    tp[1] = *((Point *) (cmnd_data + sizeof(Point)));
    tp[2] = *((Point *) (cmnd_data + 2*sizeof(Point)));
    Fig_Transform_Point(tp, 3);
    grp_rcircle(& tp[0], & tp[1], & tp[2]);
    return BOXTASK_OK;

  case ID_rfgcolor:
    BoxGWin_Set_Fg_Color(grp_win, (Color *) cmnd_data);
    return BOXTASK_OK;

  case ID_rbgcolor:
    BoxGWin_Set_Bg_Color(grp_win, (Color *) cmnd_data);
    return BOXTASK_OK;

  case ID_rgradient:
    cg = *((ColorGrad *) cmnd_data);
    cg.items = (ColorGradItem *) (cmnd_data + sizeof(ColorGrad));
    Fig_Transform_Point(& cg.point1, 1);
    Fig_Transform_Point(& cg.point2, 1);
    Fig_Transform_Point(& cg.ref1, 1);
    Fig_Transform_Point(& cg.ref2, 1);
    BoxGWin_Set_Gradient(grp_win, & cg);
    return BOXTASK_OK;

  case ID_text:
    {
      Arg4Text arg = *((Arg4Text *) cmnd_data);
      char *str = (char *) (cmnd_data + sizeof(Arg4Text));
      if (sizeof(Arg4Text) + arg.text_size + 1 <= cmnd_header->size) {
        char *ptr = (char *) (cmnd_data + sizeof(Arg4Text) + arg.text_size);
        if (*ptr == '\0') {
          Fig_Transform_Point(& arg.ctr, 1);
          Fig_Transform_Point(& arg.right, 1);
          Fig_Transform_Point(& arg.up, 1);
          BoxGWin_Gen_Text_Path(grp_win, & arg.ctr, & arg.right, & arg.up,
                                & arg.from, str);
        } else {
          g_warning("Fig_Draw_Layer: Ignoring text command (bad str)!");
        }
      } else {
        g_warning("Fig_Draw_Layer: Ignoring text command (bad size)!");
      }
    }
    return BOXTASK_OK;

  case ID_font:
    {
      void *ptr = cmnd_data;
      Int str_size = *((Int *) ptr); ptr += sizeof(Int);
      char *str = (char *) ptr; ptr += str_size; /* ptr now points to '\0' */
      if (str_size + sizeof(Int) <= cmnd_header->size) {
        if (*((char *) ptr) == '\0') {
          BoxGWin_Set_Font(grp_win, str);
        } else {
          g_warning("Fig_Draw_Layer: Ignoring font command (bad str) 1!");
        }
      } else {
        g_warning("Fig_Draw_Layer: Ignoring font command (bad size) 2!");
      }
    }
    return BOXTASK_OK;

  case ID_fake_point:
    {
      Point p = *((Point *) cmnd_data);
      Fig_Transform_Point(& p, 1);
      BoxGWin_Add_Fake_Point(grp_win, & p);
    }
    return BOXTASK_OK;

  case ID_interpret:
    return BoxGWin_Interpret_Obj(w, (BoxGObj *) cmnd_data);

  default:
    g_warning("Fig_Draw_Layer: unrecognized command (corrupted figure?)");
    return BOXTASK_FAILURE;
  }
}

/* DESCRIZIONE: Disegna il layer l della figura source sulla finestra grafica
 *  attualmente in uso. Questo disegno puo' essere "filtrato" e cioe' puo' essere
 *  ruotato, ridimensionato, traslato, ...
 *  A tal scopo occorre impostare la trasformazione con ???????????????????????
 * NOTA: Non e' possibile copiare un layer in se' stesso: infatti l'esecuzione
 *  di comandi sul layer puo' causare un suo ridimensionamento, cioe' una realloc.
 *  In tal caso il Fig_Draw_Layer continuera' a leggere sui vecchi indirizzi,
 *  senza controllare questa eventualita'! (problema   R I M O S S O !))
 */
void BoxGWin_Fig_Draw_Layer(BoxGWin *dest, BoxGWin *src, int l) {
  BoxGWin *save = grp_win;
  grp_win = dest;
  (void) BoxGWin_Fig_Iterate_Over_Layer(src, l, My_Fig_Draw_Layer_Iter, dest);
  grp_win = save;
}

/* DESCRIZIONE: Disegna la figura source sulla finestra grafica attualmente
 *  in uso. I layer vengono disegnati uno dietro l'altro con Fig_Draw_Layer
 *  a partire dal "layer bottom", fino ad arrivare al "layer top".
 */
static void _Fig_Draw_Fig(GrpWindow *source) {
  FigHeader *figh;
  LayerHeader *layh;
  BoxArr *laylist;
  long nl, cl;

  assert(source->win_type_str == fig_id_string);

  /* Trovo l'header della figura "source" */
  figh = (FigHeader *) source->wrdep;

  laylist = & figh->layerlist;

  /* Parto dall'header che sta sotto tutti gli altri */
  cl = figh->bottom;

  for (nl = figh->numlayers; nl > 0; nl--) {
    /* Disegno il layer cl */
    BoxGWin_Fig_Draw_Layer(grp_win, source, cl);

    /* Ora passo a quello successivo che lo ricopre */
    /* Trovo l'header del layer cl */
    layh = (void *) BoxArr_First_Item_Ptr(laylist) + (long) (cl - 1);
    cl = layh->previous;
  }

  if (cl != 0) {
    ERRORMSG( "Fig_Draw_Fig", "Errore interno (layer chain)" );
    return;
  }
}

void Fig_Draw_Fig_With_Matrix(GrpWindow *src, Matrix *m) {
  Matrix save_matrix = fig_matrix;
  fig_matrix = *m;
  _Fig_Draw_Fig(src);
  fig_matrix = save_matrix;
}

void Fig_Draw_Fig(GrpWindow *src) {
  Matrix save_matrix = fig_matrix;
  Grp_Matrix_Set_Identity(& fig_matrix);
  _Fig_Draw_Fig(src);
  fig_matrix = save_matrix;
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
    Matrix m;
    Grp_Matrix_Set(& m, & translation, & center, rot_angle, sx, sy);
    Fig_Draw_Fig_With_Matrix(figure, & m);
    grp_save(plan->file_name); /* Some terminals require an explicit save! */
    grp_close_win();
    grp_win = cur_win;
    return 1;
  }

  grp_win = cur_win;
  return 0;
}
