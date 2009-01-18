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

/** @file array.h
 * @brief Array object, contiguous resizable collection of objects.
 *
 * This file implements the Array object: a collection of blocks of equal
 * specified size which are stored contiguously in memory and which
 * can be resized easily. Useful to accumulate objects whose number
 * is not known a priori and which have to be referred by index.
 */

#ifndef _BOX_ARRAY_H
#  define _BOX_ARRAY_H

#  include <box/types.h>
#  include <box/error.h>

typedef void (*BoxArrFinalizer)(void *);

/** @brief The new array object */
typedef struct {
  BoxErr err;          /**< Error status */
  struct {
    unsigned int
       zero     : 1;   /**< Empty pushed items are zeroed */
  } attr;
  void *ptr;           /**< Points to region containing the array of blocks */
  BoxUInt
       dim,            /**< Size (in blocks) of the allocated region */
       size,           /**< Size (in bytes) of the allocated region */
       mindim,         /**< Minimum value for dim */
       elsize,         /**< Size of each block */
       numel;          /**< Number of blocks currently present on the array */
  BoxArrFinalizer fin; /**< Used to finalize elements before destruction */
} BoxArr;

/** Initialize the (already allocated) BoxArr object 'arr' as a new array.
 * The BoxArr object implements the automatic resizeable array. The array
 * has an the provided 'initial_size' and when elements are added a check
 * is made to guarantee that the array is big enough to contain them.
 * It the array needs to be resized this is done automatically.
 * @param arr pointer to the memory region where the BoxArr will be kept
 * @param element_size size of each element of the array
 * @param initial_size minimum size of the array (this is the size of the
 *                     array, even when no elements are contained in it)
 * @see BoxArr_Finish, BoxArr_New
 */
void BoxArr_Init(BoxArr *arr, BoxUInt element_size, BoxUInt initial_size);

/** Finalize a BoxArr object, calling the finalizer for each remaining
 * element, if necessary. The function does not deallocate the region
 * of memory which contains the BoxArr object itself, but rather deallocates
 * the data block of the array. Therefore this function is to be used
 * in couple with BoxArr_Init.
 * @see BoxArr_Init
 */
void BoxArr_Finish(BoxArr *arr);

/** Similar to BoxArr_Init, but also allocates the memory region to contain
 * the BoxArr data structure. Consequently, if you use this function,
 * you should use BoxArr_Destroy to finalize the array (you must not use
 * BoxArr_Finish).
 * @see BoxArr_Destroy, BoxArr_Init
 */
BoxArr *BoxArr_New(BoxUInt element_size, BoxUInt initial_size);

/** Similar to BoxArr_Init, but frees also the region containing
 * the BoxArr object. Suitable to be used with BoxArr_New (and must NOT
 * be used with BoxArr_Init).
 * @see BoxArr_New
 */
void BoxArr_Destroy(BoxArr *arr);

/** Attributes corresponding to different behaviours of the BoxArr object:
 * BOXARR_ERR_STATUS correspond to the error status of the array, which is set
 * when an allocation or similar problem occurs. BOXARR_ERR_TOLERANT decides
 * how to behave on such errors: abort the program, or tolerate the error and
 * just put the BoxArr object in the error state. BOXARR_CLEAR_ITEMS decides
 * whether a NULL pointer in BoxArr_MPush, BoxArr_Insert, etc. should lead
 * to insertion of zero filled items or to the insertion of uninitialised
 * items.
 */
typedef enum {
  BOXARR_ERR_STATUS=1,
  BOXARR_ERR_TOLERANT=2,
  BOXARR_CLEAR_ITEMS=4
} BoxArrAttr;

/** Set the attributes which control the behaviour of the BoxArr object.
 * @see BoxArrAttr
 */
void BoxArr_Set_Attr(BoxArr *arr, BoxArrAttr mask, BoxArrAttr value);

/** Returns 1 if the provided BoxArr object is in error state, 0 otherwise.
 */
int BoxArr_Is_Err(BoxArr *arr);

/** Sets a finalizer to call for each element which is removed
 * from the array (just before doing it).
 */
void BoxArr_Set_Finalizer(BoxArr *arr, BoxArrFinalizer fin);

/** Returns the pointer to the item of the provided array with index
 * item_index. Out of bounds errors are detected and treated conformly
 * to the error policy set with BoxArr_Set_Err_Policy
 * @see BoxArr_Set_Err_Policy
 */
void *BoxArr_Item_Ptr(BoxArr *arr, BoxUInt item_index);

/** Prototype of function used by BoxArr_Iter to iterate over the elements
 * of the provided array.
 * @param item_index the index (1..num_items) of the considered item
 * @param item_ptr pointer to the item
 * @param pass_data data provided to the BoxArr_Iter function to be passed
 *                  to the iterator.
 * @return 1 to continue the iteration, 0 to break it
 * @see BoxArr_Iter
 */
typedef int (*BoxArrIterator)(BoxUInt item_index, void *item_ptr,
                              void *pass_data);

/** Execute the iterator 'iter' for each element of the array 'arr'.
 * The iterator receives the element index, the element pointer and
 * the pointer 'pass_data', which was given in the call to BoxArr_Iter.
 * @return 0 if the iteration was broken by the iterator, 0 otherwise
 * @see BoxArrIterator
 */
int BoxArr_Iter(BoxArr *arr, BoxArrIterator iter, void *pass_data);

/** Remove all the elements of the 'arr' invoking the destructor for each
 * removed item, if necessary. After the call to this function, the BoxArr
 * object will be empty and with no allocated space for its elements
 * (if an element is then inserted an automatic malloc will restore
 * the data block for the BoxArr object).
 */
void BoxArr_Clear(BoxArr *arr);

/** Push new elements into the provided array. The items are appended
 * to the array. If 'items' is the NULL pointer the items are inserted
 * being set to zero or without being set at all, if BoxArr_Set_Attr
 * was used to disable the automatic clearing.
 * @param arr the target array
 * @param items the pointer to the items to be inserted
 * @param num_items the number of items to be inserted
 * @see BoxArr_Push, BoxArr_Set_Attr
 */
void *BoxArr_MPush(BoxArr *arr, const void *items, BoxUInt num_items);

/** Similar to BoxArr_MPush, but pushes just one item into the array.
 * @see BoxArr_MPush
 */
#define BoxArr_Push(arr, item) BoxArr_MPush((arr), (item), (BoxUInt) 1)

/** Remove the specified number of items from the top of the array
 * and put them in 'items'. If 'items' is the NULL pointer, then the items
 * are just removed from the array. The finalizer is called before moving
 * the items, if defined.
 * @see BoxArr_Pop
 */
void BoxArr_MPop(BoxArr *arr, void *items, BoxUInt num_items);

/** Similar to BoxArr_MPop, but pops just one item into from the array.
 * @see BoxArr_MPop
 */
#define BoxArr_Pop(arr, item) BoxArr_MPop((arr), (item), (BoxUInt) 1)

/** Inserts elements at the specified position in the array.
 * If 'items' is the NULL pointer the items are inserted being set to zero
 * or without being set at all, if BoxArr_Set_Attr was used to disable
 * the automatic clearing.
 * @param arr the target array
 * @param items the pointer to the items to be inserted
 * @param num_items the number of items to be inserted
 * @see BoxArr_Set_Attr, BoxArr_Push, BoxArr_Overwrite
 */
void *BoxArr_Insert(BoxArr *arr, BoxUInt insert_index,
                    const void *items, BoxUInt num_items);

/** Overwrite the array at position 'ow_index' with the provided items.
 * If 'items' is the NULL pointer the items are written as if they were zero
 * filled or without being set at all, if BoxArr_Set_Attr was used to disable
 * the automatic clearing.
 * @see BoxArr_Set_Attr, BoxArr_Push, BoxArr_Insert
 */
void *BoxArr_Overwrite(BoxArr *arr, BoxUInt ow_index,
                       const void *items, BoxUInt num_items);

/** Returns the number of elements currently inserted in the provided array.
 */
#define BoxArr_Num_Items(arr) ((arr)->numel)

/** Returns the pointer to the memory block containing the data for the array.
 */
#define BoxArr_First_Item_Ptr(arr) ((arr)->ptr)

/** Returns the pointer to the last item stored in the array.
 */
#define BoxArr_Last_Item_Ptr(arr) \
  ((arr)->ptr + ((arr)->numel - 1)*((UInt) (arr)->elsize))

#endif /* _BOX_ARRAY_H */