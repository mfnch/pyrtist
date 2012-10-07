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
  enum {GOT_NOTHING,
        GOT_POINT,
        GOT_1ST_FLOAT,
        GOT_2ND_FLOAT} state;
  struct {
    int color : 1;
  } got;
  BoxInt num_points;

  Color color;
  int close;
  BoxReal arrowscale;
  LineTracer *lt;
  LinePiece this_piece;

  GStyle default_style, style;
} WindowLine;

#else

#  ifndef _I_LINE_H
#    define _I_LINE_H
Task line_window_init(Window *w);
void line_window_destroy(Window *w);
#  endif

#endif
