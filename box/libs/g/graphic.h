/****************************************************************************
 * Copyright (C) 2008-2011 by Matteo Franchin                               *
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
#  include "obj.h"

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
  Real m11, m12, m13, m21, m22, m23;
} Matrix;

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
  Real scale, bord_width, bord_miter_limit;
  JoinStyle bord_join_style;
  Color bord_color;
  CapStyle bord_cap;
  Int bord_num_dashes;
  Real *bord_dashes, bord_dash_offset;
} DrawStyle;

/** Enumeration of possible color gradient types */
typedef enum {
  COLOR_GRAD_TYPE_LINEAR=0,
  COLOR_GRAD_TYPE_RADIAL
} ColorGradType;

/** Structure used to specify a color in the gradient vector (see ColorGrad)
 */
typedef struct {
  Real position;
  Color color;
} ColorGradItem;

/** Decides what should be done outside the gradient area. */
typedef enum {
  COLOR_GRAD_EXT_NONE=0,
  COLOR_GRAD_EXT_REPEAT,
  COLOR_GRAD_EXT_REFLECT,
  COLOR_GRAD_EXT_PAD
} ColorGradExt;

/** Structure containing the specification for linear or circular gradients.
 * the gradient can be linear or circular. In the case of linear gradients
 * P(t) is a point in the segment point1-point2 which goes linearly from
 * point1 to point2, for t going from 0 to 1.
 * The color is constant in each line passing through P(t) and changes as
 * a function of t.
 */
typedef struct {
  ColorGradType type;
  ColorGradExt extend;
  Point point1, point2, ref1, ref2;
  Real radius1, radius2;
  Int num_items;
  ColorGradItem *items;
} ColorGrad;

/** Descriptor of a graphic Window */
typedef struct _grp_window BoxGWin;
typedef BoxGWin GrpWindow;

struct _grp_window {
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


  /** @see BoxGWin_Set_Fg_Color */
  void (*set_fg_color)(BoxGWin *w, Color *c);

  /** @see BoxGWin_Set_Bg_Color */
  void (*set_bg_color)(BoxGWin *w, Color *c);

  /** @see BoxGWin_Set_Gradient */
  void (*set_gradient)(BoxGWin *w, ColorGrad *cg);

  /** @see BoxGWin_Set_Font */
  void (*set_font)(BoxGWin *w, const char *font_name);

  /** @see BoxGWin_Gen_Text_Path */
  void (*gen_text_path)(BoxGWin *w,
                        BoxPoint *ctr, BoxPoint *right, BoxPoint *up,
                        BoxPoint *from, const char *text);

  /** @see BoxGWin_Add_Fake_Point */
  void (*add_fake_point)(BoxGWin *w, BoxPoint *p);

  /** Used to save the window to a file */
  int (*save)(const char *file_name);

  /** Interpret the commands stored inside the given BoxGObj. */
  BoxTask (*interpret)(BoxGWin *w, BoxGObj *commands);

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

  /** This function can be internally used to report errors. */
  void (*_report_error)(const char *msg);

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
};

#define BoxGWin_Fail(source, msg)

/** Define some macros to provide functions which can be called as one would
 * normally expect: Method_Of_Obj(obj, arg1, arg2, ...) rather than
 * obj->method_name(obj, ...).
 */

/** Set the foreground color. */
#define BoxGWin_Set_Fg_Color(win, c) \
  ((win)->set_fg_color)((win), (c))

/** Set the background color. */
#define BoxGWin_Set_Bg_Color(win, c) \
  ((win)->set_bg_color)((win), (c))

/** Set the gradient as a source for filling. */
#define BoxGWin_Set_Gradient(win, cg) \
  ((win)->set_gradient)((win), (cg))

/** Add a fake point to the window. Fake points only have an effect on the
 * bounding box (can be used to make sure a point is visible in the figure).
 */
#define BoxGWin_Add_Fake_Point(win, p) \
  ((win)->add_fake_point)((win), (p))

/** Interpret raw commands contained inside an Obj object. */
#define BoxGWin_Interpret_Obj(win, obj) \
  ((win)->interpret)((win), (obj))

/** Set the current font from the font name provided in the string. */
#define BoxGWin_Set_Font(win, name) \
  ((win)->set_font)((win), (name))

/** Generate the path for the given text on the reference frame
  * defined by the origin 'ctr' and the two axes ('ctr', 'right' and 'up'
  * are the point at position (0, 0), (0, 1) and (1, 0), respectively in the
  * reference frame. The text is printed in the direction (0, 0) -> (1, 0)
  * with font size 1.0 (meaning that the three points 'ctr', 'right' and 'up'
  * can be used to magnify the text and transform it in any way).
  */
#define BoxGWin_Gen_Text_Path(win, ctr, right, up, from, text) \
  ((win)->gen_text_path)((win), (ctr), (right), (up), (from), (text))

enum {BOXG_CMD_SAVE=0, BOXG_CMD_RESTORE, BOXG_CMD_SET_ANTIALIAS,
      BOXG_CMD_MOVE_TO, BOXG_CMD_LINE_TO, BOXG_CMD_CURVE_TO,
      BOXG_CMD_CLOSE_PATH, BOXG_CMD_NEW_PATH, BOXG_CMD_NEW_SUB_PATH,
      BOXG_CMD_STROKE, BOXG_CMD_STROKE_PRESERVE, BOXG_CMD_FILL,
      BOXG_CMD_FILL_PRESERVE, BOXG_CMD_CLIP, BOXG_CMD_CLIP_PRESERVE,
      BOXG_CMD_RESET_CLIP, BOXG_CMD_PUSH_GROUP, BOXG_CMD_POP_GROUP_TO_SOURCE,
      BOXG_CMD_SET_OPERATOR, BOXG_CMD_PAINT, BOXG_CMD_PAINT_WITH_ALPHA,
      BOXG_CMD_COPY_PAGE, BOXG_CMD_SHOW_PAGE, BOXG_CMD_SET_LINE_WIDTH,
      BOXG_CMD_SET_LINE_CAP, BOXG_CMD_SET_LINE_JOIN, BOXG_CMD_SET_MITER_LIMIT,
      BOXG_CMD_SET_DASH, BOXG_CMD_SET_FILL_RULE, BOXG_CMD_SET_SOURCE_RGBA,
      BOXG_CMD_TEXT_PATH, BOXG_CMD_TRANSLATE,
      BOXG_CMD_EXT_JOINARC_TO, BOXG_CMD_EXT_ARC_TO,
      BOXG_CMD_EXT_SET_FONT, BOXG_CMD_EXT_TEXT_PATH
      };

/* Just for compatibility with past conventions */
#define grp_window BoxGWin

/* Dati importanti per la libreria */
/* Finestra attualmente in uso */
extern BoxGWin *grp_win;

/* Per convertire in millimetri, radianti, punti per millimetro */
extern Real grp_tomm;
extern Real grp_torad;
extern Real grp_toppmm;

/* Window type (in GrpWindowPlan) */
typedef enum {WT_NONE=-1, WT_BM1=0, WT_BM4, WT_BM8, WT_FIG,
              WT_PS, WT_EPS, WT_A1, WT_A8, WT_RGB24, WT_ARGB32,
              WT_PDF, WT_SVG, WT_MAX} WT;

/** Get the window ID (an integer number) from the window type (a string) */
int Grp_Window_Type_From_String(const char *type_str);

/** Unified function to open any kind of window */
BoxGWin *Grp_Window_Open(GrpWindowPlan *plan);

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

/** Make sure the components of 'c' lie inside [0, 1] and correct them,
 * if needed.
 */
void Color_Trunc(Color *c);

/* Dichiarazioni delle procedure della libreria */
/* Funzioni grafiche di alto livello */
BoxGWin *gr1b_open_win(Real ltx, Real lty, Real rdx, Real rdy,
                       Real resx, Real resy);
BoxGWin *gr4b_open_win(Real ltx, Real lty, Real rdx, Real rdy,
                       Real resx, Real resy);
BoxGWin *gr8b_open_win(Real ltx, Real lty, Real rdx, Real rdy,
                       Real resx, Real resy);
BoxGWin *fig_open_win(int numlayers);
BoxGWin *ps_open_win(const char *file);
BoxGWin *eps_open_win(const char *file, Real x, Real y);
int ps_save_fig(const char *file_name, BoxGWin *figure);
int eps_save_fig(const char *file_name, BoxGWin *figure);

/** Type of function called when Grp_Window_Break has been used. */
typedef void (*GrpOnError)(const char *where);

/** Block the window 'w', such that it reports errors when used. */
void BoxGWin_Block(BoxGWin *w);

/** Similar to BoxGWin_Block, but when the window 'w' is used,
 * the function on_error is called, instead of reporting an error.
 */
void BoxGWin_Break(BoxGWin *w, GrpOnError on_error);

/** Restore the window 'w', after it has been broken with BoxGWin_Block
 * or BoxGWin_Break
 */
void Grp_Window_Repair(BoxGWin *w);

/** Create a Window which displays the given error message, when someone
 * tries to use it. The error message is fprinted to the give stream.
 */
BoxGWin *Grp_Window_Error(FILE *out, const char *msg);

/** Returns 1 if the window has been created with Grp_Window_Error,
 * 0 otherwise.
 */
int Grp_Window_Is_Error(BoxGWin *w);

/** Make 'w' a dummy window which just reports errors when used. */
void Grp_Window_Make_Dummy(BoxGWin *w);

#define grp_window_block BoxGWin_Block

/* Procedure per la gestione di una palette */
void grp_color_build(Color *cb, ColorBytes *c);
void grp_color_reduce(palette *p, ColorBytes *c);
palette *grp_palette_build(long numcol, long hashdim, long hashmul, int reduce);
palitem *grp_color_find(palette *p, ColorBytes *c);
palitem *grp_color_request(palette *p, ColorBytes *c);
int grp_palette_transform(palette *p, int (*operation)(palitem *pi));
void grp_palette_destroy(palette *p);
void grp_draw_gpath(GPath *gp);

void rst_repair(BoxGWin *gw);

Point *grp_ref(Point *o, Point *v, Point *p);

/** Calculate the transformation matrix corresponding to:
 * translation vector 't', rotation center 'rcntr', rotation angle 'rang',
 * scale factors 'sx' and 'sy'. The computed matrix is put in 'm'.
 */
void Grp_Matrix_Set(Matrix *m,
                    Point *t, Point *rcntr, Real rang, Real sx, Real sy);

/** Set the matrix 'm' to the identity matrix. */
void Grp_Matrix_Set_Identity(Matrix *m);

/** Apply the matrix 'm' to the 'num_pts' points in 'pts'. */
void Grp_Matrix_Mul_Point(Matrix *m, Point *pts, int num_pts);

/** Apply the matrix 'm' to the 'num_vecs' vectors in 'vecs'. */
void Grp_Matrix_Mul_Vector(Matrix *m, Point *vecs, int num_vecs);

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
