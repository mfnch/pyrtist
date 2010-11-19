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

typedef enum {
  BOXGOBJKIND_EMPTY,
  BOXGOBJKIND_VOID,
  BOXGOBJKIND_CHAR,
  BOXGOBJKIND_INT,
  BOXGOBJKIND_REAL,
  BOXGOBJKIND_POINT,
  BOXGOBJKIND_STR,
  BOXGOBJKIND_OBJECT,
  BOXGOBJKIND_ARRAY
} BoxGObjKind;

typedef union {
  BoxChar     v_char;
  BoxInt      v_int;
  BoxReal     v_real;
  BoxPoint    v_point;
  BoxArr      v_array;
  /*BoxStr      v_str;*/
} BoxGObjValue;

typedef struct {
  BoxGObjKind  kind;
  BoxGObjValue value;
} BoxGObj;

typedef BoxGObj *BoxGObjPtr;

#endif

