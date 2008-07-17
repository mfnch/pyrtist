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

/* $Id$ */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "types.h"
#include "virtmach.h"
#include "g.h"
#include "pointlist.h"
#include "i_pointlist.h"

Task ipl_create(IPointListPtr *ipl_ptr) {
  IPointList *ipl;
  ipl = *ipl_ptr = (IPointList *) malloc(sizeof(IPointList));
  ipl->name = (char *) NULL;
  return pointlist_init(& ipl->pl);
}

Task pointlist_begin(VMProgram *vmp) {
  IPointListPtr *ipl_ptr = BOX_VM_THIS_PTR(vmp, IPointListPtr);
  return ipl_create(ipl_ptr);
}

Task pointlist_end(VMProgram *vmp) {
  PROC_OF_POINTLIST(vmp, ipl);
  if (ipl->name != (char *) NULL) {
    g_warning("You gave a name, but not the corresponding point.");
    free(ipl->name);
    ipl->name = (char *) NULL;
  }
  return Success;
}

Task ipointlist_destroy(VMProgram *vmp) {
  PROC_OF_POINTLIST(vmp, ipl);
  pointlist_destroy(& ipl->pl);
  free(ipl->name);
  ipl->name = (char *) NULL;
  return Success;
}

Task pointlist_str(VMProgram *vmp) {
  PROC_OF_POINTLIST(vmp, ipl);
  ipl->name = strdup(BOX_VM_ARGPTR1(vmp, char));
  return Success;
}

Task pointlist_point(VMProgram *vmp) {
  PROC_OF_POINTLIST(vmp, ipl);
  Point *p = BOX_VM_ARGPTR1(vmp, Point);
  Task t = pointlist_add(& ipl->pl, p, ipl->name);
  free(ipl->name);
  ipl->name = (char *) NULL;
  return t;
}

static Task _add_from_pointlist(Int index, char *name,
                                void *object, void *data)
{
  PointList *dest_pl = (PointList *) data;
  Point *p = (Point *) object;
  return pointlist_add(dest_pl, p, name);
}

Task pointlist_pointlist(VMProgram *vmp) {
  PROC_OF_POINTLIST(vmp, ipl);
  IPointList *ipl_to_add = BOX_VM_ARG1(vmp, IPointList *);
  return pointlist_iter(& ipl_to_add->pl, _add_from_pointlist, & ipl->pl);
}

Task pointlist_get_str(VMProgram *vmp) {
  PROC_OF_POINTLIST_SUBTYPE(vmp, ipl, out_p, Point);
  Point *p = pointlist_find(& ipl->pl, BOX_VM_ARGPTR1(vmp, char));
  if (p == (Point *) NULL) {
    g_error("The name you gave is not a name of a point in the PointList.");
    return Failed;
  }
  *out_p = *p;
  return Success;
}

Task pointlist_get_int(VMProgram *vmp) {
  PROC_OF_POINTLIST_SUBTYPE(vmp, ipl, out_p, Point);
  Point *p = pointlist_get(& ipl->pl, BOX_VM_ARG1(vmp, Int));
  if (p == (Point *) NULL) {
    g_error("Wrong index in PointList.Get");
    return Failed;
  }
  *out_p = *p;
  return Success;
}

static Task _get_from_point(Point *out_p, PointList *pl,
                            Real index_x, Real index_y) {
  Int index_a = (Int) index_x,
      index_b = index_a + (index_a < 0 ? -1 : 1);
  Real d_a = fabs(index_x - index_a);
  Point *p_a, *p_b, d_p;
  p_a = pointlist_get(pl, index_a);
  p_b = pointlist_get(pl, index_b);
  if (p_a == (Point *) NULL || p_b == (Point *) NULL) {
    g_error("Wrong index in PointList.Get, shouldn't happen!");
    return Failed;
  }
  d_p.x = p_b->x - p_a->x;
  d_p.y = p_b->y - p_a->y;
  out_p->x = p_a->x + d_p.x*d_a - d_p.y*index_y;
  out_p->y = p_a->y + d_p.y*d_a + d_p.x*index_y;
  return Success;

}

Task pointlist_get_real(VMProgram *vmp) {
  PROC_OF_POINTLIST_SUBTYPE(vmp, ipl, out_p, Point);
  Real real_index = BOX_VM_ARG1(vmp, Real);
  return _get_from_point(out_p, & ipl->pl, real_index, 0.0);
}

Task pointlist_get_point(VMProgram *vmp) {
  PROC_OF_POINTLIST_SUBTYPE(vmp, ipl, out_p, Point);
  Point *point_index = BOX_VM_ARG1_PTR(vmp, Point);
  return _get_from_point(out_p, & ipl->pl, point_index->x, point_index->y);
}

Task pointlist_num_begin(VMProgram *vmp) {
  PROC_OF_POINTLIST_SUBTYPE(vmp, ipl, out_num, Int);
  *out_num = pointlist_num(& ipl->pl);
  return Success;
}

Task print_pointlist(VMProgram *vmp) {
  IPointList *ipl_to_print = BOX_VM_ARG1(vmp, IPointListPtr);
  pointlist_print(IPL_POINTLIST(ipl_to_print), stdout);
  return Success;
}
