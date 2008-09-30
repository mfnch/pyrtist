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

#ifndef _ARRAY_H
#  define _ARRAY_H

#  include "types.h"

/** @brief Array object */
typedef struct {
  long ID;      /**< ID constant just used to check for intialisation */
  void *ptr;    /**< Pointer to the region containing the array of blocks */
  long dim;     /**< Size (in blocks) of the allocated region  */
  long size;    /**< Size (in bytes) of the allocated region */
  long mindim;  /**< Minimum value for dim */
  short elsize; /**< Size of each block */
  long numel;   /**< Number of block currently present on the array */
  Task (*destroy)(void *); /**< Used to finalize elements before destruction*/
  long chain;   /**< Reserved for Collection (an extension of Array) */
  long max_idx; /**< Reserved for Collection */
} Array;

/** Gives a function used to destroy objects when 'Arr_Destroy' is called */
void Arr_Destructor(Array *a, Task (*destroy)(void *));

/* Procedure definite in array.c */
Array *Arr_Recycle(Array *a, UInt elsize, UInt mindim);
Task Arr_Push(Array *a, const void *elem);
Task Arr_MPush(Array *a, const void *elem, UInt numel);
Task Arr_MPop(Array *a, UInt numel);
#define Arr_Pop(a) Arr_MPop((a), 1)
Task Arr_Insert(Array *a, Int where, Int how_many, void *items);
Task Arr_Append_Blank(Array *a, Int how_many);
Task Arr_BigEnough(Array *a, UInt numel);
Task Arr_SmallEnough(Array *a, UInt numel);
Task Arr_Clear(Array *a);
Task Arr_Data_Only(Array *a, void **data_ptr);
Task Arr_Iter(Array *a, Task (*action)(UInt, void *, void *), void *pass_data);
Task Arr_Overwrite(Array *a, Int dest, void *src, UInt n);

/* Valore che contrassegna le array correttamente inizializzate */
#define ARR_ID 0x66626468

/* Useful macros */
#define Arr_Empty Arr_Clear
#define Arr_Chain(a)	((a)->chain)
#define Arr_NumItem(a)	((a)->numel)
#define Arr_NumItems(a)	((a)->numel)
#define Arr_Ptr(a)		((a)->ptr)
#define Arr_FirstItem(a, type)	*((type *) ((a)->ptr))
#define Arr_LastItem(a, type)	*((type *) ((a)->ptr + ((a)->numel-1)*((UInt) (a)->elsize)))
#define Arr_FirstItemPtr(a, type)	((type *) ((a)->ptr))
#define Arr_LastItemPtr(a, type)	((type *) ((a)->ptr + ((a)->numel-1)*((UInt) (a)->elsize)))
#define Arr_Dec(a) Arr_SmallEnough((a), --(a)->numel)
#define Arr_Inc(a) Arr_BigEnough((a), ++(a)->numel)
#define Arr_MDec(a, n) Arr_SmallEnough((a), (a)->numel -= n)
#define Arr_MInc(a, n) Arr_BigEnough((a), (a)->numel += n)
#endif

#ifdef DEBUG_ARRAY
Array *Array_New_Debug(UInt elsize, UInt mindim, const char *file, int line);
Task Arr_New_Debug(Array **new_array, UInt elsize, UInt mindim,
 const char *file, int line);
void Arr_Destroy_Debug(Array *a, const char *file, int line);
void *Arr_ItemPtr_Debug(Array *a, int n, const char *src, int line);

#  define Arr_New(new_array, elsize, mindim) \
     Arr_New_Debug(new_array, elsize, mindim, __FILE__, __LINE__)
#  define Array_New(elsize, mindim) \
     Array_New_Debug(elsize, mindim, __FILE__, __LINE__)
#  define Arr_Destroy(a) \
     Arr_Destroy_Debug(a, __FILE__, __LINE__)
#  define Arr_ItemPtr(a, type, n) \
     ((type *) Arr_ItemPtr_Debug(a, n, __FILE__, __LINE__))
#  define Arr_Item(a, type, n) \
     *((type *) Arr_ItemPtr_Debug(a, n, __FILE__, __LINE__))

#else
Array *Array_New(UInt elsize, UInt mindim);
Task Arr_New(Array **new_array, UInt elsize, UInt mindim);

/** @brief Destroys the array.
 */
void Arr_Destroy(Array *a);

#  define Arr_ItemPtr(a, type, n) ((type *) ((a)->ptr + ((n)-1)*((UInt) (a)->elsize)))
#  define Arr_Item(a, type, n)    *((type *) ((a)->ptr + ((n)-1)*((UInt) (a)->elsize)))
#endif
