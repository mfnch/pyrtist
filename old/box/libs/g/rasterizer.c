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

/* rasterizer.c - ottobre 2002
 *
 * Questo file contiene quanto basta per poter ...
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

/* De-commentare per includere i messaggi di debug */
/*#define DEBUG*/

#include "error.h"
#include "graphic.h"
#include "g.h"

/* Conversione da coordinate relative a coordinate assolute (floating) */
#define CV_XF_A(w, x)  (((BoxReal) (x) - (w)->ltx)/(w)->stepx)
#define CV_YF_A(w, y)  (((BoxReal) (y) - (w)->lty)/(w)->stepy)
/* Conversione di lunghezze relative in lunghezze assolute */
#define CV_LXF_A(w, x)  (((BoxReal) (x))/(w)->stepx)
#define CV_LYF_A(w, y)  (((BoxReal) (y))/(w)->stepy)
/* Conversione da coordinate assolute a coordinate intermedie */
#define CV_A_MED(x)  ((int) floor(x) + (int) ceil(x))
/* Conversione da coordinate intermedie a coordinate intere */
#define CV_MED_GT(x)  (((BoxInt) (x) + 1) >> 1)
#define CV_MED_LW(x)  (((BoxInt) (x) - 1) >> 1)

/* Informazioni sulla corrente finestra grafica */
#define GRP_MLOUT    -1
#define GRP_MROUT    0x7fff

#define RSTBLOCKDIM    16384
#define MAXROWPERBLOCK  8192

#define WORD      unsigned short
#define SWORD     short

#define GRP_AXMIN(w) (0)
#define GRP_AXMAX(w) ((w)->numptx - 1)
#define GRP_AYMIN(w) (0)
#define GRP_AYMAX(w) ((w)->numpty - 1)
#define GRP_IXMIN(w) (0)
#define GRP_IXMAX(w) ((w)->numptx - 1)
#define GRP_IYMIN(w) (0)
#define GRP_IYMAX(w) ((w)->numpty - 1)

#define GRP_IXMIN(w) (0)
#define GRP_IXMAX(w) ((w)->numptx - 1)
#define GRP_IYMIN(w) (0)
#define GRP_IYMAX(w) ((w)->numpty - 1)

#define WINNUMROW(w) ((w)->numpty)

struct block_desc {
  SWORD  ymin;    /* Coordinata y corrispondente alla prima riga descritta */
  SWORD  ymax;    /* Coordinata y corrispondente all'ultima riga descritta */
  WORD  dim;    /* Dimensioni totali del buffer in WORD */
  WORD  *buffer;  /* Puntatore al buffer */
  WORD  numfree;  /* Numero di "posti" (cioe' WORD) ancora liberi */
  WORD  winext;    /* Posizione in WORD del prossimo posto libero */
  WORD  *inext;    /* Puntatore al prossimo posto libero */
  struct block_desc *next;  /* Puntatore al prossimo blocco */
};

typedef struct block_desc block_desc;

block_desc *first = 0;

/* Procedure esterne da error.c */
extern void err_print(FILE *stream);

/***************************************************************************************/
/* MACRO DEFINITE IN QUESTO FILE */

/* Questa macro viene utilizzata per inserire un marcatore di bordo
 * (nelle procedure rst__line e rst__cong) e serve a tenere conto
 * degli eventuali "sconfinamenti" del bordo rispetto allo schermo.
 */
#define MARK(w, y, x) \
  if ((x) < GRP_AXMIN(w)) \
    rst__mark((w), (y), GRP_MLOUT); \
  else if (x > GRP_AXMAX(w)) \
    rst__mark((w), (y), GRP_MROUT); \
  else \
    rst__mark((w), (y), CV_A_MED(x))

/***************************************************************************************/
/* DICHIARAZIONE DELLE PROCEDURE DEFINITE IN QUESTO FILE */

/* Le procedure che iniziano con rst__ sono procedure riservate
 * scritte per essere utilizzate solo da altre procedure di questo file
 */
void rst__block_reset(block_desc *rstblock);
block_desc *rst__trytomark(BoxGWin *w, SWORD y, SWORD x);
void rst__mark(BoxGWin *w, SWORD y, SWORD x);
void rst__line(BoxGWin *w, BoxPoint *pa, BoxPoint *pb);
void rst__cong(BoxGWin *w, BoxPoint *pa, BoxPoint *pb, BoxPoint *pc);
void rst__curve(BoxGWin *w, BoxPoint *pa, BoxPoint *pb, BoxPoint *pc, BoxReal c);
void rst__poly(BoxGWin *w, BoxPoint *p, int n);

/* Le procedure "di libreria" definite per essere chiamate dall'esterno
 * iniziano con rst_
 */
static void My_Add_Create_Path(BoxGWin *w);
static void My_Begin_Drawing(BoxGWin *w);
static void My_Draw_Path(BoxGWin *w, DrawStyle *style);
static void My_Add_Line_Path(BoxGWin *w, BoxPoint *a, BoxPoint *b);
static void My_Add_Join_Path(BoxGWin *w, BoxPoint *a, BoxPoint *b, BoxPoint *c);
static void My_Close_Path(BoxGWin *w);
void rst_curve(BoxGWin *w, BoxPoint *a, BoxPoint *b, BoxPoint *c, BoxReal cut);
static void My_Add_Circle_Path(BoxGWin *w, BoxPoint *ctr, BoxPoint *a, BoxPoint *b);
void rst_poly(BoxGWin *w, BoxPoint *p, int n);
static void My_Fg_Color(BoxGWin *w, Color *c);
static void My_Bg_Color(BoxGWin *w, Color *c);
static void My_Add_Fake_Point(BoxGWin *w, BoxPoint *p);

void rst_repair(BoxGWin *w) {
  w->create_path = My_Add_Create_Path;
  w->begin_drawing = My_Begin_Drawing;
  w->draw_path = My_Draw_Path;
  w->add_line_path = My_Add_Line_Path;
  w->add_join_path = My_Add_Join_Path;
  w->close_path = My_Close_Path;
  w->add_circle_path = My_Add_Circle_Path;
  w->set_fg_color = My_Fg_Color;
  w->set_bg_color = My_Bg_Color;
  w->add_fake_point = My_Add_Fake_Point;
}

/***************************************************************************************/
/* PROCEDURE PER LA RASTERIZZAZIONE DI POLIGONI ED ALTRE FIGURE */

/* NOME: rst__block_reset
 * DESCRIZIONE: Resetta il blocco di rasterizzazione specificato.
 *  Dunque il blocco viene svuotato e il suo contenuto va perso.
 */
void rst__block_reset(block_desc *rstblock) {
  WORD y;
  WORD *ptr;

  y = rstblock->ymax - rstblock->ymin + 1;
  rstblock->numfree = RSTBLOCKDIM - y;
  rstblock->winext = RSTBLOCKDIM - 1;
  rstblock->inext = rstblock->buffer + (WORD) rstblock->winext;

  for(ptr = rstblock->buffer; y-- > 0; ptr++)
    *ptr = (WORD) 0;
}

/* NOME: My_Add_Create_Path
 * DESCRIZIONE: Pulisce il buffer di rasterizzazione,
 *  senza deallocare memoria.
 */
static void My_Add_Create_Path(BoxGWin *w) {
  block_desc *rstblock;

  /* Facciamo un loop su tutti i blocchi */
  for(rstblock = first; rstblock != (block_desc *) 0; rstblock = rstblock->next)
    rst__block_reset(rstblock);  /* Puliamo ciascun blocco */
}

/* NOME: My_Begin_Drawing
 * DESCRIZIONE: Prepara il sistema nel suo stato iniziale,
 *  pronto per rasterizzare.
 */
static void My_Begin_Drawing(BoxGWin *w) {
  int i, numblockmin, numblockalc = 0;
  WORD ymin, ymax;
  WORD *bufptr;
  block_desc *rstblock;

  ymax = -1;
  numblockmin = (WINNUMROW(w) + MAXROWPERBLOCK - 1) / MAXROWPERBLOCK;

  /* Ora conto i blocchi fin'ora allocati */
  for(rstblock = first; rstblock != (block_desc *) 0; rstblock = rstblock->next)
    ++numblockalc;

  if (numblockalc < numblockmin) {
    /* Devo allocare altri blocchi */
    numblockalc = numblockmin - numblockalc;

    for (i=0; i<numblockalc; i++) {
      rstblock = (block_desc *) malloc( sizeof(block_desc) );
      bufptr = (WORD *) malloc( RSTBLOCKDIM * sizeof(WORD) );

      if ( (rstblock == (block_desc *) 0) || (bufptr == (WORD *) 0) ) {
        ERRORMSG("My_Begin_Drawing", "Memoria esaurita");
        return;
      }

      rstblock->buffer = bufptr;
      rstblock->dim = RSTBLOCKDIM;
      rstblock->next = first;

      first = rstblock;
    }
  } else if (numblockalc > numblockmin) {
    /* Ho blocchi in piu': li dealloco */
    numblockalc -= numblockmin;

    for (i=0; i<numblockalc; i++) {
      rstblock = first;
      first = rstblock->next;
      free(rstblock->buffer);
      free(rstblock);
    }
  }

  /* Adesso ho un numero giusto di blocchi allocati.
   * Devo assegnare loro le righe e pulirli.
   */
  ymax = -1;
  rstblock = first;
  for(i=0; i<numblockalc; i++) {
    ymin = ymax + 1;
    ymax += MAXROWPERBLOCK;
    if (ymax < WINNUMROW(w)) {
      rstblock->ymin = ymin;
      rstblock->ymax = ymax;
      rst__block_reset(rstblock);

      rstblock = rstblock->next;
    } else {
      rstblock->ymin = ymin;
      rstblock->ymax = WINNUMROW(w) - 1;
      rst__block_reset(rstblock);
    }
  }
}

/* NOME: rst__trytomark
 * DESCRIZIONE: Prova ad inserire un bordo alla riga y, in ascissa x
 *  Se non c'e' abbastanza memoria per farlo rinuncia
 *  e restituisce il puntatore al blocco pieno, in cui andava
 *  inserito il marcatore di bordo, altrimenti restituisce 0.
 *  In tal caso, a meno che non si sia verificato un errore,
 *  l'operazione e' stata completata con successo.
 */
block_desc *rst__trytomark(BoxGWin *w, SWORD y, SWORD x) {
  WORD *ptr, *newptr;
  block_desc *rstblock;

  /* Cerchiamo la riga y fra quelle presenti nei blocchi */
  for(rstblock = first; rstblock != NULL; rstblock = rstblock->next) {
    if (y >= rstblock->ymin && y <= rstblock->ymax) {
      /* Ho trovato il blocco che "contiene" la riga y.
       * Ora controllo se questo blocco pu contenere un altro bordo
       */
      if (rstblock->numfree < 2) {
        /* Il blocco e' pieno: rinuncio all'operazione */
        return rstblock;

      } else {
        /* C'e' spazio per inserire un altro marcatore di bordo.
         * Allora faccio un loop per vedere a che punto della riga
         * questo debba essere inserito.
         */
        y -= rstblock->ymin;
        for (newptr = (WORD *) rstblock->buffer + (WORD) y;
             *newptr != (WORD) 0; newptr = ptr) {
          ptr = (WORD *) rstblock->buffer + (WORD) *newptr;
          if (*(ptr+1) > x)
            break;
        }

        /* ptr punta al marcatore in seguito al quale
         * devo aggiungere quello nuovo
         */
        *(rstblock->inext--) = x;
        *(rstblock->inext--) = *newptr;
        *newptr = --rstblock->winext;
        --rstblock->winext;
        rstblock->numfree -= 2;
      }

      return NULL;
    }
  }

  ERRORMSG("rst__trytomark", "Nessun blocco contiene la riga y");
  return NULL;
}

/* NOME: rst__mark
 * DESCRIZIONE: Inserisce un marcatore di bordo alla riga y, colonna x.
 *  Se non c'e' abbastanza memoria nella riga, alloca nuova memoria
 * in modo da poter concludere l'operazione.
 */
void rst__mark(BoxGWin *w, SWORD y, SWORD x) {
  block_desc *rstblock;

  /* Se la libreria di rasterizzazione non e' stata ancora inizializzata
   * allora provvediamo subito!
   */
  if (first == NULL)
    My_Begin_Drawing(w);

  rstblock = rst__trytomark(w, y, x);
  if (rstblock != NULL) {
    /* In questo caso non c'e' spazio sufficiente nel blocco rstblock
     * per inserire un nuovo marcatore di bordo: dobbiamo allocare
     * un nuovo blocco.
     */
    WORD y;
    WORD *bufptr, *row1, *row2, *col1;
    block_desc *newrstblock;

    newrstblock = (block_desc *) malloc( sizeof(block_desc) );
    bufptr = (WORD *) malloc( RSTBLOCKDIM * sizeof(WORD) );

    if (newrstblock == NULL || bufptr == NULL) {
      ERRORMSG("rst_mark", "Memoria esaurita");
      return;
    }

    newrstblock->buffer = bufptr;
    newrstblock->dim = RSTBLOCKDIM;
    newrstblock->winext = RSTBLOCKDIM - 1;
    newrstblock->inext = rstblock->buffer + (WORD) rstblock->winext;

    /* Ora copiamo il vecchio blocco nel nuovo
     * e cosi' facendo lo riordiniamo
     */
    row1 = rstblock->buffer;
    row2 = newrstblock->buffer;

    /* Facciamo un loop su tutte le righe contenute nel blocco */
    for(y = rstblock->ymin; y <= rstblock->ymax; y++) {
      row2++;
      for (col1 = row1++; *col1 != (WORD) 0;
           col1 = (WORD *) rstblock->buffer + (WORD) *col1) {}
    }

    ERRORMSG("rst_mark", "Feature in fase di implementazione");
  }
}

/* NOME: My_Draw_Path
 * DESCRIZIONE: Legge il contenuto del buffer di rasterizzazione
 *  e traccia il disegno corrispondente sulla finestra grafica.
 */
static void My_Draw_Path(BoxGWin *w, DrawStyle *style) {
  int border;
  FillStyle fs = style->fill_style;
  SWORD y, x1=0, x2, xmin;
  WORD *row, *col;
  block_desc *rstblock;
  static int msg_already_displayed = 0;

  switch(fs) {
  case FILLSTYLE_VOID: return;
  case FILLSTYLE_EO: break;
  default:
    if (! msg_already_displayed) {
      g_warning("Unsupported drawing style: using even-odd fill algorithm!");
      msg_already_displayed = 1;
    }
    fs = FILLSTYLE_EO;
    break;
  }

  if (style->bord_width > 0.0) {
    g_warning("Unsupported drawing style: border cannot be traced!");
  }

  /* Facciamo un loop in modo da considerare tutti i blocchi */
  for(rstblock = first; rstblock != (block_desc *) 0;
      rstblock = rstblock->next) {
    /* Facciamo un loop su tutte le righe contenute nel blocco */
    row = rstblock->buffer;
    for(y = rstblock->ymin; y <= rstblock->ymax; y++) {
      /* Ora disegnamo ognuna delle righe */
      xmin = 0;
      if (fs == FILLSTYLE_EO) {
        border = 0;
        for (col = row++; *col != (WORD) 0;) {
          col = (WORD *) rstblock->buffer + (WORD) *col;

          if (border == 1) {
            /* Bordo destro: traccio la linea */
            x2 = CV_MED_LW(*(col+1));

            if (x2 >= xmin) {
              BoxGWin_Draw_Hor_Line(w, y, x1, x2);
              xmin = x2 + 1;
            }
          } else {
            /* Bordo sinistro */
            x1 = CV_MED_GT(*(col+1));
            if (x1 > xmin) xmin = x1;
          }
          border ^= 1;
        }

      } else {
        int num = 0;
        /* Plain fill */
        for (col = row++; *col != (WORD) 0;) {
          col = (WORD *) rstblock->buffer + (WORD) *col;
          if (num == 0)
            x1 = CV_MED_GT(*(col+1));
          else
            x2 = CV_MED_GT(*(col+1));
          ++num;
        }

        if (num > 1)
          BoxGWin_Draw_Hor_Line(w, y, x1, x2);
      }
    }
  }
}

/* NOME: My_Add_Line_Path
 * DESCRIZIONE: Rasterizza la linea congiungente i due punti a e b.
 * NOTA: Le coordinate dei punti sono coordinate relative.
 */
static void My_Add_Line_Path(BoxGWin *w, BoxPoint *a, BoxPoint *b) {
  BoxPoint ia, ib;
  ia.x = CV_XF_A(w, a->x); ia.y = CV_YF_A(w, a->y);
  ib.x = CV_XF_A(w, b->x); ib.y = CV_YF_A(w, b->y);

  rst__line(w, & ia, & ib);
}

/* NOME: rst__line
 * DESCRIZIONE: Rasterizza la linea congiungente i due punti *pa e *pb.
 * NOTA: Le coordinate dei punti sono coordinate assolute.
 */
void rst__line(BoxGWin *w, BoxPoint *pa, BoxPoint *pb) {
  BoxReal ly;

  if (pa->y > pb->y) { register BoxPoint *tmp; tmp = pa; pa = pb; pb = tmp; }

  ly = pb->y - pa->y;
  if (ly < 0.95) {
    /* In tal caso ho solo un valore di y, oppure nessuno */
    BoxInt y1, y2;

    if (pb->y < GRP_AYMIN(w) || pa->y > GRP_AYMAX(w)) return;

    y1 = CV_MED_GT(CV_A_MED(pa->y));
    y2 = CV_MED_LW(CV_A_MED(pb->y));

    if (y1 == y2) {
      /* Solo in questo caso la retta "si vede" */
      BoxReal x;

      x = pa->x + ((((BoxReal) y1) - pa->y)/ly)*(pb->x - pa->x);

      MARK(w, y1, x);
    }

  } else {
    /* In tal caso la retta ha una inclinazione rilevante */
    BoxInt y1, y2, y;
    BoxReal a, b, x;

    /* Esco in caso di linea non visibile (sopra o sotto) */
    if (pb->y < GRP_AYMIN(w) || pa->y > GRP_AYMAX(w)) return;

    a = (pb->x - pa->x)/ly;
    b = pa->x - pa->y * a;

    if (pa->y < GRP_AYMIN(w))
          y1 = GRP_IYMIN(w);
        else
      y1 = CV_MED_GT(CV_A_MED(pa->y));

    if (pb->y > GRP_AYMAX(w))
      y2 = GRP_IYMAX(w);
    else
      y2 = CV_MED_LW(CV_A_MED(pb->y));

    x = b + a*((BoxReal) y1);
    for(y = y1; y <= y2; y++) {

      MARK(w, y, x);

      x += a;
    }
  }
}

/* NOME: My_Add_Join_Path
 * DESCRIZIONE: Rasterizza una curva tipo "quarto di cerchio stirato".
 *  Dati 3 punti a, b, c questo tipo di curva e' un quarto di cerchio
 *  stirato lungo il parallelogramma che ha per vertici a, b, c (il quarto
 *  vertice del parallelogramma e' determinato dai 3 precedenti) in modo
 *  tale che il cerchio parte dal punto a si dirige verso b (senza toccarlo)
 *  e curva finendo in c.
 * NOTA: Le coordinate dei punti sono coordinate relative.
 */
static void My_Add_Join_Path(BoxGWin *w, BoxPoint *a, BoxPoint *b, BoxPoint *c) {
  BoxPoint ia, ib, ic;
  ia.x = CV_XF_A(w, a->x);
  ia.y = CV_YF_A(w, a->y);
  ib.x = CV_XF_A(w, b->x);
  ib.y = CV_YF_A(w, b->y);
  ic.x = CV_XF_A(w, c->x);
  ic.y = CV_YF_A(w, c->y);

  rst__cong(w, & ia, & ib, & ic);
}

static void My_Close_Path(BoxGWin *w) {}

/* NOME: rst__cong
 * DESCRIZIONE: Come My_Add_Join_Path, ma utilizza coordinate assolute.
 */
void rst__cong(BoxGWin *w, BoxPoint *pa, BoxPoint *pb, BoxPoint *pc) {
  BoxInt iymin, iymax, iy;
  BoxReal ymin, ymax;
  BoxReal v01x, v01y, v21x, v21y, v02x, v02y, h;

  /* Comincio col trovare la massima proiezione della curva
   * sull'asse y, cioe' l'intervallo di ordinate in cui stanno sicuramente
   * tutti i punti della curva: basta a tal proposito che l'intervallo
   * contenga le ordinate dei tre punti p[0], p[1], p[2]
   */
  if (pa->y < pb->y) {
    ymin = pa->y;
    ymax = pb->y;
  } else {
    ymin = pb->y;
    ymax = pa->y;
  }

  if (pc->y > ymax)
    ymax = pc->y;
  else if (pc->y < ymin)
    ymin = pc->y;

  /* Se l'intervallo sta sopra o sotto lo "schermo" e' come se non ci fosse */
  if (ymax < GRP_AYMIN(w) || ymin > GRP_AYMAX(w)) return;

  /* Devo per tagliare l'eventuale parte "invisibile" dell'intervallo */
  if (ymin < GRP_AYMIN(w)) ymin = GRP_AYMIN(w);
  if (ymax > GRP_AYMAX(w)) ymax = GRP_AYMAX(w);

  /* Ora converto le coordinate in coordinate intere */
  iymin = CV_MED_GT(CV_A_MED(ymin));
  iymax = CV_MED_LW(CV_A_MED(ymax));

  /* Calcolo i due vettori che descrivono il parallelogramma
   * in cui e' inserita la curva, diretti "fuori dall'origine".
   */
  v21x = pb->x - pc->x;
  v21y = pb->y - pc->y;

  v01x = pb->x - pa->x;
  v01y = pb->y - pa->y;

  /* Calcolo il vettore 02 */
  v02x = pc->x - pa->x;
  v02y = pc->y - pa->y;

  /* Controllo che il parallelogramma non sia troppo stretto:
   * in tal caso lo posso sostituire con una retta.
   * Devo calcolare l'altezza del triangolo 012 di base 02
   * e questa altezza deve essere molto minore di 1
   * (noi consideriamo << 0.05).
   * NOTA: h e' questa altezza al quadrato per 4.
   * NOTA: i casi degeneri(punti allineati) rientrano
   *  nella categoria dell' "h stretto"
   */
  h = pow(v21x * v01y - v21y * v01x, 2.0)/(v02x*v02x + v02y*v02y);
  if (h < 0.01) {
    rst__line(w, pa, pc);
    return;
  }

  {
  /* Ora mi calcolo i parametri a, b, c della generica retta orizzontale
   * che ha equazione ax + by + c = 0 scritta nelle coordinate
   * normalizzate (x, y). Questo fascio di rette si ottiene al variare del
   * solo parametro c, mentre a e b rimangono inalterati.
   * Detto 3 il verice del parallelogramma 0123, (x, y) sono le coordinate
   * del sistema di riferimento (non ortonormale!!!) con origine in 3
   * e versori v01 e v21
   */
  BoxReal a, b, c, c2, cstep, s1mc2;
  BoxReal i1x, i1y, i2x, i2y, rx, ry, x1, x2;

  /* cstep e' il valore da sommare a c per passare alla retta orizzontale
   * tipo Y = k alla retta orizzontale Y = k+1
   * (X, Y) sono le usuali coordinate di schermo (mentre (x, y) ...)
   */
  cstep = 1.0/sqrt(v01y*v01y + v21y*v21y);
  a = -v01y * cstep;
  b = -v21y * cstep;

  /* Parto dalla retta orizzontale Y = iymin */
  c = (v21y - pa->y + ((BoxReal) iymin))*cstep;

  for (iy = iymin; iy <= iymax; iy++) {
    c2 = c*c;

    if (c2 <= 1.0) {
      s1mc2 = sqrt(1.0 - c2);
      i1x = i2x = -a*c;
      i1y = i2y = -b*c;
      rx = -b * s1mc2;
      ry = a * s1mc2;

      /* Ecco i 2 punti di intersezione */
      if ( ((i1x += rx) < 0.0) || ((i1y += ry) < 0.0) ) {
        i2x -= rx; i2y -= ry;
        if ( (i2x >= 0.0) && (i2y >= 0.0) ) {
          x1 = pa->x + v01x*i2x + v21x*(i2y - 1.0);
          MARK(w, iy, x1);
        }
      } else {
        if ( ((i2x -= rx) < 0.0) || ((i2y -= ry) < 0.0) ) {
          x1 = pa->x + v01x*i1x + v21x*(i1y - 1.0);
          MARK(w, iy, x1);
        } else {
          x1 = pa->x + v01x*i1x + v21x*(i1y - 1.0);
          x2 = pa->x + v01x*i2x + v21x*(i2y - 1.0);
          MARK(w, iy, x1);
          MARK(w, iy, x2);
        }
      }
    }

    c += cstep;
  }

  }
}

/* NOME: rst_curve
 * DESCRIZIONE: Rasterizza una curva tipo "quarto di cerchio stirato".
 *  La procedura e' molto simile a My_Add_Join_Path ma e' piu' generale.
 *  Infatti e' possibile specificare un ulteriore parametro cut, che chiamiamo
 *  parametro di taglio, che consente di "tagliare" la curva.
 *  Se questo e' 0 la rst_curve e' equivalente a My_Add_Join_Path, mentre
 *  man mano che esso si avvicina a 1 la curva si smorza sempre di piu'
 *  fino a trasformarsi nella retta che va dal punto a al c.
 *  Viceversa per cut negativi la curva aumenta la sua "spigolosita'"
 *  Fino a trasformarsi(per cut = -1) nella spezzata che congiunge in sequenza
 *  i punti a, b e c.
 * NOTA: Le coordinate dei punti sono coordinate relative.
 */
void rst_curve(BoxGWin *w, BoxPoint *a, BoxPoint *b, BoxPoint *c, BoxReal cut) {
  BoxPoint ia, ib, ic;
  ia.x = CV_XF_A(w, a->x);
  ia.y = CV_YF_A(w, a->y);
  ib.x = CV_XF_A(w, b->x);
  ib.y = CV_YF_A(w, b->y);
  ic.x = CV_XF_A(w, c->x);
  ic.y = CV_YF_A(w, c->y);

  rst__curve(w, & ia, & ib, & ic, cut);
}

/* NOME: rst__curve
 * DESCRIZIONE: Come rst_curve ma utilizza coordinate assolute.
 */
void rst__curve(BoxGWin *w, BoxPoint *pa, BoxPoint *pb, BoxPoint *pc, BoxReal c) {
  BoxReal v10x, v10y, v12x, v12y;
  BoxPoint q[5], *pq;

  if (c < -1.0) c = -1.0;
  if (c > 1.0) c = 1.0;

  c = ((sqrt(2.0)-1.5)*c*c + c/2 + 2 - sqrt(2));

  v10x = pa->x - pb->x;
  v10y = pa->y - pb->y;
  v12x = pc->x - pb->x;
  v12y = pc->y - pb->y;

  q[0].x = pa->x;
  q[0].y = pa->y;
  q[4].x = pc->x;
  q[4].y = pc->y;

  q[1].x = pb->x + v10x * c;
  q[1].y = pb->y + v10y * c;
  q[3].x = pb->x + v12x * c;
  q[3].y = pb->y + v12y * c;

  /* 2 e' il punto medio tra 1 e 3 */
  q[2].x = (q[1].x + q[3].x)/2.0;
  q[2].y = (q[1].y + q[3].y)/2.0;

  pq = q;
  rst__cong(w, pq, pq+1, pq+2);
  rst__cong(w, pq+3, pq+4, pq+5);
}

/* DESCRIZIONE: Rasterizza una curva tipo "cerchio stirato" (cioe'
 *  ellissi, infatti stirando una circonferenza - anche lungo direzioni
 *  non ortogonali tra loro - si ottiene sempre un'ellisse!)
 *  ctr e' il centro dell'ellisse, va e vb i vettori lungo cui la circonferenza
 *  viene stirata. Per intenderci, l'equazione della curva (espressa in funzione
 *  dell'angolo theta) sara':
 *     P(theta) = va * cos(theta) + vb * sin(theta)
 *  P(theta) e' il punto della curva corrispondente all'angolo theta, il quale
 *  varia tra 0 e 2 pigreco, va e vb sono i due vettori dati da:
 *   va = a - ctr  e  vb = b - ctr, dove a e b sono passati come argomenti
 *  a My_Add_Circle_Path.
 */
static void My_Add_Circle_Path(BoxGWin *w,
                               BoxPoint *pctr, BoxPoint *pa, BoxPoint *pb) {
  BoxPoint ctr, a, b;
  BoxInt iy, y1, y2;
  BoxReal xmin, xmax, ymin, ymax;
  BoxReal C, C2, D, k1, k2, x, dx, y;

  a.x = CV_XF_A(w, pa->x - pctr->x);
  a.y = CV_YF_A(w, pa->y - pctr->y);
  b.x = CV_XF_A(w, pb->x - pctr->x);
  b.y = CV_YF_A(w, pb->y - pctr->y);
  ctr.x = CV_XF_A(w, pctr->x);
  ctr.y = CV_YF_A(w, pctr->y);

  C2 = a.y * a.y + b.y * b.y;
  C = sqrt(C2);

  ymin = ctr.y - C;
  ymax = ctr.y + C;

  /* Esco in caso di cerchio non visibile */
  if (ymax < GRP_AYMIN(w) || ymin > GRP_AYMAX(w)) return;

  D = sqrt( a.x * a.x + b.x * b.x );
  xmin = ctr.x - D;
  xmax = ctr.x + D;
  if (xmax < GRP_AXMIN(w) || xmin > GRP_AXMAX(w)) return;

  k1 = ( a.x * a.y + b.x * b.y ) / C2;
  k2 = ( a.x * b.y - a.y * b.x ) / C2;

  /* Taglio le parti che escono sopra o sotto lo "schermo" */
  if (ymin < GRP_AYMIN(w))
    y1 = GRP_IYMIN(w);
  else
    y1 = CV_MED_GT(CV_A_MED(ymin));

  if (ymax > GRP_AYMAX(w))
    y2 = GRP_IYMAX(w);
  else
    y2 = CV_MED_LW(CV_A_MED(ymax));

  y = ((BoxReal) y1) - ctr.y;
  x = ctr.x + y*k1;

  for(iy = y1; iy <= y2; iy++) {

    dx = k2 * sqrt(C2 - y*y);
    MARK(w, iy, x - dx);
    MARK(w, iy, x + dx);

    x += k1;
    ++y;
  }
}

/* NOME: rst__poly
 * DESCRIZIONE: Rasterizza un poligono chiuso di n vertici:
 * p[0], p[1], ..., p[n-1] (punti espressi in coordinate assolute).
 */
void rst__poly(BoxGWin *w, BoxPoint *p, int n) {
  int i;
  BoxPoint q;

  if (n<2) {
    WARNINGMSG("rst__poly", "Poligono con meno di 2 vertici");
    return;
  }

  q.x = p->x;
  q.y = p->y;

  for(i=1; i<n; i++) {
    BoxPoint *previous_p = p++;
    rst__line(w, previous_p, p);
  }

  rst__line(w, & q, p);
}

/* NOME: rst_poly
 * DESCRIZIONE: Rasterizza un poligono chiuso di n vertici:
 * p[0], p[1], ..., p[n-1]
 */
void rst_poly(BoxGWin *w, BoxPoint *p, int n) {
  int i, j = 1;
  BoxPoint r, q[2];

  if (n<2) {
    WARNINGMSG("rst_poly", "Poligono con meno di 2 vertici");
    return;
  }

  r.x = q[0].x = CV_XF_A(w, p->x);
  r.y = q[0].y = CV_YF_A(w, p->y);

  for(i=1; i<n; i++) {
    ++p;
    q[j].x = CV_XF_A(w, p->x);
    q[j].y = CV_YF_A(w, p->y);
    rst__line(w, & q[0], & q[1]);
    j ^= 1;
  }

  rst__line(w, & r, & q[j ^ 1]);
}

/* DESCRIZIONE: Setta il colore di primo piano.
 */
static void My_Fg_Color(BoxGWin *w, Color *c) {
  color cb;
  palitem *newcol;

  grp_color_build(c, & cb);
  newcol = grp_color_request(w->pal, & cb);
  if (newcol != NULL)
    BoxGWin_Set_Color(w, newcol->index);
}

/* DESCRIZIONE: Setta il colore di sfondo.
 */
static void My_Bg_Color(BoxGWin *w, Color *c) {
  color cb;
  grp_color_build(c, & cb);
  (w->bgcol)->c = cb;
}

static void My_Add_Fake_Point(BoxGWin *w, BoxPoint *p) {}