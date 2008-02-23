
#ifndef _OBJLIST_H
#  define _OBJLIST_H

#  include <stdio.h>

#  include "types.h"
#  include "buffer.h"


typedef struct {
  char *name;
} ObjListItem;

typedef struct {
  buff ol;
} ObjList;

typedef Task (*ObjListIterator)(Int obj_idx, char *name,
                                void *obj, void *data);

Task objlist_init(ObjList *ol, Int obj_size);
void objlist_destroy(ObjList *pl);
Task objlist_clear(ObjList *ol);
void *objlist_find(ObjList *ol, char *name);
void *objlist_get(ObjList *ol, Int index);
Task objlist_add(ObjList *ol, void *obj, char *name);
Task objlist_iter(ObjList *ol, ObjListIterator it, void *data);

#endif

