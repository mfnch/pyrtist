/***************************************************************************
 *   Copyright (C) 2006 by Matteo Franchin                                 *
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

/**
 * @file collection.h
 * @brief A collection is an array whose elements can be occupied or freed.
 *
 * This file defines the object Collection which is an extension
 * of the object Array.
 * This file adds to the Array object the possibility of releasing
 * an element. For example:
 *  you can insert three elements: 1, 2 and 3, then you can release
 *  the element number 2. Now when you insert another element, this will
 *  be put at the position occupied by 2: the most recently released item.
 */

/* collection.h,  June 2006 */

#ifndef _COLLECTION_H
#  define _COLLECTION_H

#  include "types.h"
#  include "array.h"

typedef Array Collection;

/** Creates a new Collection object.
 * @param new_clc the address of the new created object is returned here.
 * @param element_size size of the elements contained in the collection.
 *  All elements have the same size.
 * @param min_dim this is the default size of the collection:
 *  the dimension when it is empty, which will be increased when needed.
 */
Task Clc_New(Collection **new_clc, UInt element_size, UInt min_dim);

void Clc_Destructor(Collection *c, Task (*destroy)(void *));
void Clc_Destroy(Collection *c);
Task Clc_Occupy(Collection *c, void *item, UInt *assigned_index);
Task Clc_Release(Collection *c, UInt item_index);
Task Clc_Object_Ptr(Collection *c, void **item_ptr, UInt item_index);

#define Clc_MaxIndex(c) (c->max_idx)

#define Clc_ItemPtr(c, type, n) \
  ((type *) ((c)->ptr + ((n)-1)*((UInt) (c)->elsize) + sizeof(UInt)))
#define Clc_FirstItemPtr(c, type) \
  ((type *) ((c)->ptr + sizeof(UInt)))
#define Clc_LastItemPtr(c, type) \
  ((type *) ((c)->ptr + ((c)->numel-1)*((UInt) (c)->elsize) + sizeof(UInt)))
#endif
