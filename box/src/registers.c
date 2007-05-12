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

static RegAlloc ra_instance, *ra = & ra_instance;

/*  Inizializza gli array che tengono nota dei registri occupati
 *  e setta la loro "dimensione a riposo". Questa quantita'(per ciascun
 *  tipo di registro) deve essere piu' grande del numero di registri che
 *  ci si aspetta saranno occupati contemporanemente, in questo modo saranno
 *  eseguite poche realloc.
 */
Task Reg_Init(UInt typical_num_reg[NUM_TYPES]) {
  int i;

  for (i = 0; i < NUM_TYPES; i++) {
    /* Resetto i numeri massimi dei registri */
    ra->reg_max[i] = 0;
    /* Preparo le array */
    Arr_Destroy( ra->reg_occ[i] );
    if ( (ra->reg_occ[i] = Array_New(sizeof(Int), typical_num_reg[i]) )
         == NULL ) return Failed;

    /* Svuota le catene dei registri disponibili */
    Arr_Chain( ra->reg_occ[i] ) = END_OF_CHAIN;
  }

  return Success;
}

/*  Restituisce un numero di registro libero e lo occupa,
 *  in modo tale che questo numero di registro non venga piu' restituito
 *  nelle prossime chiamate a Reg_Occupy, a meno che il registro
 *  non venga liberato con Reg_Release.
 *  t e' il tipo di registro, per ciascuno dei tipi di registro
 *  Reg_Occupy funziona in maniera indipendente.
 * NOTA: Il numero di registro restituito e' sempre maggiore di 1,
 *  viene restituito 0 solo in caso di errori.
 */
Int Reg_Occupy(Int t) {
  Array *rb;

  if (t < 0 || t >= NUM_TYPES) {
    MSG_ERROR("Tipo di registro errato");
    return 0;
  }

  rb = ra->reg_occ[t];
  if (Arr_Chain(rb) == END_OF_CHAIN) {
    /* Se la catena dei registri disponibili e' vuota non resta che creare
     * un nuovo registro, contrassegnarlo come occupato e restituirlo.
     */
    Int ni;

    Arr_Inc(rb);
    Arr_LastItem(rb, Int) = OCCUPIED;

    ni = Arr_NumItem(rb);
    if (ni > ra->reg_max[t]) ra->reg_max[t] = ni;
#ifdef DEBUG
    printf("Reg_Occupy: occupo (tipo="SInt", num="SInt")\n", t, ni);
#endif
    return ni;

  } else {
    long regnum;

    regnum = Arr_Chain(rb);
    Arr_Chain(rb) = Arr_Item(rb, Int, regnum);
    Arr_Item(rb, Int, regnum) = OCCUPIED;
#ifdef DEBUG
    printf("Reg_Occupy: occupo (tipo="SInt", num="SInt")\n", t, regnum);
#endif
    return regnum;
  }
}

/* Vedi Reg_Occupy.
 */
Task Reg_Release(Int t, UInt regnum) {
  Array *rb;

#ifdef DEBUG
    printf("Reg_Release: rilascio (tipo="SInt", num="SInt")\n", t, regnum);
#endif

  if (t < 0 || t >= NUM_TYPES) {
    MSG_ERROR("Reg_Release: Tipo di registro errato");
    return Failed;
  }

  rb = ra->reg_occ[t];
  if (regnum > Arr_NumItem(rb)) {
    MSG_ERROR("Reg_Release: Registro non occupato(1)");
    return Failed;
  }

  if (Arr_Item(rb, Int, regnum) != OCCUPIED) {
    MSG_ERROR("Reg_Release: Already released register (2):"
     " type = %d, num = %d", t, regnum);
    return Failed;
  }

  Arr_Item(rb, Int, regnum) = Arr_Chain(rb);
  Arr_Chain(rb) = regnum;
  return Success;
}

/* Restituisce il numero di registro massimo fin'ora utilizzato.
 */
Int Reg_Num(Int t) {
  if (t < 0 || t >= NUM_TYPES) return 0;
    else return ra->reg_max[t];
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

/*  Inizializza gli array che tengono nota dei registri occupati
 *  e setta la loro "dimensione a riposo". Questa quantita'(per ciascun
 *  tipo di registro) deve essere piu' grande del numero di registri che
 *  ci si aspetta saranno occupati contemporanemente, in questo modo saranno
 *  eseguite poche realloc.
 */
Task Var_Init(UInt typical_num_var[NUM_TYPES]) {
  int i;

  for (i = 0; i < NUM_TYPES; i++) {
    /* Resetto i numeri massimi di variabile */
    ra->var_max[i] = 0;
    /* Preparo le array */
    Arr_Destroy( ra->var_occ[i] );
    if ( ( ra->var_occ[i] = Array_New(sizeof(VarItem), typical_num_var[i]) )
      == NULL ) return Failed;

    /* Svuota le catene dei registri disponibili */
    Arr_Chain( ra->var_occ[i] ) = END_OF_CHAIN;
  }

  return Success;
}

/*  Restituisce un numero di variabile libera e lo occupa.
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
Int Var_Occupy(Int type, Int level) {
  Array *rb;
  VarItem *vi, *last_vi;
  Int ni;
  long occ;

  if (type < 0 || type >= NUM_TYPES) {
    MSG_ERROR("Tipo di registro errato");
    return 0;
  }

  rb = ra->var_occ[type];

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
  if (ni > ra->var_max[type]) ra->var_max[type] = ni;
  return ni;
}

/* Vedi Var_Occupy.
 */
Task Var_Release(Int type, UInt varnum) {
  Array *rb;
  VarItem *vi;

  if (type < 0 || type >= NUM_TYPES) {
    MSG_ERROR("Tipo di variabile errata");
    return Failed;
  }

  rb = ra->var_occ[type];
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

/* Restituisce il numero di variabile massimo fin'ora utilizzato.
 */
Int Var_Num(Int type) {
  if (type < 0 || type >= NUM_TYPES) return 0;
    else return ra->var_max[type];
}

/* This function writes (starting at the address num_var)
 * an array of Int with the number of used variables, and (address num_reg)
 * an array of Int with the number of used registers.
 */
void RegVar_Get_Nums(Int *num_var, Int *num_reg) {
  register int i;
  for (i = 0; i < NUM_TYPES; i++) {
    *(num_reg++) = ra->reg_max[i];
    *(num_var++) = ra->var_max[i];
  }
}
