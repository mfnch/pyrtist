/****************************************************************************
 * Copyright (C) 2014 by Matteo Franchin                                    *
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
 * @file index_priv.h
 * @brief Implementation for #BoxIndex.
 */

#ifndef _BOX_INDEX_PRIV_H
#  define _BOX_INDEX_PRIV_H

#  include <box/index.h>
#  include <box/array.h>
#  include <box/hashtable.h>


struct BoxIndex_struct {
  BoxArr names_from_nums;
  BoxHT nums_from_names;
};


/**
 * @brief Initialization for #BoxIndex.
 *
 * @param idx Region of memory to initialize.
 * @param num_entries Typical number of entries to map.
 */
BOXEXPORT void
BoxIndex_Init(BoxIndex *idx, uint32_t num_entries);

/**
 * @brief Finalization for #BoxIndex.
 */
BOXEXPORT void
BoxIndex_Finish(BoxIndex *idx);

#endif /* _BOX_INDEX_PRIV_H */
