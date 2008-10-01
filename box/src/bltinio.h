/****************************************************************************
 * Copyright (C) 2008 by Matteo Franchin                                    *
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

/** @file bltinio.h
 * @brief BuiLTIN Input/Output functions.
 *
 * This file collects the basic input/output functions of Box.
 */

#ifndef _BLTINIO_H
#  define _BLTINIO_H

#  include "types.h"
#  include "typesys.h"

/** The File data type. */
extern Type type_File;

/** Register the builtin IO functions. */
Task Bltin_Io_Init(void);

/** To be called when destroying the compiler data structures. */
void Bltin_Io_Destroy(void);

#endif