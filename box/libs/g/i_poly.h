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

#ifdef _DEF_WINDOW_SUBOBJECTS

/* This is part of the Window object */
typedef struct {
  enum {POLY_GOT_NOTHING,
        POLY_GOT_POINT,
        POLY_GOT_1ST_FLOAT,
        POLY_GOT_2ND_FLOAT} state;

  struct {
    int color : 1;
  } got;

  int num_points, num_margins, last_idx;
  Point first_points[2], last_point, lastb;
  Real margin[2];

  Color color;
} WindowPoly;

#else

#  ifndef _I_POLY_H
#    define _I_POLY_H

#  endif

#endif
