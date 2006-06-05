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

/* chest.c,  June 2006
 *
 * This file defines the object Chest which is an extension of the object
 * array. This files adds to the Array object the possibility of releasing
 * an element. For example:
 *
 *  you can insert three elements: 1, 2 and 3, then you can release
 *  the element number 2. Now when you insert another element, this will
 *  be put at the position occupied by 2: the most recently released item.
 */

/*#define DEBUG*/

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "types.h"
#include "defaults.h"
#include "messages.h"
#include "array.h"
#include "chest.h"

#define CHEST_END_OF_CHAIN -1
#define CHEST_ITEM_OCCUPIED 0

/* Inizializza gli array che tengono nota dei registri occupati
 * e setta la loro "dimensione a riposo". Questa quantita'(per ciascun
 * tipo di registro) deve essere piu' grande del numero di registri che
 * ci si aspetta saranno occupati contemporanemente, in questo modo saranno
 * eseguite poche realloc.
 */
Task Chest_New(Chest **new_chest, UInt element_size, UInt min_dim) {
  Array *a;
  element_size += sizeof(int);
  TASK( Arr_New(& a, element_size, min_dim) );
  a->chain = CHEST_END_OF_CHAIN;
  a->max_idx = 0;
  *new_chest = (Chest *) a;
  return Success;
}

/* Restituisce un numero di registro libero e lo occupa,
 * in modo tale che questo numero di registro non venga piu' restituito
 * nelle prossime chiamate a Reg_Occupy, a meno che il registro
 * non venga liberato con Reg_Release.
 * t e' il tipo di registro, per ciascuno dei tipi di registro
 * Reg_Occupy funziona in maniera indipendente.
 * NOTA: Il numero di registro restituito e' sempre maggiore di 1,
 *  viene restituito 0 solo in caso di errori.
 */
Task Chest_Occupy(Chest *c, void *item, int *assigned_index) {
  Array *a = (Array *) c;
  int dummy_int, item_size;
  void *item_dst;
  assigned_index = assigned_index == NULL ? & dummy_int : assigned_index;
  item_size = a->elsize - sizeof(int);
  assert(item_size >= 0);
  if ( Arr_Chain(a) == CHEST_END_OF_CHAIN ) {
    /* Se la catena dei registri disponibili e' vuota non resta che creare
     * un nuovo registro, contrassegnarlo come occupato e restituirlo.
     */
    Arr_Inc(a);
    item_dst = Arr_LastItemPtr(a, void);
    *((int *) item_dst) = CHEST_ITEM_OCCUPIED;

    {
      Intg ni = Arr_NumItem(a);
      if (ni > a->max_idx) a->max_idx = ni;
#ifdef DEBUG
      printf("Chest_Occupy: occupo (num="SIntg")\n", ni);
#endif
      *assigned_index = ni;
    }
    if (item == NULL || item_size == 0) return Success;
    item_dst += sizeof(int);
    (void) memcpy(item_dst, item, item_size);
    return Success;

  } else {
    long free_item = a->chain;
    item_dst = Arr_ItemPtr(a, void, free_item);
    a->chain = *((int *) item_dst);
    *((int *) item_dst) = CHEST_ITEM_OCCUPIED;
#ifdef DEBUG
    printf("Chest_Occupy: occupo (num="SIntg")\n", free_item);
#endif
    *assigned_index = free_item;
    if (item == NULL || item_size == 0) return Success;
    item_dst += sizeof(int);
    (void) memcpy(item_dst, item, item_size);
    return Success;
  }
}

/* Vedi Chest_Occupy.
 */
Task Chest_Release(Chest *c, UInt item_index) {
  Array *a = (Array *) c;
#ifdef DEBUG
  printf("Chest_Release: rilascio (num="SIntg")\n", item_index);
#endif

  if (item_index > Arr_NumItem(a)) {
    MSG_ERROR("Chest_Release: Relesing a non occupied register(1)");
    return Failed;
  }

  if (Arr_Item(a, Intg, item_index) != CHEST_ITEM_OCCUPIED) {
    MSG_ERROR("Chest_Release: Already released register(2): num = %d",
      item_index);
    return Failed;
  }

  Arr_Item(a, Intg, item_index) = a->chain;
  a->chain = item_index;
  return Success;
}
