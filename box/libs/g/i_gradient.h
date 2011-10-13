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

#ifndef _I_GRADIENT_H

#  define _I_GRADIENT_H

#  include "types.h"
#  include "buffer.h"
#  include "graphic.h"
#  include "virtmach.h"

typedef struct {
  struct {
    unsigned int type : 1;
    unsigned int point1 : 1;
    unsigned int point2 : 1;
    unsigned int radius1: 1;
    unsigned int radius2: 1;
    unsigned int pause : 1;
    unsigned int pos : 1;
  } got;

  ColorGrad gradient;
  ColorGradItem this_item;
  buff items;
} Gradient;

typedef Gradient *GradientPtr;

Task x_gradient(BoxVM *vmp);
#endif

