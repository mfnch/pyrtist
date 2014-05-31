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

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "types.h"
#include "defaults.h"
#include "messages.h"
#include "array.h"
#include "registers.h"
#include "vm_priv.h"

#define END_OF_CHAIN -1
#define OCCUPIED 0

/****************************************************************************
 * Here we implement the logic to associate registers to variables.         *
 ****************************************************************************/

/*
  This is what we want to do:

   a = 1              --> a:vr1
   [b = 2]            --> b:vr2
   c = 3              --> c:vr3
   [[d = 4], [e = 5]] --> d:vr4, e:vr4

 */
typedef struct {
  uint32_t level, /**< scope level of the variable */
           chain; /**< 0 means occupied, otherwise contain a link to the next
                       register in the chain of non occupied registers. */
} VarItem;

static BoxTypeId Reg_Type(BoxTypeId type) {
  assert(type >= 0);
  return (type >= NUM_TYPES) ? BOXTYPEID_PTR : type;
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
    BoxOcc_Init(& rf->reg_occ[i], 0, 10);
    rf->lvar[i] = 0;
  }
}

static void RegFrame_Finish(void *rf_ptr) {
  RegFrame *rf = (RegFrame *) rf_ptr;
  int i;
  for(i = 0; i < NUM_TYPES; i++)
    BoxOcc_Finish(& rf->reg_occ[i]);
}

/*  Inizializza gli array che tengono nota dei registri occupati
 *  e setta la loro "dimensione a riposo". Questa quantita'(per ciascun
 *  tipo di registro) deve essere piu' grande del numero di registri che
 *  ci si aspetta saranno occupati contemporanemente, in questo modo saranno
 *  eseguite poche realloc.
 */
void Reg_Init(RegAlloc *ra) {
  int i;
  RegFrame_Init(& ra->reg_frame);
  for(i = 0; i < NUM_TYPES; i++)
    ra->gvar[i] = 0;
}

void Reg_Finish(RegAlloc *ra) {
  RegFrame_Finish(& ra->reg_frame);
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
BoxInt Reg_Occupy(RegAlloc *ra, BoxTypeId t) {
  if (t == BOXTYPEID_VOID)
    return 0;
  return (BoxInt) BoxOcc_Occupy(& ra->reg_frame.reg_occ[Reg_Type(t)], NULL);
}

/* Vedi Reg_Occupy.
 */
void Reg_Release(RegAlloc *ra, BoxInt t, BoxUInt reg_num) {
  RegFrame *rf = & ra->reg_frame;
  BoxOcc_Release(& rf->reg_occ[Reg_Type(t)], reg_num);
}

static RegFrame *Cur_RegFrame(RegAlloc *ra) {
  return (RegFrame *) & ra->reg_frame;
}

BoxInt Var_Occupy(RegAlloc *ra, BoxTypeId type, BoxInt level) {
  BoxTypeId t = Reg_Type(type);
  if (type == BOXTYPEID_VOID)
    return 0;
  return (Cur_RegFrame(ra)->lvar[t])++;
}

BoxInt GVar_Occupy(RegAlloc *ra, BoxTypeId type) {
  if (type == BOXTYPEID_VOID)
    return 0;
  return (ra->gvar[Reg_Type(type)])++;
}

/* This function writes (starting at the address num_var)
 * an array of BoxInt with the number of used variables, and (address num_reg)
 * an array of BoxInt with the number of used registers.
 */
void
RegAlloc_Get_Local_Nums(RegAlloc *ra, uint32_t *num_regs, uint32_t *num_vars)
{
  int i;
  if (num_regs)
    for (i = 0; i < NUM_REGISTER_TYPES; i++)
      *(num_regs++) = BoxOcc_Max_Index(& ra->reg_frame.reg_occ[Reg_Type(i)]);

  if (num_vars)
    for (i = 0; i < NUM_REGISTER_TYPES; i++)
      *(num_vars++) = Cur_RegFrame(ra)->lvar[Reg_Type(i)];
}

void
RegAlloc_Get_Global_Nums(RegAlloc *ra, uint32_t *num_regs, uint32_t *num_vars)
{
  int i;
  if (num_regs)
    for (i = 0; i < NUM_REGISTER_TYPES; i++)
      *(num_regs++) = (i == BOXTYPEID_PTR) ? 2 : 0;

  if (num_vars)
    for (i = 0; i < NUM_REGISTER_TYPES; i++)
      *(num_vars++) = ra->gvar[Reg_Type(i)];
}
