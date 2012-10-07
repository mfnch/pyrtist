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

#ifndef _OBJLIST_H
#  define _OBJLIST_H

#  include <stdio.h>

#  include "types.h"
#  include "buffer.h"


typedef struct {
  char *name;
} ObjListItem;

typedef struct {
  buff ol;
} ObjList;

typedef BoxTask (*ObjListIterator)(BoxInt obj_idx, char *name,
                                void *obj, void *data);

BoxTask objlist_init(ObjList *ol, BoxInt obj_size);
void objlist_destroy(ObjList *pl);
BoxTask objlist_clear(ObjList *ol);
BoxTask objlist_dup(ObjList *dest, ObjList *src);
void *objlist_find(ObjList *ol, const char *name);
void *objlist_get(ObjList *ol, size_t index);
const char *objlist_get_name(ObjList *ol, size_t index);
BoxTask objlist_add(ObjList *ol, void *obj, char *name);
BoxTask objlist_iter(ObjList *ol, ObjListIterator it, void *data);
BoxInt objlist_num(ObjList *ol);

#endif

