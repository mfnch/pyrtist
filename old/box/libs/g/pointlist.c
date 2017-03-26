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
#include <string.h>

#include "pointlist.h"

struct params_for_pointlist_print {
  BoxInt num_points;
  FILE *out;
};

static BoxTask _pointlist_print(BoxInt index, char *name,
                                void *object, void *data)
{
  struct params_for_pointlist_print *params =
   (struct params_for_pointlist_print *) data;
  BoxPoint *p = (BoxPoint *) object;
  if (name != (char *) NULL)
    fprintf(params->out, "\"%s\", ("SReal", "SReal")", name, p->x, p->y);
  else
    fprintf(params->out, "("SReal", "SReal")", p->x, p->y);
  if (index < params->num_points) fprintf(params->out, ", ");
  return BOXTASK_OK;
}

void pointlist_print(PointList *pl, FILE *out) {
  struct params_for_pointlist_print params;
  params.out = out;
  params.num_points = pointlist_num(pl);
  fprintf(out, "PointList[");
  (void) pointlist_iter(pl, _pointlist_print, & params);
  fprintf(out, "]");
}
