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
  struct {
    enum {GOT_NOT, GOT_NOW, GOT_BEFORE} center, radius_a, radius_b;
    int color : 1;
  } got;
  Color color;
  BoxPoint center;
  BoxReal radius_a, radius_b;

  GStyle default_style, style;
} WindowCircle;

#else

#  ifndef _I_CIRCLE_H
#    define _I_CIRCLE_H

#  endif

#endif
