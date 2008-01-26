/* graphic.h - Autore: Franchin Matteo - 26 dicembre 2002
 *
 * Questo file contiene le dichiarazioni di strutture, macro e procedure
 * grafiche non dipendenti dal modo in cui i dati relativi all'immagine
 * vengono memorizzati(bit per pixel).
 */

#ifndef _GRAPHIC_H
#  define _GRAPHIC_H

#  include "types.h"

/* Definisco i "tipi" necessari per la grafica */
#define ICOOR  Int
#define FCOOR  Real

/****** DEFINIZIONE DELLE STRUTTURE NECESSARIE PER LA GRAFICA ******/

/* Definisce la struttura adatta a contenere il colore di un punto */
typedef struct {
  unsigned char r;  /* Componente rossa */
  unsigned char g;  /* Componente verde */
  unsigned char b;  /* Componente blu */
} color;

/* Questa e' la struttura di un elemento della palette */
struct palitem {
  long index;    /* Numero del colore nella tavolazza */
  color c;    /* Colore */
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

/* Descrittore di una finestra grafica */
typedef struct {
  void *ptr;        /* Puntatore alla zona di memoria della finestra */
  FCOOR ltx, lty;      /* Coordinate dell'angolo in alto a sinistra (in mm)*/
  FCOOR rdx, rdy;      /* Coordinate dell'angolo in basso a destra (in mm) */
  FCOOR minx, miny;    /* Coordinate minime visualizzabili (in mm) */
  FCOOR maxx, maxy;    /* Coordinate massime visualizzabili (in mm) */
  FCOOR lx, ly;      /* Larghezza e altezza (in mm, sono positive)*/
  FCOOR versox, versoy;  /* Versori x e y (numeri uguali a +1 o -1) */
  FCOOR stepx, stepy;    /* Salto fra un punto e quello alla sua sinistra e in basso */
  FCOOR resx, resy;    /* Risoluzione x e y (in punti per mm) */
  ICOOR numptx, numpty;  /* Numero di punti nelle due direzioni */
  palitem *bgcol;      /* Colore dello sfondo */
  palitem *fgcol;      /* Colore attualmente in uso */
  palette *pal;      /* Tavolazza dei colori relativi alla finestra */
  long bitperpixel;    /* Numero di bit necessari a memorizzaze 1 pixel */
  long bytesperline;    /* Byte occupati da una linea */
  long dim;        /* Dimensione totale in byte della finestra */
  void *wrdep;      /* Puntatore alla struttura dei dati dipendenti dalla scrittura */

  /* Puntatore alle procedura per salvare la finestra su disco */
  int (*save)();

  /* Puntatore alle procedure di basso livello per gestire la finestra */
  void (**lowfn)();

  /* Puntatore alle procedure di medio livello per gestire la finestra */
  void (**midfn)();

} grp_window;

/* Dati importanti per la libreria */
/* Finestra attualmente in uso */
extern grp_window *grp_win;
/* Una finestra che da solo errori (definita in graphic.c): */
extern grp_window grp_empty_win;

/* Per convertire in millimetri, radianti, punti per millimetro */
extern FCOOR grp_tomm;
extern FCOOR grp_torad;
extern FCOOR grp_toppmm;

/* Dichiarazioni delle procedure della libreria */
/* Funzioni grafiche di alto livello */
grp_window *gr1b_open_win(FCOOR ltx, FCOOR lty, FCOOR rdx, FCOOR rdy,
 FCOOR resx, FCOOR resy);
grp_window *gr4b_open_win(FCOOR ltx, FCOOR lty, FCOOR rdx, FCOOR rdy,
 FCOOR resx, FCOOR resy);
grp_window *gr8b_open_win(FCOOR ltx, FCOOR lty, FCOOR rdx, FCOOR rdy,
 FCOOR resx, FCOOR resy);
grp_window *fig_open_win(int numlayers);
grp_window *ps_open_win(char *file);


/* Procedure per la gestione di una palette */
void grp_color_build(Real r, Real g, Real b, color *c);
void grp_color_reduce(palette *p, color *c);
palette *grp_palette_build(long numcol, long hashdim, long hashmul, int reduce);
palitem *grp_color_find(palette *p, color *c);
palitem *grp_color_request(palette *p, color *c);
int grp_palette_transform(palette *p, int (*operation)(palitem *pi));
void grp_palette_destroy(palette *p);
/* Procedure per la creazione di linee */
void grp_join_style(FCOOR *userjs);
void grp_cutting(FCOOR c);
void grp_first_line(FCOOR x1, FCOOR y1, FCOOR sp1,
 FCOOR x2, FCOOR y2, FCOOR sp2, FCOOR startlenght, int is_closed);
void grp_last_line(double lastlenght, int is_closed);
void grp_next_line(double x, double y, double sp1, double sp2, int style);
int grp_intersection(Point *p1, Point *d1, Point *p2, Point *d2,
 double *alpha1);
int grp_intersection2(Point *p1, Point *d1, Point *p2, Point *d2,
 double *alpha1, double *alpha2);
Point *grp_ref(Point *o, Point *v, Point *p);

/* Funzioni grafiche di basso livello (legate al tipo di finestra aperta) */
#define grp_close_win(p1)      (grp_win->lowfn[0])(p1)
#define grp_set_col(p1)        (grp_win->lowfn[1])(p1)
#define grp_draw_point(p1, p2)    (grp_win->lowfn[2])(p1, p2)
#define grp_hor_line(p1, p2, p3)  (grp_win->lowfn[3])(p1, p2, p3)
#define grp_save(s)          (grp_win->save)(s)

/* Funzioni grafiche di medio livello (di rasterizzazione) */
#define grp_rreset      (grp_win->midfn[0])
#define grp_rinit      (grp_win->midfn[1])
#define grp_rdraw      (grp_win->midfn[2])
#define grp_rline      (grp_win->midfn[3])
#define grp_rcong      (grp_win->midfn[4])
#define grp_rcurve      (grp_win->midfn[5])
#define grp_rcircle      (grp_win->midfn[6])
#define grp_rfgcolor    (grp_win->midfn[7])
#define grp_rbgcolor    (grp_win->midfn[8])

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


/* Conversione da coordinate relative a coordinate assolute (floating) */
#define CV_XF_A(x)  (((FCOOR) x - grp_win->ltx)/grp_win->stepx)
#define CV_YF_A(y)  (((FCOOR) y - grp_win->lty)/grp_win->stepy)
/* Conversione di lunghezze relative in lunghezze assolute */
#define CV_LXF_A(x)  (((FCOOR) x)/grp_win->stepx)
#define CV_LYF_A(y)  (((FCOOR) y)/grp_win->stepy)
/* Conversione da coordinate assolute a coordinate intermedie */
#define CV_A_MED(x)  ((int) floor(x) + (int) ceil(x))
/* Conversione da coordinate intermedie a coordinate intere */
#define CV_MED_GT(x)  (((ICOOR) x + 1) >> 1)
#define CV_MED_LW(x)  (((ICOOR) x - 1) >> 1)

/* Restituisce 1 se la coordinata intermedia è un intero esatto, 0 altrimenti */
#define IS_EXACT_INT(x)  ((ICOOR) x & 1)

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
