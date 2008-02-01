#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "g.h"
#include "buffer.h"
#include "pointlist.h"

Task point_list_init(PointList *pl) {
  if (buff_create(& pl->pl, sizeof(PointList), 8))
    return Success;
  return Failed;
}

void point_list_destroy(PointList *pl) {
  PointListItem *pli = buff_firstitemptr(& pl->pl, PointListItem);
  int i, n = buff_numitem(& pl->pl);
  for(i=0; i < n; i++) free((pli++)->name);
  buff_free(& pl->pl);
}

Point *point_list_find(PointList *pl, char *name) {
  PointListItem *pli = buff_firstitemptr(& pl->pl, PointListItem);
  int i, n = buff_numitem(& pl->pl);
  for(i=0; i < n; i++) {
    if (strcmp(pli->name, name) == 0)
       return & pli->point;
    ++pli;
  }
  return (Point *) NULL;
}

Task point_list_add(PointList *pl, Point *p, char *name) {
  PointListItem pli;
  if (point_list_find(pl, name) != (Point *) NULL) {
    g_error("Another point with the same name exists!");
    return Failed;
  }

  pli.point = *p;
  if (name == (char *) NULL) {
    pli.name = (char *) NULL;

  } else {
    pli.name = strdup(name);
    if (pli.name == (char *) NULL) {
      g_error("point_list_add: strcpy failed!");
      return Failed;
    }
  }

  if (buff_push(& pl->pl, & pli)) return Success;
  return Failed;
}

Task point_list_clear(PointList *pl) {
  if (buff_clear(& pl->pl)) return Success;
  return Failed;
}
