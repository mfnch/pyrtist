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


/*#define DEBUG*/

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

/* BOX_ABBREV to use UInt as a shorthand of BoxUInt, etc. */
#define BOX_ABBREV
#include "types.h"
#include "error.h"
#include "defaults.h"
#include "messages.h"
#include "array.h"
#include "occupation.h"

/** Index to signal end of the chain of released items */
#define END_OF_CHAIN 0

/** Header which is prepended to each item for recording its occupation state.
 */
typedef struct {
  struct {
    unsigned int
       occupied : 1;     /**< NULL if not occupied, otherwise contains the
                              finalizer function to be called */
  } is;
  BoxUInt chain;            /**< If not the end of chain, then this is the next
                              item in the chain. Must be END_OF_CHAIN if
                              there isn't one */
} ItemHeader;

void BoxOcc_Init(BoxOcc *occ, BoxUInt element_size, BoxUInt initial_size) {
  BoxArr_Init(& occ->array, element_size + sizeof(ItemHeader), initial_size);
  BoxErr_Set_Tolerance(& occ->array.err, 1); /* So that we can catch errors */
  occ->elsize = element_size;
  occ->chain = END_OF_CHAIN;
  occ->max_idx = 0;
  occ->fin = NULL;
  BoxErr_Init(& occ->err);
}

static int Internal_Finalizer(BoxUInt idx, void *my_item, void *occ_ptr) {
  ItemHeader *head = (ItemHeader *) my_item;
  my_item += sizeof(ItemHeader);
  BoxOcc *occ = (BoxOcc *) occ_ptr;
  if (head->is.occupied) {
    if (occ->fin != NULL)
      occ->fin(my_item);
    head->is.occupied = 0;
  }
  return 1;
}

void BoxOcc_Finish(BoxOcc *occ) {
  /* Finalize all occupied items */
  BoxArr_Iter(& occ->array, Internal_Finalizer, occ);
  /* Destroy BoxOcc.array */
  BoxArr_Finish(& occ->array);
}

BoxOcc *BoxOcc_New(BoxUInt element_size, BoxUInt initial_size) {
  BoxOcc *occ = BoxMem_Alloc(sizeof(BoxOcc));
  if (occ == NULL) return NULL;
  BoxOcc_Init(occ, element_size, initial_size);
  return occ;
}

void BoxOcc_Destroy(BoxOcc *occ) {
  BoxOcc_Finish(occ);
  BoxMem_Free(occ);
}

void BoxOcc_Set_Finalizer(BoxOcc *occ, BoxOccFinalizer fin) {
  occ->fin = fin;
}

BoxUInt BoxOcc_Occupy(BoxOcc *occ, void *item) {
  BoxArr *arr = & occ->array;
  ItemHeader *head;
  void *my_item;

  if (occ->chain == END_OF_CHAIN) {
    /* empty chain: add a new item to the tail */
    BoxUInt idx;

    my_item = BoxArr_Push(arr, NULL);
    if (BoxErr_Propagate(& occ->err, & arr->err)) return 0;

    head = (ItemHeader *) my_item;
    my_item += sizeof(ItemHeader);

    head->is.occupied = 1;
    head->chain = END_OF_CHAIN;
    memcpy(my_item, item, occ->elsize);

    idx = BoxArr_Num_Items(arr); /* index of tail item */
    if (idx > occ->max_idx)
      occ->max_idx = idx;
    return idx;

  } else {
    /* non empty chain: recycle an item from the chain */
    BoxUInt idx = occ->chain;
    my_item = BoxArr_Item_Ptr(arr, idx);
    BoxErr_Assert(& arr->err);
    head = (ItemHeader *) my_item;
    my_item += sizeof(ItemHeader);

    occ->chain = head->chain;
    assert(head->is.occupied == 0);
    head->is.occupied = 1;
    memcpy(my_item, item, occ->elsize);
    return idx;
  }
}

void BoxOcc_Release(BoxOcc *occ, BoxUInt item_index) {
  BoxArr *arr = & occ->array;
  ItemHeader *head;
  void *my_item;


  my_item = (void *) BoxArr_Item_Ptr(arr, item_index);
  if (BoxErr_Propagate(& occ->err, & arr->err)) return;

  head = (ItemHeader *) my_item;
  if (!head->is.occupied) {
    BoxErr_Report(& occ->err, BOXERR_DOUBLE_RELEASE);
    return;
  }

  (void) Internal_Finalizer(item_index, my_item, occ);
  head->chain = occ->chain;
  occ->chain = item_index;
}

void *BoxOcc_Item_Ptr(BoxOcc *occ, BoxUInt item_index) {
  BoxArr *arr = & occ->array;
  void *my_item = (void *) BoxArr_Item_Ptr(arr, item_index);
  if (BoxErr_Propagate(& occ->err, & arr->err)) return NULL;
  return (occ->elsize > 0) ? my_item + sizeof(ItemHeader) : NULL;
}

BoxUInt BoxOcc_Max_Index(BoxOcc *occ) {return occ->max_idx;}
