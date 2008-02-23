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

/* $Id$ */

#include <stdlib.h>
#include <stdio.h>

#include "types.h"
#include "virtmach.h"
#include "g.h"
#include "pointlist.h"
#include "i_pointlist.h"

Task pointlist_begin(VMProgram *vmp) {
  PROC_OF_POINTLIST(vmp, ipl);
  ipl = *ipl_ptr = (IPointList *) malloc(sizeof(IPointList));
  ipl->name = (char *) NULL;
  return pointlist_init(& ipl->pl);
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

Task pointlist_get_begin(VMProgram *vmp) {
  PROC_OF_POINTLIST_SUBTYPE(vmp, ipl, get);

  return Success;
}

Task pointlist_get_end(VMProgram *vmp) {
  return Success;
}

Task pointlist_get_str(VMProgram *vmp) {
  return Success;
}

Task pointlist_get_int(VMProgram *vmp) {
  return Success;
}

Task pointlist_get_real(VMProgram *vmp) {
  return Success;
}
