#include <math.h>
#include <assert.h>

#include "types.h"
#include "virtmach.h"
#include "graphic.h"
#include "g.h"
#include "i_window.h"

Task poly_window_init(Window *w) {
  g_optcolor_alternative_set(& w->poly.color, & w->fg_color);
  g_optcolor_unset(& w->poly.color);
  return Success;
}

Task poly_color(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  Color *c = BOX_VM_ARGPTR1(vmp, Color);
  return g_optcolor_set(& w->poly.color, c);
}
