/* Example of Box external C-library */

#include <stdlib.h>
#include <stdio.h>

#include <box/types.h>
#include <box/virtmach.h>
#include <box/bltinstr.h>

Task mylib_print_str(BoxVM *vm) {
  BoxStr *s = BOX_VM_ARGPTR1(vm, BoxStr);
  printf("%s", s->ptr);
  return Success;
}

