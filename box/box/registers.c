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
#include "virtmach.h"

#define END_OF_CHAIN -1
#define OCCUPIED 0

/****************************************************************************
 * Here we implements the logic to associate registers to variables.        *
 ****************************************************************************/

/*
  This is what we want to do:

   a = 1              --> a:vr1
   [b = 2]            --> b:vr2
   c = 3              --> c:vr3
   [[d = 4], [e = 5]] --> d:vr4, e:vr4

 */
typedef struct {
  Int level, /**< scope level of the variable */
      chain; /**< 0 means occupied, otherwise contain a link to the next
                  register in the chain of non occupied registers. */
} VarItem;

static Int Reg_Type(Int type) {
  assert(type >= 0);
  return (type >= NUM_TYPES) ? TYPE_OBJ : type;
}

static void VarFrame_Init(VarFrame *vf) {
  BoxArr_Init(& vf->regs, sizeof(VarItem), 32);
  vf->chain = END_OF_CHAIN;
  vf->max = 0;
}

static void VarFrame_Finish(VarFrame *vf) {
  BoxArr_Finish(& vf->regs);
}

/* See Var_Occupy */
static Int VarFrame_Occupy(VarFrame *vf, UInt level) {
  BoxArr *regs = & vf->regs;
  VarItem *vi, *last_vi;
  Int idx;

  /* Scorro la catena delle variabili libere finche'
   * non ne trovo una di livello non inferiore a level
   */
  last_vi = NULL;
  for (idx = vf->chain; idx != END_OF_CHAIN;) {
    vi = (VarItem *) BoxArr_Item_Ptr(regs, idx);
    if (vi->level <= level) {
      /* Ho trovato quel che cercavo! */
      /* La variabile non e' piu' libera: la tolgo dalla catena! */
      if (last_vi == NULL) {
        vf->chain = vi->chain;
        vi->chain = OCCUPIED;
        return idx;

      } else {
        last_vi->chain = vi->chain;
        vi->chain = OCCUPIED;
        return idx;
      }
    }
    idx = vi->chain;
    last_vi = vi;
  }

  /* Se la catena delle variabili libere non si puo' sfruttare non resta che
   * creare una nuova variabile, contrassegnarla come occupata e restituirla.
   */
  vi = BoxArr_Push(regs, NULL);
  vi->chain = OCCUPIED;
  vi->level = level;

  idx = BoxArr_Num_Items(regs);
  if (idx > vf->max) vf->max = idx;
  return idx;
}

/* See Var_Release */
static void VarFrame_Release(VarFrame *vf, Int idx) {
  BoxArr *regs = & vf->regs;
  VarItem *vi = (VarItem *) BoxArr_Item_Ptr(regs, idx);
  vi->chain = vf->chain;
  vf->chain = idx;
}

/******************************************************************************
 * Le procedure che seguono servono a gestire le liste di occupazione dei     *
 * registri. Tali liste servono per compilare le espressioni matematiche e    *
 * dicono quali registri sono occupati (cioe' contengono un valore che serve  *
 * al compilatore) e quali invece sono liberi.                                *
 ******************************************************************************/

static void RegFrame_Init(RegFrame *rf) {
  int i;
  for(i = 0; i < NUM_TYPES; i++) {
    BoxOcc_Init(& rf->reg_occ[i], 0, REG_OCC_TYP_SIZE);
    VarFrame_Init(& rf->lvar[i]);
  }
}

static void RegFrame_Finish(void *rf_ptr) {
  RegFrame *rf = (RegFrame *) rf_ptr;
  int i;
  for(i = 0; i < NUM_TYPES; i++) {
    BoxOcc_Finish(& rf->reg_occ[i]);
    VarFrame_Finish(& rf->lvar[i]);
  }
}

/*  Inizializza gli array che tengono nota dei registri occupati
 *  e setta la loro "dimensione a riposo". Questa quantita'(per ciascun
 *  tipo di registro) deve essere piu' grande del numero di registri che
 *  ci si aspetta saranno occupati contemporanemente, in questo modo saranno
 *  eseguite poche realloc.
 */
void Reg_Init(RegAlloc *ra) {
  int i;
  BoxArr_Init(& ra->reg_frame, sizeof(RegFrame), 2);
  BoxArr_Set_Finalizer(& ra->reg_frame, RegFrame_Finish);
  Reg_Frame_Push(ra);
  for(i = 0; i < NUM_TYPES; i++)
    VarFrame_Init(& ra->gvar[i]);
}

void Reg_Destroy(RegAlloc *ra) {
  int i;
  BoxArr_Finish(& ra->reg_frame);
  for(i = 0; i < NUM_TYPES; i++)
    VarFrame_Finish(& ra->gvar[i]);
}

void Reg_Frame_Push(RegAlloc *ra) {
  RegFrame *new_frame = BoxArr_Push(& ra->reg_frame, NULL);
  RegFrame_Init(new_frame);
}

void Reg_Frame_Pop(RegAlloc *ra) {
  BoxArr_Pop(& ra->reg_frame, NULL);
}

Int Reg_Frame_Get(RegAlloc *ra) {
  return BoxArr_Num_Items(& ra->reg_frame);
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
Int Reg_Occupy(RegAlloc *ra, Int t) {
  RegFrame *rf = (RegFrame *) BoxArr_Last_Item_Ptr(& ra->reg_frame);
  return (Int) BoxOcc_Occupy(& rf->reg_occ[Reg_Type(t)], NULL);
}

/* Vedi Reg_Occupy.
 */
void Reg_Release(RegAlloc *ra, Int t, UInt reg_num) {
  RegFrame *rf = (RegFrame *) BoxArr_Last_Item_Ptr(& ra->reg_frame);
  BoxOcc_Release(& rf->reg_occ[Reg_Type(t)], reg_num);
}

/* Restituisce il numero di registro massimo fin'ora utilizzato. */
Int Reg_Num(RegAlloc *ra, Int t) {
  RegFrame *rf = (RegFrame *) BoxArr_Last_Item_Ptr(& ra->reg_frame);
  return BoxOcc_Max_Index(& rf->reg_occ[Reg_Type(t)]);
}

static RegFrame *Cur_RegFrame(RegAlloc *ra) {
  return (RegFrame *) BoxArr_Last_Item_Ptr(& ra->reg_frame);
}

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
Int Var_Occupy(RegAlloc *ra, Int type, Int level) {
  Int t = Reg_Type(type);
  return VarFrame_Occupy(& Cur_RegFrame(ra)->lvar[t], level);
}

/* Vedi Var_Occupy. */
void Var_Release(RegAlloc *ra, Int type, UInt varnum) {
  Int t = Reg_Type(type);
  VarFrame_Release(& Cur_RegFrame(ra)->lvar[t], varnum);
}

/* Restituisce il numero di variabile massimo fin'ora utilizzato.
 */
Int Var_Num(RegAlloc *ra, Int type) {
  return Cur_RegFrame(ra)->lvar[Reg_Type(type)].max;
}

Int GVar_Occupy(RegAlloc *ra, Int type) {
  return VarFrame_Occupy(& ra->gvar[Reg_Type(type)], 0);
}

/* Vedi Var_Occupy. */
void GVar_Release(RegAlloc *ra, Int type, UInt varnum) {
  VarFrame_Release(& ra->gvar[Reg_Type(type)], varnum);
}

/* Restituisce il numero di variabile massimo fin'ora utilizzato.
 */
Int GVar_Num(RegAlloc *ra, Int type) {
  return ra->gvar[Reg_Type(type)].max;
}

/* This function writes (starting at the address num_var)
 * an array of Int with the number of used variables, and (address num_reg)
 * an array of Int with the number of used registers.
 */
void RegLVar_Get_Nums(RegAlloc *ra, Int *num_reg, Int *num_lvar) {
  int i;
  for (i = 0; i < NUM_TYPES; i++) {
    *(num_reg++) = Reg_Num(ra, i);
    *(num_lvar++) = Var_Num(ra, i);
  }
}
