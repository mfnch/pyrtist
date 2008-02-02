
#ifndef _POINTLIST_H
#  define _POINTLIST_H

#  include <stdio.h>

#  include "types.h"
#  include "buffer.h"

typedef struct {
  Point point;
  char *name;
} PointListItem;

typedef struct {
  buff pl;
} PointList;

Task pointlist_init(PointList *pl);
void pointlist_destroy(PointList *pl);
Point *pointlist_find(PointList *pl, char *name);
Task pointlist_add(PointList *pl, Point *p, char *name);
Task pointlist_clear(PointList *pl);
void pointlist_fprint(PointList *pl, FILE *out);

#endif

