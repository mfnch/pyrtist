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

#include <math.h>

#include <box/types.h>

#include "matrix.h"
#include "winmap.h"

void BoxGWinMap_Init(BoxGWinMap *wm, BoxGMatrix *m) {
  BoxReal m11 = m->m11, m21 = m->m21;
  wm->matrix = *m;
  wm->width_a = sqrt(m11*m11 + m21*m21);
}

void BoxGWinMap_Map_Point(BoxGWinMap *wm, BoxPoint *out, BoxPoint *in) {
  BoxGMatrix_Map_Point(& wm->matrix, out, in);
}

void BoxGWinMap_Map_Vector(BoxGWinMap *wm, BoxPoint *out, BoxPoint *in) {
  BoxGMatrix_Map_Vector(& wm->matrix, out, in);
}

void BoxGWinMap_Map_Width(BoxGWinMap *wm, BoxReal *out, BoxReal *in) {
  *out = *in * wm->width_a + wm->width_b;
}
