/****************************************************************************
 * Copyright (C) 2006, 2008 by Matteo Franchin                              *
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

/** @file builtins.h
 * @brief Register built-in types and functions(calling other bltin modules).
 *
 * This file registers the fundamental types of Box, calling also the init
 * functions for other modules, such as bltinstr and bltinio.
 */

#ifndef _BUILTINS_H
#  define _BUILTINS_H

#  include "types.h"

/* Important builtin types */
extern Type type_Point, type_RealNum, type_IntNum, type_CharNum,
            type_CharArray, type_Print;

Task Builtins_Init(void);

void Builtins_Destroy(void);

#endif
