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

/* Questo file contiene le dichiarazioni di strutture, macro e procedure
 * grafiche non dipendenti dal modo in cui i dati relativi all'immagine
 * vengono memorizzati(bit per pixel).
 */

#ifndef _GRAPHIC_H
#  define _GRAPHIC_H

#  include "types.h"
#  include "gpath.h"

/****** DEFINIZIONE DELLE STRUTTURE NECESSARIE PER LA GRAFICA ******/

typedef enum {
  FILLSTYLE_VOID=0,
  FILLSTYLE_PLAIN,
  FILLSTYLE_EO,
  FILLSTYLE_CLIP,
  FILLSTYLE_EOCLIP
} FillStyle;

/* The color type */
typedef struct {
  Real r, g, b, a;
} Color;

typedef struct {
  Point max, min;
  Int num;
} BB;

/* Definisce la struttura adatta a contenere il colore di un punto */
typedef struct {
  unsigned char r;  /* Componente rossa */
  unsigned char g;  /* Componente verde */
  unsigned char b;  /* Componente blu */
} ColorBytes;

#define color ColorBytes

/* Questa e' la struttura di un elemento della palette */
struct palitem {
  long index;    /* Numero del colore nella tavolazza */
  ColorBytes c;    /* Colore */
  struct palitem *next;  /* Puntatore al prossimo elemento */
};

typedef struct palitem palitem;

/* Questa e' la struttura di una palette (= tavolazza di colori) */
typedef struct {
  long dim;  /* Dimensione della tavolazza (numero di colori inseribili) */
  long num;  /* Numero dei colori attualmente inseriti */
  long hashdim;  /* Dimensione della hash-table */
  long hashmul;  /* Numero utilizzato dalla hash-function */
  int reduce;    /* Livello di riduzione (approssimazione) dei colori */
  palitem **hashtable;  /* Puntatore alla hash-table */
} palette;

/** This structure is used to make uniform the functions for opening
 * graphic windows of different types.
 */
typedef struct {
  struct {
    int type : 1, origin : 1, size : 1,
        resolution : 1, file_name : 1, num_layers: 1;
  } have;
  int type;
  Point origin, size,
        resolution; /** Resolution in points per mm */
  char *file_name;
  int num_layers;
} GrpWindowPlan;

/** Font feature: slant */
typedef enum {
  FONT_SLANT_NORMAL, FONT_SLANT_ITALIC, FONT_SLANT_OBLIQUE
} FontSlant;

/** Font feature: weight */
typedef enum {
  FONT_WEIGHT_NORMAL, FONT_WEIGHT_BOLD
} FontWeight;

/** Alternatives for joining two segments of a line */
typedef enum {
  JOIN_STYLE_MITER=0, JOIN_STYLE_ROUND, JOIN_STYLE_BEVEL
} JoinStyle;

/** Alternatives for line endings. */
typedef enum {CAP_STYLE_BUTT=0, CAP_STYLE_ROUND, CAP_STYLE_SQUARE} CapStyle;

/** All what is needed by the rdraw method */
typedef struct {
  FillStyle fill_style;
  Real bord_width, bord_miter_limit;
  JoinStyle bord_join_style;
  Color bord_color;
  CapStyle bord_cap;
  Int bord_num_dashes;
  Real *bord_dashes, bord_dash_offset;
} DrawStyle;

/** Descriptor of a graphic Window */
typedef struct _grp_window {
  /** String which identifies the type of the window */
  char *win_type_str;

  /* High level functions */
  void (*rreset)(void);
  void (*rinit)(void);
  void (*rdraw)(DrawStyle *style);
  void (*rline)(Point *a, Point *b);
  void (*rcong)(Point *a, Point *b, Point *c);
  void (*rclose)(void);
  void (*rcircle)(Point *ctr, Point *a, Point *b);
  void (*rfgcolor)(Color *c);
  void (*rbgcolor)(Color *c);
  void (*text)(Point *p, const char *text);
  void (*font)(const char *font_name, Real size);
  void (*fake_point)(Point *p);
  /** Used to save the window to a file */
  int (*save)(const char *file_name);

  /** If set to 1, inhibits error messages */
  int quiet;

  /* Low level functions */
  void (*close_win)(void);
  void (*set_col)(int col);
  void (*draw_point)(Int ptx, Int pty);
  void (*hor_line)(Int y, Int x1, Int x2);

  /** Restore the window methods, which may have been changed after an error
   * has occurred.
   */
  void (*repair)(struct _grp_window *w);

  void *ptr;           /* Puntatore alla zona di memoria della finestra */
  Real ltx, lty;       /* Coordinate dell'angolo in alto a sinistra (in mm)*/
  Real rdx, rdy;       /* Coordinate dell'angolo in basso a destra (in mm) */
  Real minx, miny;     /* Coordinate minime visualizzabili (in mm) */
  Real maxx, maxy;     /* Coordinate massime visualizzabili (in mm) */
  Real lx, ly;         /* Larghezza e altezza (in mm, sono positive)*/
  Real versox, versoy; /* Versori x e y (numeri uguali a +1 o -1) */
  Real stepx, stepy;   /* Salto fra un punto e quello alla sua sinistra e in basso */
  Real resx, resy;     /* Risoluzione x e y (in punti per mm) */
  Int numptx, numpty;  /* Numero di punti nelle due direzioni */
  palitem *bgcol;      /* Colore dello sfondo */
  palitem *fgcol;      /* Colore attualmente in uso */
  palette *pal;        /* Tavolazza dei colori relativi alla finestra */
  long bitperpixel;    /* Numero di bit necessari a memorizzaze 1 pixel */
  long bytesperline;   /* Byte occupati da una linea */
  long dim;            /* Dimensione totale in byte della finestra */
  void *wrdep;         /* Puntatore alla struttura dei dati dipendenti dalla scrittura */
} GrpWindow;

/* Just for compatibility with past conventions */
#define grp_window GrpWindow

/* Dati importanti per la libreria */
/* Finestra attualmente in uso */
extern GrpWindow *grp_win;

/* Per convertire in millimetri, radianti, punti per millimetro */
extern Real grp_tomm;
extern Real grp_torad;
extern Real grp_toppmm;

/* Window type (in GrpWindowPlan) */
typedef enum {WT_NONE=-1, WT_BM1=0, WT_BM4, WT_BM8, WT_FIG,
              WT_PS, WT_EPS, WT_A1, WT_A8, WT_RGB24, WT_ARGB32,
              WT_PDF, WT_SVG, WT_MAX} WT;

/** Get the window ID (an integer number) from the window type (a string) */
int grp_window_type_from_string(const char *type_str);

/** Unified function to open any kind of window */
GrpWindow *grp_window_open(GrpWindowPlan *plan);

/** Initialise bounding box object */
void Grp_BB_Init(BB *bb);

/** Enlarge bounding box object 'bb', such that it contains the point 'p'. */
void Grp_BB_Must_Contain(BB *bb, Point *p);

/** Enlarge bounding box object 'dest' to contain the points of 'src'.. */
void Grp_BB_Fuse(BB *dst, BB *src);

/** Return the volume (area) occupied by the bounding box. */
Real Grp_BB_Volume(BB *bb);

/** Enlarge the bounding box adding margins. */
void Grp_BB_Margins(BB *bb, Point *margin_min, Point *margin_max);

/** Enlarge the bounding box adding equal margins. */
void Grp_BB_Margin(BB *bb, Real margin);

/* Dichiarazioni delle procedure della libreria */
/* Funzioni grafiche di alto livello */
GrpWindow *gr1b_open_win(Real ltx, Real lty, Real rdx, Real rdy,
                         Real resx, Real resy);
GrpWindow *gr4b_open_win(Real ltx, Real lty, Real rdx, Real rdy,
                         Real resx, Real resy);
GrpWindow *gr8b_open_win(Real ltx, Real lty, Real rdx, Real rdy,
                         Real resx, Real resy);
GrpWindow *fig_open_win(int numlayers);
GrpWindow *ps_open_win(const char *file);
GrpWindow *eps_open_win(const char *file, Real x, Real y);
int ps_save_fig(const char *file_name, GrpWindow *figure);
int eps_save_fig(const char *file_name, GrpWindow *figure);

void grp_window_block(GrpWindow *w);

/* Procedure per la gestione di una palette */
void grp_color_build(Color *cb, ColorBytes *c);
void grp_color_reduce(palette *p, ColorBytes *c);
palette *grp_palette_build(long numcol, long hashdim, long hashmul, int reduce);
palitem *grp_color_find(palette *p, ColorBytes *c);
palitem *grp_color_request(palette *p, ColorBytes *c);
int grp_palette_transform(palette *p, int (*operation)(palitem *pi));
void grp_palette_destroy(palette *p);
void grp_draw_gpath(GPath *gp);

void rst_repair(GrpWindow *gw);

Point *grp_ref(Point *o, Point *v, Point *p);

/* Funzioni grafiche di basso livello (legate al tipo di finestra aperta) */
#define grp_close_win  (grp_win->close_win)
#define grp_set_col    (grp_win->set_col)
#define grp_draw_point (grp_win->draw_point)
#define grp_hor_line   (grp_win->hor_line)

/* Funzioni grafiche di medio livello (di rasterizzazione) */
#define grp_rreset     (grp_win->rreset)
#define grp_rinit      (grp_win->rinit)
#define grp_rdraw      (grp_win->rdraw)
#define grp_rline      (grp_win->rline)
#define grp_rcong      (grp_win->rcong)
#define grp_rclose     (grp_win->rclose)
#define grp_rcircle    (grp_win->rcircle)
#define grp_rfgcolor   (grp_win->rfgcolor)
#define grp_rbgcolor   (grp_win->rbgcolor)
#define grp_text       (grp_win->text)
#define grp_font       (grp_win->font)
#define grp_fake_point (grp_win->fake_point)
#define grp_save       (grp_win->save)

/* Macro per la conversione fra diverse unità di misura */
/* Lunghezze */
#define grp_mm(x)  (x/grp_tomm)
#define grp_cm(x)  (x*10.0/grp_tomm)
#define grp_dm(x)  (x*100.0/grp_tomm)
#define grp_m(x)  (x*1000.0/grp_tomm)
/* Risoluzioni */
#define grp_toppu(x)  (x*grp_toppmm*grp_tomm)
#define grp_dpi(x)    (x/(25.4*grp_toppmm))
#define grp_ppmm(x)    (x/grp_toppmm)
/* Angoli */
#define grp_rad(x)  (x/grp_torad)
#define grp_deg(x)  (x*0.01745329252/grp_torad)
#define grp_grad(x)  (x*0.01570796327/grp_torad)

/* Costanti per la conversione */
#define grp_mmperinch  25.4
#define grp_radperdeg  0.01745329252
#define grp_radpergrad  0.01570796327
#define grp_ppmmperdpi  0.03937007874
#define grp_mm_per_inch 25.4
#define grp_inch_per_psunit (1.0/72.0)

/* Conversione da coordinate relative a coordinate assolute (floating) */
#define CV_XF_A(x)  (((Real) x - grp_win->ltx)/grp_win->stepx)
#define CV_YF_A(y)  (((Real) y - grp_win->lty)/grp_win->stepy)
/* Conversione di lunghezze relative in lunghezze assolute */
#define CV_LXF_A(x)  (((Real) x)/grp_win->stepx)
#define CV_LYF_A(y)  (((Real) y)/grp_win->stepy)
/* Conversione da coordinate assolute a coordinate intermedie */
#define CV_A_MED(x)  ((int) floor(x) + (int) ceil(x))
/* Conversione da coordinate intermedie a coordinate intere */
#define CV_MED_GT(x)  (((Int) x + 1) >> 1)
#define CV_MED_LW(x)  (((Int) x - 1) >> 1)

/* Restituisce 1 se la coordinata intermedia è un intero esatto, 0 altrimenti */
#define IS_EXACT_INT(x)  ((Int) x & 1)

#define WINNUMROW    grp_win->numpty

/* Informazioni sulla corrente finestra grafica */
#define GRP_AXMIN    0
#define GRP_AXMAX    (grp_win->numptx - 1)
#define GRP_AYMIN    0
#define GRP_AYMAX    (grp_win->numpty - 1)
#define GRP_IXMIN    0
#define GRP_IXMAX    (grp_win->numptx - 1)
#define GRP_IYMIN    0
#define GRP_IYMAX    (grp_win->numpty - 1)
#define GRP_MLOUT    -1
#define GRP_MROUT    0x7fff

/* Costanti matematiche */
#define pigreco 3.14159265358979323846

#endif
