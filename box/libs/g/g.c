/* Example of Box external C-library */

#include <stdlib.h>
#include <stdio.h>

#include "types.h"
#include "virtmach.h"

typedef struct {
  Int num_points;
  Point prev;
} Line;

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

