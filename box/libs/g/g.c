/* Example of Box external C-library */

#include <stdlib.h>
#include <stdio.h>

#include "types.h"
#include "virtmach.h"

typedef struct {
  Int width, height;
  char *save_file_name;
} Window;

typedef void *WindowPtr;

typedef struct {
  Int num_points;
  Point prev;
} Line;

Task window_begin(VMProgram *vmp) {
  WindowPtr *wp = BOX_VM_CURRENTPTR(vmp, WindowPtr);
  Window *w;
  w = *wp = (WindowPtr) malloc(sizeof(Window)); /* Should use Mem_Alloc, but this requires to link against the internal box library (which still does not exist) */
  if (w == (WindowPtr) NULL) return Failed;
  printf("Window object allocated\n");
  w->width = 0;
  w->height = 0;
  w->save_file_name = (char *) NULL;
  return Success;
}

Task window_destroy(VMProgram *vmp) {
  WindowPtr wp = BOX_VM_CURRENT(vmp, WindowPtr);
  (void) free(wp);
  printf("Window object deallocated\n");
  return Success;
}

Task window_save_end(VMProgram *vmp) {
  Window *w = BOX_VM_CURRENT(vmp, WindowPtr);
  if (1 || w->save_file_name == (char *) NULL) {
    printf("window not saved: need a file name!\n");
    return Failed;
  }
  printf("Saved window to '%s'\n", w->save_file_name);
  return Success;
}

Task line_begin(VMProgram *vmp) {
  Line *l = BOX_VM_CURRENTPTR(vmp, Line);
  l->num_points = 0;
  return Success;
}

Task line_point(VMProgram *vmp) {
  Line *l = BOX_VM_CURRENTPTR(vmp, Line);
  Point *p = BOX_VM_ARGPTR1(vmp, Point);
  if (l->num_points > 0) {
    printf("Line from (%f, %f) to (%f, %f)\n",
     l->prev.x, l->prev.y, p->x, p->y);
  }
  l->prev = *p;
  ++(l->num_points);
  return Success;
}

