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

static void BoxArr_Error(BoxArr *arr) {
  arr->attr.err = 1;
  assert(arr->attr.tolerant);
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
  arr->attr.err = 0;
  arr->attr.tolerant = 0;
  arr->attr.zero = 1;
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
    arr->attr.err = ((value & BOXARR_ERR_STATUS) != 0);
  if ((mask & BOXARR_CLEAR_ITEMS) != 0)
    arr->attr.zero = ((value & BOXARR_CLEAR_ITEMS) != 0);
  if ((mask & BOXARR_ERR_TOLERANT) != 0)
    arr->attr.tolerant = ((value & BOXARR_ERR_TOLERANT) != 0);
}

int BoxArr_Is_Err(BoxArr *arr) {
  return arr->attr.err;
}

void BoxArr_Set_Finalizer(BoxArr *arr, BoxArrFinalizer fin) {
  assert(arr != NULL);
  arr->fin = fin;
}

/* Apply a function to all the elements of an array */
int BoxArr_Iter(BoxArr *arr, BoxArrIterator iter, void *pass_data) {
  if (iter == NULL) {
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
  assert(item_index >= 1 && item_index <= arr->numel);
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
      BoxArr_Error(arr);
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
      BoxArr_Error(arr);
      return;
    }
    arr->ptr = new_ptr;
    arr->dim = new_dim;
    arr->size = new_size;
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










/*#define DEBUG*/
/*#define DEBUG_RESIZE*/

#ifdef DEBUG_RESIZE
#define DEBUG_MSG(msg) printf(msg)
#else
#define DEBUG_MSG(msg)
#endif

/** This macro is used to expand (resize) the Array a such that it can
 * contain n elements.
 */
#define ARRAY_EXPAND(a, n)                               \
  if ( n > a->dim ) {                                    \
    if ( a->dim == 0 ) {                                 \
      for( a->dim = a->mindim; a->dim < n; a->dim *= 2); \
      a->size = (a->dim) * a->elsize;                    \
      DEBUG_MSG("ARRAY_EXPAND: array allocated!\n");     \
      a->ptr = malloc( a->size );                        \
    } else {                                             \
      for(; a->dim < n; a->dim *= 2);                    \
      a->size = (a->dim)*a->elsize;                      \
      a->ptr = realloc( a->ptr, a->size );               \
      DEBUG_MSG("ARRAY_EXPAND: array resized!\n");       \
    }                                                    \
    if ( a->ptr == NULL ) {                              \
      MSG_FATAL("Out of memory");                        \
      return Failed;                                     \
    }                                                    \
  }

/** This macro reduces the size of the array, whenever more than 3/4 is empty.
 * The contraction makes sure that the array is - at least - half filled
 * n is the number of elements in the array.
 */
#define ARRAY_SHRINK(a, n)                               \
  if (a->dim > a->mindim) {                              \
    for(; 4*n < a->dim; a->dim /= 2);                    \
    if (a->dim < a->mindim)                              \
      a->dim = a->mindim;                                \
    a->size = (a->dim)*a->elsize;                        \
    a->ptr = realloc( a->ptr, a->size );                 \
    DEBUG_MSG("ARRAY_SHRINK: array resized!\n");         \
    if ( a->ptr == NULL ) {                              \
      MSG_ERROR("realloc failed!");                      \
      return Failed;                                     \
    }                                                    \
  }

/* DESCRIZIONE: Crea una nuova array con:
 *  elsize = numero byte per elemento
 *  mindim = dimensione minima dell'array (in elementi)
 *  Restituisce il puntatore all'array (tipo Array *) se tutto va bene,
 *  NULL in caso di errore.
 */
#ifdef DEBUG_ARRAY
Array *Array_New_Debug(UInt elsize, UInt mindim, const char *src, int line)
#else
Array *Array_New(UInt elsize, UInt mindim)
#endif
{
  Array *a;

  if ( elsize < 1 ) {
    MSG_ERROR("Array_New: elsize is negative!");
    return NULL;
  }

  a = (Array *) malloc( sizeof(Array) );
  if ( a == NULL ) {
    MSG_ERROR("Array_New: out of memory!");
    return NULL;
  }

#ifdef DEBUG_ARRAY
  fprintf(stderr, "Array_New, called in '%s' (%d): allocating array at %p\n",
   src, line, a);
  fflush(stderr);
#endif

  a->ID = ARR_ID;
  a->ptr = NULL;
  a->dim = 0;
  a->size = 0;
  a->numel = 0;
  a->mindim = mindim;
  a->elsize = elsize;
  a->destroy = NULL;
  return a;
}

/* Alternative interface for the function Array_New
 */
#ifdef DEBUG_ARRAY
Task Arr_New_Debug(Array **new_array, UInt elsize, UInt mindim,
 const char *src, int line) {
  return
   (*new_array = Array_New_Debug(elsize, mindim, src, line)) == NULL ?
   Failed : Success;
}
#else
Task Arr_New(Array **new_array, UInt elsize, UInt mindim) {
  return (*new_array = Array_New(elsize, mindim)) == (Array *) NULL ?
      Failed : Success;
}
#endif

static Task Destroy_Item(UInt item_num, void *item, void *destructor) {
  return ((Task (*)(void *)) destructor)(item);
}


#ifdef DEBUG_ARRAY
void Arr_Destroy_Debug(Array *a, const char *src, int line)
#else
void Arr_Destroy(Array *a)
#endif
{
  if ( a != NULL) {
#ifdef DEBUG_ARRAY
    fprintf(stderr, "Arr_Destroy, called in '%s' (%d): destroying "
     "array at %p\n", src, line, a);
    fflush(stderr);
#endif
    assert(a->ID == ARR_ID);
    /* If a destructor is given, uses it to iterate over all the elements
     * for the last time.
     */
    if ( a->destroy != NULL )
      (void) Arr_Iter((Array *) a, Destroy_Item, a->destroy);
    a->ID = 0; /* Can be useful to detect reference to free-d Array */
    free(a->ptr);
    free(a);
  }
}


void Arr_Destructor(Array *a, Task (*destroy)(void *)) {
  a->destroy = destroy;
}

#ifdef DEBUG_ARRAY
void *Arr_ItemPtr_Debug(Array *a, int n, const char *src, int line) {
  if (n < 1 || n > a->numel) {
    fprintf(stderr, "Arr_ItemPtr, called in '%s' (%d): "
     "index (=%d) out of bounds (=1..%d)\n",
     src, line, n, a->numel);
    fflush(stderr);
    exit(EXIT_FAILURE);
  }
  return a->ptr + (n-1)*((UInt) a->elsize);
}
#endif

/* Converte l'array specificata, in una nuova array
 * con elementi di dimensione diversa (elsize).
 * mindim = dimensione minima dell'array (in elementi)
 * Restituisce il puntatore alla nuova array se tutto va bene,
 * NULL in caso di errore.
 * NOTA: L'array creata sara' vuota.
 */
Array *Arr_Recycle(Array *a, UInt elsize, UInt mindim) {
  if ( elsize < 1 ) {
  MSG_ERROR("Arr_Recycle: elsize is negative!");
  return NULL;
  }

  if ( a->ID != ARR_ID ) {
    MSG_ERROR("Arr_Recycle: array is not initialized (or damaged).");
    return NULL;
  }

  /* Calcolo quanti elementi di dimensione elsize ci stanno nella zona
    * allocata per la vecchia array
    */
  a->dim = a->size / elsize;
  if ( a->dim < 1 ) {
    /* Non c'e' memoria sufficiente:
      * distruggo la vecchia array e ne creo una nuova!
      */
    free(a->ptr);
    a->ID = 0;
    return Array_New(elsize, mindim);
  }

  a->numel = 0;
  a->mindim = mindim;
  a->elsize = elsize;
  return a;
}

/* Inserisce un elemento in coda nel array,
 * preoccupandosi di reallocare la memoria se necessario.
 * Restituisce 1 solo in caso di successo.
 */
Task Arr_Push(Array *a, const void *elem) {
  if (a->ID == ARR_ID) {
    void *tptr;
    UInt tpos = a->numel++ * a->elsize;

    /* Controlla che l'array non debba essere allargata */
    ARRAY_EXPAND(a, a->numel);

    tptr = a->ptr + tpos;
    memcpy(tptr, elem, a->elsize);
    return Success;

  } else {
    MSG_ERROR("Array is not initialized (or damaged)");
    return Failed;
  }
}

Task Arr_MPush(Array *a, const void *elem, UInt numel) {
  if (a->ID == ARR_ID) {
    void *tptr;
    UInt tpos;

    if (numel < 1) return Success;
    tpos = a->numel * a->elsize;
    a->numel += numel;

    /* Controlla che l'array non debba essere allargata */
    ARRAY_EXPAND(a, a->numel);

    tptr = a->ptr + tpos;
    if (elem != NULL)
      memcpy(tptr, elem, numel * a->elsize);
    else
      (void) memset(tptr, 0, numel * a->elsize);
    return Success;

  } else {
    MSG_ERROR("Array is not initialized (or damaged)");
    return Failed;
  }
}

Task Arr_MPop(Array *a, UInt numel) {
  Task t = Success;
  UInt item_index, i;
  item_index = a->numel;
  if (numel > item_index) {
    MSG_ERROR("Arr_MPop: Trying to remove more elements than present");
    return Failed;
  }
  i = numel;
  for(; i > 0; i--) {
    void *item_ptr = Arr_ItemPtr(a,  void *, item_index--);
    if IS_FAILED(a->destroy(item_ptr)) t = Failed;
  }
  if IS_FAILED(Arr_SmallEnough(a, a->numel -= numel)) t = Failed;
  return t;
}

/* Se necessario allarga l'array in modo che contenga
 * numel elementi. Restituisce 1 in caso di successo.
 */
Task Arr_BigEnough(Array *a, UInt numel) {
  if (a->ID == ARR_ID) {
    /* Controlla che l'array non debba essere allargata  */
    ARRAY_EXPAND(a, numel);
    return Success;

  } else {
    MSG_ERROR("Array is not initialized (or damaged)");
    return Failed;
  }
}

/* DESCRIZIONE: Se necessario riduce l'array in modo che non sia
 *  troppo vuota. Restituisce 1 in caso di successo.
 */
Task Arr_SmallEnough(Array *a, UInt numel) {
  if (a->ID == ARR_ID) {
    /* Controlla che l'array non debba essere allargato */
    ARRAY_SHRINK(a, numel);
    return Success;

  } else {
    MSG_ERROR("Array is not initialized (or damaged)");
    return Failed;
  }
}

/* DESCRIPTION: This function inserts how_many items (items is the pointer
 *  to these items) into the array a at position where.
 */
Task Arr_Insert(Array *a, Int where, Int how_many, void *items) {
  if ( how_many < 1 ) return Success;
  if ( a->ID == ARR_ID ) {
    register Int numel = a->numel, new_dim, elsize = a->elsize, to_move;
    register void *src;

    if (where < 1) {
      MSG_ERROR("Negative insert index!"); return Failed;

    } else if (where > numel) {
      new_dim = where + how_many - 1;
      to_move = 0;

    } else {
      new_dim = numel + how_many;
      to_move = numel - where + 1;
    }

    /* Now we expand the array */
    ARRAY_EXPAND(a, (a->numel = new_dim));

    /* Now we create the space to contain the items to be inserted */
    src = a->ptr + ((where-1) * elsize);
    numel = how_many * elsize;
    if ( to_move > 0 ) (void) memmove(src + numel, src, to_move*elsize);

    /* Now we insert the items */
    (void) memcpy(src, items, numel);
    return Success;

  } else {
      MSG_ERROR("Array is not initialized (or damaged)");
      return Failed;
  }
}

/* This function appends to the queque of the array a,
 * 'how_many' blank elements (initialized with 0).
 */
Task Arr_Append_Blank(Array *a, Int how_many) {
    /* Very similar to Arr_Push */
  if (a->ID == ARR_ID) {
    UInt tpos;
    if ( how_many < 1 ) return Success;
    tpos = a->numel * a->elsize;
    ARRAY_EXPAND(a, (a->numel += how_many));
    (void) memset(a->ptr + tpos, 0, how_many * a->elsize);
    return Success;

  } else {
    MSG_ERROR("Array is not initialized (or damaged)");
    return Failed;
  }
}

/* Clear the array and realloc it if its dimension is greater than
 * "the starting dimension".
 */
Task Arr_Clear(Array *a) {
  if IS_SUCCESSFUL(Arr_SmallEnough(a, 0)) {
    a->numel = 0;
    return Success;
  }
  return Failed;
}

/* Extract the data from an 'Array' object.
 * After the call to this function, the 'Array' will be empty.
 */
Task Arr_Data_Only(Array *a, void **data_ptr) {
  if ( a != NULL) {
    if (a->ID == ARR_ID) {
      *data_ptr = a->ptr;
      a->dim = 0;
      a->size = 0;
      a->numel = 0;
      a->ptr = NULL;
      return Success;
    }
  }
  return Failed;
}

/* Apply a function to all the elements of an array
 */
Task Arr_Iter(Array *a, Task (*action)(UInt, void *, void *), void *pass_data)
{
  assert(a != NULL && action != NULL);
  if (a->ID == ARR_ID) {
    int i;
    void *item_ptr = a->ptr;
    for(i = 1; i <= a->numel; i++) {
      if IS_FAILED( action(i, item_ptr, pass_data) ) return Failed;
      item_ptr += a->elsize;
    }
    return Success;

  } else {
    MSG_ERROR("Array is not initialized (or damaged)");
    return Failed;
  }
}

/* This function takes 'n' item stored in 'src' ('src' is the pointer to the
 * first item) and copy them inside the array 'a', overwriting its elements.
 * The first overwritten element will be 'dest'.
 */
Task Arr_Overwrite(Array *a, Int dest, void *src, UInt n) {
  assert(a != NULL);
  if (a->ID == ARR_ID) {
    void *dest_ptr;
    int required_dim;
    if (n < 1) return Success;
    required_dim = dest + n - 1;
    if ( required_dim > a->numel ) a->numel = required_dim;
    ARRAY_EXPAND(a, a->numel); /* We expand the array, if necessary */
    dest_ptr = Arr_ItemPtr(a, void, dest);
    (void) memcpy(dest_ptr, src, a->elsize*n);
    return Success;

  } else {
    MSG_ERROR("Array is not initialized (or damaged)");
    return Failed;
  }
}
