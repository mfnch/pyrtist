
#ifndef _POINTLIST_H
#  define _POINTLIST_H

#  include "objlist.h"

#  define PointList ObjList

#  define pointlist_init(pl) (objlist_init((pl), sizeof(Point)))
#  define pointlist_destroy(pl) (objlist_destroy((pl)))
#  define pointlist_find(pl, name) ((Point *) objlist_find((pl), (name)))
#  define pointlist_get(pl, index) ((Point *) objlist_get((pl), (index)))
#  define pointlist_add(pl, p, name) (objlist_add((pl), (p), name))

#endif

