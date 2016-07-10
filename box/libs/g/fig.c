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

/**
 * @file fig.c
 * @brief Implementation of recording windows
 *
 * This files provides the implementation of recording windows.
 * A recording window remembers the commands it receives by storing them
 * in lists (layers). It then offers the capability of replaying these
 * commands by sending them to another window. The concept of layer is
 * implemented below, but not currently exported to the Box API.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include <box/types.h>
#include <box/mem.h>
#include <box/array.h>

#include "error.h"
#include "matrix.h"
#include "winmap.h"
#include "graphic.h"
#include "g.h"
#include "fig.h"
#include "bb.h"
#include "autoput.h"
#include "obj.h"
#include "cmd.h"

/* Dimensione iniziale di un layer in bytes = dimensione iniziale dello spazio
 * in cui vengono memorizzate le istruzioni di ciascun layer
 */
#define MY_INITIAL_LAYER_SIZE 128

/* Macro used to align arguments. */
#define MY_ARG_SIZE(sz) ((((sz) + 7)/8)*8)

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
  int num_layers,    /**< Number of layers in the figure */
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
typedef enum MyCmdId_enum {
  MYCMDID_RRESET = 1, MYCMDID_RINIT, MYCMDID_RDRAW, MYCMDID_RLINE,
  MYCMDID_RCONG, MYCMDID_RCLOSE, MYCMDID_RCIRCLE, MYCMDID_RFGCOLOR,
  MYCMDID_RBGCOLOR, MYCMDID_RGRADIENT, MYCMDID_TEXT, MYCMDID_FONT,
  MYCMDID_FAKE_POINT, MYCMDID_INTERPRET
} MyCmdId;


/* Arguments for MYCMDID_RDRAW. */
typedef struct {
  DrawStyle draw_style;
} MyCmdRDrawArgs;

/* Arguments for MYCMDID_RLINE. */
typedef struct {
  BoxPoint points[2];
} MyCmdRLineArgs;

/* Arguments for MYCMDID_RCONG. */
typedef struct {
  BoxPoint points[3];
} MyCmdRCongArgs;

/* Arguments for MYCMDID_RCIRCLE. */
typedef struct {
  BoxPoint points[3];
} MyCmdRCircleArgs;

/* Arguments for MYCMDID_RFGCOLOR. */
typedef struct {
  Color color;
} MyCmdRFgColorArgs;

/* Arguments for MYCMDID_RBGCOLOR. */
typedef struct {
  Color color;
} MyCmdRBgColorArgs;

/* Arguments for MYCMDID_RGRADIENT. */
typedef struct {
  ColorGrad gradient;
} MyCmdRGradientArgs;

/* Arguments for MYCMDID_TEXT. */
typedef struct {
  BoxPoint ctr, right, up, from;
  BoxInt text_size;
} MyCmdTextArgs;

/* Arguments for MYCMDID_FONT. */
typedef struct {
  int str_size;
} MyCmdFontArgs;

/* Arguments for MYCMDID_FAKE_POINT. */
typedef struct {
  BoxPoint point;
} MyCmdFakePointArgs;

/* Arguments for MYCMDID_INTERPRET. */
typedef struct {
  BoxGObj gobj;
} MyCmdInterpretArgs;

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
    /* Find the address of the current instruction.
     * NOTE: we need to recompute the address each time, as during the while
     * loop the BoxArr may change (be reallocated). This happens when one
     * layer is being written into itself.
     */
    FigCmndHeader *cmnd_header = (FigCmndHeader *) BoxArr_Item_Ptr(layer, ip);
    size_t cmnd_size = cmnd_header->size;
    void *cmnd_data = (void *) cmnd_header + sizeof(FigCmndHeader);
    BoxTask task = iter(cmnd_header, cmnd_data, pass);

    if (task != BOXTASK_OK)
      return task;

    /* Continue to next command */
    ip += sizeof(FigCmndHeader) + cmnd_size;
    --nc;
  }

  return BOXTASK_OK;
}

BoxTask BoxGWin_Fig_Iterate_Over_Layer(BoxGWin *source, int nr_layer,
                                       FigCmndIter iter, void *pass) {
  FigHeader *figh = (FigHeader *) source->data;
  int l = CIRCULAR_INDEX(figh->num_layers, nr_layer);
  LayerHeader *lh = (LayerHeader *) BoxArr_Item_Ptr(& figh->layerlist, l);
  return My_Fig_Iterate_Over_Layer(lh, iter, pass);
}

static BoxTask My_Layer_Finish_Iter(FigCmndHeader *cmnd_header,
                                    void *cmnd_data, void *pass) {
  switch (cmnd_header->ID) {
  case MYCMDID_INTERPRET:
    BoxGObj_Finish(& ((MyCmdInterpretArgs *) cmnd_data)->gobj);
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
  for (i = 0; args[i].arg_data_size > 0; i++) {
    /* Enforce 8 byte alignment. */
    int aligned_size = MY_ARG_SIZE(args[i].arg_data_size);
    total_size += aligned_size;
  }

  /* Compile command header */
  ch.ID = id;
  ch.size = total_size;

  BoxArr_MPush(layer, & ch, sizeof(FigCmndHeader));
  for (i = 0; args[i].arg_data_size > 0; i++) {
    int aligned_size = MY_ARG_SIZE(args[i].arg_data_size);
    BoxArr_MPush(layer, args[i].arg_data, aligned_size);
  }

  ++lh->numcmnd; /* Increase counter for number of commands in the layer */
}

BoxGErr My_Transform_Commands(BoxGCmd cmd, BoxGCmdSig sig, int num_args,
                              BoxGCmdArgKind *kinds, void **args,
                              BoxGCmdArg *aux, void *pass) {
  BoxGWinMap *map = pass;
  int i;

  for (i = 0; i < num_args; i++) {
    BoxGCmdArg *my_arg = & aux[i];
    void *arg = args[i];
    BoxGCmdArgKind kind = kinds[i];
    switch (kind) {
    case BOXGCMDARGKIND_WIDTH:
      BoxGWinMap_Map_Width(map, & my_arg->w, (BoxReal *) arg);
      args[i] = & my_arg->w;
      break;
    case BOXGCMDARGKIND_POINT:
      BoxGWinMap_Map_Point(map, & my_arg->p, (BoxPoint *) arg);
      args[i] = & my_arg->p;
      break;
    case BOXGCMDARGKIND_VECTOR:
      BoxGWinMap_Map_Vector(map, & my_arg->p, (BoxPoint *) arg);
      args[i] = & my_arg->p;
      break;
    default:
      break;
    }
  }

  return BOXGERR_NO_ERR;
}

static BoxTask My_Fig_Interpret(BoxGWin *w, BoxGObj *obj, BoxGWinMap *map) {
  MyCmdInterpretArgs arg0;

  assert(obj != NULL && w != NULL);

  BoxGObj_Init(& arg0.gobj);
  if (BoxGCmdIter_Filter_Append(My_Transform_Commands,
                                & arg0.gobj, obj, map) == BOXTASK_OK) {
    /* Note that here we are assuming that we can safely relocate BoxGObj
     * objects in memory, i.e. memcopy them and completely forget about the
     * originals (which means we do not call BoxGObj_Finish for them).
     * We then handle the copies as if they were effectively equivalent to the
     * originals. Typically this can be done for objects that are not being
     * referenced by other objects. Here we can do this as we are creating
     * a new copy of obj.
     */
    CmndArg args[] = {{sizeof(arg0), & arg0}, {0, (void *) NULL}};
    My_Fig_Push_Commands(w, MYCMDID_INTERPRET, args);
    return BOXTASK_OK;
  }

  return BOXTASK_FAILURE;
}

void My_Fig_Create_Path(BoxGWin *w) {
  CmndArg args[] = {{0, NULL}};
  My_Fig_Push_Commands(w, MYCMDID_RRESET, args);
}

void My_Fig_Begin_Drawing(BoxGWin *w) {
  CmndArg args[] = {{0, NULL}};
  My_Fig_Push_Commands(w, MYCMDID_RINIT, args);
}

void My_Fig_Draw_Path(BoxGWin *w, DrawStyle *style) {
  MyCmdRDrawArgs arg0;
  CmndArg args[] = {{sizeof(arg0), & arg0}, {0, NULL}, {0, NULL}};

  arg0.draw_style = *style;
  arg0.draw_style.bord_dashes = NULL;
  if (style->bord_num_dashes > 0) {
    args[1].arg_data_size = sizeof(BoxReal)*style->bord_num_dashes;
    args[1].arg_data = style->bord_dashes;
  }

  My_Fig_Push_Commands(w, MYCMDID_RDRAW, args);
}

void My_Fig_Add_Line_Path(BoxGWin *w, BoxPoint *a, BoxPoint *b) {
  MyCmdRLineArgs arg0;
  CmndArg args[] = {{sizeof(arg0), & arg0}, {0, NULL}};
  arg0.points[0] = *a;
  arg0.points[1] = *b;
  My_Fig_Push_Commands(w, MYCMDID_RLINE, args);
}

void My_Fig_Add_Join_Path(BoxGWin *w, BoxPoint *a, BoxPoint *b, BoxPoint *c) {
  MyCmdRCongArgs arg0;
  CmndArg args[] = {{sizeof(arg0), & arg0}, {0, NULL}};
  arg0.points[0] = *a;
  arg0.points[1] = *b;
  arg0.points[2] = *c;
  My_Fig_Push_Commands(w, MYCMDID_RCONG, args);
}

void My_Fig_Close_Path(BoxGWin *w) {
  CmndArg args[] = {{0, NULL}};
  My_Fig_Push_Commands(w, MYCMDID_RCLOSE, args);
}

void My_Fig_Circle_Path(BoxGWin *w, BoxPoint *ctr, BoxPoint *a, BoxPoint *b) {
  MyCmdRCircleArgs arg0;
  CmndArg args[] = {{sizeof(arg0), & arg0}, {0, NULL}};
  arg0.points[0] = *ctr;
  arg0.points[1] = *a;
  arg0.points[2] = *b;
  My_Fig_Push_Commands(w, MYCMDID_RCIRCLE, args);
}

void My_Fig_Set_Fg_Color(BoxGWin *w, Color *c) {
  MyCmdRFgColorArgs arg0;
  CmndArg args[] = {{sizeof(arg0), & arg0}, {0, NULL}};
  arg0.color = *c;
  My_Fig_Push_Commands(w, MYCMDID_RFGCOLOR, args);
}

void My_Fig_Set_Bg_Color(BoxGWin *w, Color *c) {
  MyCmdRBgColorArgs arg0;
  CmndArg args[] = {{sizeof(arg0), & arg0}, {0, NULL}};
  arg0.color = *c;
  My_Fig_Push_Commands(w, MYCMDID_RBGCOLOR, args);
}

static void My_Fig_Add_Text_Path(BoxGWin *w,
                                 BoxPoint *ctr, BoxPoint *right, BoxPoint *up,
                                 BoxPoint *from, const char *text) {
  BoxInt text_size = strlen(text);
  MyCmdTextArgs arg0;
  CmndArg args[] = {{sizeof(arg0), & arg0},
		    {text_size + 1, (char *) text},
		    {0, NULL}};

  arg0.ctr = *ctr;
  arg0.right = *right;
  arg0.up = *up;
  arg0.from = *from;
  arg0.text_size = text_size;
  My_Fig_Push_Commands(w, MYCMDID_TEXT, args);
}

static void My_Fig_Set_Font(BoxGWin *w, const char *font) {
  MyCmdFontArgs arg0;
  BoxInt font_size = strlen(font);
  CmndArg args[] = {{sizeof(arg0), & arg0},
                    {font_size + 1, (void *) font},
                    {0, NULL}};
  arg0.str_size = font_size;
  My_Fig_Push_Commands(w, MYCMDID_FONT, args);
}

static void My_Fig_Fake_Point(BoxGWin *w, BoxPoint *p) {
  MyCmdFakePointArgs arg0;
  CmndArg args[] = {{sizeof(arg0), & arg0}, {0, NULL}};
  arg0.point = *p;
  My_Fig_Push_Commands(w, MYCMDID_FAKE_POINT, args);
}

static void My_Fig_Finish(BoxGWin *w) {
  FigHeader *fh = (FigHeader *) w->data;
  LayerHeader *lh = (LayerHeader *) BoxArr_First_Item_Ptr(& fh->layerlist);
  size_t i, n = BoxArr_Num_Items(& fh->layerlist);

  for (i = 0; i < n; i++)
    My_Layer_Finish(& lh[i]);

  BoxArr_Finish(& fh->layerlist);
  Box_Mem_Free(fh);
}

static void My_Fig_Set_Gradient(BoxGWin *w, ColorGrad *cg) {
  MyCmdRGradientArgs arg0;
  CmndArg args[] = {{sizeof(arg0), & arg0},
                    {0, NULL},
                    {0, NULL}};

  arg0.gradient = *cg;
  if (cg->num_items > 0) {
    args[1].arg_data_size = sizeof(ColorGradItem)*cg->num_items;
    args[1].arg_data = cg->items;
  }

  My_Fig_Push_Commands(w, MYCMDID_RGRADIENT, args);
}

static int My_Fig_Save_To_File(BoxGWin *w, const char *file_name) {
  char *out_type = "eps";
  BoxGWinPlan plan;

  enum {EXT_EPS=0, EXT_BMP, EXT_PNG, EXT_PDF, EXT_PS, EXT_SVG, EXT_NUM};
  char *ext[] = {"eps", "bmp", "png", "pdf", "ps", "svg", (char *) NULL};
  switch(file_extension(ext, file_name)) {
  case EXT_EPS: out_type = "cairo:eps"; break;
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
  plan.type = BoxGWin_Type_From_String(out_type);
  plan.have.type = 1;
  assert(plan.type >= 0);
  plan.have.size = 0;
  plan.have.origin = 0;
  plan.resolution.x = plan.resolution.y = 100.0/grp_mmperinch; /* ~ 100 dpi */
  plan.have.resolution = 1;
  plan.have.num_layers = 0;
  return BoxGWin_Fig_Save_Fig(w, & plan);
}

/** Set the default methods to the gr1b window */
static void My_Fig_Repair(BoxGWin *w) {
  BoxGWin_Block(w);
  w->create_path = My_Fig_Create_Path;
  w->begin_drawing = My_Fig_Begin_Drawing;
  w->draw_path = My_Fig_Draw_Path;
  w->add_line_path = My_Fig_Add_Line_Path;
  w->add_join_path = My_Fig_Add_Join_Path;
  w->close_path = My_Fig_Close_Path;
  w->add_circle_path = My_Fig_Circle_Path;
  w->set_fg_color = My_Fig_Set_Fg_Color;
  w->set_bg_color = My_Fig_Set_Bg_Color;
  w->set_gradient = My_Fig_Set_Gradient;
  w->add_text_path = My_Fig_Add_Text_Path;
  w->set_font = My_Fig_Set_Font;
  w->add_fake_point = My_Fig_Fake_Point;
  w->save_to_file = My_Fig_Save_To_File;
  w->interpret = My_Fig_Interpret;
  w->finish = My_Fig_Finish;
}

BoxTask BoxGWin_Init_Fig(BoxGWin *wd, int num_layers) {
  FigHeader *figh;
  LayerHeader *layh;
  BoxArr *laylist;
  int i;

  if (num_layers < 1) {
    ERRORMSG("BoxGWin_Create_Fig", "Figura senza layers");
    return BOXTASK_ERROR;
  }

  /* Creo gli headers della figura (con tutte le informazioni utili
   * per la gestione dei layers)
   */
  figh = (FigHeader *) Box_Mem_Alloc(sizeof(FigHeader));
  if (figh == NULL) {
    ERRORMSG("BoxGWin_Create_Fig", "Out of memory");
    return BOXTASK_ERROR;
  }

  /* Creo la lista dei layers con num_layers elementi */
  laylist = & figh->layerlist;
  BoxArr_Init(& figh->layerlist, sizeof(LayerHeader), num_layers);

  /* Compilo l'header della figura */
  figh->num_layers = num_layers;
  figh->top = 1;
  figh->bottom = num_layers;
  figh->firstfree = 0;  /* Nessun layer libero */

  /* Adesso creo la lista dei layer */
  layh = (LayerHeader *) BoxArr_MPush(& figh->layerlist, NULL, num_layers);
  for (i = 0; i < num_layers; i++, layh++) {
    /* Intialise the data space for each layer */
    BoxArr_Init(& layh->layer, 1, MY_INITIAL_LAYER_SIZE);

    /* Fill the layer header */
    layh->ID = FIGLAYER_ID_ACTIVE;
    layh->numcmnd = 0;
    layh->previous = (i > 0) ? i - 1 : 0;
    layh->next = (i + 1) % num_layers;
  }


  /* Window dependent data */
  wd->data = figh;

  /* Pointer to current layer */
  wd->ptr = BoxArr_First_Item_Ptr(laylist);

  wd->quiet = 0;
  wd->repair = My_Fig_Repair;
  wd->repair(wd);
  wd->win_type_str = fig_id_string;
  return BOXTASK_OK;
}

/****************************************************************************/
/* DESCRIZIONE: Apre una finestra di tipo "fig", che consiste essenzialmente
 *  in un "registratore" di comandi grafici. Infatti ogni comando che
 *  la finestra riceve viene memorizzato in diversi "contenitori": i layer.
 *  Questi sono ordinati dal piu' basso, cioe' quello che viene disegnato
 *  per primo (e quindi sara' ricoperto da tutti i successivi), al piu' alto,
 *  che viene disegnato per ultimo (e quindi ricopre tutti gli altri).
 *  num_layers specifica proprio il numero dei layer della figura.
 *  L'ordine dei layer puo' essere modificato (altre procedure di questo file).
 *  Ad ogni layer e' associato un numero (da 1 a num_layers) e questo viene
 *  utilizzato per far riferimento ad esso.
 */
BoxGWin *BoxGWin_Create_Fig(int num_layers) {
  BoxGWin *wd = Box_Mem_Alloc(sizeof(BoxGWin));
  if (wd == NULL) {
    ERRORMSG("BoxGWin_Create_Fig", "Memoria esaurita");
    return NULL;
  }

  if (BoxGWin_Init_Fig(wd, num_layers) == BOXTASK_OK)
    return wd;

  else {
    Box_Mem_Free(wd);
    return NULL;
  }
}

/* DESCRIZIONE: Elimina un layer con tutto il suo contenuto.
 *  l e' il numero del layer da distruggere.
 */
int BoxGWin_Fig_Destroy_Layer(BoxGWin *w, int l) {
  FigHeader *figh = (FigHeader *) w->data;
  BoxArr *laylist = & figh->layerlist;
  LayerHeader *flayh = BoxArr_First_Item_Ptr(laylist),
              *llayh, *layh;
  int p, n;

  if (figh->num_layers < 2) {
    ERRORMSG("BoxGWin_Fig_Destroy_Layer", "Figura senza layers");
    return 0;
  }

  l = CIRCULAR_INDEX(figh->num_layers, l);

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
  --figh->num_layers;
  if (figh->current == l) {
    WARNINGMSG("BoxGWin_Fig_Destroy_Layer",
               "Layer attivo distrutto: nuovo layer attivo = 1");
    BoxGWin_Fig_Select_Layer(w, 1);
  }

  return 1;
}

/* DESCRIZIONE: Crea un nuovo layer vuoto e ne restituisce il numero
 *  identificativo (> 0) o 0 in caso di errore.
 */
int BoxGWin_Fig_New_Layer(BoxGWin *w) {
  FigHeader *figh = (FigHeader *) w->data;
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
    BoxGWin_Fig_Select_Layer(w, figh->current);

  } else {
    /* Esiste un posto vuoto: lo occupo! */
    /* Trovo l'header del layer libero (figh->firstfree) */
    l = figh->firstfree;
    flayh = BoxArr_First_Item_Ptr(laylist);
    llayh = flayh + l - 1;

    /* Verifica che si tratta di un header libero */
    if (llayh->ID != FIGLAYER_ID_FREE) {
      ERRORMSG("BoxGWin_Fig_New_Layer", "Errore interno (bad layer ID, 1)");
      return 0;
    }
    /* Ora posso prendermelo! */
    figh->firstfree = llayh->next;
  }

  /* Definisco lo spazio dove verranno memorizzate le istruzioni */
  BoxArr_Init(& llayh->layer, 1, MY_INITIAL_LAYER_SIZE);

  /* Allungo la lista dei layers: il nuovo andra' sopra gli altri! */
  layh = flayh + figh->bottom - 1;
  layh->next = l;

  /* Compilo l'header del layer */
  layh->ID = FIGLAYER_ID_ACTIVE;
  llayh->numcmnd = 0;
  llayh->previous = figh->bottom;
  llayh->next = 0;
  figh->bottom = l;
  ++figh->num_layers;
  return l;
}

/* DESCRIZIONE: Seleziona il layer l: fino alla prossima istruzione
 * BoxGWin_Fig_Select_Layer, i comandi grafici saranno inviati a quel layer.
 */
void BoxGWin_Fig_Select_Layer(BoxGWin *w, int l) {
  BoxArr *laylist;
  FigHeader *figh;
  LayerHeader *layh;

  /* Trovo l'header della figura attualmente attiva */
  figh = (FigHeader *) w->data;

  /* Setto il layer attivo a l */
  l = CIRCULAR_INDEX(figh->num_layers, l);
  figh->current = l;

  /* Trovo l'header del layer l */
  laylist = & figh->layerlist;
  layh = BoxArr_First_Item_Ptr(laylist) + l - 1;
  /* Per convenzione w->ptr punta a tale header: lo setto! */
  w->ptr = layh;
}

/* DESCRIZIONE: Pulisce il contenuto del layer l.
 */
void BoxGWin_Fig_Clear_Layer(BoxGWin *w, int l) {
  BoxArr *laylist;
  FigHeader *figh;
  LayerHeader *layh;

  /* Trovo l'header della figura attualmente attiva */
  figh = (FigHeader *) w->data;

  /* Trovo l'header del layer l */
  l = CIRCULAR_INDEX(figh->num_layers, l);
  laylist = & figh->layerlist;
  layh = BoxArr_First_Item_Ptr(laylist) + l - 1;

  /* Cancello il contenuto del layer */
  layh->numcmnd = 0;
  BoxArr_Clear(& layh->layer);

  if ( figh->current == l )
    BoxGWin_Fig_Select_Layer(w, l);
}

/*****************************************************************************/
/* PROCEDURE PER DISEGNARE I LAYER SULLA FINESTRA ATTIVA */

typedef struct {
  BoxGWin    *win;
  BoxGWinMap *map;
} MyFigDrawLayerData;

static BoxTask My_Fig_Draw_Layer_Iter(FigCmndHeader *cmnd_header,
                                      void *cmnd_data, void *pass) {
  BoxGWin *w = ((MyFigDrawLayerData *) pass)->win;
  BoxGWinMap *map = ((MyFigDrawLayerData *) pass)->map;

  switch (cmnd_header->ID) {
  case MYCMDID_RRESET:
    BoxGWin_Create_Path(w);
    return BOXTASK_OK;

  case MYCMDID_RINIT:
    BoxGWin_Begin_Drawing(w);
    return BOXTASK_OK;

  case MYCMDID_RDRAW:
    {
      MyCmdRDrawArgs arg0 = *((MyCmdRDrawArgs *) cmnd_data);

      arg0.draw_style.bord_dashes =
	(BoxReal *) ((char *) cmnd_data + MY_ARG_SIZE(sizeof(MyCmdRDrawArgs)));
      BoxGWinMap_Map_Width(map, & arg0.draw_style.scale,
			   & arg0.draw_style.scale);
      BoxGWin_Draw_Path(w, & arg0.draw_style);
      return BOXTASK_OK;
    }

  case MYCMDID_RLINE:
    {
      MyCmdRLineArgs arg0, *src = (MyCmdRLineArgs *) cmnd_data;

      BoxGWinMap_Map_Points(map, & arg0.points[0], & src->points[0], 2);
      BoxGWin_Add_Line_Path(w, & arg0.points[0], & arg0.points[1]);
      return BOXTASK_OK;
    }

  case MYCMDID_RCONG:
    {
      MyCmdRCongArgs arg0, *src = (MyCmdRCongArgs *) cmnd_data;

      BoxGWinMap_Map_Points(map, & arg0.points[0], & src->points[0], 3);
      BoxGWin_Add_Join_Path(w, & arg0.points[0], & arg0.points[1],
			    & arg0.points[2]);
      return BOXTASK_OK;
    }

  case MYCMDID_RCLOSE:
    BoxGWin_Close_Path(w);
    return BOXTASK_OK;

  case MYCMDID_RCIRCLE:
    {
      MyCmdRCircleArgs arg0, *src = (MyCmdRCircleArgs *) cmnd_data;

      BoxGWinMap_Map_Points(map, & arg0.points[0], & src->points[0], 3);
      BoxGWin_Add_Circle_Path(w, & arg0.points[0], & arg0.points[1],
			      & arg0.points[2]);
      return BOXTASK_OK;
    }

  case MYCMDID_RFGCOLOR:
    BoxGWin_Set_Fg_Color(w, & ((MyCmdRFgColorArgs *) cmnd_data)->color);
    return BOXTASK_OK;

  case MYCMDID_RBGCOLOR:
    BoxGWin_Set_Bg_Color(w, & ((MyCmdRBgColorArgs *) cmnd_data)->color);
    return BOXTASK_OK;

  case MYCMDID_RGRADIENT:
    {
      MyCmdRGradientArgs arg0, *src = (MyCmdRGradientArgs *) cmnd_data;

      arg0 = *src;
      arg0.gradient.items =
	(ColorGradItem *) ((char *) cmnd_data
			   + MY_ARG_SIZE(sizeof(MyCmdRGradientArgs)));

      BoxGWinMap_Map_Point(map, & arg0.gradient.point1,
			   & src->gradient.point1);
      BoxGWinMap_Map_Point(map, & arg0.gradient.point2,
			   & src->gradient.point2);
      BoxGWinMap_Map_Point(map, & arg0.gradient.ref1, & src->gradient.ref1);
      BoxGWinMap_Map_Point(map, & arg0.gradient.ref2, & src->gradient.ref2);
      BoxGWin_Set_Gradient(w, & arg0.gradient);
      return BOXTASK_OK;
    }

  case MYCMDID_TEXT:
    {
      MyCmdTextArgs arg0 = *((MyCmdTextArgs *) cmnd_data);
      int arg0_size = MY_ARG_SIZE(sizeof(MyCmdTextArgs));
      char *str = (char *) cmnd_data + arg0_size;

      if (arg0_size + arg0.text_size + 1 <= cmnd_header->size) {
	if (str[arg0.text_size] == '\0') {
	  BoxGWinMap_Map_Point(map, & arg0.ctr, & arg0.ctr);
	  BoxGWinMap_Map_Point(map, & arg0.right, & arg0.right);
	  BoxGWinMap_Map_Point(map, & arg0.up, & arg0.up);
	  BoxGWin_Add_Text_Path(w, & arg0.ctr, & arg0.right, & arg0.up,
				& arg0.from, str);
	} else
	  g_warning("Fig_Draw_Layer: Ignoring text command (bad str)!");
      } else
	g_warning("Fig_Draw_Layer: Ignoring text command (bad size)!");
      return BOXTASK_OK;
    }

  case MYCMDID_FONT:
    {
      MyCmdFontArgs *arg0 = (MyCmdFontArgs *) cmnd_data;
      int arg0_size = MY_ARG_SIZE(sizeof(MyCmdFontArgs));
      char *str = (char *) cmnd_data + arg0_size;

      if (arg0_size + arg0->str_size <= cmnd_header->size) {
        if (str[arg0->str_size] == '\0')
          BoxGWin_Set_Font(w, str);
        else
          g_warning("Fig_Draw_Layer: Ignoring font command (bad str)!");
      }	else
	g_warning("Fig_Draw_Layer: Ignoring font command (bad size)!");
      return BOXTASK_OK;
    }

  case MYCMDID_FAKE_POINT:
    {
      MyCmdFakePointArgs arg0, *src = (MyCmdFakePointArgs *) cmnd_data;
      BoxGWinMap_Map_Point(map, & arg0.point, & src->point);
      BoxGWin_Add_Fake_Point(w, & arg0.point);
    }
    return BOXTASK_OK;

  case MYCMDID_INTERPRET:
    return BoxGWin_Interpret_Obj(w, & ((MyCmdInterpretArgs *) cmnd_data)->gobj,
				 map);

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
void BoxGWin_Fig_Draw_Layer(BoxGWin *dest, BoxGWin *src, BoxGWinMap *map, int l) {
  MyFigDrawLayerData data;
  data.win = dest;
  data.map = map;
  (void) BoxGWin_Fig_Iterate_Over_Layer(src, l, My_Fig_Draw_Layer_Iter, & data);
}

/* DESCRIZIONE: Disegna la figura source sulla finestra grafica attualmente
 *  in uso. I layer vengono disegnati uno dietro l'altro con Fig_Draw_Layer
 *  a partire dal "layer bottom", fino ad arrivare al "layer top".
 */
static void My_Fig_Draw_Fig(BoxGWin *dest, BoxGWin *source, BoxGWinMap *map) {
  FigHeader *figh;
  LayerHeader *layh;
  BoxArr *laylist;
  long nl, cl;

  assert(source->win_type_str == fig_id_string);

  /* Trovo l'header della figura "source" */
  figh = (FigHeader *) source->data;

  laylist = & figh->layerlist;

  /* Parto dall'header che sta sotto tutti gli altri */
  cl = figh->bottom;

  for (nl = figh->num_layers; nl > 0; nl--) {
    /* Disegno il layer cl */
    BoxGWin_Fig_Draw_Layer(dest, source, map, cl);

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

void BoxGWin_Fig_Draw_Fig_With_Matrix(BoxGWin *dest, BoxGWin *src,
                                      BoxGMatrix *m) {
  BoxGWinMap map;
  BoxGWinMap_Init(& map, m);
  My_Fig_Draw_Fig(dest, src, & map);
}

void BoxGWin_Fig_Draw_Fig(BoxGWin *dest, BoxGWin *src) {
  BoxGWinMap map;
  BoxGMatrix identity;
  BoxGMatrix_Set_Identity(& identity);
  BoxGWinMap_Init(& map, & identity);
  My_Fig_Draw_Fig(dest, src, & map);
}

int BoxGWin_Fig_Save_Fig(BoxGWin *src, BoxGWinPlan *plan) {
  BoxPoint translation, center;
  BoxReal sx, sy, rot_angle;
  BoxGWin *dest;

  if (plan->file_name == NULL) {
    g_error("To save the \"fig\" Window you need to provide a filename!");
    return 0;
  }

  if (!(plan->have.size && plan->have.origin)) {
    BoxGBBox bbox;
    if (!BoxGBBox_Compute(& bbox, src)) {
      g_warning("Computed bounding box is degenerate: "
                "cannot save the figure!");
      return 0;
    }
    /*printf("Bounding box (%f, %f) - (%f, %f)\n",
           bb_min.x, bb_min.y, bb_max.x, bb_max.y);*/

    plan->size.x = fabs(bbox.max.x - bbox.min.x);
    plan->size.y = fabs(bbox.max.y - bbox.min.y);
    plan->have.size = 1;

    plan->origin.x = bbox.min.x;
    plan->origin.y = bbox.min.y;
  }

  translation.x = -plan->origin.x;
  translation.y = -plan->origin.y;
  center.y = center.x = 0.0;
  sy = sx = 1.0;
  rot_angle = 0.0;

  plan->origin.x = 0.0;
  plan->origin.y = 0.0;
  plan->have.origin = 1;
  dest = BoxGWin_Create(plan);
  if (dest != NULL) {
    BoxGMatrix m;
    BoxGMatrix_Set(& m, & translation, & center, rot_angle, sx, sy);
    BoxGWin_Fig_Draw_Fig_With_Matrix(dest, src, & m);
    BoxGWin_Save_To_File(dest, plan->file_name); /* Some terminals require an explicit save! */
    BoxGWin_Destroy(dest);
    return 1;
  }

  return 0;
}
