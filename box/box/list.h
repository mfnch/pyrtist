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

/** @file list.h
 * @brief Implementation of the list type.
 */


#ifndef _BOX_LIST_H
#  define _BOX_LIST_H

/* Used in BoxList_Append_String */
#  include <string.h>

#  include <box/types.h>

typedef Task (*BoxListIterator)(void *item, void *pass_data);
typedef void (*BoxListDestruct)(void *item);

typedef struct __BoxListItemHead {
  struct __BoxListItemHead *previous;
  struct __BoxListItemHead *next;
} BoxListItemHead;

typedef struct {
  BoxUInt item_size;
  BoxUInt length;
  BoxListDestruct destructor;
  BoxListItemHead head_tail;
} BoxList;

/* Used with the function BoxList_Product_Iter */
typedef BoxTask (*BoxListProduct)(void **tuple, void *pass);

BOXEXPORT void BoxList_Init(BoxList *l, BoxUInt item_size);
BOXEXPORT void BoxList_Finish(BoxList *l);
BOXEXPORT BoxList *BoxList_New(BoxUInt item_size);
BOXEXPORT void BoxList_Destroy(BoxList *l);
BOXEXPORT BoxUInt BoxList_Length(BoxList *l);
BOXEXPORT void BoxList_Remove(BoxList *l, void *item);
BOXEXPORT void BoxList_Insert_With_Size(BoxList *l, void *item_where,
                                        const void *item_what, BoxUInt size);
BOXEXPORT BoxTask BoxList_Iter(BoxList *l, BoxListIterator i,
                               void *pass_data);
BOXEXPORT BoxTask BoxList_Item_Get(BoxList *l, void **item, BoxUInt index);
BOXEXPORT void BoxList_Append_Strings(BoxList *l, const char *strings,
                                      char separator);
BOXEXPORT BoxTask BoxList_Product_Iter(BoxList *l, BoxListProduct product,
                                       void *pass);

#  define BoxList_Insert(list, item_where, item_what) \
     BoxList_Insert_With_Size((list), item_where, item_what, (list)->item_size)
#  define BoxList_Append_With_Size(list, item, size) \
     BoxList_Insert_With_Size((list), NULL, (item), size)
#  define BoxList_Append(list, item) \
     BoxList_Insert_With_Size((list), NULL, (item), (list)->item_size)
#  define  BoxList_Append_String(list, s) \
     BoxList_Insert_With_Size((list), NULL, (s), strlen(s)+1)

#endif
