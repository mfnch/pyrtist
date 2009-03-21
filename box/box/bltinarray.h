/****************************************************************************
 * Copyright (C) 2009 by Matteo Franchin                                    *
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

/** @file bltinarray.h
 * @brief Implementation of the Box Array object.
 */

#ifndef _BLTINARRAY_H
#  define _BLTINARRAY_H

#  include <box/types.h>
#  include <box/vmalloc.h>


typedef struct {
  BoxInt num_dim;   /**< Number of dimensions of the array */
  size_t item_size; /**< Size of the items of the array */
  size_t *sizes;    /**< Pointer to the array of maximum indices */
  BoxObj data;      /**< Pointer to the actual data */
} BoxArray;

/** Create a new BoxArray with the specified number of dimensions.
 * The array size has to be specified with BoxArray_Set_Size before the array
 * can be used.
 */
BoxTask BoxArray_Init(BoxArray *a, BoxInt num_dim);

/** Set the size for the next dimension of the BoxArray object.
 * The BoxArray object knows about what dimensions have already been set
 * and proceeds with the next one. A typical initialisation of a BoxArray
 * object looks like that:
 * For an array a[10][20][30],
 *
 *    BoxArray a;
 *    BoxArray_Init(& a, 3);
 *    BoxArray_Set_Size(& a, 10);
 *    BoxArray_Set_Size(& a, 20);
 *    BoxArray_Set_Size(& a, 30);
 */
BoxTask BoxArray_Set_Size(BoxArray *a, BoxInt size);

/** Destroy the given BoxArray object, calling a finaliser if needed
 * (this is we require to pass the vm object)
 */
void BoxArray_Finish(BoxVM *vm, BoxArray *a);

/** This function provides raw access to the data contained in the array.
 * It returns on 'item' the extended pointer to the byte at address 'addr'
 * counting (in bytes) from the beginning of the data.
 */
void BoxArray_Access(BoxArray *a, BoxObj *item, size_t addr);

/** Assist in calculating the address of an element of the array.
 * Example: suppose you have an array a[10][20][30] and you want to access
 * the element a[9][19][29], then you could use the following code:
 *
 *   BoxObj item;
 *   size_t addr = 9;
 *   BoxArray_Calc_Address(& a, & addr, 0, 19);
 *   BoxArray_Calc_Address(& a, & addr, 1, 29);
 *   BoxArray_Access(& a, & item, addr);
 */
BoxTask BoxArray_Calc_Address(BoxArray *a, size_t *addr,
                              BoxInt dim, BoxInt index);

#endif
