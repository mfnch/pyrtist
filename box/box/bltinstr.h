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

#ifndef _BLTINSTR_H
#  define _BLTINSTR_H

#  include "types.h"

/** @brief The Str object.
 */
typedef struct {
  BoxInt length;      /**< the current length of the string */
  BoxInt buffer_size; /**< the size of the block allocated to contain
                           the string. */
  char   *ptr;        /**< the pointer to the block allocated to contain
                           the string. */
} BoxStr;

/** Just for compatibility */
typedef BoxStr Str;

extern Type type_Str, type_StrSpecies;

void Str_Init(Str *s);

Task Str_Large_Enough(Str *s, Int length);

Task Str_Concat(Str *s, const char *ca);

Task Bltin_Str_Init(void);

void Bltin_Str_Destroy(void);

#  define Str_Get_CStr(s) ((s)->ptr)

#endif
