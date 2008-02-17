#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "types.h"
#include "g.h"
#include "buffer.h"
#include "pointlist.h"

Task pointlist_init(PointList *pl) {
  if (buff_create(& pl->pl, sizeof(PointListItem), 8))
    return Success;
  return Failed;
}

void pointlist_destroy(PointList *pl) {
  PointListItem *pli = buff_firstitemptr(& pl->pl, PointListItem);
  int i, n = buff_numitem(& pl->pl);
  for(i=0; i < n; i++) free((pli++)->name);
  buff_free(& pl->pl);
}

Point *pointlist_find(PointList *pl, char *name) {
  PointListItem *pli = buff_firstitemptr(& pl->pl, PointListItem);
  int i, n = buff_numitem(& pl->pl);
  if (name == (char *) NULL) return (Point *) NULL;
  for(i=0; i < n; i++) {
    if (pli->name != (char *) NULL) {
      if (strcmp(pli->name, name) == 0)
         return & pli->point;
    }
    ++pli;
  }
  return (Point *) NULL;
}

Point *pointlist_get(PointList *pl, Int index) {
  Int n = buff_numitem(& pl->pl);
  PointListItem *pli;
  if (index < 1 || index > n) return (Point *) NULL;
  pli = buff_itemptr(& pl->pl, PointListItem, index);
  return & pli->point;
}

Task pointlist_add(PointList *pl, Point *p, char *name) {
  PointListItem pli;

  pli.point = *p;
  if (name == (char *) NULL) {
    pli.name = (char *) NULL;

  } else {
    if (pointlist_find(pl, name) != (Point *) NULL) {
      g_error("Another point with the same name exists!");
      return Failed;
    }

    pli.name = strdup(name);
    if (pli.name == (char *) NULL) {
      g_error("pointlist_add: strcpy failed!");
      return Failed;
    }
  }

  if (buff_push(& pl->pl, & pli)) return Success;
  return Failed;
}

Task pointlist_clear(PointList *pl) {
  if (buff_clear(& pl->pl)) return Success;
  return Failed;
}

void pointlist_fprint(PointList *pl, FILE *out) {
  PointListItem *pli = buff_firstitemptr(& pl->pl, PointListItem);
  int i, n = buff_numitem(& pl->pl);
  for(i=0; i < n; i++) {
    if (pli->name == (char *) NULL) {
      fprintf(out, "%d: (%g, %g)\n",
             i, pli->point.x, pli->point.y);
    } else {
      fprintf(out, "%d: (%g, %g) <-- '%s'\n",
              i, pli->point.x, pli->point.y, pli->name);
    }
    ++pli;
  }
}

