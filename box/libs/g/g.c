/* Example of Box external C-library */

#include <stdlib.h>
#include <stdio.h>

#include "types.h"
#include "virtmach.h"
#include "g.h"

void g_error(const char *msg) {
  printf("Error: %s\n", msg);
}

Task g_optcolor_set(OptColor *oc, Color *c) {
  if (c->r < -0.5) {
    if (oc->alternative == (OptColor *) NULL) {
      g_error("Cannot unset the color.");
      return Failed;
    }
    oc->selected = oc->alternative;
    return Success;

  } else {
    oc->selected = oc;
    oc->local = *c;
    return Success;
  }
}

Task g_optcolor_set_rgb(OptColor *oc, Real r, Real g, Real b) {
  Color c;
  c.r = r; c.g = g; c.b = b;
  return g_optcolor_set(oc, & c);
}

Color *g_optcolor_get(OptColor *oc) {
  if (oc == (OptColor *) NULL || oc->selected == oc)
    return & oc->local;
  else
    return g_optcolor_get(oc->selected);
}

void g_optcolor_alternative_set(OptColor *oc, OptColor *alternative) {
  oc->alternative = alternative;
}
