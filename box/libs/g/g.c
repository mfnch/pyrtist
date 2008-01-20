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
  printf("Window object allocated (%p)\n", w);
  printf("WindowPtr object: %p\n", wp);
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

Task window_save_str(VMProgram *vmp) {
  Subtype *s = BOX_VM_CURRENTPTR(vmp, Subtype);
  WindowPtr *wp = SUBTYPE_PARENT_PTR(s, WindowPtr);
  Window *w = *wp;
  char *file_name = BOX_VM_ARGPTR1(vmp, char);

  if (w->save_file_name != (char *) NULL) {
    printf("Window.Save: changing save target from '%s' to '%s'\n",
           w->save_file_name, file_name);
  }

  w->save_file_name = file_name;
  return Success;
}

Task window_save_end(VMProgram *vmp) {
  Subtype *s = BOX_VM_CURRENTPTR(vmp, Subtype);
  WindowPtr *wp = SUBTYPE_PARENT_PTR(s, WindowPtr);
  Window *w = *wp;

  if (w->save_file_name == (char *) NULL) {
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

