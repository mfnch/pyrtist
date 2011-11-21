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

#ifndef _BB_H
#  define _BB_H

#include <box/types.h>

typedef struct {
  BoxPoint min,
           max;
  BoxInt   num;

} BoxGBBox;

/** Initialise bounding box object */
void BoxGBBox_Init(BoxGBBox *bb);

/** Enlarge bounding box object 'bb', such that it contains the point 'p'. */
void BoxGBBox_Extend(BoxGBBox *bb, BoxPoint *p);

/** Enlarge bounding box object 'dest' to contain the points of 'src'.. */
void BoxGBBox_Fuse(BoxGBBox *dst, BoxGBBox *src);

/** Return the volume (area) occupied by the bounding box. */
BoxReal BoxGBBox_Get_Volume(BoxGBBox *bb);

/** Enlarge the bounding box adding margins. */
void BoxGBBox_Extend_Margins(BoxGBBox *bb, BoxPoint *margin_min,
                             BoxPoint *margin_max);

/** Enlarge the bounding box adding equal margins. */
void BoxGBBox_Extend_Margin(BoxGBBox *bb, BoxReal margin);

/** Compute the bounding box of the given figure. If the bounding box
 * is degenerate returns 0. Returns 1 otherwise.
 */
int BoxGBBox_Compute(BoxGBBox *bbox, BoxGWin *figure);

#endif
