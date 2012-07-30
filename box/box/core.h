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
 * @file core.h
 * @brief Definition of core types and objects.
 *
 * This module defines Box core types and objects.
 */

#ifndef _BOX_CORE_H
#  define _BOX_CORE_H

#include <box/ntypes.h>

/** Box core types. */
typedef struct BoxCoreTypes_struct {
  BoxType root_type,
          init_type,
          finish_type,
          type_type,
          callable_type,
          char_type,
          int_type,
          real_type,
          point_type,
          pointer_type;
} BoxCoreTypes;

/** Object containing all the core types of Box. */
extern BoxCoreTypes box_core_types;

/* Initialize the core types of Box. */
BoxBool BoxCoreTypes_Init(BoxCoreTypes *core_types);

#endif /* _BOX_CORE_H */
