/* graphic.c - Autore: Franchin Matteo - 26 dicembre 2002
 *
 * Questo file contiene procedure di grafica generali
 * NOTA: da compilare con error.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

#include "error.h"			/* Serve per la segnalazione degli errori */
#include "graphic.h"		/* Dichiaro alcune strutture grafiche generali */

/* Descrittore della finestra attualmente in uso: lo faccio puntare
 * ad una finestra che da solo errori (vedi piu' avanti).
 */
grp_window *grp_win = & grp_empty_win;

/* Costanti moltiplicative usate per convertire: */
/*  - lunghezze in millimetri: */
FCOOR grp_tomm	= 1.0;
/* - angoli in radianti: */
FCOOR grp_torad = grp_radperdeg;
/* - risoluzioni in punti per millimetro: */
FCOOR grp_toppmm = grp_ppmmperdpi;

/********************************************************************/
/*           FUNZIONI DI INIZIALIZZAZIONE DELLA LIBRERIA            */
/********************************************************************/

/* Creo una finestra priva di qualsiasi funzione: essa dara' un errore
 * per qualsiasi operazione s tenti di eseguire.
 * La finestra cosi' creata sara' la finestra iniziale, che corrisponde
 * allo stato "nessuna finestra ancora aperta" (in modo da evitare
 * accidentali "segmentation fault").
 */

static void win_not_opened(void)
{
	ERRORMSG("win_not_opened", "Nessuna finestra aperta");
	return;
}

/* Lista delle funzioni di basso livello (non disponibili) */
static void (*wno_lowfn[])() = {
	win_not_opened, win_not_opened, win_not_opened, win_not_opened
};

/* Lista delle funzioni di rasterizzazione */
static void (*wno_midfn[])() = {
	win_not_opened, win_not_opened, win_not_opened,
	win_not_opened, win_not_opened, win_not_opened,
	win_not_opened, win_not_opened, win_not_opened
};

grp_window grp_empty_win = {
	NULL, 0.0, 0.0, 0.0, 0.0,		/* ptr, ltx, lty, rdx, rdy */
	0.0, 0.0, 0.0, 0.0, 0.0, 0.0,	/* minx, miny, maxx, maxy, lx, ly */
	0.0, 0.0, 0.0, 0.0, 0.0, 0.0,	/* versox, versoy, stepx, stepy, resx, resy */
	0, 0, NULL, NULL, NULL,			/* numptx, numpty, bgcol, fgcol, pal */
	0, 0, 0, NULL, 					/* bitperpixel, bytesperline, dim, wrdep */
	win_not_opened,	/* save */
	wno_lowfn,		/* lowfn */
	wno_midfn		/* midfn */
};

/********************************************************************/
/*             FUNZIONI GENERALI DI CALCOLO GEOMETRICO              */
/********************************************************************/

/* DESCRIZIONE: Questa procedura serve a cambiare sistema di riferimento:
 *  Il nuovo sistema di riferimento e' specificato dal punto d'origine o
 *  dal vettore v che FISSA LA DIREZIONE dell'asse x (non la scala).
 *  L'ultimo argomento p e' un punto le cui coordinate sono specificate
 *  rispetto a questo nuovo sistema di riferimento.
 *  La funzione restituisce le coordinate di p nel vecchio sistema.
 * NOTA: v viene normalizzato e messo in vx, questo viene ruotato in direzione
 *  oraria (assex -> assey) di 90° per ottenere vy e infine viene restituito
 *  il valore di (p.x * vx + p.y * vy).
 * ATTENZIONE! Ad ogni chiamata la procedura mette il risultato in una
 *  variabile statica e restituisce sempre il relativo puntatore
 *  (che percio' non cambia da una chiamata all'altra!).
 *  Bisogna quindi salvare o comunque utilizzare il risultato prima
 *  di ri-chiamare grp_ref!
 */
Point *grp_ref(Point *o, Point *v, Point *p)
{
	register FCOOR c, cx, cy;
	static Point result;

	/* Normalizzo v e trovo pertanto vx */
	cx = v->x; cy = v->y;
	c = sqrt(cx*cx + cy*cy);
	if (c == 0.0) {
		ERRORMSG("grp_ref",
		"Punti coincidenti: impossibile costruire il riferimento cartesiano!");
		return NULL;
	}

	cx /= c; cy /= c;

	result.x = o->x + p->x * cx - p->y * cy;
	result.y = o->y + p->x * cy + p->y * cx;

	return & result;
}

/********************************************************************/
/*           GESTIONE DELLE TAVOLAZZE DI COLORI (PALETTE)           */
/********************************************************************/

/* Dichiaro la procedura di hash per trovare velocemente i colori */
static unsigned long color_hash(palette *p, color *c);

/* Questa macro serve a determinare se due colori coincidono o meno
 * (c1 e c2 sono i due puntatori a strutture di tipo color)
 */
#define COLOR_EQUAL(c1, c2) \
 ( ((c1)->r == (c2->r)) && ((c1)->g == (c2->g)) && ((c1)->b == (c2->b)) )


/* DESCRIZIONE: Traduce un colore con componenti RGB date come numeri
 *  compresi tra 0.0 e 1.0, in una struttura di tipo color.
 *  r, g, b sono le componenti iniziali, *c conterra' la struttura finale.
 */
void grp_color_build(Real r, Real g, Real b, color *c)
{
	if ( r < 0.0 ) c->r = 0; else if ( r > 1.0 ) c->r = 255; else c->r = 255*r;
	if ( g < 0.0 ) c->g = 0; else if ( g > 1.0 ) c->g = 255; else c->g = 255*g;
	if ( b < 0.0 ) c->b = 0; else if ( b > 1.0 ) c->b = 255; else c->b = 255*b;
	return;
}

/* DESCRIZIONE: Arrotonda un colore. Il risultato e' simile all'arrotondamento
 *  di un numero reale in un numero intero: usando questa funzione si riduce
 *  il numero complessivo dei colori utilizzati.
 */
void grp_color_reduce(palette *p, color *c)
{
	register unsigned int mask, add, col;
	unsigned int mtable[8] =
	 {0777, 0776, 0774, 0770, 0760, 0740, 0700, 0600};
	unsigned int atable[8] =
	 {0x0, 0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40};

	mask = mtable[p->reduce];
	add = atable[p->reduce];
	col = ( ((unsigned int) c->r) + add ) & mask;
	c->r = (col >= 255) ? 255 : col;
	col = ( ((unsigned int) c->g) + add ) & mask;
	c->g = (col >= 255) ? 255 : col;
	col = ( ((unsigned int) c->b) + add ) & mask;
	c->b = (col >= 255) ? 255 : col;
	return;
}

/* DESCRIZIONE: Costruisce una tavolazza di colori.
 *  numcol specifica il numero dei colori. La funzione restituisce il puntatore
 *  alla nuova struttura di tipo palette, oppure restituisce NULL, in caso di
 *  insuccesso.
 */
palette *grp_palette_build(long numcol, long hashdim, long hashmul, int reduce)
{
	palette *p;

	if ( (numcol < 1) || (hashmul < 2) ) {
		ERRORMSG("grp_build_palette", "Errore nei parametri");
		return NULL;
	}

	p = (palette *) malloc( sizeof(palette) );
	if ( p == NULL ) {
		ERRORMSG("grp_build_palette", "Memoria esaurita");
		return NULL;
	}

	p->hashdim = hashdim;
	p->hashmul = hashmul;
	p->hashtable = (palitem **) calloc( p->hashdim, sizeof(palitem *) );
	if ( p->hashtable == NULL ) {
		ERRORMSG("grp_build_palette", "Memoria esaurita");
		return NULL;
	}

	p->dim = numcol;
	p->num = 0;

	p->reduce = ( (reduce > 0) && (reduce < 8) ) ?
	 reduce : 0;

	return p;
}

/* DESCRIZIONE: Cerca il colore c fra i colori della tavolazza
 */
palitem *grp_color_find(palette *p, color *c)
{
	palitem *pi;

	for ( pi = p->hashtable[color_hash(p, c)];
	 pi != (palitem *) NULL; pi = pi->next )
		if ( COLOR_EQUAL( & pi->c, c ) )
			return pi;

	return (palitem *) NULL;
}

/* DESCRIZIONE: Inserisce un nuovo colore, fra quelli gia' presenti nella
 *  tavolazza e restituisce la sua posizione in essa. Se e' gia' presente
 *  lo stesso colore (nei limiti di tolleranza definiti in grp_build_palette)
 *  restituisce la posizione di questo (senza creare nulla!).
 *  Se la tavolazza e' piena, si comporta come stabilito durante la creazione
 *  della tavolazza (vedi grp_build_palette per maggiori dettagli).
 *  Se la richiesta di colore non puo' essere soddisfatta restituisce -1.
 */
palitem *grp_color_request(palette *p, color *c)
{
	color c2;
	palitem *new;

	c2 = *c;
	grp_color_reduce(p, & c2);

	new = grp_color_find(p, & c2);

	if (  new == NULL ) {
		/* Colore non ancora introdotto */
		palitem *pi;
		unsigned long hashval;

		if ( p->num >= p->dim) {
			ERRORMSG("grp_color_request", "Tavolazza piena");
			return NULL;
		}

		pi = (palitem *) malloc( sizeof(palitem) );

		if ( pi == NULL ) {
			ERRORMSG("grp_color_request", "Memoria esaurita");
			return NULL;
		}

		pi->index = p->num++;
		pi->c = c2;

		hashval = color_hash(p, & c2);
		pi->next = p->hashtable[hashval];
		p->hashtable[hashval] = pi;

		return pi;
	}

	return new;
}

/* DESCRIZIONE: Scorre tutta la tavolazza dei colori eseguendo l'operazione
 *  operation per ogni elemento *pi della tavolazza.
 *  L'operazione viene svolta chiamando la funzione operator, la quale
 *  restituisce 1 se va tutto bene (in tal caso la scansione della tabella
 *  continua fino alla fine), oppure restituisce 0 se si e' verificato qualche
 *  errore (in tal caso la scansione ha immediatamente termine e
 *  grp_palette_transform esce restituendo 0).
 */
int grp_palette_transform( palette *p, int (*operation)(palitem *pi) )
{
	int i;
	palitem *pi;

	/* Scorriamo tutta la hash-table in cerca di tutti gli elementi */
	for ( i = 0; i < p->hashdim; i++ ) {

		for ( pi = p->hashtable[i];
		 pi != (palitem *) NULL; pi = pi->next ) {

			/* Eseguo l'operazione su ciascun elemento */
			if ( ! operation(pi) ) return 0;
		}

	}

	return 1;
}

/* DESCRIZIONE: Distrugge la palette p, liberando la memoria da essa occupata.
 */
void grp_palette_destroy(palette *p)
{
	int i;
	palitem *pi, *nextpi;

	/* Scorriamo tutta la hash-table in cerca di elementi da cancellare */
	for ( i = 0; i < p->hashdim; i++ ) {

		for ( pi = p->hashtable[i];
		 pi != (palitem *) NULL; pi = nextpi ) {

			nextpi = pi->next;
			free( pi );
		}

	}

	/* Elimino la hash-table */
	free(p->hashtable);
	/* Elimino la struttura "palette" */
	free(p);

	return;
}

/* DESCRIZIONE: Funzione che associa ad ogni colore, un indice (della tavola
 *  di hash) in modo "abbastanza casuale". Serve per trovare velocemente i colori,
 *  senza scorrere tutta la tavolazza.
 */
static unsigned long color_hash(palette *p, color *c)
{
	return ( ( (unsigned long ) c->b ) + (c->g + c->r * p->hashmul) * p->hashmul ) % p->hashdim;
}