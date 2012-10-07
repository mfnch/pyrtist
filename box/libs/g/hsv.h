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

#ifndef _HSV_H
#  define _HSV_H

#  include "types.h"

typedef struct {
  BoxReal h, s, v, a;
} HSV;

/** Make sure the components of 'hsv' lie to the right intervals
 * ('hsv' is corrected, if needed).
 */
void HSV_Trunc(HSV *hsv);

/** Convert the RGB Color 'c' to an 'HSV' value in 'hsv'.
  * NOTE: the function assumes that the components of 'c' are included
  *   in [0, 1]
  */
void HSV_From_Color(HSV *hsv, Color *c);

/** Convert the HSV value 'hsv' to a RGB Color in 'c'
  * NOTE: the function assumes that the components of 'hsv' are included
  *   in [0, 360[ for 'hsv->h' and in [0, 1] for 'hsv->s' and 'hsv->v'.
  */
void HSV_To_Color(Color *c, HSV *hsv);

#endif
