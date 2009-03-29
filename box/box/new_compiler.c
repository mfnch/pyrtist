/****************************************************************************
 * Copyright (C) 2009 by Matteo Franchin                                    *
 *                                                                          *
 * This file is part of Box.                                                *
 *                                                                          *
 *   Box is free software: you can redistribute it and/or modify it         *
 *   under the terms of the GNU Lesser General Public License as published  *
 *   by the Free Software Foundation, either version 3 of the License, or   *
 *   (at your option) any later version.                                    *
 *                                                                          *
 *   Box is distributed in the hope that it will be useful,                 *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Lesser General Public License for more details.                    *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with Box.  If not, see <http://www.gnu.org/licenses/>.   *
 ****************************************************************************/

#include <assert.h>

#include "types.h"
#include "mem.h"
#include "array.h"
#include "ast.h"
#include "expr.h"
#include "new_compiler.h"

void BoxCmp_Init(BoxCmp *c) {
  BoxArr_Init(& c->stack, 1, 1024);
}

void BoxCmp_Finish(BoxCmp *c) {
  BoxArr_Finish(& c->stack);
}

BoxCmp *BoxCmp_New(void) {
  BoxCmp *c = BoxMem_Alloc(sizeof(BoxCmp));
  if (c == NULL) return NULL;
  BoxCmp_Init(c);
  return c;
}

void BoxCmp_Destroy(BoxCmp *c) {
  BoxCmp_Finish(c);
  BoxMem_Free(c);
}

void BoxCmp_Push_Expr(BoxCmp *c, Expr *e) {


}

Expr *BoxCmp_Pop_Expr(BoxCmp *c) {
  return NULL;
}


void BoxCmp_Compile(BoxCmp *c, ASTNode *program) {
  ASTNode *s;

  if (program == NULL)
    return;

  assert(program->type == ASTNODETYPE_BOX);

  for(s = program->attr.box.first_statement;
      s != NULL;
      s = s->attr.statement.next_statement) {
    ASTNode *target;
    assert(s->type == ASTNODETYPE_STATEMENT);

  }
}

#if 0
void Compile_BinOp(BoxCmp *c, ASTNode *n) {
  Expr *left, *right;

  right = BoxCmp_Pop_Expr(c);
  left  = BoxCmp_Pop_Expr(c);
  if (Is_Error(right) || Is_Error(left)) {
    BoxCmp_Push_Error(c);
    return;
  }


  if (left->is.typed && right->is.typed) {
    Expr *result = Cmp_Operator_Exec(opr, left, right);
    if (result == NULL) return Failed;
    *rs = *result;
    return Success;

  } else {
    if ( opr->can_define ) {
      return Prs_Def_Operator(opr, rs, a, b);
    } else {
      MSG_ERROR("The expression should have type!");
      return Failed;
    }
  }

  Expr_Destroy(left);
  Expr_Destroy(right);
}
#endif

