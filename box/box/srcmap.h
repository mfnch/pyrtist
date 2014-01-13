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
 * @file srcmap.h
 * @brief Mapping between linear and full position in source (text) files.
 *
 * This module provides functionality to map between linear position in source
 * files
 */

#ifndef _BOX_SRCMAP_H
#  define _BOX_SRCMAP_H

#  include <stdint.h>

#  include <box/core.h>


/**
 * @brief Allow to define and extrapolate a mapping linear to full position.
 *
 * This object allows to provide a mapping between linear and full position
 * in a source (text) file. Once the correspondence is defined with
 * BoxSrcMap_Give(), the object can be used to map any linear position to
 * a full position with BoxSrcMap_Map().
 */
typedef struct BoxSrcMap_struct BoxSrcMap;

/**
 * @brief Create a #BoxSrcMap object.
 */
BOXEXPORT BoxSrcMap *
BoxSrcMap_Create(void);

/**
 * @brief Destroy a #BoxSrcMap object created with BoxSrcMap_Create().
 */
BOXEXPORT void
BoxSrcMap_Destroy(BoxSrcMap *sm);

/**
 * @brief Return the depth of the tree.
 */
BOXEXPORT int
BoxSrcMap_Get_Depth(BoxSrcMap *sm);

/**
 * @brief Provide a mapping linear to source position.
 */
BOXEXPORT void
BoxSrcMap_Give(BoxSrcMap *sm, uint32_t lin_pos,
               const char *file_name, uint32_t line, uint32_t col);

#endif /* _BOX_SRCMAP_H */
