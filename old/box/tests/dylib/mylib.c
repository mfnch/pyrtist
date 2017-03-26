/* Example of Box external C-library */

#include <stdlib.h>
#include <stdio.h>

#include <box/types.h>
#include <box/vm_priv.h>
#include <box/str.h>

BoxTask mylib_simple(BoxVMX *vm) {
  printf("simple");
  return BOXTASK_OK;
}

BoxTask mylib_print_str_a(BoxVMX *vm) {
  BoxStr *s = BOX_VM_ARG1_PTR(vm, BoxStr);
  printf("A:%s", s->ptr);
  return BOXTASK_OK;
}

BoxTask mylib_print_str_b(BoxVMX *vm) {
  BoxStr *s = BOX_VM_ARG1_PTR(vm, BoxStr);
  printf("B:%s", s->ptr);
  return BOXTASK_OK;
}
