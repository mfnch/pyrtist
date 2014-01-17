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
 * This module provides functionality to map between linear and full position
 * in source files. The idea is to construct a table which allows to map from
 * a positive integer (e.g. the index of the character read in the input
 * stream) to a "full" position, consisting of the file name, the line number
 * and the column number. This allows simplifying handling of positions in the
 * compiler and allows reducing considerably the size of AST nodes.
 * In particular, storing the start and end position in an AST node would
 * require adding the following data structure twice to every node:
 *
 *   struct{uint32 line, column; const char *file_name;}
 *
 * This requires 32 bytes for 64-bit architectures. Using linear positions we
 * need to store just two integers => 8 bytes. It is also easier to propagate
 * positions to parent nodes. This functionality can also be used to map
 * positions in the bytecode to full positions in the sources.
 * The implementation here provided is optimised for both speed and memory.
 * This is done with a hybrid binary-search/linear-search algorithm which, for
 * a source file with N newlines, gives about ~2N bytes of memory requirement
 * to store the map. The map is stored by allocating ~log N blocks, putting
 * low stress on malloc. Also the mapping is performed in about log N steps.
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
 * @brief Provide a mapping linear to source position.
 *
 * @param sm The #BoxSrcMap where to store the map information.
 * @param lin_pos The input linear position.
 * @param file_name The file name corresponding to @p lin_pos.
 * @param line The line number corresponding to @p lin_pos.
 * @param col The column corresponding to @p lin_pos.
 * @return Whether the map could be stored.
 */
BOXEXPORT BoxBool
BoxSrcMap_Store(BoxSrcMap *sm, uint32_t lin_pos,
                const char *file_name, uint32_t line, uint32_t col);

/**
 * @brief Map from linear to full position.
 *
 * @param sm The #BoxSrcMap where the map information is stored.
 * @param lin_pos The input linear position.
 * @param file_name Where to put the file name corresponding to @p lin_pos.
 * @param line Where to put the line number corresponding to @p lin_pos.
 * @param col Where to put the column number corresponding to @p lin_pos.
 * @return Whether the map was performed successfully.
 */
BOXEXPORT BoxBool
BoxSrcMap_Map(BoxSrcMap *sm, uint32_t pos,
              const char **file_name, uint32_t *line, uint32_t *col);

#endif /* _BOX_SRCMAP_H */
