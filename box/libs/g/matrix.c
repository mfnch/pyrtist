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

#include <stdlib.h>
#include <math.h>

#include <box/types.h>

#include "matrix.h"

void BoxGMatrix_Set(BoxGMatrix *m,
                    BoxPoint *t, BoxPoint *rcntr, BoxReal rang,
                    BoxReal sx, BoxReal sy) {
  BoxReal c, s, m11, m12, m21, m22;

  c = cos(rang); s = sin(rang);
  m11 = sx * c; m12 = -(sy * s);
  m21 = sx * s; m22 = sy * c;

  m->m11 = m11; m->m12 = m12;
  m->m21 = m21; m->m22 = m22;
  m->m13 = (1.0 - m11) * rcntr->x - m12 * rcntr->y + t->x;
  m->m23 = (1.0 - m22) * rcntr->y - m21 * rcntr->x + t->y;
}

void BoxGMatrix_Set_Identity(BoxGMatrix *m) {
  m->m11 = m->m22 = 1.0;
  m->m12 = m->m21 = m->m13 = m->m23 = 0.0;
}

void BoxGMatrix_Map_Points(BoxGMatrix *m, BoxPoint *out, BoxPoint *in,
                           size_t num_points) {
  BoxReal m11 = m->m11, m12 = m->m12,
          m21 = m->m21, m22 = m->m22,
          m13 = m->m13, m23 = m->m23;
  size_t i;

  for (i = 0; i < num_points; i++) {
    BoxReal in_x = in[i].x; /* Just in case in == out */
    out[i].x = m11 * in_x + m12 * in[i].y + m13;
    out[i].y = m21 * in_x + m22 * in[i].y + m23;
  }
}

void BoxGMatrix_Map_Point(BoxGMatrix *m, BoxPoint *out, BoxPoint *in) {
  BoxReal in_x = in->x; /* Just in case in == out */
  out->x = m->m11 * in_x + m->m12 * in->y + m->m13;
  out->y = m->m21 * in_x + m->m22 * in->y + m->m23;
}

void BoxGMatrix_Map_Vectors(BoxGMatrix *m, BoxPoint *out, BoxPoint *in,
                            size_t num_vecs) {
  BoxReal m11 = m->m11, m12 = m->m12,
          m21 = m->m21, m22 = m->m22;
  size_t i;

  for (i = 0; i < num_vecs; i++) {
    BoxReal in_x = in[i].x;
    out[i].x = m11 * in_x + m12 * in[i].y;
    out[i].y = m21 * in_x + m22 * in[i].y;
  }
}

void BoxGMatrix_Map_Vector(BoxGMatrix *m, BoxPoint *out, BoxPoint *in) {
  BoxReal in_x = in->x;
  out->x = m->m11 * in_x + m->m12 * in->y;
  out->y = m->m21 * in_x + m->m22 * in->y;
}
