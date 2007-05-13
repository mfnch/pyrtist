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
#include <assert.h>

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

static RegAlloc ra_obj, *ra = & ra_obj;

static Task Frame_Destructor(void *reg_frame_ptr) {
  RegFrame *reg_frame = (RegFrame *) reg_frame_ptr;
  int i;
  for(i=0; i<NUM_TYPES; i++) {
    if (reg_frame->reg_occ[i] != (Collection *) NULL)
      Clc_Destroy(reg_frame->reg_occ[i]);
    if (reg_frame->var_occ[i] != (Array *) NULL)
      Arr_Destroy(reg_frame->var_occ[i]);
  }
  return Success;
}

/*  Inizializza gli array che tengono nota dei registri occupati
 *  e setta la loro "dimensione a riposo". Questa quantita'(per ciascun
 *  tipo di registro) deve essere piu' grande del numero di registri che
 *  ci si aspetta saranno occupati contemporanemente, in questo modo saranno
 *  eseguite poche realloc.
 */
Task Reg_Init(void) {
  TASK( Arr_New(& ra->reg_frame, sizeof(RegFrame), 2) );
  Arr_Destructor(ra->reg_frame, Frame_Destructor);
  Reg_Frame_Push();
  return Success;
}

void Reg_Destroy(void) {
  Arr_Destroy(ra->reg_frame);
}

void Reg_Frame_Push(void) {
  int i;
  RegFrame new_frame;
  for(i=0; i<NUM_TYPES; i++) {
    new_frame.reg_occ[i] = (Collection *) NULL;
    new_frame.var_occ[i] = (Array *) NULL;
  }
  (void) Arr_Push(ra->reg_frame, & new_frame);
}

Task Reg_Frame_Pop(void) {
  return Arr_Pop(ra->reg_frame);
}

Int Reg_Frame_Get(void) {return Arr_NumItem(ra->reg_frame);}

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
  UInt reg_num;
  RegFrame *rf = Arr_LastItemPtr(ra->reg_frame, RegFrame);
  Task task;

  assert(t >= 0 && t < NUM_TYPES);
  if (rf->reg_occ[t] == (Collection *) NULL) {
    task = Clc_New(& rf->reg_occ[t], 0, REG_OCC_TYP_SIZE);
    assert(task == Success);
  }

  task = Clc_Occupy(rf->reg_occ[t], NULL, & reg_num);
  assert(task == Success);
  return (Int) reg_num;
}

/* Vedi Reg_Occupy.
 */
Task Reg_Release(Int t, UInt reg_num) {
  RegFrame *rf = Arr_LastItemPtr(ra->reg_frame, RegFrame);

  assert(t >= 0 && t < NUM_TYPES);
  if (rf->reg_occ[t] == (Collection *) NULL) {
    MSG_ERROR("Reg_Release: trying to release a non-occupied register!");
    return Failed;
  }

  return Clc_Release(rf->reg_occ[t], reg_num);
}

/* Restituisce il numero di registro massimo fin'ora utilizzato.
 */
Int Reg_Num(Int t) {
  RegFrame *rf = Arr_LastItemPtr(ra->reg_frame, RegFrame);

  assert(t >= 0 && t < NUM_TYPES);
  if (rf->reg_occ[t] == (Collection *) NULL) return 0;
  return Clc_MaxIndex(rf->reg_occ[t]);
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

/*  Restituisce un numero di variabile libera e lo occupa.
 *  Questo numero non verra' piu' restituito fino a quando la variabile verra'
 *  liberata con una chiamata a Var_Release.
 *  Se la variabile e' libera Var_Occupy puo' restituirla a patto che
 *  il suo precedente livello (il livello che essa possedeva al momento
 *  dell'ultima liberazione con Var_Release) sia superiore a level.
 *  t e' il tipo di registro, per ciascuno dei tipi di registro
 *  Var_Occupy funziona in maniera indipendente.
 * NOTA: Il numero di registro restituito e' sempre maggiore di 1,
 *  viene restituito 0 solo in caso di errori.
 */
Int Var_Occupy(Int type, Int level) {
  RegFrame *rf = Arr_LastItemPtr(ra->reg_frame, RegFrame);
  Array *rb;
  VarItem *vi, *last_vi;
  Int ni;
  long occ;

  assert(type >= 0 && type < NUM_TYPES);

  if (rf->var_occ[type] == (Array *) NULL) {
    /* Preparo le array */
    Task t = Arr_New(& rf->var_occ[type], sizeof(VarItem), VAR_OCC_TYP_SIZE);
    assert(t == Success);
    /* Svuota le catene dei registri disponibili */
    Arr_Chain(rf->var_occ[type]) = END_OF_CHAIN;
    rf->var_max[type] = 0; /* Resetto i numeri massimi di variabile */
  }

  rb = rf->var_occ[type];

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
  if (ni > rf->var_max[type]) rf->var_max[type] = ni;
  return ni;
}

/* Vedi Var_Occupy.
 */
Task Var_Release(Int type, UInt varnum) {
  RegFrame *rf = Arr_LastItemPtr(ra->reg_frame, RegFrame);
  Array *rb;
  VarItem *vi;

  assert(type >= 0 && type < NUM_TYPES);

  if (rf->var_occ[type] == (Array *) NULL) {
    MSG_ERROR("Var_Release: trying to release a non-occupied variable!");
    return Failed;
  }

  rb = rf->var_occ[type];
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
  RegFrame *rf = Arr_LastItemPtr(ra->reg_frame, RegFrame);
  assert(type >= 0 && type < NUM_TYPES);
  if (rf->var_occ[type] == (Array *) NULL) return 0;
  return rf->var_max[type];
}

/* This function writes (starting at the address num_var)
 * an array of Int with the number of used variables, and (address num_reg)
 * an array of Int with the number of used registers.
 */
void RegVar_Get_Nums(Int *num_var, Int *num_reg) {
  int i;
  for (i = 0; i < NUM_TYPES; i++) {
    *(num_reg++) = Reg_Num(i);
    *(num_var++) = Var_Num(i);
  }
}
