/* Example of Box external C-library */

#include <stdlib.h>
#include <stdio.h>

#include "types.h"
#include "virtmach.h"

Task mylib_print_str(VMProgram *vmp) {
  printf("%s", BOX_VM_ARGPTR1(vmp, char));
  return Success;
}
