
#ifndef _POINTLIST_H
#  define _POINTLIST_H

typedef struct {
  Point point;
  char *name;
} PointListItem;

typedef struct {
  buff pl;
} PointList;

Task point_list_init(PointList *pl);
void point_list_destroy(PointList *pl);
Point *point_list_find(PointList *pl, char *name);
Task point_list_add(PointList *pl, Point *p, char *name);
Task point_list_clear(PointList *pl);

#endif
