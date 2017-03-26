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

/* autoput.h
 *
 * Dichiarazione delle funzioni definite in autoput.c
 */

#ifndef _AUTOPUT_H
#  define _AUTOPUT_H

#  include "graphic.h"

void aput_identity_matrix(BoxReal *matrix);
void aput_get(BoxPoint *rot_center, BoxPoint *trsl_vect,
              BoxReal *rot_angle, BoxReal *scale_x, BoxReal *scale_y );
void aput_set(BoxPoint *rot_center, BoxPoint *trsl_vect,
              BoxReal *rot_angle, BoxReal *scale_x, BoxReal *scale_y );
int aput_autoput(BoxPoint *F, BoxPoint *R, BoxReal *weight, int n, int needed);
int aput_allow(const char *permissions, int *needed);

#endif
