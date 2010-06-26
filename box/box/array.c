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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* BOX_ABBREV to use UInt as a shorthand of BoxUInt, etc. */
#define BOX_ABBREV
#include "types.h"

#include "messages.h"
#include "mem.h"
#include "array.h"

/** Similar to memcpy, but if the src pointer is NULL, then it just clears
 * the destination region or, if clear == 0, it just returns dest.
 */
static void *Mem_Copy_or_Clear(void *dest, const void *src, size_t n,
                               int clear) {
  if (src != NULL)
    return memcpy(dest, src, n);

  else {
    if (clear)
      return memset(dest, 0, n);
    else
      return dest;
  }
}

static void BoxArr_Reinit(BoxArr *arr) {
  assert(arr != NULL);
  arr->ptr = NULL;
  arr->dim = 0;
  arr->size = 0;
  arr->numel = 0;
}

void BoxArr_Init(BoxArr *arr, UInt element_size, UInt initial_size) {
  BoxArr_Reinit(arr);
  assert(element_size > 0);
  arr->mindim = initial_size;
  arr->elsize = element_size;
  arr->fin = NULL;
  arr->attr.zero = 1;
  BoxErr_Init(& arr->err);
}

void BoxArr_Finish(BoxArr *arr) {
  BoxArr_Clear(arr); /* Just dealloc the array buffer */
}

BoxArr *BoxArr_New(UInt element_size, UInt initial_size) {
  BoxArr *arr = BoxMem_Alloc(sizeof(BoxArr));
  if (arr == NULL) return NULL;
  BoxArr_Init(arr, element_size, initial_size);
  return arr;
}

void BoxArr_Destroy(BoxArr *arr) {
  BoxArr_Finish(arr);
  BoxMem_Free(arr);
}

void BoxArr_Set_Attr(BoxArr *arr, BoxArrAttr mask, BoxArrAttr value) {
  assert(arr != NULL);
  if ((mask & BOXARR_ERR_STATUS) != 0)
    BoxErr_Set_Error(& arr->err, ((value & BOXARR_ERR_STATUS) != 0));
  if ((mask & BOXARR_ERR_TOLERANT) != 0)
    BoxErr_Set_Tolerance(& arr->err, ((value & BOXARR_ERR_STATUS) != 0));
  if ((mask & BOXARR_CLEAR_ITEMS) != 0)
    arr->attr.zero = ((value & BOXARR_CLEAR_ITEMS) != 0);
}

int BoxArr_Is_Err(BoxArr *arr) {
  return BoxErr_Have_Err(& arr->err);
}

void BoxArr_Set_Finalizer(BoxArr *arr, BoxArrFinalizer fin) {
  assert(arr != NULL);
  arr->fin = fin;
}

/* Apply a function to all the elements of an array */
int BoxArr_Iter(BoxArr *arr, BoxArrIterator iter, void *pass_data) {
  if (iter != NULL) {
    int i;
    void *item_ptr, *ptr;

    assert(arr != NULL && iter != NULL);

    item_ptr = ptr = arr->ptr;
    for(i = 1; i <= arr->numel; i++) {
      if (! iter(i, item_ptr, pass_data)) return 0;

      assert(arr->ptr == ptr);

      item_ptr += arr->elsize;
    }
    return 1;

  } else
    return 1; /* iter == NULL */
}

typedef struct {
  BoxArr    *arr;
  void      *right;
  void      *pass_data;
  BoxArrCmp cmp;
  size_t    idx;
} MyIteratorForFind;

static int My_Default_Cmp(BoxUInt idx, void *item_ptr, void *pass_data) {
  MyIteratorForFind *d = (MyIteratorForFind *) pass_data;
  d->idx = idx;
  return (memcmp(item_ptr, d->right, d->arr->elsize) != 0);
}

static int My_User_Cmp(BoxUInt idx, void *item_ptr, void *pass_data) {
  MyIteratorForFind *d = (MyIteratorForFind *) pass_data;
  d->idx = idx;
  return (d->cmp(item_ptr, d->right, d->pass_data) != 0);
}

size_t BoxArr_Find(BoxArr *arr, void *item, BoxArrCmp cmp, void *pass_data) {
  MyIteratorForFind d = {arr, item, pass_data, cmp, (size_t) 0};
  BoxArr_Iter(arr, (cmp != 0) ? My_User_Cmp : My_Default_Cmp, & d);
  return d.idx;
}

static int Finalise_Item(UInt item_num, void *item, void *fin) {
  assert(fin != NULL);
  ((void (*)(void *)) fin)(item);
  return 1;
}

void BoxArr_Clear(BoxArr *arr) {
  assert(arr != NULL);
  /* If a destructor is given, uses it to iterate over all the elements
   * for the last time.
   */
  if (arr->fin != NULL)
    (void) BoxArr_Iter(arr, Finalise_Item, arr->fin);

  BoxMem_Free(arr->ptr);
  BoxArr_Reinit(arr); /* Re-init */
}

void *BoxArr_Item_Ptr(BoxArr *arr, UInt item_index) {
  if (item_index < 1 || item_index > arr->numel) {
    BoxErr_Report(& arr->err, BOXERR_OUT_OF_BOUNDS);
    return NULL;
  }
  return arr->ptr + (item_index - 1)*arr->elsize;
}

/* This function is used to expand (resize) the Array 'arr' such that it can
 * contain at least 'num_items' elements.
 */
static void BoxArr_Expand(BoxArr *arr, UInt num_items) {
  if (num_items > arr->dim) {
    UInt new_dim = arr->dim, new_size;
    void *new_ptr;
    if (new_dim == 0) {
      for(new_dim = arr->mindim; new_dim < num_items; new_dim *= 2);
      new_size = new_dim*arr->elsize;
      new_ptr = BoxMem_Alloc(new_size);

    } else {
      for(; new_dim < num_items; new_dim *= 2);
      new_size = new_dim*arr->elsize;
      new_ptr = BoxMem_Realloc(arr->ptr, new_size);
    }
    if (new_ptr == NULL) {
      BoxErr_Report(& arr->err, BOXERR_OUT_OF_MEMORY);
      return;
    }
    arr->ptr = new_ptr;
    arr->dim = new_dim;
    arr->size = new_size;
  }
}

/* This macro reduces the size of the array, whenever more than 3/4 is empty.
 * The contraction makes sure that the array is - at least - half filled
 * 'num_items' is the number of elements in the array.
 */
static void BoxArr_Shrink(BoxArr *arr, UInt num_items) {
  if (arr->dim > arr->mindim) { /* May not be allocated, yet */
    UInt n4 = 4*num_items, new_dim, new_size;
    void *new_ptr;
    for(new_dim = arr->dim; n4 < new_dim; new_dim /= 2);
    if (new_dim < arr->mindim)
      new_dim = arr->mindim;
    new_size = arr->dim*arr->elsize;
    new_ptr = BoxMem_Realloc(arr->ptr, new_size);
    if (new_ptr == NULL) {
      BoxErr_Report(& arr->err, BOXERR_OUT_OF_MEMORY);
      return;
    }
    arr->ptr = new_ptr;
    arr->dim = new_dim;
    arr->size = new_size;
  }
}

void BoxArr_Compactify(BoxArr *arr) {
  if (arr->dim != arr->numel) {
    if (arr->dim > arr->numel) {
      size_t new_dim = arr->numel,
             new_size = new_dim*arr->elsize;
      void   *new_ptr = BoxMem_Realloc(arr->ptr, new_size);
      if (new_ptr != NULL) {
        arr->ptr = new_ptr;
        arr->dim = new_dim;
        arr->size = new_size;
      }

    } else
      BoxErr_Report(& arr->err, BOXERR_OUT_OF_MEMORY);
  }
}

void *BoxArr_MPush(BoxArr *arr, const void *items, UInt num_items) {
  void *ptr;
  UInt pos;

  if (num_items < 1) return NULL;

  assert(arr != NULL);

  /* Position in bytes where to copy the element */
  pos = arr->numel * arr->elsize;

  /* First, expand the array if necessary */
  BoxArr_Expand(arr, arr->numel + num_items);
  if (BoxArr_Is_Err(arr)) return NULL;

  arr->numel += num_items;
  ptr = arr->ptr + pos;
  return Mem_Copy_or_Clear(ptr, items, num_items*arr->elsize, arr->attr.zero);
}

void BoxArr_MPop(BoxArr *arr, void *items, UInt num_items) {
  UInt item_index, numel;
  void *item_ptr;

  if (num_items < 1) return;

  assert(arr != NULL);

  numel = arr->numel;
  if (num_items > numel) num_items = numel;

  assert(arr->ptr != NULL);

  item_index = numel - num_items;
  item_ptr = arr->ptr + item_index*arr->elsize;

  /* First destroy all the items that are being removed from the array */
  if (arr->fin != NULL) {
    void *my_item_ptr = item_ptr;
    UInt i;
    for(i = 0; i < num_items; i++) {
      arr->fin(my_item_ptr);
      my_item_ptr += arr->elsize;
    }
  }

  if (items != NULL) {
    UInt tot_size = num_items*arr->elsize;
    memcpy(items, item_ptr, tot_size);
  }

  BoxArr_Shrink(arr, arr->numel -= num_items);
}

/* This function inserts how_many items (items is the pointer to these items)
 * into the array a at position where.
 */
void *BoxArr_Insert(BoxArr *arr, UInt insert_index,
                    const void *items, UInt num_items) {
  UInt new_dim, elsize, num_items_to_move, insert_size;
  void *insert_ptr;

  assert(arr != NULL);
  assert(insert_index > 0);

  if (num_items < 1) return NULL;

  new_dim = arr->numel;
  if (insert_index > new_dim) {
    num_items_to_move = 0;
    new_dim = insert_index + num_items - 1;

  } else {
    num_items_to_move = new_dim - insert_index + 1;
    new_dim += num_items;
  }

  /* Now we expand the array */
  BoxArr_Expand(arr, new_dim);
  if (BoxArr_Is_Err(arr)) return NULL;
  arr->numel = new_dim;

  /* Now we move the elements following the insert position */
  elsize = arr->elsize;
  insert_size = num_items*elsize;
  insert_ptr = arr->ptr + (insert_index - 1)*elsize;
  if (num_items_to_move > 0) {
    void *dest = insert_ptr + insert_size;
    (void) memmove(dest, insert_ptr, num_items_to_move*elsize);
  }

  /* Now we insert the items */
  return Mem_Copy_or_Clear(insert_ptr, items, insert_size, arr->attr.zero);
}

/* This function takes 'n' items stored in 'src' ('src' is the pointer to the
 * first item) and copy them inside the array 'a', overwriting its elements.
 * The first overwritten element will be 'dest'.
 */
void *BoxArr_Overwrite(BoxArr *arr, UInt ow_index,
                       const void *items, UInt num_items) {
  void *ow_ptr;
  UInt required_dim;

  assert(arr != NULL);

  if (num_items < 1) return NULL;

  required_dim = ow_index + num_items - 1;
  if (required_dim > arr->numel) {
    BoxArr_Expand(arr, required_dim); /* We expand the array, if necessary */
    if (BoxArr_Is_Err(arr)) return NULL;
    arr->numel = required_dim;
  }

  ow_ptr = BoxArr_Item_Ptr(arr, ow_index);
  return Mem_Copy_or_Clear(ow_ptr, items, arr->elsize*num_items,
                           arr->attr.zero);
}
