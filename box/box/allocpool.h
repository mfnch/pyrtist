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
 * @file allocpool.h
 * @brief Utility to allocate pools of objects.
 *
 * A pool of objects is a collection of objects all having the same type and
 * size which can be allocated in sequence but are freed all at once. This
 * allows to allocate them in a reduced number of regions in memory making
 * allocation and deallocation faster to handle.
 */

#ifndef _BOX_ALLOCPOOL_H
#  define _BOX_ALLOCPOOL_H

#  include <stdio.h>

#  include <box/types.h>

/**
 * @brief Allocation pool.
 *
 * This object is useful when allocating several objects which need to be
 * freed all at once.
 */
typedef struct BCAllocPool_struct BCAllocPool;

/**
 * @brief Create a new allocation pool.
 *
 * @param item_size Size of the items in the pool.
 * @param initial_capacity Capacity of the smallest allocation made.
 * @return A new allocation pool or @c NULL, if the operation failed.
 */
BOXEXPORT BCAllocPool *
BCAllocPool_Create(size_t item_size,
		   size_t initial_capacity);

/**
 * @brief Destroy the given allocation pool.
 */
BOXEXPORT void
BCAllocPool_Destroy(BCAllocPool *pool);

/**
 * @brief Allocate a new item of the pool.
 *
 * @param pool The allocation pool.
 * @return A new item from the pool.
 */
BOXEXPORT void *
BCAllocPool_Alloc(BCAllocPool *pool);

/**
 * @brief Print statistics about the pool (debugging function).
 *
 * @param pool The allocation pool.
 * @param out The output stream.
 */
BOXEXPORT void
BCAllocPool_Print_Stats(BCAllocPool *pool, FILE *out);

#endif /* _BOX_ALLOCPOOL_H */
