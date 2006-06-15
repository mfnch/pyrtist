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

/* collection.h,  June 2006 */

#ifndef _COLLECTION_H
#define _COLLECTION_H

typedef Array Collection;

Task Clc_New(Collection **new_clc, UInt element_size, UInt min_dim);
void Clc_Destructor(Collection *c, Task (*destroy)(void *));
void Clc_Destroy(Collection *c);
Task Clc_Occupy(Collection *c, void *item, int *assigned_index);
Task Clc_Release(Collection *c, UInt item_index);
Task Clc_Object_Ptr(Collection *c, void **item_ptr, UInt item_index);

#define Clc_MaxIndex(c) (c->max_idx)

#define Clc_ItemPtr(c, type, n) \
  ((type *) ((c)->ptr + ((n)-1)*((UInt) (c)->elsize) + sizeof(int)))
#define Clc_FirstItemPtr(c, type) \
  ((type *) ((c)->ptr + sizeof(int)))
#define Clc_LastItemPtr(c, type) \
  ((type *) ((c)->ptr + ((c)->numel-1)*((UInt) (c)->elsize) + sizeof(int)))
#endif
