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
 * A pool of objects is a collection of objects which can be allocated in
 * sequence but are all freed at once. This allows to allocate them in a
 * reduced number of regions in memory making allocation and deallocation
 * faster to handle.
 */

#ifndef _BOX_ALLOCPOOL_H
#  define _BOX_ALLOCPOOL_H

#  include <stdio.h>
#  include <stdint.h>

#  include <box/types.h>

/**
 * @brief Allocation pool.
 *
 * This object is useful when allocating several objects which need to be
 * freed all at once.
 */
typedef struct BoxAllocPool_struct BoxAllocPool;

/**
 * @brief Create a new allocation pool.
 *
 * @param initial_capacity Capacity of the smallest allocation made (in bytes).
 * @return A new allocation pool or @c NULL, if the operation failed.
 */
BOXEXPORT BoxAllocPool *
BoxAllocPool_Create(uint32_t initial_capacity);

/**
 * @brief Destroy the given allocation pool.
 */
BOXEXPORT void
BoxAllocPool_Destroy(BoxAllocPool *pool);

/**
 * @brief Allocate a new block of memory from the pool.
 *
 * @param pool The allocation pool.
 * @param size Size of memory to allocate.
 * @return Pointer to a region of memory ready to contain @p size bytes.
 */
BOXEXPORT void *
BoxAllocPool_Alloc(BoxAllocPool *pool, uint32_t num_items);

/**
 * @brief Allocate a new block of memory from the pool with given alignment.
 *
 * @param pool The allocation pool.
 * @param size Size of memory to allocate.
 * @param alignment Required alignment (must be a power of two).
 * @return Aligned pointer to a region of memory ready to contain @p size bytes.
 */
BOXEXPORT void *
BoxAllocPool_Alloc_Aligned(BoxAllocPool *pool, uint32_t size,
                           unsigned int alignment);

/**
 * @brief Copy a string in the allocation pool.
 *
 * @param pool The allocation pool.
 * @param str The string to copy.
 * @param str_length The length of the string to copy.
 * @return A NUL-terminated string in the allocation pool or @c NULL if the
 *   allocation failed.
 */
BOXEXPORT char *
BoxAllocPool_Str_NDup(BoxAllocPool *pool, const char *str, uint32_t str_length);

/**
 * @brief Print statistics about the pool (debugging function).
 *
 * @param pool The allocation pool.
 * @param out The output stream.
 */
BOXEXPORT void
BoxAllocPool_Print_Stats(BoxAllocPool *pool, FILE *out);

#endif /* _BOX_ALLOCPOOL_H */
