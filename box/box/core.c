/****************************************************************************
 * Copyright (C) 2012 by Matteo Franchin                                    *
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

#include <box/types.h>
#include <box/core.h>

/**
 * Structure collection all Box core types.
 */
BoxCoreTypes box_core_types;


/* Initialize the core types of Box. */
BoxBool BoxCoreTypes_Init(BoxCoreTypes *core_types) {
  BoxBool success = BOXBOOL_TRUE;

  struct {
    BoxType *dest;
    const char *name;
    BoxTypeId id;
    size_t size;
    size_t alignment;
  } *row, table[] = {
    {& core_types->type_type, "Type", BOXTYPEID_TYPE,
     sizeof(BoxType), __alignof__(BoxType)},
    {& core_types->char_type, "Char", BOXTYPEID_CHAR,
     sizeof(BoxChar), __alignof__(BoxChar)},
    {& core_types->int_type, "Int", BOXTYPEID_INT,
     sizeof(BoxInt), __alignof__(BoxInt)},
    {& core_types->real_type, "Real", BOXTYPEID_REAL,
     sizeof(BoxReal), __alignof__(BoxReal)},
    {& core_types->point_type, "Point", BOXTYPEID_POINT,
     sizeof(BoxPoint), __alignof__(BoxPoint)},
    {& core_types->pointer_type, "Ptr", BOXTYPEID_PTR,
     sizeof(BoxPtr), __alignof__(BoxPtr)},
    {NULL, (const char *) NULL, BOXTYPEID_NONE,
     (size_t) 0, (size_t) 0}
  };

  for (row = & table[0]; row->dest; row++) {
    BoxType t = BoxType_Create_Primary(row->id, row->size, row->alignment);

    if (t) {
      BoxType id = BoxType_Create_Ident(t, row->name);
      if (id)
        *row->dest = id;

      else
        BoxType_Unlink(t);
    
    } else {
      success = BOXBOOL_FALSE;
      *row->dest = NULL;
    }
  }

  return success;
}
