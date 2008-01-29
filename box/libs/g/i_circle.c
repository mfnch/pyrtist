#include <math.h>
#include <assert.h>

#include "types.h"
#include "virtmach.h"
#include "graphic.h"
#include "g.h"
#include "i_window.h"

Task circle_begin(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  return Success;
}

Task circle_end(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  return Success;
}

Task circle_point(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);

  return Success;
}

Task circle_real(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  return Success;
}

Task circle_pause(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  return Success;
}
