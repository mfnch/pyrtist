/* Example of Box external C-library */

#include <stdlib.h>
#include <stdio.h>

#include "types.h"
#include "virtmach.h"
#include "g.h"

void g_error(const char *msg) {
  printf("Error: %s\n", msg);
}
