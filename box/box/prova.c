#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "types.h"
#include "defaults.h"
#include "messages.h"
#include "array.h"
#include "str.h"
#include "vm_private.h"
#include "registers.h"
#include "compiler.h"



/* Change the container used by the expression *e.
 */
Task Cmp_Expr_Container_Change(Expression *e, Container *c) {
}


int main(void) {
  Expression e;
  TASK( Cmp_Expr_Container_New(& e, CONTAINER_LREG(0)) );
  return 0;
}
