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
#include <stdio.h>

#include "mem.h"
#include "ast.h"

/** Used for generic parsing of tree
 * (to destroy or print the tree, for example)
 */
int AstNode_Get_Subnodes(AstNode *node, AstNode **subnodes[AST_MAX_NUM_SUBNODES]) {
  switch(node->type) {
  case ASTNODETYPE_ERROR:
    return 0;
  case ASTNODETYPE_TYPENAME:
    subnodes[0] = & node->attr.typenm.scope;
    return 1;
  case ASTNODETYPE_SUBTYPE:
    subnodes[0] = & node->attr.subtype.scope;
    return 1;
  case ASTNODETYPE_BOX:
    subnodes[0] = & node->attr.box.parent;
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
  case ASTNODETYPE_ARRAYGET:
    subnodes[0] = & node->attr.array_get.array;
    subnodes[1] = & node->attr.array_get.index;
    return 2;
  case ASTNODETYPE_MEMBERGET:
    subnodes[0] = & node->attr.member_get.struc;
    return 1;
  }
  assert(0); /* Should never happen! */
  return 0;
}

const char *AstNodeType_To_Str(AstNodeType t) {
  switch(t) {
  case ASTNODETYPE_ERROR:     return "Error";
  case ASTNODETYPE_TYPENAME:  return "TypeName";
  case ASTNODETYPE_SUBTYPE:   return "SubType";
  case ASTNODETYPE_BOX:       return "Box";
  case ASTNODETYPE_STATEMENT: return "Statement";
  case ASTNODETYPE_CONST:     return "Const";
  case ASTNODETYPE_STRING:    return "String";
  case ASTNODETYPE_VAR:       return "Var";
  case ASTNODETYPE_UNOP:      return "UnOp";
  case ASTNODETYPE_BINOP:     return "BinOp";
  case ASTNODETYPE_MEMBER:    return "Member";
  case ASTNODETYPE_STRUC:     return "Struc";
  case ASTNODETYPE_ARRAYGET:  return "ArrayGet";
  case ASTNODETYPE_MEMBERGET: return "MemberGet";
  default:                    return "UnknownNode";
  }
  return "???";
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

void AstNode_Set_Error(AstNode *node) {
}

/* Used to draw a tree using the characters | \ and - */
typedef struct __IndentStr {
  const char *this;
  struct __IndentStr *next;
} IndentStr;

static void Indent_Push(IndentStr *indent, IndentStr *next) {
  for(; indent->next != NULL; indent = indent->next);
  indent->next = next;
  next->next = NULL;
}

static void Indent_Pop(IndentStr *indent) {
  if (indent->next != NULL) {
    IndentStr *next = indent->next;
    for(; next->next != NULL;) {
      indent = next;
      next = next->next;
    }
    indent->next = NULL;
  }
}

static const char *branch_sep  = "--";
static const char *branch_down = " |";
static const char *branch_last = " \\";
static const char *branch_end  = "  ";

static void Indent_Print(FILE *out, IndentStr *indent) {
  for(; indent != NULL; indent = indent->next) {
    fprintf(out, "%s", indent->this);
    if (indent->this == branch_last)
      indent->this = branch_end;
  }
}

static void My_Node_Print(FILE *out, AstNode *node, IndentStr *indent) {
  AstNode **subnode[AST_MAX_NUM_SUBNODES];
  int i, num_subnodes;

  if (node == NULL) {
    Indent_Print(out, indent);
    fprintf(out, "%sEMPTY NODE\n", branch_sep);
    return;
  }

  /* Get subnodes */
  num_subnodes = AstNode_Get_Subnodes(node, subnode);

  Indent_Print(out, indent);
  fprintf(out, "%s%s(num_subnodes=%d)\n",
          branch_sep, AstNodeType_To_Str(node->type), num_subnodes);

  if (num_subnodes > 0) {
    IndentStr new_indent;
    new_indent.this = branch_down;
    Indent_Push(indent, & new_indent);

    for(i = 0; i < num_subnodes; i++) {
      AstNode *child = *subnode[i];
      if (i == num_subnodes - 1)
        new_indent.this = branch_last;
      My_Node_Print(out, child, indent);
    }

    Indent_Pop(indent);
  }
}

void AstNode_Print(FILE *out, AstNode *node) {
  IndentStr indent;
  indent.this = "";
  indent.next = NULL;
  My_Node_Print(out, node, & indent);
}

AstNode *AstNodeError_New(void) {
  return AstNode_New(ASTNODETYPE_ERROR);
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

static void AstNodeSubtype_Finaliser(AstNode *node) {
  assert(node->type == ASTNODETYPE_SUBTYPE);
  BoxMem_Free(node->attr.subtype.name);
}

AstNode *AstNodeSubtype_New(const char *name, size_t name_len) {
  AstNode *node = AstNode_New(ASTNODETYPE_SUBTYPE);
  node->attr.subtype.name =
    (name_len > 0) ? BoxMem_Strndup(name, name_len) : BoxMem_Strdup(name);
  node->attr.subtype.scope = NULL;
  node->finaliser = AstNodeSubtype_Finaliser;
  return node;
}

AstNode *AstNodeStatement_New(AstNode *expr) {
  AstNode *node = AstNode_New(ASTNODETYPE_STATEMENT);
  node->attr.statement.expression = expr;
  return node;
}

AstNode *AstNodeBox_New(AstNode *parent, AstNode *first_statement) {
  AstNode *node = AstNode_New(ASTNODETYPE_BOX);
  node->attr.box.first_statement = first_statement;
  node->attr.box.last_statement = first_statement;
  node->attr.box.parent = parent;
  return node;
}

AstNode *AstNodeBox_Add_Statement(AstNode *box, AstNode *statement) {
  assert(box->type == ASTNODETYPE_BOX);

  if (statement == NULL) {
    return box;

  } else {
    AstNode *last_statement = box->attr.box.last_statement;

    assert(statement->type == ASTNODETYPE_STATEMENT);

    if (last_statement == NULL) {
      assert(box->attr.box.first_statement == NULL);
      box->attr.box.first_statement = box->attr.box.last_statement = statement;

    } else {
      last_statement->attr.statement.next_statement = statement;
      box->attr.box.last_statement = statement;
    }
    return box;
  }
}

AstNode *AstNodeBox_Set_Parent(AstNode *box, AstNode *parent) {
  assert(box->type == ASTNODETYPE_BOX);
  box->attr.box.parent = parent;
  return box;
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
  node->attr.string.str = BoxMem_Strndup(str, str_len);
  node->finaliser = AstNodeString_Finaliser;
  return node;
}

AstNode *AstNodeString_Concat(AstNode *str1, AstNode *str2) {
  assert(str1->type == ASTNODETYPE_STRING &&
         str2->type == ASTNODETYPE_STRING);
  char *old_str = str1->attr.string.str;
  str1->attr.string.str = BoxMem_Str_Merge(old_str, str2->attr.string.str);
  BoxMem_Free(old_str);
  AstNode_Destroy(str2);
  return str1;
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

AstNode *AstNodeStruc_New(AstNode *first_name, AstNode *first_expr) {
  AstNode *first_member = NULL, *node;
  assert(!(first_name != NULL && first_expr == NULL));
  if (first_expr != NULL)
    first_member = AstNodeMember_New(first_name, first_expr);

  node = AstNode_New(ASTNODETYPE_STRUC);
  node->attr.struc.first_member = first_member;
  node->attr.struc.last_member = first_member;
  return node;
}

AstNode *AstNodeStruc_Add_Member(AstNode *struc,
                                 AstNode *this_name, AstNode *this_expr) {
  AstNode *this_member = NULL;
  assert(struc->type == ASTNODETYPE_STRUC);
  assert(!(this_name != NULL && this_expr == NULL));
  if (this_expr == NULL) return struc;

  this_member = AstNodeMember_New(this_name, this_expr);
  if (struc->attr.struc.last_member == NULL) {
    assert(struc->attr.struc.first_member == NULL);
    struc->attr.struc.first_member = this_member;
    struc->attr.struc.last_member = this_member;
    return struc;

  } else {
    AstNode *last_member = struc->attr.struc.last_member;
    last_member->attr.member.next = this_member;
    struc->attr.struc.last_member = this_member;
  }
  return struc;
}

AstNode *AstNodeArrayGet_New(AstNode *array, AstNode *index) {
  AstNode *node = AstNode_New(ASTNODETYPE_ARRAYGET);
  node->attr.array_get.array = array;
  node->attr.array_get.index = index;
  return node;
}

static void AstNodeMemberGet_Finaliser(AstNode *node) {
  assert(node->type == ASTNODETYPE_MEMBERGET);
  BoxMem_Free(node->attr.member_get.member);
}

AstNode *AstNodeMemberGet_New(AstNode *struc,
                              const char *member, int member_len) {
  AstNode *node = AstNode_New(ASTNODETYPE_MEMBERGET);
  node->attr.member_get.struc = struc;
  node->attr.member_get.member = (member_len > 0) ?
                                  BoxMem_Strndup(member, member_len) :
                                  BoxMem_Strdup(member);
  node->finaliser = AstNodeMemberGet_Finaliser;
  return node;
}
