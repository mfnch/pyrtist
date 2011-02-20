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

/** @file bltinstr.h
 * @brief BuiLTIN implementation of the STRing object.
 *
 * Here we provide the implementation of the Str object, used for in Box
 * for storing strings which can be concatenated and resized easily.
 */

#ifndef _BOX_BLTINSTR_H
#  define _BOX_BLTINSTR_H

#  include <stdlib.h>

#  include <box/types.h>
#  include <box/cmpptrs.h>

void Bltin_Str_Register_Procs(BoxCmp *c);

Task Bltin_Str_Init(void);

void Bltin_Str_Destroy(void);

#endif
