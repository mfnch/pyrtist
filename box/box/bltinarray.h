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
 * @file bltinarray.h
 * @brief Heterogeneous arrays (box Array object).
 */

#ifndef _BOX_BLTINARRAY_H
#  define _BOX_BLTINARRAY_H

#  include <box/core.h>
#  include <box/obj.h>

/**
 * @brief Box native heterogeneous Array type (ARRAY).
 */
typedef struct BoxArray_struct BoxArray;

/**
 * @brief Box's Get type.
 */
typedef BoxAny BoxGet;

/**
 * @brief Install @c ARRAY and @c Array as a core type (internal).
 *
 * @param ct Where the @c ARRAY and @c Array types are to be installed.
 * @return Whether the operation suceeded.
 * @note The types are created and their combinations are defined.
 */
BOXEXPORT BoxBool
BoxCoreTypes_Register_Array(BoxCoreTypes *ct);

#endif /* _BOX_BLTINARRAY_H */
