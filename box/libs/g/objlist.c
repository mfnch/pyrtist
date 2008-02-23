#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "types.h"
#include "g.h"
#include "buffer.h"
#include "objlist.h"

Task objlist_init(ObjList *ol, Int obj_size) {
  Int tot_obj_size = sizeof(ObjListItem) + obj_size;
  assert(obj_size >= 0);
  if (buff_create(& ol->ol, tot_obj_size, 8))
    return Success;
  return Failed;
}

void objlist_destroy(ObjList *ol) {
  ObjListItem *oli = buff_firstitemptr(& ol->ol, ObjListItem);
  int i, n = buff_numitem(& ol->ol);
  for(i=0; i < n; i++) free((oli++)->name);
  buff_free(& ol->ol);
}

Task objlist_clear(ObjList *ol) {
  ObjListItem *oli = buff_firstitemptr(& ol->ol, ObjListItem);
  int i, n = buff_numitem(& ol->ol);
  for(i=0; i < n; i++) free((oli++)->name);
  if (buff_clear(& ol->ol)) return Success;
  return Failed;
}

void *objlist_find(ObjList *ol, char *name) {
  ObjListItem *oli = buff_firstitemptr(& ol->ol, ObjListItem);
  int i, n = buff_numitem(& ol->ol);
  if (name == (char *) NULL) return (void *) NULL;
  for(i=0; i < n; i++) {
    if (oli->name != (char *) NULL) {
      if (strcmp(oli->name, name) == 0)
        return (void *) oli + sizeof(ObjListItem);
    }
    ++oli;
  }
  return (void *) NULL;
}

void *objlist_get(ObjList *ol, Int index) {
  Int n = buff_numitem(& ol->ol);
  ObjListItem *oli;
  if (index < 1 || index > n) return (void *) NULL;
  oli = buff_itemptr(& ol->ol, ObjListItem, index);
  return (void *) oli + sizeof(ObjListItem);
}

Task objlist_add(ObjList *ol, void *obj, char *name) {
  ObjListItem *oli;
  void *dest_obj;

  if (!buff_bigenough(& ol->ol, ol->ol.numel+1)) return Failed;
  ++ol->ol.numel;
  oli = buff_lastitemptr(& ol->ol, ObjListItem);
  dest_obj = (void *) oli + sizeof(ObjListItem);
  (void) memcpy(dest_obj, obj, ol->ol.elsize);

  if (name == (char *) NULL) {
    oli->name = (char *) NULL;

  } else {
    if (objlist_find(ol, name) != (void *) NULL) {
      g_error("Another object with the same name exists!");
      return Failed;
    }

    oli->name = strdup(name);
    if (oli->name == (char *) NULL) {
      g_error("pointlist_add: strdup failed!");
      return Failed;
    }
  }

  return Success;
}

Task objlist_iter(ObjList *ol, ObjListIterator it, void *data) {
  ObjListItem *oli = buff_firstitemptr(& ol->ol, ObjListItem);
  int i, n = buff_numitem(& ol->ol);
  for(i=1; i <= n; i++) {
    void *obj = (void *) oli + sizeof(ObjListItem);
    if IS_FAILED( it(i, oli->name, obj, data) )
      return Failed;
    ++oli;
  }
  return Success;
}
