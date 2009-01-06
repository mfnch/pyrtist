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

void BoxOcc_Init(BoxOcc *occ, UInt element_size, UInt initial_size) {
  element_size += sizeof(UInt);
  BoxArr_Init(& occ->array, element_size, initial_size);
  occ->elsize = element_size;
  occ->chain = CLC_END_OF_CHAIN;
  occ->max_idx = 0;
  occ->fin = (BoxOccFinalizer) NULL;
}

void BoxOcc_Finish(BoxOcc *occ) {
  BoxArr_Finish(& occ->array);
}

BoxOcc *BoxOcc_New(UInt element_size, UInt initial_size) {
  BoxOcc *occ = BoxMem_Alloc(sizeof(BoxOcc));
  if (occ == NULL) return NULL;
  BoxOcc_Init(occ, element_size, initial_size);
  return occ;
}

void BoxOcc_Destroy(BoxOcc *occ) {
  BoxOcc_Finish(occ);
  BoxMem_Free(occ);
}

static void Internal_Finalizer(void *item) {


  /*if ( *((int *) item_container) == CLC_ITEM_OCCUPIED ) {
    void *item = item_container + sizeof(UInt);
    return ((Task (*)(void  *)) destructor)(item);
  }*/
}

void BoxOcc_Set_Finalizer(BoxOcc *occ, BoxOccFinalizer fin) {
  occ->fin = fin;
  BoxArr_Set_Finalizer(& occ->array, Internal_Finalizer);
}

typedef struct {
  BoxArrFinalizer occupied;
  UInt            chain;
} ItemHeader;

UInt BoxOcc_Occupy(BoxOcc *occ, void *item) {
  BoxArr *arr = & occ->array;
  ItemHeader *head;
  void *my_item;

  if (occ->chain < 1) { /* empty chain */
    my_item = BoxArr_Push(arr, NULL);
    head = (ItemHeader *) my_item;
    my_item += sizeof(ItemHeader);

    head->chain = occ->chain;
    occ->chain = BoxArr_Num_Items(arr);
    head->occupied = Internal_Finalizer;
    memcpy(my_item, item, occ->elsize);
    if (occ->chain > occ->max_idx)
      occ->max_idx = occ->chain;
    return occ->chain;

#ifdef DEBUG
      printf("Clc_Occupy(1): %p: occupo (num="SInt")\n", c, ni);
#endif

  } else {
    my_item = BoxArr_Item_Ptr(arr, occ->chain);
    head = (ItemHeader *) my_item;
    my_item += sizeof(ItemHeader);

    occ->chain = head->chain;
    head->occupied = Internal_Finalizer;
    memcpy(my_item, item, occ->elsize);
    return occ->chain;

#ifdef DEBUG
    printf("Clc_Occupy(2): %p: occupo (num="SInt")\n", c, free_item);
#endif
  }
}

void BoxOcc_Release(BoxOcc *occ, UInt item_index) {
  BoxArr *arr = & occ->arr;
  void *my_item;

#ifdef DEBUG
  printf("Clc_Release: %p: rilascio (num="SInt")\n", c, item_index);
#endif

  if (item_index > BoxArr_Num_Items(arr)) {
    MSG_ERROR("BoxOcc_Release: Releasing a non occupied item");
    return;
  }

  my_item = (void *) BoxArr_Item_Ptr(arr, item_index);
  head = (ItemHeader *) my_item;
  my_item += sizeof(ItemHeader);

  if (head->occupied != NULL) {
    MSG_ERROR("BoxOcc_Release: Item already released: num = %d", item_index);
    return;
  }

  head->occupied = Internal_Finalizer;
  head->chain = occ->chain;
  occ->chain = item_index;

  if ( c->destroy == (Task (*)(void *)) NULL ) return Success;
  c->destroy(item_ptr + sizeof(UInt));
}












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
      Int ni = Arr_NumItem(a);
      if (ni > a->max_idx) a->max_idx = ni;
#ifdef DEBUG
      printf("Clc_Occupy(1): %p: occupo (num="SInt")\n", c, ni);
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
    printf("Clc_Occupy(2): %p: occupo (num="SInt")\n", c, free_item);
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
  printf("Clc_Release: %p: rilascio (num="SInt")\n", c, item_index);
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
