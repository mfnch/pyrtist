/* bm4bit.c - Autore: Franchin Matteo - 27 dicembre 2002
 *
 * Questo file contiene quanto basta per poter disegnare con 16 colori.
 * Ho cercato di includervi solo le procedure dipendenti dal tipo di scrittura
 * (4 bit per pixel), mentre procedure piu' generali sono state incluse
 * nel file graphic.c
 * NOTA: da compilare con error.c
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "error.h"		/* Serve per la segnalazione degli errori */
#include "graphic.h"	/* Dichiaro alcune strutture grafiche generali */
#include "bmcommon.h"	/* Dichiarazioni delle procedure di bmcommon.c */

/* Questa struttura contiene i dati della finestra che dipendono
 * dal modo in cui viene memorizzato il disegno (numero bit per pixel)
 */
typedef struct {
	unsigned char andmask[2];	/* Puntatore alla maschera and per il disegno di punti */
	unsigned char xormask[2];	/* Puntatore alla maschera xor per il disegno di punti */
	unsigned char fandmask;		/* Maschera and per disegnare 2 punti alla volta */
	unsigned char fxormask;		/* Maschera xor per disegnare 2 punti alla volta */
} gr4b_wrdep;

/* Funzioni esterne di rasterizzazione (definite in rasterizer.c) */
extern void (*rst_midfn[])();

/* Procedure definite in questo file */
grp_window *gr4b_open_win(FCOOR ltx, FCOOR lty, FCOOR rdx, FCOOR rdy,
 FCOOR resx, FCOOR resy);
void gr4b_close_win(void);
void gr4b_set_col(int col);
void gr4b_draw_point(ICOOR ptx, ICOOR pty);
void gr4b_hor_line(ICOOR y, ICOOR x1, ICOOR x2);

/* Array di puntatori a queste procedure */
static void (*gr4b_lowfn[])() = {
	gr4b_close_win,
	gr4b_set_col,
	gr4b_draw_point,
	gr4b_hor_line
};

/* Questa macro restituisce il segno di un double
 * (1 se il numero e' positivo, -1 se e' negativo)
 */
#define sign(x) (x < 0.0 ? -1.0 : 1.0)

/* Questa macro viene usata per rendere piu' leggibili i riferimenti
 * ai dati della finestra dipendenti dai bit per pixel
 */
#define WRDP(s)	((gr4b_wrdep *) s->wrdep)

/***************************************************************************************/
/* PROCEDURE DI GESTIONE DELLA FINESTRA GRAFICA */

/* NOME: gr4b_open_win
 * DESCRIZIONE: Apre una finestra grafica di forma rettangolare
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
grp_window *gr4b_open_win(FCOOR ltx, FCOOR lty, FCOOR rdx, FCOOR rdy,
 FCOOR resx, FCOOR resy)
{
 	void *winptr;
 	grp_window *wd;
	long numptx, numpty, bytesperline, windim;
	FCOOR lx, ly, versox, versoy;

	if (
	 (wd = (grp_window *) malloc(sizeof(grp_window))) == NULL ) {
		ERRORMSG("gr4b_open_win", "Memoria esaurita");
		return (grp_window *) 0;
	}
	
	if ( (wd->wrdep = (gr4b_wrdep *) malloc(sizeof(gr4b_wrdep))) == NULL ) {
		ERRORMSG("gr4b_open_win", "Memoria esaurita");
		return (grp_window *) 0;
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
		return 0;
	}

 	/* Trovo quanti byte servono per contenere numptx pixel da 4 bit ciascuno */
	bytesperline = (numptx + 1)>>1;

	/* Trovo il numero di byte occupati dalla finestra */
	windim = bytesperline * numpty;

	/* Creo la finestra e la svuoto */
	if ( (winptr = (void *) calloc(windim+4, 1)) == NULL ) {
		ERRORMSG("gr4b_open_win",
		 "Memoria non sufficiente per creare una finestra di queste dimensioni");
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
	wd->save = grbm_save_to_bmp;
	wd->lowfn = gr4b_lowfn;
	wd->midfn = rst_midfn;
	return wd;
}

/* SUBROUTINE: gr4b_close_win
 * Chiude una finestra grafica aperta in precedenza
 * con la funzione gr4b_open_win.
 */
void gr4b_close_win(void)
{
	free( grp_win->ptr );
	free( grp_win->wrdep );
	free( grp_win );
	grp_palette_destroy( grp_win->pal );

	return;
}

/***************************************************************************************/
/* PROCEDURE DI GRAFICA ELEMENTARI
 */

/* SUBROUTINE: gr4b_draw_point
 * Disegna un punto in posizione (ptx, pty)
 * (espressa in coordinate fisiche, cioe' numero riga, numero colonna)
 */
void gr4b_draw_point(ICOOR ptx, ICOOR pty)
{
	long q, r;
	char *ptr;

	if ((ptx < 0) || (ptx >= grp_win->numptx)) return;
	if ((pty < 0) || (pty >= grp_win->numpty)) return;

	q = (long) (ptx >> 1);
	r = (long) (ptx & 1);

	ptr = grp_win->ptr + pty * grp_win->bytesperline + q;

	*ptr &= WRDP(grp_win)->andmask[r];
	*ptr ^= WRDP(grp_win)->xormask[r];

	return;
}

/* SUBROUTINE: gr4b_set_col
 * Setta il colore di tracciatura.
 * Se il numero di colore specificato e' positivo viene usata la modalita'
 * "colore ricoprente" (lo sfondo viene ricoperto dai nuovi disegni),
 * se e' negativo viene usata la modalita' xor ( i nuovi disegni si "combinano"
 * con quelli nello sfondo per dar luogo a nuovi colori).
 * In caso di colore al di fuori dell'intervallo -15, ..., 15
 * viene utilizzato un colore "trasparente", cioe' le procedure di tracciatura
 * non hanno alcun effetto.
 */
void gr4b_set_col(int col)
{
	if ( (col < -15) || (col > 15) ) {
		WRDP(grp_win)->andmask[0] = WRDP(grp_win)->andmask[1] =
		 WRDP(grp_win)->fandmask = 0xff;
		WRDP(grp_win)->xormask[0] = WRDP(grp_win)->xormask[1] =
		 WRDP(grp_win)->fxormask = 0x00;
		return;
	}

	if (col < 0) {
		col = -col;
		WRDP(grp_win)->andmask[0] = WRDP(grp_win)->andmask[1] =
		 WRDP(grp_win)->fandmask = 0xff;
		WRDP(grp_win)->xormask[0] = 0x10 * col; WRDP(grp_win)->xormask[1] = 0x01 * col;
		WRDP(grp_win)->fxormask = 0x11 * col;
		return;

	} else {
		WRDP(grp_win)->andmask[0] = 0x0f; WRDP(grp_win)->andmask[1] = 0xf0;
		WRDP(grp_win)->fandmask = 0x00;
		WRDP(grp_win)->xormask[0] = 0x10 * col; WRDP(grp_win)->xormask[1] = 0x01 * col;
		WRDP(grp_win)->fxormask = 0x11 * col;
		return;
	}

	return;
}

/***************************************************************************************/
/* PROCEDURE DI TRACCIATURA INTERMEDIE */

/* FUNZIONE: gr4b_hor_line
 * Scrive una linea sulla riga y, da colonna x1 a x2
 * (queste sono tutte coordinate intere).
 */
void gr4b_hor_line(ICOOR y, ICOOR x1, ICOOR x2)
{
	long lenght, nbyte, xbyte, xpix, i;
	char *ptr;

	if (x1 < 0) x1 = 0;
	if (x2 >= grp_win->numptx) x2 = grp_win->numptx - 1;

	if ((lenght = x2 - x1 + 1) <= 0) return;
	if (y < 0) return;
	if (y >= grp_win->numpty) return;

	xbyte = x1 >> 1;
	xpix = x1 & 1;

	/* Calcola l'indirizzo del byte che contiene il punto */
	ptr = ((char *) grp_win->ptr) + (grp_win->bytesperline * y + xbyte);

	if (lenght > xpix) {
		if (xpix > 0) {
			/* Completo il byte corrente */
			*ptr &= WRDP(grp_win)->andmask[1];
			*ptr ^= WRDP(grp_win)->xormask[1];
			++ptr;
			lenght -= 1;
		}

		/* Scrivo byte per byte */
		nbyte = lenght >> 1;

		for (i=0; i<nbyte; i++) {
			*ptr &= WRDP(grp_win)->fandmask;
			*ptr ^= WRDP(grp_win)->fxormask;
			++ptr;
		}

		/* Scrivo l'eventuale pixel che resta */
		if (lenght & 1) {
			*ptr &= WRDP(grp_win)->andmask[0];
			*ptr ^= WRDP(grp_win)->xormask[0];
		}

	} else {
		
		*ptr &= WRDP(grp_win)->andmask[1];
		*ptr |= WRDP(grp_win)->xormask[1];
	}

	return;
}
