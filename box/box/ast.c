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

#include "mem.h"
#include "ast.h"

/** Used for generic parsing of tree
 * (to destroy or print the tree, for example)
 */
int AstNode_Get_Subnodes(AstNode *node, AstNode **subnodes[AST_MAX_NUM_SUBNODES]) {
  switch(node->type) {
  case ASTNODETYPE_TYPENAME:
    subnodes[0] = & node->attr.typenm.scope;
    return 1;
  case ASTNODETYPE_BOX:
    subnodes[0] = & node->attr.box.type;
    subnodes[1] = & node->attr.box.first_statement;
    return 2;
  case ASTNODETYPE_STATEMENT:
    subnodes[0] = & node->attr.statement.expression;
    subnodes[1] = & node->attr.statement.next_statement;
    return 2;
  case ASTNODETYPE_CONST:
    return 0;
  case ASTNODETYPE_STRING:
    return 0;
  case ASTNODETYPE_VAR:
    subnodes[0] = & node->attr.var.scope;
    return 1;
  case ASTNODETYPE_UNOP:
    subnodes[0] = & node->attr.un_op.expr;
    return 1;
  case ASTNODETYPE_BINOP:
    subnodes[0] = & node->attr.bin_op.left;
    subnodes[1] = & node->attr.bin_op.right;
    return 2;
  case ASTNODETYPE_MEMBER:
    subnodes[0] = & node->attr.member.name;
    subnodes[1] = & node->attr.member.expr;
    subnodes[2] = & node->attr.member.next;
    return 3;
  case ASTNODETYPE_STRUC:
    subnodes[0] = & node->attr.struc.first_member;
    return 1;
  }
  assert(0); /* Should never happen! */
  return 0;
}

AstNode *AstNode_New(AstNodeType t) {
  AstNode *node,
          **subnode[AST_MAX_NUM_SUBNODES];
  int i, num_subnodes;

  node = BoxMem_Alloc(sizeof(AstNode));
  assert(node != NULL);

  node->type = t;
  node->finaliser = NULL;
  num_subnodes = AstNode_Get_Subnodes(node, subnode);
  assert(num_subnodes <= AST_MAX_NUM_SUBNODES);
  for(i = 0; i < num_subnodes; i++)
    *subnode[i] = NULL;
  return node;
}

void AstNode_Destroy(AstNode *node) {
  if (node == NULL)
    return;

  else {
    AstNode **subnode[AST_MAX_NUM_SUBNODES];
    int i, num_subnodes;

    /* First destroy children */
    num_subnodes = AstNode_Get_Subnodes(node, subnode);
    for(i = 0; i < num_subnodes; i++) {
      AstNode *child = *subnode[i];
      AstNode_Destroy(child);
    }

    if (node->finaliser != NULL)
      node->finaliser(node);

    BoxMem_Free(node); /* The free the structure */
  }
}

static void AstNodeTypeName_Finaliser(AstNode *node) {
  assert(node->type == ASTNODETYPE_TYPENAME);
  BoxMem_Free(node->attr.typenm.name);
}

AstNode *AstNodeTypeName_New(const char *name, size_t name_len) {
  AstNode *node = AstNode_New(ASTNODETYPE_TYPENAME);
  node->attr.typenm.name =
    (name_len > 0) ? BoxMem_Strndup(name, name_len) : BoxMem_Strdup(name);
  node->attr.typenm.scope = NULL;
  node->finaliser = AstNodeTypeName_Finaliser;
  return node;
}

AstNode *AstNodeBox_New(AstNode *type) {
  AstNode *node = AstNode_New(ASTNODETYPE_BOX);
  node->attr.box.first_statement = NULL;
  node->attr.box.last_statement = NULL;
  node->attr.box.type = type;
  return node;
}

void AstNodeBox_Add_Statement(AstNode *box, AstNode *statement) {
  assert(box->type == ASTNODETYPE_BOX
         && statement->type == ASTNODETYPE_STATEMENT);

  AstNode *last_statement = box->attr.box.last_statement;
  if (last_statement == NULL) {
    assert(box->attr.box.first_statement == NULL);
    box->attr.box.first_statement = box->attr.box.last_statement = statement;

  } else {
    AstNode *next_statement_after_last =
      last_statement->attr.statement.next_statement;
    next_statement_after_last = statement;
  }
}

AstNode *AstNodeConst_New(AstConstType t, AstConst c) {
  AstNode *node = AstNode_New(ASTNODETYPE_CONST);
  node->attr.constant.type = t;
  node->attr.constant.value = c;
  return node;
}

static void AstNodeString_Finaliser(AstNode *node) {
  assert(node->type == ASTNODETYPE_STRING);
  BoxMem_Free(node->attr.string.str);
}

AstNode *AstNodeString_New(const char *str, size_t str_len) {
  AstNode *node = AstNode_New(ASTNODETYPE_STRING);
  node->attr.var.name = (str_len > 0) ?
                        BoxMem_Strndup(str, str_len) : BoxMem_Strdup(str);
  node->finaliser = AstNodeString_Finaliser;
  return node;
}

static void AstNodeVar_Finaliser(AstNode *node) {
  assert(node->type == ASTNODETYPE_VAR);
  BoxMem_Free(node->attr.var.name);
}

AstNode *AstNodeVar_New(const char *name, size_t name_len) {
  AstNode *node = AstNode_New(ASTNODETYPE_VAR);
  node->attr.var.name = (name_len > 0) ?
                        BoxMem_Strndup(name, name_len) : BoxMem_Strdup(name);
  node->attr.var.scope = NULL;
  node->finaliser = AstNodeVar_Finaliser;
  return node;
}

AstNode *AstNodeUnOp_New(AstUnOp op, AstNode *expr) {
  AstNode *node = AstNode_New(ASTNODETYPE_UNOP);
  node->attr.un_op.operation = op;
  node->attr.un_op.expr = expr;
  return node;
}

AstNode *AstNodeBinOp_New(AstBinOp op, AstNode *left, AstNode *right) {
  AstNode *node = AstNode_New(ASTNODETYPE_BINOP);
  node->attr.bin_op.operation = op;
  node->attr.bin_op.left = left;
  node->attr.bin_op.right = right;
  return node;
}

AstNode *AstNodeMember_New(AstNode *name, AstNode *expr) {
  AstNode *node = AstNode_New(ASTNODETYPE_MEMBER);
  node->attr.member.name = name;
  node->attr.member.expr = expr;
  node->attr.member.next = NULL;
  return node;
}

AstNode *AstNodeStruc_New(void) {
  AstNode *node = AstNode_New(ASTNODETYPE_STRUC);
  node->attr.struc.first_member = NULL;
  node->attr.struc.last_member = NULL;
  return node;
}

