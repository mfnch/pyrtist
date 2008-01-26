/* bm8bit.c - Autore: Franchin Matteo - 12 ottobre 2004
 *
 * Questo file contiene quanto basta per poter disegnare con 256 colori.
 * Ho cercato di includervi solo le procedure dipendenti dal tipo di scrittura
 * (8 bit per pixel), mentre procedure piu' generali sono state incluse
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
	unsigned char andmask;	/* Puntatore alla maschera and per il disegno di punti */
	unsigned char xormask;	/* Puntatore alla maschera xor per il disegno di punti */
} gr8b_wrdep;

/* Funzioni esterne di rasterizzazione (definite in rasterizer.c) */
extern void (*rst_midfn[])();

/* Procedure definite in questo file */
grp_window *gr8b_open_win(FCOOR ltx, FCOOR lty, FCOOR rdx, FCOOR rdy,
 FCOOR resx, FCOOR resy);
void gr8b_close_win(void);
void gr8b_set_col(int col);
void gr8b_draw_point(ICOOR ptx, ICOOR pty);
void gr8b_hor_line(ICOOR y, ICOOR x1, ICOOR x2);

/* Array di puntatori a queste procedure */
static void (*gr8b_lowfn[])() = {
	gr8b_close_win,
	gr8b_set_col,
	gr8b_draw_point,
	gr8b_hor_line
};

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
grp_window *gr8b_open_win(FCOOR ltx, FCOOR lty, FCOOR rdx, FCOOR rdy,
 FCOOR resx, FCOOR resy)
{
 	void *winptr;
 	grp_window *wd;
	long numptx, numpty, bytesperline, windim;
	FCOOR lx, ly, versox, versoy;

	if (
	 (wd = (grp_window *) malloc(sizeof(grp_window))) == NULL ) {
		ERRORMSG("gr4b_open_win", "Memoria esaurita");
		return NULL;
	}
	
	if ( (wd->wrdep = (gr8b_wrdep *) malloc(sizeof(gr8b_wrdep))) == NULL ) {
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
	wd->save = grbm_save_to_bmp;
	wd->lowfn = gr8b_lowfn;
	wd->midfn = rst_midfn;
	return wd;
}

/* DESCRIZIONE: Chiude una finestra grafica aperta in precedenza
 *  con la funzione gr8b_open_win.
 */
void gr8b_close_win(void)
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

/* DESCRIZIONE: Disegna un punto in posizione (ptx, pty)
 * (espressa in coordinate fisiche, cioe' numero riga, numero colonna)
 */
void gr8b_draw_point(ICOOR ptx, ICOOR pty)
{
	char *ptr;

	if ((ptx < 0) || (ptx >= grp_win->numptx)) return;
	if ((pty < 0) || (pty >= grp_win->numpty)) return;

	ptr = grp_win->ptr + pty * grp_win->bytesperline + ptx;

	*ptr &= WRDP(grp_win)->andmask;
	*ptr ^= WRDP(grp_win)->xormask;

	return;
}

/* DESCRIZIONE: Setta il colore di tracciatura.
 * Se il numero di colore specificato e' positivo viene usata la modalita'
 * "colore ricoprente" (lo sfondo viene ricoperto dai nuovi disegni),
 * se e' negativo viene usata la modalita' xor ( i nuovi disegni si "combinano"
 * con quelli nello sfondo per dar luogo a nuovi colori).
 * In caso di colore al di fuori dell'intervallo -255, ..., 255
 * viene utilizzato un colore "trasparente", cioe' le procedure di tracciatura
 * non hanno alcun effetto.
 */
void gr8b_set_col(int col)
{
	if ( (col < -255) || (col > 255) ) {
		WRDP(grp_win)->andmask = 0xff;
		WRDP(grp_win)->xormask = 0x00;
		return;
	}

	if (col < 0) {
		col = -col;
		WRDP(grp_win)->andmask = 0xff;
		WRDP(grp_win)->xormask = col;
		return;

	} else {
		WRDP(grp_win)->andmask = 0x00;
		WRDP(grp_win)->xormask = col;
		return;
	}

	return;
}

/***************************************************************************************/
/* PROCEDURE DI TRACCIATURA INTERMEDIE */

/* DESCRIZIONE: Scrive una linea sulla riga y, da colonna x1 a x2
 * (queste sono tutte coordinate intere).
 */
void gr8b_hor_line(ICOOR y, ICOOR x1, ICOOR x2)
{
	long lenght, i;
	char *ptr;

	if (x1 < 0) x1 = 0;
	if (x2 >= grp_win->numptx) x2 = grp_win->numptx - 1;

	if (
	 ( (lenght = x2 - x1 + 1) <= 0 ) || (y < 0) || (y >= grp_win->numpty) )
   return;

	/* Calcola l'indirizzo del byte che contiene il punto */
	ptr = ((char *) grp_win->ptr) + (grp_win->bytesperline * y + x1);

	for (i = 0; i < lenght; i++) {
		*ptr &= WRDP(grp_win)->andmask;
		*ptr ^= WRDP(grp_win)->xormask;
		++ptr;
	}

	return;
}
