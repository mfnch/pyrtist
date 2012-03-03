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
#  include "matrix.h"
#  include "winmap.h"

/** Enumeration of possible errors occuring in the graphic library */
typedef enum {
  BOXGERR_NO_ERR=0,
  BOXGERR_UNEXPECTED,
  BOXGERR_NO_MEMORY,
  BOXGERR_MISS_WIN_TYPE,
  BOXGERR_CAIRO_MISSES_PS,
  BOXGERR_CAIRO_MISSES_PDF,
  BOXGERR_CAIRO_MISSES_SVG,
  BOXGERR_UNKNOWN_WIN_TYPE,
  BOXGERR_WIN_SIZE_MISSING,
  BOXGERR_WIN_RES_MISSING,
  BOXGERR_WIN_FILENAME_MISSING,
  BOXGERR_CAIRO_SURFACE_ERR,
  BOXGERR_CAIRO_CONTEXT_ERR,
  BOXGERR_CAIRO_PATTERN_ERR,
  BOXGERR_CMD_BAD_ARGS,
  BOXGERR_CMD_MISSING_ARGS,
  BOXGERR_CMD_UNEXPECTED_ARGS,
  BOXGERR_CMD_EXEC,
  BOXGERR_CMD_BAD,
  BOXGERR_CMD_EMPTY,
  BOXGERR_CMD_BAD_INDEX

} BoxGErr;

/** Return a string corresponding to the given error code. */
const char *BoxGErr_To_Str(BoxGErr err);

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
} BoxGWinPlan;

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

/* BoxGWin would need some cleaning. At the moment it contains stuff which
 * is not relevant for all kinds of windows...
 */
struct _grp_window {
  /** String which identifies the type of the window */
  char *win_type_str;

  /* High level functions */

  /** @see BoxGWin_Create_Path */
  void (*create_path)(BoxGWin *w);

  /** @see BoxGWin_Begin_Drawing */
  void (*begin_drawing)(BoxGWin *w);

  /** @see BoxGWin_Draw_Path */
  void (*draw_path)(BoxGWin *w, DrawStyle *style);
  
  /** @see BoxGWin_Add_Line_Path */
  void (*add_line_path)(BoxGWin *w, BoxPoint *a, BoxPoint *b);

  /** @see BoxGWin_Add_Join_Path */
  void (*add_join_path)(BoxGWin *w, BoxPoint *a, BoxPoint *b, BoxPoint *c);

  /** @see BoxGWin_Close_Path */
  void (*close_path)(BoxGWin *w);

  /** @see BoxGWin_Add_Circle_Path */
  void (*add_circle_path)(BoxGWin *w,
                          BoxPoint *ctr, BoxPoint *a, BoxPoint *b);

  /** @see BoxGWin_Set_Fg_Color */
  void (*set_fg_color)(BoxGWin *w, Color *c);

  /** @see BoxGWin_Set_Bg_Color */
  void (*set_bg_color)(BoxGWin *w, Color *c);

  /** @see BoxGWin_Set_Gradient */
  void (*set_gradient)(BoxGWin *w, ColorGrad *cg);

  /** @see BoxGWin_Set_Font */
  void (*set_font)(BoxGWin *w, const char *font_name);

  /** @see BoxGWin_Add_Text_Path */
  void (*add_text_path)(BoxGWin *w,
                        BoxPoint *ctr, BoxPoint *right, BoxPoint *up,
                        BoxPoint *from, const char *text);

  /** @see BoxGWin_Add_Fake_Point */
  void (*add_fake_point)(BoxGWin *w, BoxPoint *p);

  /** @see BoxGWin_Save_To_File */
  int (*save_to_file)(BoxGWin *w, const char *file_name);

  /** @see BoxGWin_Interpret_Obj */
  BoxTask (*interpret)(BoxGWin *w, BoxGObj *commands, BoxGWinMap *map);

  /** If set to 1, inhibits error messages */
  int quiet;

  /* Low level functions (should probably be moved out of here) */

  /** terminates drawing operations on the window and make it unusable.
   * @see BoxGWin_Finish, BoxGWin_Destroy
   */
  void (*finish)(BoxGWin *w);

  /** @see BoxGWin_Set_Color */
  void (*set_color)(BoxGWin *w, int col);

  /** @see BoxGWin_Draw_Point */
  void (*draw_point)(BoxGWin *w, Int ptx, Int pty);

  /** @see BoxGWin_Draw_Hor_Line */
  void (*draw_hor_line)(BoxGWin *w, Int y, Int x1, Int x2);

  /** Restore the window methods, which may have been changed after an error
   * has occurred.
   */
  void (*repair)(BoxGWin *w);

  /** This function can be internally used to report errors. */
  void (*_report_error)(BoxGWin *w, const char *msg);

  void *ptr;           /**< Pointer to the window data */
  void *data;          /**< Pointer to extra data dependent on the window
                            type */

  BoxReal
    ltx, lty,          /**< Coordinates of top left-corner (in mm)*/
    rdx, rdy,          /**< Coordinates of bottom right-corner (in mm) */
    minx, miny,        /**< Minimum coordinates (in mm) */
    maxx, maxy,        /**< Maximum coordinates (in mm) */
    lx, ly,            /**< Width and height (in mm, positive) */
    versox, versoy,    /**< Unit vectors x and y (either +1 or -1) */
    stepx, stepy,      /**< Coordinate variations to move a pixel on the left
                            and on the bottom */
    resx, resy;        /**< x and y resolution (points per mm) */

  Int numptx, numpty;  /**< Number of points in each direction */
  palitem *bgcol;      /**< Background color */
  palitem *fgcol;      /**< Current foreground color */
  palette *pal;        /**< Color palette */
  long bitperpixel;    /**< Bit necessary to store a single pixel */
  long bytesperline;   /**< Bytes required for each row */
  long dim;            /**< Total size (in bytes) of the window */
};

#define BoxGWin_Fail(source, msg)

/* Define some macros to provide functions which can be called as one would
 * normally expect: Method_Of_Obj(obj, arg1, arg2, ...) rather than
 * obj->method_name(obj, ...).
 */

/****************************************************************************
 * HIGH LEVEL ROUTINES
 */

/** Initialize the window for drawing */
#define BoxGWin_Begin_Drawing(win) ((win)->begin_drawing)(win)

/** Finalize the window */
#define BoxGWin_Finish(win) ((win)->finish)(win)

/** Create a new path (rreset) */
#define BoxGWin_Create_Path(win) ((win)->create_path)(win)

/** Close the current path (rclose) */
#define BoxGWin_Close_Path(win) ((win)->close_path)(win)

/** Draw the current path using the given drawing style. (rdraw) */
#define BoxGWin_Draw_Path(win, style) ((win)->draw_path)((win), (style))

/** Generate the path for a line connecting the two points 'a' and 'b' */
#define BoxGWin_Add_Line_Path(win, a, b) \
  ((win)->add_line_path)((win), (a), (b))

/** Generate the join connecting point 'a' to 'c' with corner in 'b'. */
#define BoxGWin_Add_Join_Path(win, a, b, c) \
  ((win)->add_join_path)((win), (a), (b), (c))

/** Generate the path for a circle in the reference system identified by
 * the origin 'ctr' and the unit axis points 'a' and 'b'.
 */
#define BoxGWin_Add_Circle_Path(win, ctr, right, up) \
  ((win)->add_circle_path)((win), (ctr), (right), (up))

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
#define BoxGWin_Interpret_Obj(win, obj, map) \
  ((win)->interpret)((win), (obj), (map))

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
#define BoxGWin_Add_Text_Path(win, ctr, right, up, from, text) \
  ((win)->add_text_path)((win), (ctr), (right), (up), (from), (text))

/** Save the window to a file */
#define BoxGWin_Save_To_File(win, filename) \
  ((win)->save_to_file)((win), (filename))

/****************************************************************************
 * LOW LEVEL ROUTINES
 */

/** Draw a point. */
#define BoxGWin_Draw_Point(win, x, y) \
  ((win)->draw_point)((win), (x), (y))

/** Draw an horizontal line. */
#define BoxGWin_Draw_Hor_Line(win, y, x1, x2) \
  ((win)->draw_hor_line)((win), (y), (x1), (x2))

/** Finalize the window */
#define BoxGWin_Set_Color(win, col_index) \
  ((win)->set_color)((win), (col_index))

/****************************************************************************/

/* Per convertire in millimetri, radianti, punti per millimetro */
extern Real grp_tomm;
extern Real grp_torad;
extern Real grp_toppmm;

/* Window type (in BoxGWinPlan) */
typedef enum {WT_NONE=-1, WT_BM1=0, WT_BM4, WT_BM8, WT_FIG,
              WT_PS, WT_EPS, WT_A1, WT_A8, WT_RGB24, WT_ARGB32,
              WT_PDF, WT_SVG, WT_MAX} WT;

/** Get the window ID (an integer number) from the window type (a string) */
int BoxGWin_Type_From_String(const char *type_str);

/** Create an invalid BoxGWin object. All BoxGWin object should be constructed
 * by extending an invalid BoxGWin object (and should thus call the function
 * below in their constructors).
 */
BoxGWin *BoxGWin_Create_Invalid(BoxGErr *err);

/** Destroy a BoxGWin object created with BoxGWin_Create_Invalid. All BoxGWin
 * destructors should be implemented by calling this function.
 */
void BoxGWin_Destroy(BoxGWin *w);

/** Unified function to open any kind of window */
BoxGWin *BoxGWin_Create(BoxGWinPlan *plan);

/** Make sure the components of 'c' lie inside [0, 1] and correct them,
 * if needed.
 */
void Color_Trunc(Color *c);

/* Dichiarazioni delle procedure della libreria */
/* Funzioni grafiche di alto livello */
BoxGWin *BoxGWin_Create_BM1(BoxReal ltx, BoxReal lty, BoxReal rdx, BoxReal rdy,
                            BoxReal resx, BoxReal resy);
BoxGWin *BoxGWin_Create_BM4(BoxReal ltx, BoxReal lty, BoxReal rdx, BoxReal rdy,
                            BoxReal resx, BoxReal resy);
BoxGWin *BoxGWin_Create_BM8(BoxReal ltx, BoxReal lty, BoxReal rdx, BoxReal rdy,
                            BoxReal resx, BoxReal resy);
BoxGWin *BoxGWin_Create_Fig(int numlayers);
BoxGWin *BoxGWin_Create_PS(const char *file);
BoxGWin *BoxGWin_Create_EPS(const char *file, BoxReal x, BoxReal y);
int ps_save_fig(const char *file_name, BoxGWin *figure);
int eps_save_fig(const char *file_name, BoxGWin *figure);

/** Type of function called when BoxGWin_Block_With has been used. */
typedef void (*BoxGOnError)(BoxGWin *w, const char *where);

/** Block the window 'w', such that it reports errors when used. */
void BoxGWin_Block(BoxGWin *w);

/** Similar to BoxGWin_Block, but when the window 'w' is used,
 * the function on_error is called, instead of reporting an error.
 */
void BoxGWin_Block_With(BoxGWin *w, BoxGOnError on_error);

/** Restore the window 'w', after it has been blocked with BoxGWin_Block
 * or BoxGWin_Block_With
 */
#define BoxGWin_Repair(w) \
  (w)->repair(w)

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

/* Procedure per la gestione di una palette */
void grp_color_build(Color *cb, ColorBytes *c);
void grp_color_reduce(palette *p, ColorBytes *c);
palette *grp_palette_build(long numcol, long hashdim, long hashmul, int reduce);
palitem *grp_color_find(palette *p, ColorBytes *c);
palitem *grp_color_request(palette *p, ColorBytes *c);
int grp_palette_transform(palette *p, int (*operation)(palitem *pi));
void grp_palette_destroy(palette *p);

/** Draw a path encoded inside a GPath object. */
void BoxGWin_Draw_GPath(BoxGWin *w, GPath *gp);

void rst_repair(BoxGWin *gw);

Point *grp_ref(Point *o, Point *v, Point *p);

/* Costanti per la conversione */
#define grp_mmperinch  25.4
#define grp_radperdeg  0.01745329252
#define grp_radpergrad  0.01570796327
#define grp_ppmmperdpi  0.03937007874
#define grp_mm_per_inch 25.4
#define grp_inch_per_psunit (1.0/72.0)

/* Conversione da coordinate relative a coordinate assolute (floating) */
#define CV_XF_A(w, x)  (((Real) (x) - (w)->ltx)/(w)->stepx)
#define CV_YF_A(w, y)  (((Real) (y) - (w)->lty)/(w)->stepy)
/* Conversione di lunghezze relative in lunghezze assolute */
#define CV_LXF_A(w, x)  (((Real) (x))/(w)->stepx)
#define CV_LYF_A(w, y)  (((Real) (y))/(w)->stepy)
/* Conversione da coordinate assolute a coordinate intermedie */
#define CV_A_MED(x)  ((int) floor(x) + (int) ceil(x))
/* Conversione da coordinate intermedie a coordinate intere */
#define CV_MED_GT(x)  (((Int) (x) + 1) >> 1)
#define CV_MED_LW(x)  (((Int) (x) - 1) >> 1)

/* Restituisce 1 se la coordinata intermedia è un intero esatto, 0 altrimenti */
#define IS_EXACT_INT(x)  ((Int) x & 1)

/* Informazioni sulla corrente finestra grafica */
#define GRP_MLOUT    -1
#define GRP_MROUT    0x7fff

/* Costanti matematiche */
#define pigreco 3.14159265358979323846

#endif
