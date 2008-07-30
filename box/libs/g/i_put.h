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

typedef struct {
  Point on_src, on_dest;
  Real weight;
  struct {
    int on_src : 1;
    int on_dest : 1;
    int weight : 1;
  } have;
} WindowPutNear;

/* This is part of the Window object */
typedef struct {
  WindowPutNear near;

  int auto_transforms;
  buff fig_points, back_points, weights;
  Real rot_angle;
  Point rot_center, translation, scale;
  Matrix matrix;

  void *figure;
  struct {
    int constraints : 1;
    int figure : 1;
    int compute : 1;
    int translation : 1;
    int rot_angle : 1;
    int scale : 1;
    int matrix : 1;
  } got;
} WindowPut;

#else

#  ifndef _I_PUT_H
#    define _I_PUT_H

Task put_window_init(Window *w);
void put_window_destroy(Window *w);

#  endif

#endif
