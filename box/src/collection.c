/****************************************************************************
 * Copyright (C) 2006, 2008 by Matteo Franchin                              *
 *                                                                          *
 * This file is part of Box.                                                *
 *                                                                          *
 *   Box is free software: you can redistribute it and/or modify it         *
 *   under the terms of the GNU Lesser General Public License as published  *
 *   by the Free Software Foundation, either version 3 of the License, or   *
 *   (at your option) any later version.                                    *
 *                                                                          *
 *   Box is distributed in the hope that it will be useful,                 *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Lesser General Public License for more details.                    *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with Box.  If not, see <http://www.gnu.org/licenses/>.   *
 ****************************************************************************/

/* $Id$ */

/*#define DEBUG*/

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "types.h"
#include "defaults.h"
#include "messages.h"
#include "array.h"
#include "collection.h"

#define CLC_END_OF_CHAIN -1
#define CLC_ITEM_OCCUPIED 0

Task Clc_New(Collection **new_clc, UInt element_size, UInt min_dim) {
  Array *a;
  element_size += sizeof(UInt);
  TASK( Arr_New(& a, element_size, min_dim) );
  a->chain = CLC_END_OF_CHAIN;
  a->max_idx = 0;
  a->destroy = (Task (*)(void *)) NULL;
  *new_clc = (Collection *) a;
  return Success;
}

/* Gives a function used to destroy objects when 'Clc_Release' is called */
void Clc_Destructor(Collection *c, Task (*destroy)(void *)) {
  Arr_Destructor(c, destroy);
}

static Task item_container_destructor(UInt item_num,
 void *item_container, void *destructor) {
  if ( *((int *) item_container) == CLC_ITEM_OCCUPIED ) {
    void *item = item_container + sizeof(UInt);
    return ((Task (*)(void  *)) destructor)(item);
  }
  return Success;
}

void Clc_Destroy(Collection *c) {
#ifdef DEBUG
  fprintf(stderr, "Clc_Destroy: %p: destructor invoked\n", c);
#endif
  if ( c == (Collection *) NULL ) return;
  if ( c->destroy != NULL ) {
    (void) Arr_Iter((Array *) c, item_container_destructor, c->destroy);
    c->destroy = NULL;
  }
  Arr_Destroy((Array *) c);
}

/* Restituisce un numero di registro libero e lo occupa,
 * in modo tale che questo numero di registro non venga piu' restituito
 * nelle prossime chiamate a Clc_Occupy, a meno che il registro
 * non venga liberato con Clc_Release.
 * t e' il tipo di registro, per ciascuno dei tipi di registro
 * Clc_Occupy funziona in maniera indipendente.
 * NOTA: Il numero di registro restituito e' sempre maggiore di 1,
 *  viene restituito 0 solo in caso di errori.
 */
Task Clc_Occupy(Collection *c, void *item, UInt *assigned_index) {
  Array *a = (Array *) c;
  UInt dummy_int, item_size;
  void *item_dst;
  assigned_index = assigned_index == NULL ? & dummy_int : assigned_index;
  item_size = a->elsize - sizeof(UInt);
  assert(item_size >= 0);
  if ( Arr_Chain(a) == CLC_END_OF_CHAIN ) {
    /* Se la catena dei registri disponibili e' vuota non resta che creare
     * un nuovo registro, contrassegnarlo come occupato e restituirlo.
     */
    Arr_Inc(a);
    item_dst = Arr_LastItemPtr(a, void);
    *((int *) item_dst) = CLC_ITEM_OCCUPIED;

    {
      Intg ni = Arr_NumItem(a);
      if (ni > a->max_idx) a->max_idx = ni;
#ifdef DEBUG
      printf("Clc_Occupy(1): %p: occupo (num="SIntg")\n", c, ni);
#endif
      *assigned_index = ni;
    }
    if (item == NULL || item_size == 0) return Success;
    item_dst += sizeof(UInt);
    (void) memcpy(item_dst, item, item_size);
    return Success;

  } else {
    long free_item = a->chain;
    item_dst = Arr_ItemPtr(a, void, free_item);
    a->chain = *((int *) item_dst);
    *((int *) item_dst) = CLC_ITEM_OCCUPIED;
#ifdef DEBUG
    printf("Clc_Occupy(2): %p: occupo (num="SIntg")\n", c, free_item);
#endif
    *assigned_index = free_item;
    if (item == NULL || item_size == 0) return Success;
    item_dst += sizeof(UInt);
    (void) memcpy(item_dst, item, item_size);
    return Success;
  }
}

/* Vedi Clc_Occupy.
 */
Task Clc_Release(Collection *c, UInt item_index) {
  Array *a = (Array *) c;
  void *item_ptr;
#ifdef DEBUG
  printf("Clc_Release: %p: rilascio (num="SIntg")\n", c, item_index);
#endif

  if (item_index > Arr_NumItem(a)) {
    MSG_ERROR("Clc_Release: Releasing a non occupied item(1)");
    return Failed;
  }

  item_ptr = Arr_ItemPtr(a, void, item_index);
  if (*((int *) item_ptr) != CLC_ITEM_OCCUPIED) {
    MSG_ERROR("Clc_Release: Already released item(2): num = %d",
      item_index);
    return Failed;
  }

  *((int *) item_ptr) = a->chain;
  a->chain = item_index;

#ifdef DEBUG
  fprintf(stderr, "Calling destructor...\n"); fflush(stderr);
#endif
  if ( c->destroy == (Task (*)(void *)) NULL ) return Success;
  c->destroy(item_ptr + sizeof(UInt));
#ifdef DEBUG
  fprintf(stderr, "done!\n"); fflush(stderr);
#endif
  return Success;
}

Task Clc_Object_Ptr(Collection *c, void **item_ptr, UInt item_index) {
  Array *a = (Array *) c;
  void *ip;

  assert(c != (Collection *) NULL);
  assert(a->ID == ARR_ID);

  if (item_index > Arr_NumItem(a) || item_index  < 1) {
    MSG_ERROR("Clc_Object_Ptr: Index out of bounds.");
    return Failed;
  }

  ip = Arr_ItemPtr(a, void, item_index);
  if (*((int *) ip) != CLC_ITEM_OCCUPIED) {
    MSG_ERROR("Clc_Object_Ptr: Index refers to bad item.");
    return Failed;
  }

  *item_ptr = ip + sizeof(UInt);
  return Success;
}
