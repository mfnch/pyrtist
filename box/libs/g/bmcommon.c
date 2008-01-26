
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "error.h"			/* Serve per la segnalazione degli errori */
#include "graphic.h"		/* Dichiaro alcune strutture grafiche generali */
#include "bitmap.h"			/* Contiene la struttura di un file bitmap */
#include "bmcommon.h"		/* Dichiarazioni delle procedure definite in questo file */

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

/* FUNZIONE: grbm_save_to_bmp
 * Salva la finestra in un file in formato BMP, scrivendolo su stream.
 * Restituisce 1 in caso di successo, 0 altrimenti.
 */
int grbm_save_to_bmp(FILE *stream)
{
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
	ih.headsize = 40;
	ih.width = grp_win->numptx;
	ih.height = grp_win->numpty;
	ih.numplanes = 1;
	ih.bitperpixel = grp_win->bitperpixel;
	ih.compression = 0;
	ih.compsize = imgsize;
	ih.horres = grp_win->resx * 1000.0;
	ih.vertres = grp_win->resy * 1000.0;
	ih.impcolors = ih.numcolors = pal->dim;

	/* Trova l'offset dell'immagine nel file */
	imgoffs = 14 + ih.headsize + pal->dim * 4;

	/* Compila l'header del file */
	fh.id = 0x4d42;
	fh.imgoffs = imgoffs;
	fh.size = imgoffs + imgsize;
	fh.reserved1 = 0;
	fh.reserved2 = 0;

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
