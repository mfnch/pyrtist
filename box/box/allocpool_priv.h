/****************************************************************************
 * Copyright (C) 2013 by Matteo Franchin                                    *
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

/**
 * @file allocpool_priv.h
 * @brief Private definitions for allocpool.h.
 */

#ifndef _BOX_ALLOCPOOL_PRIV_H
#  define _BOX_ALLOCPOOL_PRIV_H

#  include <stdlib.h>
#  include <stdint.h>

#  include <box/allocpool.h>


typedef struct BCAllocSubPool_struct BCAllocSubPool;

struct BCAllocSubPool_struct {
  BCAllocSubPool *next;     /**< Next pool (NULL for none). */
  void           *items;    /**< Pointer to the items in the pool. */
  size_t         capacity;  /**< Capacity of this pool. */
  size_t         num_items; /**< Number of items in the pool. */
};

struct BCAllocPool_struct {
  BCAllocSubPool sub_pool;  /**< The largest/first pool. */
  size_t         item_size; /**< Size of items in the pool. */
  size_t         num_items; /**< Number of items in all the pools. */
};


BOXEXPORT BCAllocPool *
BCAllocPool_Init(BCAllocPool *pool, size_t item_size,
                 size_t initial_capacity);

BOXEXPORT void
BCAllocPool_Finish(BCAllocPool *pool);

#endif /* _BOX_ALLOCPOOL_PRIV_H */
