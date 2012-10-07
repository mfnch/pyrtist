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

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "types.h"
#include <box/vm_priv.h>
#include "str.h"
#include "g.h"
#include "pointlist.h"
#include "i_pointlist.h"

Task ipl_create(IPointListPtr *ipl_ptr) {
  IPointList *ipl;
  ipl = *ipl_ptr = (IPointList *) malloc(sizeof(IPointList));
  ipl->name = (char *) NULL;
  return pointlist_init(& ipl->pl);
}

Task pointlist_begin(BoxVMX *vmp) {
  IPointListPtr *ipl_ptr = BOX_VM_THIS_PTR(vmp, IPointListPtr);
  return ipl_create(ipl_ptr);
}

Task pointlist_end(BoxVMX *vmp) {
  IPointList *ipl = BOX_VM_THIS(vmp, IPointListPtr);
  if (ipl->name != (char *) NULL) {
    g_warning("You gave a name, but not the corresponding point.");
    free(ipl->name);
    ipl->name = (char *) NULL;
  }
  return BOXTASK_OK;
}

Task ipointlist_destroy(BoxVMX *vmp) {
  IPointList *ipl = BOX_VM_THIS(vmp, IPointListPtr);
  pointlist_destroy(& ipl->pl);
  free(ipl->name);
  ipl->name = (char *) NULL;
  return BOXTASK_OK;
}

Task pointlist_str(BoxVMX *vm) {
  BoxStr *s = BOX_VM_ARG_PTR(vm, BoxStr);
  IPointList *ipl = BOX_VM_THIS(vm, IPointListPtr);
  ipl->name = strdup((char *) s->ptr);
  return BOXTASK_OK;
}

Task pointlist_point(BoxVMX *vmp) {
  IPointList *ipl = BOX_VM_THIS(vmp, IPointListPtr);
  BoxPoint *p = BOX_VM_ARG1_PTR(vmp, BoxPoint);
  Task t = pointlist_add(& ipl->pl, p, ipl->name);
  free(ipl->name);
  ipl->name = (char *) NULL;
  return t;
}

static BoxTask
_add_from_pointlist(BoxInt index, char *name, void *object, void *data)
{
  PointList *dest_pl = (PointList *) data;
  BoxPoint *p = (BoxPoint *) object;
  return pointlist_add(dest_pl, p, name);
}

Task pointlist_pointlist(BoxVMX *vmp) {
  IPointList *ipl = BOX_VM_THIS(vmp, IPointListPtr);
  IPointList *ipl_to_add = BOX_VM_ARG1(vmp, IPointList *);
  return pointlist_iter(& ipl_to_add->pl, _add_from_pointlist, & ipl->pl);
}

Task pointlist_get_str(BoxVMX *vm) {
  IPointList *ipl = BOX_VM_SUB_PARENT(vm, IPointListPtr);
  BoxPoint *out_p = BOX_VM_SUB_CHILD_PTR(vm, BoxPoint);
  BoxStr *s = BOX_VM_ARG_PTR(vm, BoxStr);
  BoxPoint *p = pointlist_find(& ipl->pl, (char *) s->ptr);
  if (p == NULL) {
    g_error("The name you gave is not a name of a point in the PointList.");
    return BOXTASK_FAILURE;
  }
  *out_p = *p;
  return BOXTASK_OK;
}

Task pointlist_get_int(BoxVMX *vmp) {
  IPointList *ipl = BOX_VM_SUB_PARENT(vmp, IPointListPtr);
  BoxPoint *out_p = BOX_VM_SUB_CHILD_PTR(vmp, BoxPoint);
  BoxPoint *p = pointlist_get(& ipl->pl, BOX_VM_ARG1(vmp, BoxInt));
  if (p == (BoxPoint *) NULL) {
    g_error("Wrong index in PointList.Get");
    return BOXTASK_FAILURE;
  }
  *out_p = *p;
  return BOXTASK_OK;
}

static Task _get_from_point(BoxPoint *out_p, PointList *pl,
                            BoxReal index_x, BoxReal index_y) {
  BoxInt index_a = (BoxInt) index_x,
         index_b = index_a + (index_a < 0 ? -1 : 1);
  BoxReal d_a = fabs(index_x - index_a);
  BoxPoint *p_a, *p_b, d_p;
  p_a = pointlist_get(pl, index_a);
  p_b = pointlist_get(pl, index_b);
  if (p_a == NULL || p_b == NULL) {
    g_error("Wrong index in PointList.Get, shouldn't happen!");
    return BOXTASK_FAILURE;
  }
  d_p.x = p_b->x - p_a->x;
  d_p.y = p_b->y - p_a->y;
  out_p->x = p_a->x + d_p.x*d_a - d_p.y*index_y;
  out_p->y = p_a->y + d_p.y*d_a + d_p.x*index_y;
  return BOXTASK_OK;

}

Task pointlist_get_real(BoxVMX *vmp) {
  IPointList *ipl = BOX_VM_SUB_PARENT(vmp, IPointListPtr);
  BoxPoint *out_p = BOX_VM_SUB_CHILD_PTR(vmp, BoxPoint);
  BoxReal real_index = BOX_VM_ARG1(vmp, BoxReal);
  return _get_from_point(out_p, & ipl->pl, real_index, 0.0);
}

Task pointlist_get_point(BoxVMX *vmp) {
  IPointList *ipl = BOX_VM_SUB_PARENT(vmp, IPointListPtr);
  BoxPoint *out_p = BOX_VM_SUB_CHILD_PTR(vmp, BoxPoint);
  BoxPoint *point_index = BOX_VM_ARG1_PTR(vmp, BoxPoint);
  return _get_from_point(out_p, & ipl->pl, point_index->x, point_index->y);
}

Task pointlist_num_begin(BoxVMX *vmp) {
  IPointList *ipl = BOX_VM_SUB_PARENT(vmp, IPointListPtr);
  BoxInt *out_num = BOX_VM_SUB_CHILD_PTR(vmp, BoxInt);
  *out_num = pointlist_num(& ipl->pl);
  return BOXTASK_OK;
}

Task print_pointlist(BoxVMX *vmp) {
  IPointList *ipl_to_print = BOX_VM_ARG1(vmp, IPointListPtr);
  pointlist_print(IPL_POINTLIST(ipl_to_print), stdout);
  return BOXTASK_OK;
}
