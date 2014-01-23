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
 * @file index.h
 * @brief Mapping between strings and integers.
 */

#ifndef _BOX_INDEX_H
#  define _BOX_INDEX_H

#  include <stdint.h>

#  include <box/core.h>


typedef struct BoxIndex_struct BoxIndex;


/**
 * @brief Obtain the index corresponding to the given name.
 */
BOXEXPORT uint32_t
BoxIndex_Get_Idx_From_Name(BoxIndex *idx, const char *name);

/**
 * @brief Obtain the name corresponding to the given index.
 */
BOXEXPORT const char *
BoxIndex_Get_Name_From_Idx(BoxIndex *idx, uint32_t num);

#endif /* _BOX_INDEX_H */
