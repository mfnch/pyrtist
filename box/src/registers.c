/***************************************************************************
 *   Copyright (C) 2006 by Matteo Franchin                                 *
 *   fnch@libero.it                                                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

/* $Id$ */

/* registers.c, agosto 2004
 *
 * Questo file contiene le funzioni necessarie per tener conto dei registri
 * temporanei occupati dal parser nella fase di compilazione.
 */

/*#define DEBUG*/

#include <stdlib.h>
#include <stdio.h>

#include "types.h"
#include "defaults.h"
#include "messages.h"
#include "array.h"
#include "registers.h"

#define END_OF_CHAIN -1
#define OCCUPIED 0

/******************************************************************************
 * Le procedure che seguono servono a gestire le liste di occupazione dei     *
 * registri. Tali liste servono per compilare le espressioni matematiche e    *
 * dicono quali registri sono occupati (cioe' contengono un valore che serve  *
 * al compilatore) e quali invece sono liberi.                                *
 ******************************************************************************/

/* Variabili interne */
static Array *reg_occ[NUM_TYPES];
static Intg reg_max[NUM_TYPES];

/* DESCRIZIONE: Inizializza gli array che tengono nota dei registri occupati
 *  e setta la loro "dimensione a riposo". Questa quantita'(per ciascun
 *  tipo di registro) deve essere piu' grande del numero di registri che
 *  ci si aspetta saranno occupati contemporanemente, in questo modo saranno
 *  eseguite poche realloc.
 */
Task Reg_Init(UInt typical_num_reg[NUM_TYPES])
{
	int i;

	MSG_LOCATION("Reg_Init");

	for (i = 0; i < NUM_TYPES; i++) {
		/* Resetto i numeri massimi dei registri */
		reg_max[i] = 0;
		/* Preparo le array */
		Arr_Destroy( reg_occ[i] );
		if ( ( reg_occ[i] = Array_New(sizeof(Intg), typical_num_reg[i]) )
		 == NULL ) return Failed;

		/* Svuota le catene dei registri disponibili */
		Arr_Chain( reg_occ[i] ) = END_OF_CHAIN;
	}

	return Success;
}

/* DESCRIZIONE: Restituisce un numero di registro libero e lo occupa,
 *  in modo tale che questo numero di registro non venga piu' restituito
 *  nelle prossime chiamate a Reg_Occupy, a meno che il registro
 *  non venga liberato con Reg_Release.
 *  t e' il tipo di registro, per ciascuno dei tipi di registro
 *  Reg_Occupy funziona in maniera indipendente.
 * NOTA: Il numero di registro restituito e' sempre maggiore di 1,
 *  viene restituito 0 solo in caso di errori.
 */
Intg Reg_Occupy(Intg t) {
  Array *rb;

  if ( (t < 0) || (t >= NUM_TYPES) ) {
    MSG_ERROR("Tipo di registro errato");
    return 0;
  }

  rb = reg_occ[t];
  if (Arr_Chain(rb) == END_OF_CHAIN) {
    /* Se la catena dei registri disponibili e' vuota non resta che creare
     * un nuovo registro, contrassegnarlo come occupato e restituirlo.
     */
    Intg ni;

    Arr_Inc(rb);
    Arr_LastItem(rb, Intg) = OCCUPIED;

    ni = Arr_NumItem(rb);
    if (ni > reg_max[t]) reg_max[t] = ni;
#ifdef DEBUG
    printf("Reg_Occupy: occupo (tipo="SIntg", num="SIntg")\n", t, ni);
#endif
    return ni;

  } else {
    long regnum;

    regnum = Arr_Chain(rb);
    Arr_Chain(rb) = Arr_Item(rb, Intg, regnum);
    Arr_Item(rb, Intg, regnum) = OCCUPIED;
#ifdef DEBUG
    printf("Reg_Occupy: occupo (tipo="SIntg", num="SIntg")\n", t, regnum);
#endif
    return regnum;
  }
}

/* DESCRIZIONE: Vedi Reg_Occupy.
 */
Task Reg_Release(Intg t, UInt regnum) {
  Array *rb;

#ifdef DEBUG
    printf("Reg_Release: rilascio (tipo="SIntg", num="SIntg")\n", t, regnum);
#endif

  if ( (t < 0) || (t >= NUM_TYPES) ) {
    MSG_ERROR("Reg_Release: Tipo di registro errato");
    return Failed;
  }

  rb = reg_occ[t];
  if (regnum > Arr_NumItem(rb)) {
    MSG_ERROR("Reg_Release: Registro non occupato(1)");
    return Failed;
  }

  if (Arr_Item(rb, Intg, regnum) != OCCUPIED) {
    MSG_ERROR("Reg_Release: Already released register (2):"
     " type = %d, num = %d", t, regnum);
    return Failed;
  }

  Arr_Item(rb, Intg, regnum) = Arr_Chain(rb);
  Arr_Chain(rb) = regnum;
  return Success;
}

/* DESCRIZIONE: Restituisce il numero di registro massimo fin'ora utilizzato.
 */
Intg Reg_Num(Intg t) {
	MSG_LOCATION("Reg_Num");

	if ( (t < 0) || (t >= NUM_TYPES) ) return 0;
	 else return reg_max[t];
}

/******************************************************************************/
/* La parte che resta di questo file implementa una variante dell'algoritmo
 * di occupazione dei registri che e' stato implementato nella prima parte
 * di questo file. Le seguenti funzioni infatti servono a tener conto
 * dell'occupazione delle variabili (compito piu' delicato di quello relativo
 * ai registri).
 */

/* Struttura che serve a costruire l'array che contiene le informazioni
 * delle variabili occupate e di quelle libere.
 */
typedef struct {
	int level; /* livello di sessione */
	int chain; /* Se = 0 indica occupazione, altrimenti serve per
				  costruire una catena di elementi non occupati. */
} VarItem;

static Array *var_occ[NUM_TYPES];
static Intg var_max[NUM_TYPES];

/* DESCRIZIONE: Inizializza gli array che tengono nota dei registri occupati
 *  e setta la loro "dimensione a riposo". Questa quantita'(per ciascun
 *  tipo di registro) deve essere piu' grande del numero di registri che
 *  ci si aspetta saranno occupati contemporanemente, in questo modo saranno
 *  eseguite poche realloc.
 */
Task Var_Init(UInt typical_num_var[NUM_TYPES]) {
	int i;

	MSG_LOCATION("Var_Init");

	for (i = 0; i < NUM_TYPES; i++) {
		/* Resetto i numeri massimi di variabile */
		var_max[i] = 0;
		/* Preparo le array */
		Arr_Destroy( var_occ[i] );
		if ( ( var_occ[i] = Array_New(sizeof(VarItem), typical_num_var[i]) )
		 == NULL ) return Failed;

		/* Svuota le catene dei registri disponibili */
		Arr_Chain( var_occ[i] ) = END_OF_CHAIN;
	}

	return Success;
}

/* DESCRIZIONE: Restituisce un numero di variabile libera e lo occupa.
 *  Questo numero non verra' piu' restituito fino a quando la variabile verra'
 *  liberata con una chiamata a Var_Release.
 *  Se la variabile e' libera Var_Occupy puo' restituirla a patto che
 *  il suo precedente livello (il livello che essa possedeva al momento
 *  dell'ultima liberazione con Var_Release) e' superiore a level.
 *  t e' il tipo di registro, per ciascuno dei tipi di registro
 *  Var_Occupy funziona in maniera indipendente.
 * NOTA: Il numero di registro restituito e' sempre maggiore di 1,
 *  viene restituito 0 solo in caso di errori.
 */
Intg Var_Occupy(Intg type, Intg level) {
	Array *rb;
	VarItem *vi, *last_vi;
	Intg ni;
	long occ;

	MSG_LOCATION("Var_Occupy");

	if ( (type < 0) || (type >= NUM_TYPES) ) {
		MSG_ERROR("Tipo di registro errato");
		return 0;
	}

	rb = var_occ[type];

	/* Scorro la catena delle variabili libere finche'
	 * non ne trovo una di livello non inferiore a level
	 */
	last_vi = NULL;
	for ( occ = Arr_Chain(rb); occ != END_OF_CHAIN; ) {
		vi = Arr_ItemPtr(rb, VarItem, occ);
		if ( vi->level <= level ) {
			/* Ho trovato quel che cercavo! */
			/* La variabile non e' piu' libera: la tolgo dalla catena! */
			if ( last_vi == NULL ) {
				Arr_Chain(rb) = vi->chain;
				vi->chain = OCCUPIED;
				return occ;
			}
			last_vi->chain = vi->chain;
			vi->chain = OCCUPIED;
			return occ;
		}
		occ = vi->chain;
		last_vi = vi;
	}

	/* Se la catena delle variabili libere non si puo' sfruttare non resta che
	 * creare una nuova variabile, contrassegnarla come occupata e restituirla.
	 */
	Arr_Inc(rb);
	vi = Arr_LastItemPtr(rb, VarItem);
	vi->chain = OCCUPIED;
	vi->level = level;

	ni = Arr_NumItem(rb);
	if (ni > var_max[type]) var_max[type] = ni;
	return ni;
}

/* DESCRIZIONE: Vedi Var_Occupy.
 */
Task Var_Release(Intg type, UInt varnum)
{
	Array *rb;
	VarItem *vi;

	MSG_LOCATION("Var_Release");

	if ( (type < 0) || (type >= NUM_TYPES) ) {
		MSG_ERROR("Tipo di variabile errata");
		return Failed;
	}

	rb = var_occ[type];
	if (varnum > Arr_NumItem(rb)) {
		MSG_ERROR("Variabile non occupata(1)");
		return Failed;
	}

	vi = Arr_ItemPtr(rb, VarItem, varnum);
	if (vi->chain != OCCUPIED) {
		MSG_ERROR("Variabile non occupata(2)");
		return Failed;
	}

	vi->chain = Arr_Chain(rb);
	Arr_Chain(rb) = varnum;
	return Success;
}

/* DESCRIZIONE: Restituisce il numero di variabile massimo fin'ora utilizzato.
 */
Intg Var_Num(Intg type) {
	MSG_LOCATION("Var_Num");

	if ( (type < 0) || (type >= NUM_TYPES) ) return 0;
	 else return var_max[type];
}

/* DESCRIPTION: This function writes (starting at the address num_var)
 *  an array of Intg with the number of used variables, and (address num_reg)
 * an array of Intg with the number of used registers.
 */
void RegVar_Get_Nums(Intg *num_var, Intg *num_reg) {
  register int i;
  for (i = 0; i < NUM_TYPES; i++) {
    *(num_reg++) = reg_max[i];
    *(num_var++) = var_max[i];
  }
}
