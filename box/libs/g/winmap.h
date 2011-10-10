/****************************************************************************
 * Copyright (C) 2011 by Matteo Franchin                                    *
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

#ifndef _BOX_LIBG_WINMAP_H
#  define _BOX_LIBG_WINMAP_H

#  include <box/types.h>

#  include "matrix.h"

/** Object used to map a Window to another Window by transforming coordinate
 * system, widths, etc  (in future may allow color transformation or
 * whatsoever).
 */
typedef struct {
  BoxGMatrix matrix;
  BoxReal    width_a, width_b;

} BoxGWinMap;

void BoxGWinMap_Init(BoxGWinMap *wm, BoxGMatrix *m);

#define BoxGWinMap_Finish(wm) do {} while(0)

/** Transform a point 'in' to 'out' by using the window map 'wm'. */
void BoxGWinMap_Map_Point(BoxGWinMap *wm, BoxPoint *out, BoxPoint *in);

/** Transform a vector 'in' to 'out' by using the window map 'wm'. */
void BoxGWinMap_Map_Vector(BoxGWinMap *wm, BoxPoint *out, BoxPoint *in);

/** Transform a line width 'in' to 'out' by using the window map 'wm'. */
void BoxGWinMap_Map_Width(BoxGWinMap *wm, BoxReal *out, BoxReal *in);

#endif /* _BOX_LIBG_WINMAP_H */
