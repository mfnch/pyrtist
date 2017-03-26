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

#ifndef _POINTLIST_H
#  define _POINTLIST_H

#  include <stdio.h>

#  include "objlist.h"

#  define PointList ObjList

#  define pointlist_init(pl) (objlist_init((pl), sizeof(BoxPoint)))
#  define pointlist_destroy(pl) (objlist_destroy((pl)))
#  define pointlist_dup(dest, src) (objlist_dup((dest), (src)))
#  define pointlist_find(pl, name) ((BoxPoint *) objlist_find((pl), (name)))
#  define pointlist_get(pl, index) ((BoxPoint *) objlist_get((pl), (index)))
#  define pointlist_add(pl, p, name) (objlist_add((pl), (p), name))
#  define pointlist_iter(pl, it, data) (objlist_iter((pl), (it), (data)))
#  define pointlist_num(pl) (objlist_num((pl)))
#  define pointlist_get_name objlist_get_name

void pointlist_print(PointList *pl, FILE *out);

#endif

