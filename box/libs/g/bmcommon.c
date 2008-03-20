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

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "error.h"      /* Serve per la segnalazione degli errori */
#include "graphic.h"    /* Dichiaro alcune strutture grafiche generali */
#include "bitmap.h"      /* Contiene la struttura di un file bitmap */
#include "bmcommon.h"    /* Dichiarazioni delle procedure definite in questo file */

/* Puntatore alla tavola dei colori da inserire nel file bitmap
 * (il puntatore e' dichiarato come statico in modo da poter essere condiviso
 * sia da grbm_save_to_bmp che da grbm_write_palette)
 */
static struct bmp_color *colormap;

/***************************************************************************************/

static int grbm_write_palette(palitem *pi);

/* DESCRIZIONE: Questa funzione e' passata come argomento alla funzione
 *  grp_palette_transform, per prelevare la tavolazza dei colori(componenti RGB)
 *  da scrivere nel file bitmap.
 */
static int grbm_write_palette(palitem *pi)
{
  colormap[pi->index] = (struct bmp_color) {
    pi->c.b, /* blue */
    pi->c.g, /* green */
    pi->c.r  /* red */
  };

  return 1;
}

void write_leuint16(LEUInt16 *dest, unsigned long src) {
  dest->byte[0] = src & 0xff;
  dest->byte[1] = (src >> 8) & 0xff;
}

void write_leuint32(LEUInt32 *dest, unsigned long src) {
  dest->byte[0] = src & 0xff;
  dest->byte[1] = (src >> 8) & 0xff;
  dest->byte[2] = (src >> 16) & 0xff;
  dest->byte[3] = (src >> 24) & 0xff;
}

/* SUBROUTINE: grbm_draw_point
 * Disegna un punto in posizione (x, y) (espressa in millimetri)
 */
void grbm_draw_point(FCOOR x, FCOOR y)
{
  long ptx, pty;

  ptx = rint((x - grp_win->ltx)/grp_win->stepx);
  pty = rint((y - grp_win->lty)/grp_win->stepy);

  grp_draw_point(ptx, pty);

  return;
}

/***************************************************************************************/
/* PROCEDURE DI SALVATAGGIO SU DISCO DELLA FINESTRA GRAFICA */

static int _grbm_save_to_bmp(FILE *stream) {
  int i;
  long bytesperline, imgsize, imgoffs;
  void *lineptr;
  palette *pal;

  struct bmp_file_header fh;
  struct bmp_image_header ih;

  pal = grp_win->pal;
  colormap = (struct bmp_color*)
   calloc( pal->dim, sizeof(struct bmp_color) );
  if (colormap == NULL ) {
    ERRORMSG("save_to_bmp", "Memoria esaurita");
    return 0;
  }

  /* Trova il piu' piccolo numero di byte sufficiente a contenere
   * una riga della finestra, che sia un multiplo di 4
   * (il formato BMP viene generato con questa particolarita'
   * le righe sono formate da un certo numero di dwords)
   */
  bytesperline = (grp_win->bytesperline + 3)/4;
  bytesperline *= 4;

  /* Trova la dimensione dell'immagine  */
  imgsize = bytesperline * grp_win->numpty;


  /* Compila l'header di immagine */
#define BMP_HEADSIZE 40
  write_leuint32(& ih.headsize, BMP_HEADSIZE);
  write_leuint32(& ih.width, grp_win->numptx);
  write_leuint32(& ih.height, grp_win->numpty);
  write_leuint16(& ih.numplanes, 1);
  write_leuint16(& ih.bitperpixel, grp_win->bitperpixel);
  write_leuint32(& ih.compression, 0);
  write_leuint32(& ih.compsize, imgsize);
  write_leuint32(& ih.horres, grp_win->resx * 1000.0);
  write_leuint32(& ih.vertres, grp_win->resy * 1000.0);
  write_leuint32(& ih.impcolors, pal->dim);
  write_leuint32(& ih.numcolors, pal->dim);

  /* Trova l'offset dell'immagine nel file */
  imgoffs = 14 + BMP_HEADSIZE + pal->dim * 4;

  /* Compila l'header del file */
  write_leuint16(& fh.id, 0x4d42);
  write_leuint32(& fh.imgoffs, imgoffs);
  write_leuint32(& fh.size, imgoffs + imgsize);
  write_leuint16(& fh.reserved1, 0);
  write_leuint16(& fh.reserved2, 0);

  /*Compila la mappa dei colori */
  if ( ! grp_palette_transform( pal, grbm_write_palette ) ) {
    ERRORMSG("save_to_bmp", "Impossibile scrivere il file.");
    return 0;
  }

  /* Scrive il file header */
  if (fwrite(&fh, sizeof(struct bmp_file_header), 1, stream)<1)
    {ERRORMSG("save_to_bmp", "Impossibile scrivere il file."); return 0;}

  /* Scrive l'image header */
  if (fwrite(&ih, sizeof(struct bmp_image_header), 1, stream)<1)
    {ERRORMSG("save_to_bmp", "Impossibile scrivere il file."); return 0;}

  /* Scrive la mappa dei colori */
  if (fwrite(colormap, sizeof(struct bmp_color), pal->dim, stream)
   < pal->dim) {
     ERRORMSG("save_to_bmp", "Impossibile scrivere il file.");
     return 0;
  }

  /* I file bitmap vengono scritti dall'ultima alla prima riga
   * dunque mi posiziono in fondo alla finestra
   */
  lineptr = grp_win->ptr + grp_win->dim - grp_win->bytesperline;

  for (i=0; i<grp_win->numpty; i++) {
    /* Scrive ciascuna linea della finestra */
    if (fwrite(lineptr, bytesperline, 1, stream)<1) {
      ERRORMSG("save_to_bmp", "Impossibile scrivere il file.");
      return 0;
    }
    lineptr -= grp_win->bytesperline;
  }

  return 1;
}

/* FUNZIONE: grbm_save_to_bmp
 * Salva la finestra in un file in formato BMP, scrivendolo su stream.
 * Restituisce 1 in caso di successo, 0 altrimenti.
 */
int grbm_save_to_bmp(const char *file_name) {
  FILE *file = fopen(file_name, "w");
  int status = _grbm_save_to_bmp(file);
  fclose(file);
  return status;
}
