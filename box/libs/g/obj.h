/****************************************************************************
 * Copyright (C) 2010 by Matteo Franchin                                    *
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

#ifndef _BOX_LIBG_CONTAINER_H
#  define _BOX_LIBG_CONTAINER_H

#  include <box/types.h>
#  include <box/array.h>
#  include <box/str.h>

typedef enum {
  BOXGOBJKIND_EMPTY,
  BOXGOBJKIND_VOID,
  BOXGOBJKIND_CHAR,
  BOXGOBJKIND_INT,
  BOXGOBJKIND_REAL,
  BOXGOBJKIND_POINT,
  BOXGOBJKIND_OBJECT,
  BOXGOBJKIND_STR,
  BOXGOBJKIND_COMPOSITE,
  BOXGOBJKIND_TYPE
} BoxGObjKind;

typedef union {
  BoxChar     v_char;
  BoxInt      v_int;
  BoxReal     v_real;
  BoxPoint    v_point;
  BoxArr      v_array;
  BoxStr      v_str;
} BoxGObjValue;

typedef struct {
  BoxGObjKind  kind;
  BoxGObjValue value;
} BoxGObj;

typedef BoxGObj *BoxGObjPtr;

/** Initialisation routine for BoxGObj objects. */
void BoxGObj_Init(BoxGObj *gobj);

/** Finalisation routine for BoxGObj objects. */
void BoxGObj_Finish(BoxGObj *gobj);

BoxGObj *BoxGObj_New(void);

void BoxGObj_Destroy(BoxGObj *gobj);

/** Create a copy of gobj_src in gobj_dest. gobj_dest should not contain
 * a proper BoxGObj object. In other words it should be either uninitalized
 * or be a BOXGOBJKIND_EMPTY object (just BoxGObj_Init has been called on it).
 */
void BoxGObj_Init_From(BoxGObj *gobj_dest, BoxGObj *gobj_src);

/** Retrieve a subobject of the given BoxGObj object.
 * Return NULL if the index it out of bounds.
 */
BoxGObj *BoxGObj_Get(BoxGObj *gobj, BoxInt idx);

/** Get the number of subobject contained in gobj. */
size_t BoxGObj_Get_Length(BoxGObj *gobj);

#endif

