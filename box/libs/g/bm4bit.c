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

/* Questo file contiene quanto basta per poter disegnare con 16 colori.
 * Ho cercato di includervi solo le procedure dipendenti dal tipo di scrittura
 * (4 bit per pixel), mentre procedure piu' generali sono state incluse
 * nel file graphic.c
 * NOTA: da compilare con error.c
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "error.h"    /* Serve per la segnalazione degli errori */
#include "graphic.h"  /* Dichiaro alcune strutture grafiche generali */
#include "bmcommon.h"  /* Dichiarazioni delle procedure di bmcommon.c */

/* Questa struttura contiene i dati della finestra che dipendono
 * dal modo in cui viene memorizzato il disegno (numero bit per pixel)
 */
typedef struct {
  unsigned char andmask[2];  /* Puntatore alla maschera and per il disegno di punti */
  unsigned char xormask[2];  /* Puntatore alla maschera xor per il disegno di punti */
  unsigned char fandmask;    /* Maschera and per disegnare 2 punti alla volta */
  unsigned char fxormask;    /* Maschera xor per disegnare 2 punti alla volta */
} gr4b_wrdep;

/* Procedure definite in questo file */
static void gr4b_repair(BoxGWin *wd);

/* Questa macro restituisce il segno di un double
 * (1 se il numero e' positivo, -1 se e' negativo)
 */
#define sign(x) (x < 0.0 ? -1.0 : 1.0)

/* Questa macro viene usata per rendere piu' leggibili i riferimenti
 * ai dati della finestra dipendenti dai bit per pixel
 */
#define WRDP(s)  ((gr4b_wrdep *) s->data)

/***************************************************************************************/
/* PROCEDURE DI GESTIONE DELLA FINESTRA GRAFICA */

/* DESCRIZIONE: Apre una finestra grafica di forma rettangolare
 *  associando all'angolo in alto a sinistra le coordinate (ltx, lty)
 *  e all'angolo in basso a destra le coordinate (rdx, rdy).
 *  Tutte queste coordinate sono espresse in "unita' (che indichiamo con "u"
 *  e 1 u = (1 * grp_tomm) millimetri, dove grp_tomm e' definito nel file
 *  "graphic.c"). Le risoluzioni orizzontale e verticale risx e risy
 *  sono espresse in punti per unita' (che indichiamo con "ppu" e
 *  1 ppu = (1/grp_tomm) punti per millimetro = (25.4/grp_tomm) dpi).
 *  La funzione restituisce un puntatore ad una struttura contenente
 *  i dati relativi alla nuova finestra creata, la quale viene utilizzata
 *  per la gestione della finestra stessa.
 *  In caso di errore invece restituisce 0.
 */
BoxGWin *BoxGWin_Create_BM4(BoxReal ltx, BoxReal lty, BoxReal rdx, BoxReal rdy,
                            BoxReal resx, BoxReal resy) {
   void *winptr;
   BoxGWin *wd;
  long numptx, numpty, bytesperline, windim;
  Real lx, ly, versox, versoy;

  wd = (BoxGWin *) malloc(sizeof(BoxGWin));
  if (wd == NULL ) {
    ERRORMSG("BoxGWin_Create_BM4", "Memoria esaurita");
    return NULL;
  }

  wd->data = (gr4b_wrdep *) malloc(sizeof(gr4b_wrdep));
  if (wd->data == NULL ) {
    ERRORMSG("BoxGWin_Create_BM4", "Memoria esaurita");
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
    ERRORMSG("BoxGWin_Create_BM4", "Dimensioni della finestra troppo piccole");
    return 0;
  }

   /* Trovo quanti byte servono per contenere numptx pixel da 4 bit ciascuno */
  bytesperline = (numptx + 1)>>1;

  /* Trovo il numero di byte occupati dalla finestra */
  windim = bytesperline * numpty;

  /* Creo la finestra e la svuoto */
  if ( (winptr = (void *) calloc(windim+4, 1)) == NULL ) {
    ERRORMSG("BoxGWin_Create_BM4", "Out of memory");
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
  wd->bitperpixel = 4;
  wd->bytesperline = bytesperline;
  wd->dim = windim;

  /* Costruisco una tavolazza di colori da associare a questa finestra */
  wd->pal = grp_palette_build(16, 16, 5, 2);
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

  /* Setto il colore iniziale = 15 */
  WRDP(wd)->andmask[0] = 0x0f; WRDP(wd)->andmask[1] = 0xf0;
  WRDP(wd)->fandmask = 0x00;
  WRDP(wd)->xormask[0] = 0x10 * 15; WRDP(wd)->xormask[1] = 0x01 * 15;
  WRDP(wd)->fxormask = 0x11 * 15;

  /* Ora do' le procedure per gestire la finestra */
  wd->quiet = 0;
  wd->repair = gr4b_repair;
  wd->repair(wd);
  wd->win_type_str = "bm4";
  return wd;
}

/* Chiude una finestra grafica aperta in precedenza
 * con la funzione BoxGWin_Create_BM4.
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
 * (espressa in coordinate fisiche, cioe' numero riga, numero colonna)
 */
static void My_Draw_Point(BoxGWin *w, Int ptx, Int pty) {
  long q, r;
  char *ptr;

  if (   ptx < 0 || ptx >= w->numptx
      || pty < 0 || pty >= w->numpty)
    return;

  q = (long) (ptx >> 1);
  r = (long) (ptx & 1);

  ptr = w->ptr + pty * w->bytesperline + q;

  *ptr &= WRDP(w)->andmask[r];
  *ptr ^= WRDP(w)->xormask[r];
}

/* Setta il colore di tracciatura.
 * Se il numero di colore specificato e' positivo viene usata la modalita'
 * "colore ricoprente" (lo sfondo viene ricoperto dai nuovi disegni),
 * se e' negativo viene usata la modalita' xor ( i nuovi disegni si "combinano"
 * con quelli nello sfondo per dar luogo a nuovi colori).
 * In caso di colore al di fuori dell'intervallo -15, ..., 15
 * viene utilizzato un colore "trasparente", cioe' le procedure di tracciatura
 * non hanno alcun effetto.
 */
static void My_Set_Color(BoxGWin *w, int col) {
  if (col < -15 || col > 15) {
    WRDP(w)->andmask[0] = WRDP(w)->andmask[1] = WRDP(w)->fandmask = 0xff;
    WRDP(w)->xormask[0] = WRDP(w)->xormask[1] = WRDP(w)->fxormask = 0x00;
    return;
  }

  if (col < 0) {
    col = -col;
    WRDP(w)->andmask[0] = WRDP(w)->andmask[1] = WRDP(w)->fandmask = 0xff;
    WRDP(w)->xormask[0] = 0x10 * col; WRDP(w)->xormask[1] = 0x01 * col;
    WRDP(w)->fxormask = 0x11 * col;

  } else {
    WRDP(w)->andmask[0] = 0x0f; WRDP(w)->andmask[1] = 0xf0;
    WRDP(w)->fandmask = 0x00;
    WRDP(w)->xormask[0] = 0x10 * col; WRDP(w)->xormask[1] = 0x01 * col;
    WRDP(w)->fxormask = 0x11 * col;
  }
}

/***************************************************************************************/
/* PROCEDURE DI TRACCIATURA INTERMEDIE */

/* Scrive una linea sulla riga y, da colonna x1 a x2
 * (queste sono tutte coordinate intere).
 */
static void My_Draw_Hor_Line(BoxGWin *w, Int y, Int x1, Int x2) {
  long length, nbyte, xbyte, xpix, i;
  char *ptr;

  if (x1 < 0)
    x1 = 0;

  if (x2 >= w->numptx)
    x2 = w->numptx - 1;

  length = x2 - x1 + 1;
  if (length <= 0 || y < 0 || y >= w->numpty)
    return;

  xbyte = x1 >> 1;
  xpix = x1 & 1;

  /* Calcola l'indirizzo del byte che contiene il punto */
  ptr = ((char *) w->ptr) + (w->bytesperline * y + xbyte);

  if (length > xpix) {
    if (xpix > 0) {
      /* Completo il byte corrente */
      *ptr &= WRDP(w)->andmask[1];
      *ptr ^= WRDP(w)->xormask[1];
      ++ptr;
      length -= 1;
    }

    /* Scrivo byte per byte */
    nbyte = length >> 1;

    for (i=0; i<nbyte; i++) {
      *ptr &= WRDP(w)->fandmask;
      *ptr ^= WRDP(w)->fxormask;
      ++ptr;
    }

    /* Scrivo l'eventuale pixel che resta */
    if (length & 1) {
      *ptr &= WRDP(w)->andmask[0];
      *ptr ^= WRDP(w)->xormask[0];
    }

  } else {

    *ptr &= WRDP(w)->andmask[1];
    *ptr |= WRDP(w)->xormask[1];
  }
}

/** Set the default methods to the gr4b window */
static void gr4b_repair(BoxGWin *wd) {
  BoxGWin_Block(wd);
  rst_repair(wd);
  wd->save_to_file = grbm_save_to_bmp;

  wd->finish = My_Finish;
  wd->set_color = My_Set_Color;
  wd->draw_point = My_Draw_Point;
  wd->draw_hor_line = My_Draw_Hor_Line;
}
