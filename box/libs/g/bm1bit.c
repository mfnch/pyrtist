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

/* Questo file contiene quanto basta per poter disegnare in bianco e nero(2 colori).
 * Ho cercato di includervi solo le procedure dipendenti dal tipo di scrittura
 * (1 bit per pixel), mentre procedure pi generali sono state incluse
 * nel file graphic.c
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "error.h"    /* Serve per la segnalazione degli errori */
#include "graphic.h"  /* Dichiaro alcune strutture grafiche generali */
#include "bmcommon.h"  /* Dichiarazioni delle procedure di bmcommon.c */

/* Questa struttura contiene i dati della finestra che dipendono
 * dal modo in cui viene memorizzato il disegno (numero bit per pixel)
 */
typedef struct {
  unsigned char *andmask;    /* Puntatore alla maschera and per il disegno di punti */
  unsigned char *xormask;    /* Puntatore alla maschera xor per il disegno di punti */
  unsigned char fandmask;    /* Maschera and per disegnare 8 punti alla volta */
  unsigned char fxormask;    /* Maschera xor per disegnare 8 punti alla volta */
} gr1b_wrdep;

/* Procedure definite in questo file */
static void gr1b_repair(BoxGWin *wd);

/* Variabili locali */
static unsigned char xormask[2][8] = {
  {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
  {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01}
};

static unsigned char andmask[2][8] = {
  {0x7f, 0xbf, 0xdf, 0xef, 0xf7, 0xfb, 0xfd, 0xfe},  /* cancella il pixel */
  {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}  /* lascia il pixel */
};

static unsigned char fxormask[2] = {0x00, 0xff};
static unsigned char fandmask[2] = {0x00, 0xff};

/* Questa macro restituisce il segno di un double
 * (1 se il numero �positivo, -1 se �negativo)
 */
#define sign(x) (x < 0.0 ? -1.0 : 1.0)

/* Questa macro viene usata per rendere pi leggibili i riferimenti
 * ai dati della finestra dipendenti dai bit per pixel
 */
#define WRDP(s)  ((gr1b_wrdep *) s->data)

/***************************************************************************************/
/* PROCEDURE DI GESTIONE DELLA FINESTRA GRAFICA */

/* DESCRIZIONE: Apre una finestra grafica di forma rettangolare
 *  associando all'angolo in alto a sinistra le coordinate (ltx, lty)
 *  e all'angolo in basso a destra le coordinate (rdx, rdy).
 *  Tutte queste coordinate sono espresse in "unit� (che indichiamo con "u"
 *  e 1 u = (1 * grp_tomm) millimetri, dove grp_tomm �definito nel file
 *  "graphic.c"). Le risoluzioni orizzontale e verticale risx e risy
 *  sono espresse in punti per unit�(che indichiamo con "ppu" e
 *  1 ppu = (1/grp_tomm) punti per millimetro = (25.4/grp_tomm) dpi).
 *  La funzione restituisce un puntatore ad una struttura contenente
 *  i dati relativi alla nuova finestra creata, la quale viene utilizzata
 *  per la gestione della finestra stessa.
 *  In caso di errore invece restituisce 0.
 */
BoxGWin *BoxGWin_Create_BM1(BoxReal ltx, BoxReal lty, BoxReal rdx, BoxReal rdy,
                            BoxReal resx, BoxReal resy) {
  void *winptr;
  BoxGWin *wd;
  long numptx, numpty, bytesperline, windim;
  BoxReal lx, ly, versox, versoy;

  wd = (BoxGWin *) malloc(sizeof(BoxGWin));
  if (wd == NULL) {
    ERRORMSG("BoxGWin_Create_BM1", "Memoria esaurita");
    return NULL;
  }

  wd->data = (gr1b_wrdep *) malloc(sizeof(gr1b_wrdep));
  if (wd->data == NULL) {
    ERRORMSG("BoxGWin_Create_BM1", "Memoria esaurita");
    return NULL;
  }

  lx = rdx - ltx;
  ly = rdy - lty;

  /* Trovo i versori nelle due direzioni */
  versox = sign(lx);
  versoy = sign(ly);

  /* Trovo il numero di punti per lato */
  numptx = fabs(lx) * resx;
  numpty = fabs(ly) * resy;

  if ((numptx < 2) || (numpty < 2)) {
    ERRORMSG("BoxGWin_Create_BM1", "Dimensioni della finestra troppo piccole");
    return 0;
  }

   /* Trovo quanti byte servono per contenere numptx pixel da 1 bit ciascuno */
  bytesperline = (numptx + 7)/8;

  /* Trovo il numero di byte occupati dalla finestra */
  windim = bytesperline * numpty;

  /* Creo la finestra e la svuoto */
  if ( (winptr = (void *) calloc(windim+4, 1)) == 0 ) {
    ERRORMSG("BoxGWin_Create_BM1", "Out of memory");
    return 0;
  }

  wd->ptr = winptr;
  wd->ltx = ltx;
  wd->lty = lty;
  wd->rdx = rdx;
  wd->rdy = rdy;

  if (lx > 0.0) {
    wd->minx = wd->ltx;
    wd->maxx = wd->rdx;
  } else {
    wd->minx = wd->rdx;
     wd->maxx = wd->ltx;
  }

  if (ly > 0.0) {
    wd->miny = wd->lty;
    wd->maxy = wd->rdy;
  } else {
    wd->miny = wd->rdy;
    wd->maxy = wd->lty;
  }

  wd->lx = fabs(lx);
  wd->ly = fabs(ly);
  wd->versox = versox;
  wd->versoy = versoy;
  wd->stepx = lx/(numptx-1);
  wd->stepy = ly/(numpty-1);
  wd->resx = fabs(1.0/(wd->stepx * grp_tomm));
  wd->resy = fabs(1.0/(wd->stepy * grp_tomm));
  wd->numptx = numptx;
  wd->numpty = numpty;
  wd->bitperpixel = 1;
  wd->bytesperline = bytesperline;
  wd->dim = windim;

  /* Costruisco una tavolazza di colori da associare a questa finestra */
  wd->pal = grp_palette_build(2, 2, 3, 4);
  if ( wd->pal == NULL ) return NULL;

  /* La prima richiesta di colore corrisponde all'indice di colore 0,
   * cioe' all'indice dello sfondo della figura.
   * Quindi setto il colore dello sfondo a RGB = {255, 255, 255} = bianco
   */
  wd->bgcol = grp_color_request( wd->pal, & ((color) {255, 255, 255}) );
  if ( wd->bgcol == NULL ) return NULL;

  /* Setto il colore di primo piano a RGB = {0, 0, 0} = nero */
  wd->fgcol = grp_color_request( wd->pal, & ((color) {0, 0, 0}) );
  if ( wd->fgcol == NULL ) return NULL;

  WRDP(wd)->andmask = (unsigned char *) &andmask[0];
  WRDP(wd)->xormask = (unsigned char *) &xormask[1];
  WRDP(wd)->fandmask = fandmask[0];
  WRDP(wd)->fxormask = fxormask[1];

  /* Ora do' le procedure per gestire la finestra */
  wd->quiet = 0;
  wd->repair = gr1b_repair;
  wd->repair(wd);
  wd->win_type_str = "bm1";
  return wd;
}

/* Chiude una finestra grafica aperta in precedenza
 * con la funzione gr2b_open_win.
 */
static void My_Finish(BoxGWin *w) {
  free(w->ptr);
  free(w->data);
  grp_palette_destroy(w->pal);
}

/***************************************************************************************/
/* PROCEDURE DI GRAFICA ELEMENTARI
 */

/* Disegna un punto in posizione (ptx, pty)
 * (espressa in coordinate fisiche, cio�numero riga,
 * numero colonna)
 */
static void My_Draw_Point(BoxGWin *w, BoxInt ptx, BoxInt pty) {
  long q, r;
  char *ptr;

  if (   ptx < 0 || ptx >= w->numptx
      || pty < 0 || pty >= w->numpty)
    return;

  q = (long) (ptx >> 3);
  r = (long) (ptx & 7);

  ptr = w->ptr + pty * w->bytesperline + q;

  *ptr &= WRDP(w)->andmask[r];
  *ptr ^= WRDP(w)->xormask[r];
}

/*Setta il colore di tracciatura.
 * I colori disponibili sono 4:
 *  0    sovrascrive i pixel col colore 0
 *  1    sovrascrive i pixel col colore 1
 *  -1    inverte il colore dei pixel
 *  altro  non colora cioe' le procedure di tracciatura non hanno effetto.
 */
static void My_Set_Color(BoxGWin *w, int col) {
  switch (col) {
   case 0:
    WRDP(w)->andmask = (unsigned char *) &andmask[0];
    WRDP(w)->fandmask = fandmask[0];
    WRDP(w)->xormask = (unsigned char *) &xormask[0];
    WRDP(w)->fxormask = fxormask[0];
    break;

   case 1:
    WRDP(w)->andmask = (unsigned char *) &andmask[0];
    WRDP(w)->fandmask = fandmask[0];
    WRDP(w)->xormask = (unsigned char *) &xormask[1];
    WRDP(w)->fxormask = fxormask[1];
    break;

   case -1:
    WRDP(w)->andmask = (unsigned char *) &andmask[1];
    WRDP(w)->fandmask = fandmask[1];
    WRDP(w)->xormask = (unsigned char *) &xormask[1];
    WRDP(w)->fxormask = fxormask[1];
    break;

   default:
    WRDP(w)->andmask = (unsigned char *) &andmask[1];
    WRDP(w)->fandmask = fandmask[1];
    WRDP(w)->xormask = (unsigned char *) &xormask[0];
    WRDP(w)->fxormask = fxormask[0];
    break;
  }
}

/***************************************************************************************/
/* PROCEDURE DI TRACCIATURA INTERMEDIE */

/* Scrive una linea sulla riga y, da colonna x1 a x2
 * (queste sono tutte coordinate intere).
 */
static void My_Draw_Hor_Line(BoxGWin *w, BoxInt y, BoxInt x1, BoxInt x2) {
  long length, nbyte, xbyte, xpix, bl, i;
  char *ptr;

  if (x1 < 0)
    x1 = 0;

  if (x2 >= w->numptx)
    x2 = w->numptx - 1;

  length = x2 - x1 + 1;
  if (length <= 0 || y < 0 || y >= w->numpty)
    return;

  xbyte = x1 >> 3;
  xpix = x1 & 7;

  /* bl �il numero di pixel che mancano per completare il byte */
  bl = (8 - xpix) & 7;

  /* Calcola l'indirizzo del byte che contiene il punto */
  ptr = ((char *) w->ptr) + (w->bytesperline * y + xbyte);

  if (length > bl) {
    if (bl > 0) {
      /* Completo il byte corrente */
      for (i=0; i<bl; i++) {
        *ptr &= WRDP(w)->andmask[xpix];
        *ptr ^= WRDP(w)->xormask[xpix];
        ++xpix;
      }
      ++ptr;
      length -= bl;
    }

    /* Scrivo byte per byte */
    nbyte = length >> 3;

    for (i=0; i<nbyte; i++) {
      *ptr &= WRDP(w)->fandmask;
      *ptr ^= WRDP(w)->fxormask;
      ++ptr;
    }

    /* Scrivo i pixel che restano */
    bl = length & 7;
    for (i=0; i<bl; i++) {
      *ptr &= WRDP(w)->andmask[i];
      *ptr ^= WRDP(w)->xormask[i];
    }

  } else {
    for (i=0; i<length; i++) {
      *ptr &= WRDP(w)->andmask[xpix];
      *ptr |= WRDP(w)->xormask[xpix];
      ++xpix;
    }
  }
}

/** Set the default methods to the gr1b window */
static void gr1b_repair(BoxGWin *wd) {
  BoxGWin_Block(wd);
  rst_repair(wd);
  wd->save_to_file = grbm_save_to_bmp;

  wd->finish = My_Finish;
  wd->set_color = My_Set_Color;
  wd->draw_point = My_Draw_Point;
  wd->draw_hor_line = My_Draw_Hor_Line;
}

