/* Example of Box external C-library */

#include <stdlib.h>
#include <stdio.h>

#include <box/types.h>
#include <box/vm_private.h>
#include <box/str.h>

Task mylib_simple(BoxVM *vm) {
  printf("simple");
  return Success;
}

Task mylib_print_str_a(BoxVM *vm) {
  BoxStr *s = BOX_VM_ARG1_PTR(vm, BoxStr);
  printf("A:%s", s->ptr);
  return Success;
}

Task mylib_print_str_b(BoxVM *vm) {
  BoxStr *s = BOX_VM_ARG1_PTR(vm, BoxStr);
  printf("B:%s", s->ptr);
  return Success;
}
