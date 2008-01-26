/* ps.c - Autore: Franchin Matteo - 15 aprile 2004
 *
 * Questo file contiene le funzioni che implementano una finestra di tipo
 * postscript (ps). In questa finestra i comandi grafici vengono immediatamente
 * diretti sul file postscript di output.
 */

#include <stdlib.h>
#include <stdio.h>
/*#include <math.h>*/

#include "error.h"		/* Serve per la segnalazione degli errori */
#include "types.h"
#include "graphic.h"	/* Dichiaro alcune strutture grafiche generali */

static void not_available(void);
static void ps_close_win(void);
static void ps_rreset(void);
static void ps_rinit(void);
static void ps_rdraw(void);
static void ps_rline(Point a, Point b);
static void ps_rcong(Point a, Point b, Point c);
static void ps_rcircle(Point ctr, Point a, Point b);
static void ps_rfgcolor(Real r, Real g, Real b);
static void ps_rbgcolor(Real r, Real g, Real b);
static int ps_save(void);

static void not_available(void)
{
	ERRORMSG("not_available", "Funzione non disponibile per finestre postscript");
	return;
}

/* Queste funzioni non sono disponibili per finestre postscript
 */
static void (*ps_lowfn[])() = {
	ps_close_win, not_available, not_available, not_available
};

/* Lista delle funzioni di rasterizzazione */
static void (*ps_midfn[])() = {
	ps_rreset, ps_rinit, ps_rdraw,
	ps_rline, ps_rcong, not_available,
	ps_rcircle, ps_rfgcolor, not_available
};

/* Variabili usate dalle procedure per scrivere il file postscript */
static int beginning_of_line = 1, beginning_of_path = 1;
static long previous_px, previous_py;

/* Le coordinate dei punti passati alle funzioni grafiche in questo file,
 * sono espresse in millimetri: le converto nelle unita' postscript:
 *  1 unita' = 1/72 inch = 0.35277777... millimetri
 */
#define PS_POINT(p, px, py) \
	long px = (p.x * 2.8346457), \
	      py = (p.y * 2.8346457);

static void ps_close_win(void)
{
	fclose((FILE *) grp_win->ptr);
	return;
}

static void ps_rreset(void)
{
	beginning_of_line = 1;
	beginning_of_path = 1;
	return;
}

static void ps_rinit(void)
{
	return;
}

static void ps_rdraw(void)
{
	if ( ! beginning_of_path )
		fprintf( (FILE *) grp_win->ptr, " eofill\n");

	return;
}

static void ps_rline(Point a, Point b)
{
	PS_POINT(a, ax, ay); PS_POINT(b, bx, by);

	if ( beginning_of_line ) {
		beginning_of_line = 0;
		if ( beginning_of_path ) {
			fprintf( (FILE *) grp_win->ptr,
			 " newpath %ld %ld moveto %ld %ld lineto", ax, ay, bx, by );
			previous_px = bx; previous_py = by;
			beginning_of_path = 0;
			return;

		} else {
			fprintf( (FILE *) grp_win->ptr,
			 " %ld %ld moveto %ld %ld lineto", ax, ay, bx, by );
			previous_px = bx; previous_py = by;
			beginning_of_path = 0;
			return;
		}

	} else {
		if ( (ax == previous_px) && (ay == previous_py) ) {
			fprintf( (FILE *) grp_win->ptr,
			 " %ld %ld lineto", bx, by );
			previous_px = bx; previous_py = by;
			return;

		} else {
			fprintf( (FILE *) grp_win->ptr,
			 " %ld %ld moveto %ld %ld lineto", ax, ay, bx, by );
			previous_px = bx; previous_py = by;
			return;
		}
	}

	return;
}

static void ps_rcong(Point a, Point b, Point c)
{
	PS_POINT(a, ax, ay); PS_POINT(b, bx, by); PS_POINT(c, cx, cy);

	if ( beginning_of_path )
		fprintf( (FILE *) grp_win->ptr, " newpath" );

	fprintf( (FILE *) grp_win->ptr,
	 " %ld %ld %ld %ld %ld %ld cong", ax, ay, bx, by, cx, cy );
	previous_px = cx; previous_py = cy;
	beginning_of_line = 0;
	return;
}

static void ps_rcircle(Point ctr, Point a, Point b)
{
	PS_POINT(ctr, cx, cy); PS_POINT(a, ax, ay); PS_POINT(b, bx, by);

	if ( beginning_of_path )
		fprintf( (FILE *) grp_win->ptr, " newpath" );

	fprintf( (FILE *) grp_win->ptr,
	 " %ld %ld %ld %ld %ld %ld circle", cx, cy, ax, ay, bx, by );
	beginning_of_line = 1;
	beginning_of_path = 0;
	return;
}

static void ps_rfgcolor(Real r, Real g, Real b)
{
	fprintf( (FILE *) grp_win->ptr,
	 "  %g %g %g setrgbcolor\n", r, g, b );
	return;
}

static void ps_rbgcolor(Real r, Real g, Real b)
{
	return;
}


/***************************************************************************************/
/* PROCEDURE DI GESTIONE DELLA FINESTRA GRAFICA */

/* NOME: ps_open_win
 * DESCRIZIONE: Apre una finestra grafica di tipo postscript.
 *  Tale finestra tradurra' i comandi grafici ricevuti in istruzioni postscript,
 *  che saranno scritte immediatamente su file.
 */
grp_window *ps_open_win(char *file)
{
	grp_window *wd;
	FILE *winstream;

	/* Creo la finestra */
	wd = (grp_window *) malloc( sizeof(grp_window) );
	if ( wd == NULL ) {
		ERRORMSG("ps_open_win", "Memoria esaurita");
		return NULL;
	}

	winstream = (FILE *) malloc( sizeof(FILE) );
	if ( winstream == NULL) {
		ERRORMSG("ps_open_win", "Memoria esaurita");
		free(wd);
		return NULL;
	}

	/* Apro il file su cui verranno scritte le istruzioni postscript */
	winstream = fopen(file, "w");
	if ( winstream == NULL ) {
		ERRORMSG("ps_open_win", "Impossibile aprire il file");
		free(wd); free(winstream);
		return NULL;
	}

	wd->ptr = (void *) winstream;

	/* Costruisco una tavolazza di colori da associare a questa finestra *
	wd->pal = grp_palette_build(2, 2, 3, 4);
	if ( wd->pal == NULL ) return NULL;

	* La prima richiesta di colore corrisponde all'indice di colore 0,
	 * cioe' all'indice dello sfondo della figura.
	 * Quindi setto il colore dello sfondo a RGB = {255, 255, 255} = bianco
	 *
	wd->bgcol = grp_color_request( wd->pal, & ((color) {255, 255, 255}) );
	if ( wd->bgcol == NULL ) return NULL;

	* Setto il colore di primo piano a RGB = {0, 0, 0} = nero *
	wd->fgcol = grp_color_request( wd->pal, & ((color) {0, 0, 0}) );
	if ( wd->fgcol == NULL ) return NULL;*/

	/* Ora do' le procedure per gestire la finestra */
	wd->save = ps_save;
	wd->lowfn = ps_lowfn;
	wd->midfn = ps_midfn;

	/* Scrivo l'intestazione del file */
	fprintf(winstream,
	 "%%!PS-Adobe-2.0\n%%%%Title: %s\n%%%%Creator: gdl Version ???\n"
	 "%%%%Orientation: Landscape\n%%%%Pages: 1\n%%%%BoundingBox: 0 0 612 792\n"
	 "%%%%BeginSetup\n%%%%IncludeFeature: *PageSize Letter\n%%%%EndSetup\n"
	 "%%%%Magnification: 1.0000\n%%%%EndComments\n\n"
	 "/congdict 8 dict def\n\ncongdict /mtrx matrix put\n"
	 "/cong {\ncongdict begin\n  /yc exch def /xc exch def\n"
	 "  /yb exch def /xb exch def\n  /ya exch def /xa exch def\n\n"
	 "    /xu xb xc sub def /yu yb yc sub def\n"
	 "    /xv xb xa sub def /yv yb ya sub def\n"
	 "    /xo xa xu sub def /yo ya yu sub def\n\n"
	 "    /savematrix mtrx currentmatrix def\n    [xu yu xv yv xo yo] setmatrix\n"
	 "    0 0 1 0 90 arc\n    savematrix setmatrix\n\n  end\n} def\n\n"
	 "/circledict 8 dict def\n\ncircledict /mtrx matrix put\n"
	 "/circle {\ncircledict begin\n  /yb exch def /xb exch def\n"
	 "  /ya exch def /xa exch def\n  /yo exch def /xo exch def\n\n"
	 "    /xu xa xo sub def /yu ya yo sub def\n"
	 "    /xv xb xo sub def /yv yb yo sub def\n\n"
	 "    /savematrix mtrx currentmatrix def\n    [xu yu xv yv xo yo] setmatrix\n"
	 "    0 0 1 0 360 arc\n    savematrix setmatrix\n\n  end\n} def\n\n"
	 "[1 0 0 1 0 0] setmatrix\n\n"
	 , file
	);

	return wd;
}

static int ps_save(void)
{
	fclose((FILE *) grp_win->ptr);
	return 1;
}