/****************************************************************************
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

#ifndef _I_POINTLIST_H
#  define _I_POINTLIST_H

#  include "types.h"
#  include "buffer.h"
#  include "pointlist.h"

typedef struct {
  PointList pl;
  char *name;
} IPointList;

#  define PROC_OF_POINTLIST(vmp, ipl) \
    IPointList **ipl##_ptr = BOX_VM_CURRENTPTR(vmp, IPointList *), \
              *ipl = *ipl##_ptr

#  define PROC_OF_POINTLIST_SUBTYPE(vmp, ipl, subtype) \
    IPointList *ipl = *( (IPointList **) \
      SUBTYPE_PARENT_PTR(BOX_VM_CURRENTPTR(vmp, Subtype), IPointList *) )

/*
    IPointList *ipl = *( (IPointList **) \
      SUBTYPE_CHILD_PTR(BOX_VM_CURRENTPTR(vmp, Subtype), IPointList *) );
*/

#endif
