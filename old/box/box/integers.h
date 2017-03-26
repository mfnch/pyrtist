/****************************************************************************
 * Copyright (C) 2012 by Matteo Franchin                                    *
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
 * @file integers.h
 * @brief Wrapper for the stdint.h header, defining various integer types.
 *
 * This header provides some typedef-s which wrap the definitions typically
 * given in stdint.h. The idea is that - in case this header is missing -
 * this header can be used to quickly provide the missing definitions.
 */

#ifndef _BOX_INTEGERS_H
#  define _BOX_INTEGERS_H

#  include <stdint.h>

typedef uint16_t BoxUInt16;
typedef uint32_t BoxUInt32;

#endif /* _BOX_INTEGERS_H */
