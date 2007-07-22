/***************************************************************************
 *   Copyright (C) 2007 by Matteo Franchin                                 *
 *   fnch@libero.it                                                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

/* $Id$ */

/** @file list.h
 * @brief Implementation of the list type.
 */


#ifndef _LIST_H
#  define _LIST_H

/* Used in List_Append_String */
#  include <string.h>

#  include "types.h"

typedef Task (*ListIterator)(void *item, void *pass_data);
typedef void (*ListDestructor)(void *item);

typedef struct __ListItemHead {
  struct __ListItemHead *previous;
  struct __ListItemHead *next;
} ListItemHead;

typedef struct {
  UInt item_size;
  UInt length;
  ListDestructor destructor;
  ListItemHead head_tail;
} List;

/* Used with the function List_Product_Iter */
typedef Task (*ListProduct)(void **tuple, void *pass);

void List_New(List **l, UInt item_size);
void List_Destroy(List *l);
UInt List_Length(List *l);
void List_Remove(List *l, void *item);
void List_Insert_With_Size(List *l, void *item_where,
 const void *item_what, UInt size);
Task List_Iter(List *l, ListIterator i, void *pass_data);
Task List_Item_Get(List *l, void **item, UInt index);
void List_Append_Strings(List *l, const char *strings, char separator);
Task List_Product_Iter(List *l, ListProduct product, void *pass);

#  define List_Insert(list, item_where, item_what) \
     List_Insert_With_Size((list), item_where, item_what, (list)->item_size)
#  define List_Append_With_Size(list, item, size) \
     List_Insert_With_Size((list), NULL, (item), size)
#  define List_Append(list, item) \
     List_Insert_With_Size((list), NULL, (item), (list)->item_size)
#  define  List_Append_String(list, s) \
     List_Insert_With_Size((list), NULL, (s), strlen(s)+1)

#endif
