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

/* Questo file contiene quanto basta per poter disegnare con 256 colori.
 * Ho cercato di includervi solo le procedure dipendenti dal tipo di scrittura
 * (8 bit per pixel), mentre procedure piu' generali sono state incluse
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
  unsigned char andmask;  /* Puntatore alla maschera and per il disegno di punti */
  unsigned char xormask;  /* Puntatore alla maschera xor per il disegno di punti */
} gr8b_wrdep;

/* Procedure definite in questo file */
BoxGWin *gr8b_open_win(Real ltx, Real lty, Real rdx, Real rdy,
                          Real resx, Real resy);
static void gr8b_repair(BoxGWin *wd);

/* Questa macro restituisce il segno di un double
 * (1 se il numero e' positivo, -1 se e' negativo)
 */
#define sign(x) (x < 0.0 ? -1.0 : 1.0)

/* Questa macro viene usata per rendere piu' leggibili i riferimenti
 * ai dati della finestra dipendenti dai bit per pixel
 */
#define WRDP(s) ((gr8b_wrdep *) s->wrdep)

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
BoxGWin *gr8b_open_win(Real ltx, Real lty, Real rdx, Real rdy,
                          Real resx, Real resy) {
   void *winptr;
   BoxGWin *wd;
  long numptx, numpty, bytesperline, windim;
  Real lx, ly, versox, versoy;

  wd = (BoxGWin *) malloc(sizeof(BoxGWin));
  if (wd == NULL ) {
    ERRORMSG("gr4b_open_win", "Memoria esaurita");
    return NULL;
  }

  wd->wrdep = (gr8b_wrdep *) malloc(sizeof(gr8b_wrdep));
  if (wd->wrdep == NULL) {
    ERRORMSG("gr4b_open_win", "Memoria esaurita");
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
    ERRORMSG("gr4b_open_win",
     "Dimensioni della finestra troppo piccole");
    return NULL;
  }

   /* Trovo quanti byte servono per contenere numptx pixel da 8 bit ciascuno */
  bytesperline = numptx;

  /* Trovo il numero di byte occupati dalla finestra */
  windim = bytesperline * numpty;

  /* Creo la finestra e la svuoto */
  if ( (winptr = (void *) calloc(windim+4, 1)) == NULL ) {
    ERRORMSG("gr8b_open_win",
     "Memoria non sufficiente per creare una finestra di queste dimensioni");
    return NULL;
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
  wd->bitperpixel = 8;
  wd->bytesperline = bytesperline;
  wd->dim = windim;

  /* Costruisco una tavolazza di colori da associare a questa finestra */
  wd->pal = grp_palette_build(256, 256, 17, 2);
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
  WRDP(wd)->andmask = 0x00;
  WRDP(wd)->xormask = 0x01;

  /* Ora do' le procedure per gestire la finestra */
  wd->quiet = 0;
  wd->repair = gr8b_repair;
  wd->repair(wd);
  wd->win_type_str = "bm8";
  return wd;
}

/* DESCRIZIONE: Chiude una finestra grafica aperta in precedenza
 *  con la funzione gr8b_open_win.
 */
static void My_Finish_Drawing(BoxGWin *w) {
  free(w->ptr);
  free(w->wrdep );
  grp_palette_destroy(w->pal);
  free(w);
}

/***************************************************************************************/
/* PROCEDURE DI GRAFICA ELEMENTARI
 */

/* Disegna un punto in posizione (ptx, pty)
 * (espressa in coordinate fisiche, cioe' numero riga, numero colonna)
 */
static void My_Draw_Point(BoxGWin *w, Int ptx, Int pty) {
  char *ptr;

  if (   ptx < 0 || ptx >= w->numptx
      || pty < 0 || pty >= w->numpty)
    return;

  ptr = w->ptr + pty * w->bytesperline + ptx;

  *ptr &= WRDP(w)->andmask;
  *ptr ^= WRDP(w)->xormask;
}

/* Se il numero di colore specificato e' positivo viene usata la modalita'
 * "colore ricoprente" (lo sfondo viene ricoperto dai nuovi disegni),
 * se e' negativo viene usata la modalita' xor ( i nuovi disegni si "combinano"
 * con quelli nello sfondo per dar luogo a nuovi colori).
 * In caso di colore al di fuori dell'intervallo -255, ..., 255
 * viene utilizzato un colore "trasparente", cioe' le procedure di tracciatura
 * non hanno alcun effetto.
 */
static void My_Set_Color(BoxGWin *w, int col) {
  if (col < -255 || col > 255) {
    WRDP(w)->andmask = 0xff;
    WRDP(w)->xormask = 0x00;
    return;
  }

  if (col < 0) {
    col = -col;
    WRDP(w)->andmask = 0xff;
    WRDP(w)->xormask = col;

  } else {
    WRDP(w)->andmask = 0x00;
    WRDP(w)->xormask = col;
  }
}

/***************************************************************************************/
/* PROCEDURE DI TRACCIATURA INTERMEDIE */

/* DESCRIZIONE: Scrive una linea sulla riga y, da colonna x1 a x2
 * (queste sono tutte coordinate intere).
 */
static void My_Draw_Hor_Line(BoxGWin *w, Int y, Int x1, Int x2) {
  long length, i;
  char *ptr;

  if (x1 < 0) x1 = 0;
  if (x2 >= w->numptx) x2 = w->numptx - 1;

  length = x2 - x1 + 1 <= 0;
  if (length <= 0 || y < 0 || y >= w->numpty)
   return;

  /* Calcola l'indirizzo del byte che contiene il punto */
  ptr = ((char *) w->ptr) + (w->bytesperline * y + x1);

  for (i = 0; i < length; i++) {
    *ptr &= WRDP(w)->andmask;
    *ptr ^= WRDP(w)->xormask;
    ++ptr;
  }
}

/** Set the default methods to the gr1b window */
static void gr8b_repair(BoxGWin *wd) {
  BoxGWin_Block(wd);
  rst_repair(wd);
  wd->save_to_file = grbm_save_to_bmp;

  wd->finish_drawing = My_Finish_Drawing;
  wd->set_color = My_Set_Color;
  wd->draw_point = My_Draw_Point;
  wd->draw_hor_line = My_Draw_Hor_Line;
}
