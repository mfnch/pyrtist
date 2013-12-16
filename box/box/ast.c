/****************************************************************************
 * Copyright (C) 2009-2013 by Matteo Franchin                               *
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
#include <string.h>

#include <box/mem.h>
#include <box/print.h>
#include <box/srcpos.h>

#include <box/ast_priv.h>


/**
 * Used for generic parsing of tree
 * (to destroy or print the tree, for example)
 */
int ASTNode_Get_Subnodes(ASTNode *node,
                         ASTNode **subnodes[AST_MAX_NUM_SUBNODES])
{
  switch(node->type) {
  case ASTNODETYPE_ERROR:
    return 0;
  case ASTNODETYPE_TYPENAME:
    subnodes[0] = & node->attr.typenm.scope;
    return 1;
  case ASTNODETYPE_TYPETAG:
    return 0;
  case ASTNODETYPE_SUBTYPE:
    subnodes[0] = & node->attr.subtype.parent;
    return 1;
  case ASTNODETYPE_INSTANCE:
    subnodes[0] = & node->attr.instance.type;
    return 1;
  case ASTNODETYPE_BOX:
    subnodes[0] = & node->attr.box.parent;
    subnodes[1] = & node->attr.box.first_statement;
    return 2;
  case ASTNODETYPE_STATEMENT:
    subnodes[0] = & node->attr.statement.target;
    subnodes[1] = & node->attr.statement.next_statement;
    return 2;
  case ASTNODETYPE_CONST:
    return 0;
  case ASTNODETYPE_STRING:
    return 0;
  case ASTNODETYPE_VAR:
    subnodes[0] = & node->attr.var.scope;
    return 1;
  case ASTNODETYPE_IGNORE:
    subnodes[0] = & node->attr.ignore.expr;
    return 1;
  case ASTNODETYPE_UNOP:
    subnodes[0] = & node->attr.un_op.expr;
    return 1;
  case ASTNODETYPE_BINOP:
    subnodes[0] = & node->attr.bin_op.left;
    subnodes[1] = & node->attr.bin_op.right;
    return 2;
  case ASTNODETYPE_MEMBER:
    subnodes[0] = & node->attr.member.expr;
    subnodes[1] = & node->attr.member.next;
    return 2;
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
  case ASTNODETYPE_RAISE:
    subnodes[0] = & node->attr.raise.expr;
    return 1;
  case ASTNODETYPE_SUBTYPEBLD:
    subnodes[0] = & node->attr.subtype_bld.parent;
    return 1;
  case ASTNODETYPE_SELFGET:
    return 0;
  case ASTNODETYPE_PROCDEF:
    subnodes[0] = & node->attr.proc_def.child_type;
    subnodes[1] = & node->attr.proc_def.parent_type;
    subnodes[2] = & node->attr.proc_def.c_name;
    subnodes[3] = & node->attr.proc_def.implem;
    return 4;
  case ASTNODETYPE_TYPEDEF:
    subnodes[0] = & node->attr.type_def.name;
    subnodes[1] = & node->attr.type_def.src_type;
    return 2;
  case ASTNODETYPE_STRUCTYPE:
    subnodes[0] = & node->attr.struc_type.first_member;
    return 1;
  case ASTNODETYPE_MEMBERTYPE:
    subnodes[0] = & node->attr.member_type.type;
    subnodes[1] = & node->attr.member_type.next;
    return 2;
  case ASTNODETYPE_RAISETYPE:
    subnodes[0] = & node->attr.raise_type.type;
    return 1;
  case ASTNODETYPE_SPECTYPE:
    subnodes[0] = & node->attr.spec_type.first_member;
    return 1;
  }
  assert(0); /* Should never happen! */
  return 0;
}

const char *ASTNodeType_To_Str(ASTNodeType t) {
  switch(t) {
  case ASTNODETYPE_ERROR:      return "Error";
  case ASTNODETYPE_TYPENAME:   return "TypeName";
  case ASTNODETYPE_TYPETAG:    return "TypeTag";
  case ASTNODETYPE_SUBTYPE:    return "SubType";
  case ASTNODETYPE_INSTANCE:   return "Instance";
  case ASTNODETYPE_BOX:        return "Box";
  case ASTNODETYPE_STATEMENT:  return "Statement";
  case ASTNODETYPE_CONST:      return "Const";
  case ASTNODETYPE_STRING:     return "String";
  case ASTNODETYPE_VAR:        return "Var";
  case ASTNODETYPE_IGNORE:     return "Ignore";
  case ASTNODETYPE_UNOP:       return "UnOp";
  case ASTNODETYPE_BINOP:      return "BinOp";
  case ASTNODETYPE_MEMBER:     return "Member";
  case ASTNODETYPE_STRUC:      return "Struc";
  case ASTNODETYPE_ARRAYGET:   return "ArrayGet";
  case ASTNODETYPE_MEMBERGET:  return "MemberGet";
  case ASTNODETYPE_SUBTYPEBLD: return "SubtypeBld";
  case ASTNODETYPE_SELFGET:    return "SelfGet";
  case ASTNODETYPE_TYPEDEF:    return "TypeDef";
  default:                     return "UnknownNode";
  }
  return "???";
}

const char *ASTUnOp_To_String(ASTUnOp op) {
  switch(op) {
  case BOXASTUNOP_PLUS: return "+";
  case BOXASTUNOP_NEG: return "-";
  case BOXASTUNOP_LINC: return "++";
  case BOXASTUNOP_LDEC: return "--";
  case BOXASTUNOP_RINC: return "++";
  case BOXASTUNOP_RDEC: return "--";
  case BOXASTUNOP_NOT: return "!";
  case BOXASTUNOP_BNOT: return "~";
  default: break;
  }
  return "?";
}

int ASTUnOp_Is_Right(ASTUnOp op) {
 switch(op) {
 case BOXASTUNOP_RINC:
 case BOXASTUNOP_RDEC:
   return 1;
 default:
   return 0;
 }
  return 0;
}

const char *ASTBinOp_To_String(ASTBinOp op) {
  switch(op) {
  case BOXASTBINOP_ADD: return "+";
  case BOXASTBINOP_SUB: return "-";
  case BOXASTBINOP_MUL: return "*";
  case BOXASTBINOP_DIV: return "/";
  case BOXASTBINOP_REM: return "%";
  case BOXASTBINOP_POW: return "**";
  case BOXASTBINOP_SHL: return "<<";
  case BOXASTBINOP_SHR: return ">>";
  case BOXASTBINOP_EQ: return "==";
  case BOXASTBINOP_NE: return "!=";
  case BOXASTBINOP_LT: return "<";
  case BOXASTBINOP_LE: return "<=";
  case BOXASTBINOP_GT: return ">";
  case BOXASTBINOP_GE: return ">=";
  case BOXASTBINOP_BAND: return "&";
  case BOXASTBINOP_BXOR: return "^";
  case BOXASTBINOP_BOR: return "|";
  case BOXASTBINOP_LAND: return "&&";
  case BOXASTBINOP_LOR: return "||";
  case BOXASTBINOP_ASSIGN: return "=";
  case BOXASTBINOP_APLUS: return "+=";
  case BOXASTBINOP_AMINUS: return "-=";
  case BOXASTBINOP_ATIMES: return "*=";
  case BOXASTBINOP_AREM: return "%=";
  case BOXASTBINOP_ADIV: return "/=";
  case BOXASTBINOP_ASHL: return "<<=";
  case BOXASTBINOP_ASHR: return ">>=";
  case BOXASTBINOP_ABAND: return "&=";
  case BOXASTBINOP_ABXOR: return "^=";
  case BOXASTBINOP_ABOR: return "|=";
  default: break;
  }
  return "?";
}

ASTNode *ASTNode_New(ASTNodeType t) {
  ASTNode *node,
          **subnode[AST_MAX_NUM_SUBNODES];
  int i, num_subnodes;

  node = (ASTNode *) Box_Mem_Alloc(sizeof(ASTNode));
  assert(node != NULL);

  node->type = t;
  node->finaliser = NULL;
  num_subnodes = ASTNode_Get_Subnodes(node, subnode);
  assert(num_subnodes <= AST_MAX_NUM_SUBNODES);
  for(i = 0; i < num_subnodes; i++)
    *subnode[i] = NULL;

  BoxSrc_Init(& node->src);
  return node;
}

void ASTNode_Destroy(ASTNode *node) {
  if (node == NULL)
    return;

  else {
    ASTNode **subnode[AST_MAX_NUM_SUBNODES];
    int i, num_subnodes;

    /* First destroy children */
    num_subnodes = ASTNode_Get_Subnodes(node, subnode);
    for(i = 0; i < num_subnodes; i++) {
      ASTNode *child = *subnode[i];
      ASTNode_Destroy(child);
    }

    if (node->finaliser != NULL)
      node->finaliser(node);

    Box_Mem_Free(node);
  }
}

void ASTNode_Set_Error(ASTNode *node) {
}

/* Used to draw a tree using the characters | \ and - */
typedef struct __IndentStr {
  const char *str;
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
    fprintf(out, "%s", indent->str);
    if (indent->str == branch_last)
      indent->str = branch_end;
  }
}

static void My_Node_Print(FILE *out, ASTNode *node, IndentStr *indent) {
  ASTNode **subnode[AST_MAX_NUM_SUBNODES];
  int i, num_subnodes;

  if (node == NULL) {
    Indent_Print(out, indent);
    fprintf(out, "%sEMPTY NODE\n", branch_sep);
    return;
  }

  /* Get subnodes */
  num_subnodes = ASTNode_Get_Subnodes(node, subnode);

  Indent_Print(out, indent);
  fprintf(out, "%s%s(num_subnodes=%d)\n",
          branch_sep, ASTNodeType_To_Str(node->type), num_subnodes);

  if (num_subnodes > 0) {
    IndentStr new_indent;
    new_indent.str = branch_down;
    Indent_Push(indent, & new_indent);

    for(i = 0; i < num_subnodes; i++) {
      ASTNode *child = *subnode[i];
      if (i == num_subnodes - 1)
        new_indent.str = branch_last;
      My_Node_Print(out, child, indent);
    }

    Indent_Pop(indent);
  }
}

void ASTNode_Print(FILE *out, ASTNode *node) {
  IndentStr indent;
  indent.str = "";
  indent.next = NULL;
  My_Node_Print(out, node, & indent);
}

ASTNode *ASTNodeError_New(void) {
  return ASTNode_New(ASTNODETYPE_ERROR);
}

static void ASTNodeTypeName_Finaliser(ASTNode *node) {
  assert(node->type == ASTNODETYPE_TYPENAME);
  Box_Mem_Free(node->attr.typenm.name);
}

ASTNode *ASTNodeTypeName_New(const char *name, size_t name_len) {
  ASTNode *node = ASTNode_New(ASTNODETYPE_TYPENAME);
  node->attr.typenm.name =
    (name_len > 0) ? Box_Mem_Strndup(name, name_len) : Box_Mem_Strdup(name);
  node->attr.typenm.scope = NULL;
  node->finaliser = ASTNodeTypeName_Finaliser;
  return node;
}

ASTNode *ASTNodeTypeTag_New(BoxTypeId value) {
  ASTNode *node = ASTNode_New(ASTNODETYPE_TYPETAG);
  node->attr.typetag.type = value;
  return node;
}

static void ASTNodeSubtype_Finaliser(ASTNode *node) {
  assert(node->type == ASTNODETYPE_SUBTYPE);
  Box_Mem_Free(node->attr.subtype.name);
}

ASTNode *ASTNodeSubtype_New(ASTNode *parent_type, const char *name) {
  ASTNode *node = ASTNode_New(ASTNODETYPE_SUBTYPE);
  node->attr.subtype.name = Box_Mem_Strdup(name);
  node->attr.subtype.parent = parent_type;
  node->finaliser = ASTNodeSubtype_Finaliser;
  return node;
}

ASTNode *ASTNodeStatement_New(ASTNode *target) {
  ASTNode *node = ASTNode_New(ASTNODETYPE_STATEMENT);
  node->attr.statement.target = target;
  node->attr.statement.sep = ASTSEP_VOID;
  return node;
}

ASTNode *ASTNodeSep_New(ASTSep sep) {
  ASTNode *node = ASTNode_New(ASTNODETYPE_STATEMENT);
  node->attr.statement.sep = sep;
  return node;
}

ASTNode *ASTNodeInstance_New(ASTNode *type) {
  ASTNode *node = ASTNode_New(ASTNODETYPE_INSTANCE);
  node->attr.instance.type = type;
  return node;
}

static void ASTNodeBox_Finaliser(ASTNode *node) {
  assert(node->type == ASTNODETYPE_BOX);
  BoxSrcName *srcname = node->attr.box.file_names;
  if (srcname != NULL)
    BoxSrcName_Destroy(srcname);
}

ASTNode *ASTNodeBox_New(ASTNode *parent, ASTNode *first_statement) {
  ASTNode *node = ASTNode_New(ASTNODETYPE_BOX);
  node->attr.box.first_statement = first_statement;
  node->attr.box.last_statement = first_statement;
  node->attr.box.parent = parent;
  node->attr.box.file_names = NULL;
  node->finaliser = ASTNodeBox_Finaliser;
  return node;
}

ASTNode *ASTNodeBox_Add_Statement(ASTNode *box, ASTNode *statement) {
  assert(box->type == ASTNODETYPE_BOX);

  if (statement == NULL) {
    return box;

  } else {
    ASTNode *last_statement = box->attr.box.last_statement;

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

ASTNode *ASTNodeBox_Add_Sep(ASTNode *box, ASTSep sep) {
  if (sep == ASTSEP_VOID)
    return box;
  else {
    ASTNode *sep_node = ASTNodeSep_New(sep);
    return ASTNodeBox_Add_Statement(box, sep_node);
  }
}

ASTNode *ASTNodeBox_Set_Parent(ASTNode *box, ASTNode *parent) {
  assert(box->type == ASTNODETYPE_BOX);
  box->attr.box.parent = parent;
  return box;
}

ASTNode *ASTNodeConst_New(ASTConstType t, ASTConst c) {
  ASTNode *node = ASTNode_New(ASTNODETYPE_CONST);
  node->attr.constant.type = t;
  node->attr.constant.value = c;
  return node;
}

static void ASTNodeString_Finaliser(ASTNode *node) {
  assert(node->type == ASTNODETYPE_STRING);
  Box_Mem_Free(node->attr.string.str);
}

ASTNode *ASTNodeString_New(const char *str, size_t str_len) {
  ASTNode *node = ASTNode_New(ASTNODETYPE_STRING);
  node->attr.string.str =
    Box_Mem_Str_Merge_With_Len(str, str_len, NULL, 0);
  node->finaliser = ASTNodeString_Finaliser;
  return node;
}

ASTNode *ASTNodeString_Concat(ASTNode *str1, ASTNode *str2) {
  assert(str1->type == ASTNODETYPE_STRING &&
         str2->type == ASTNODETYPE_STRING);
  char *old_str = str1->attr.string.str;
  str1->attr.string.str = Box_Mem_Str_Merge(old_str, str2->attr.string.str);
  Box_Mem_Free(old_str);
  ASTNode_Destroy(str2);
  return str1;
}

static void ASTNodeVar_Finaliser(ASTNode *node) {
  assert(node->type == ASTNODETYPE_VAR);
  Box_Mem_Free(node->attr.var.name);
}

ASTNode *ASTNodeVar_New(const char *name, size_t name_len) {
  ASTNode *node = ASTNode_New(ASTNODETYPE_VAR);
  node->attr.var.name = (name_len > 0) ?
                        Box_Mem_Strndup(name, name_len) : Box_Mem_Strdup(name);
  node->attr.var.scope = NULL;
  node->finaliser = ASTNodeVar_Finaliser;
  return node;
}

ASTNode *ASTNodeIgnore_New(ASTNode *expr, int do_ignore) {
  ASTNode *node = ASTNode_New(ASTNODETYPE_IGNORE);
  node->attr.ignore.expr = expr;
  node->attr.ignore.ignore = do_ignore;
  return node;
}

ASTNode *ASTNodeUnOp_New(ASTUnOp op, ASTNode *expr) {
  ASTNode *node = ASTNode_New(ASTNODETYPE_UNOP);
  node->attr.un_op.operation = op;
  node->attr.un_op.expr = expr;
  return node;
}

ASTNode *ASTNodeBinOp_New(ASTBinOp op, ASTNode *left, ASTNode *right) {
  ASTNode *node = ASTNode_New(ASTNODETYPE_BINOP);
  node->attr.bin_op.operation = op;
  node->attr.bin_op.left = left;
  node->attr.bin_op.right = right;
  return node;
}

static void My_ASTNodeMember_Finaliser(ASTNode *node) {
  assert(node->type == ASTNODETYPE_MEMBER);
  Box_Mem_Free(node->attr.member.name);
}

ASTNode *ASTNodeMember_New(const char *name, ASTNode *expr) {
  ASTNode *node = ASTNode_New(ASTNODETYPE_MEMBER);
  node->attr.member.name = (name == NULL) ? NULL : Box_Mem_Strdup(name);
  node->attr.member.expr = expr;
  node->attr.member.next = NULL;
  node->finaliser = My_ASTNodeMember_Finaliser;
  return node;
}

ASTNode *ASTNodeRaise_New(ASTNode *expr) {
  ASTNode *node = ASTNode_New(ASTNODETYPE_RAISE);
  node->attr.raise.expr = expr;
  return node;
}

ASTNode *ASTNodeStruc_New(const char *first_name, ASTNode *first_expr) {
  ASTNode *first_member = NULL, *node;
  assert(!(first_name != NULL && first_expr == NULL));
  if (first_expr != NULL)
    first_member = ASTNodeMember_New(first_name, first_expr);

  node = ASTNode_New(ASTNODETYPE_STRUC);
  node->attr.struc.first_member = first_member;
  node->attr.struc.last_member = first_member;
  return node;
}

ASTNode *ASTNodeStruc_Add_Member(ASTNode *struc,
                                 const char *this_name, ASTNode *this_expr) {
  ASTNode *this_member = NULL;
  assert(struc->type == ASTNODETYPE_STRUC);
  assert(!(this_name != NULL && this_expr == NULL));
  if (this_expr == NULL) return struc;

  this_member = ASTNodeMember_New(this_name, this_expr);
  if (struc->attr.struc.last_member == NULL) {
    assert(struc->attr.struc.first_member == NULL);
    struc->attr.struc.first_member = this_member;
    struc->attr.struc.last_member = this_member;
    return struc;

  } else {
    ASTNode *last_member = struc->attr.struc.last_member;
    last_member->attr.member.next = this_member;
    struc->attr.struc.last_member = this_member;
  }
  return struc;
}

ASTNode *ASTNodeArrayGet_New(ASTNode *array, ASTNode *index) {
  ASTNode *node = ASTNode_New(ASTNODETYPE_ARRAYGET);
  node->attr.array_get.array = array;
  node->attr.array_get.index = index;
  return node;
}

static void ASTNodeMemberGet_Finaliser(ASTNode *node) {
  assert(node->type == ASTNODETYPE_MEMBERGET);
  Box_Mem_Free(node->attr.member_get.member);
}

ASTNode *ASTNodeMemberGet_New(ASTNode *struc,
                              const char *member, int member_len) {
  ASTNode *node = ASTNode_New(ASTNODETYPE_MEMBERGET);
  node->attr.member_get.struc = struc;
  node->attr.member_get.member = (member_len > 0) ?
                                  Box_Mem_Strndup(member, member_len) :
                                  Box_Mem_Strdup(member);
  node->finaliser = ASTNodeMemberGet_Finaliser;
  return node;
}

static void ASTNodeSubtype_Build_Finaliser(ASTNode *node) {
  assert(node->type == ASTNODETYPE_SUBTYPEBLD);
  Box_Mem_Free(node->attr.subtype_bld.subtype);
}

ASTNode *ASTNodeSubtype_Build(ASTNode *parent, const char *subtype) {
  ASTNode *node = ASTNode_New(ASTNODETYPE_SUBTYPEBLD);
  node->attr.subtype_bld.parent = parent;
  node->attr.subtype_bld.subtype = Box_Mem_Strdup(subtype);
  node->finaliser = ASTNodeSubtype_Build_Finaliser;
  return node;
}

ASTNode *ASTNodeSelfGet_New(BoxInt level) {
  ASTNode *node = ASTNode_New(ASTNODETYPE_SELFGET);
  node->attr.self_get.level = level;
  return node;
}

ASTNode *ASTNodeProcDef_New(ASTNode *child_type, BoxCombType combine,
                            ASTNode *parent_type) {
  ASTNode *node = ASTNode_New(ASTNODETYPE_PROCDEF);
  node->attr.proc_def.child_type = child_type;
  node->attr.proc_def.parent_type = parent_type;
  node->attr.proc_def.combine = combine;
  return node;
}

ASTNode *ASTNodeProcDef_Set(ASTNode *proc_def, ASTNode *c_name,
                            ASTNode *implem) {
  assert(proc_def->type == ASTNODETYPE_PROCDEF);
  if (c_name != NULL)
    proc_def->attr.proc_def.c_name = c_name;
  if (implem != NULL)
    proc_def->attr.proc_def.implem = implem;
  return proc_def;
}

ASTNode *ASTNodeTypeDef_New(ASTNode *name, ASTNode *src_type) {
  ASTNode *node = ASTNode_New(ASTNODETYPE_TYPEDEF);
  node->attr.type_def.name = name;
  node->attr.type_def.src_type = src_type;
  return node;
}

static void My_ASTNodeMemberType_Finaliser(ASTNode *node) {
  assert(node->type == ASTNODETYPE_MEMBERTYPE);
  Box_Mem_Free(node->attr.member_type.name);
}

ASTNode *ASTNodeMemberType_New(ASTTypeMemb *m) {
  ASTNode *node = ASTNode_New(ASTNODETYPE_MEMBERTYPE);
  node->attr.member_type.name = (m->name == NULL) ?
                                NULL : Box_Mem_Strdup(m->name);
  node->attr.member_type.type = m->type;
  node->attr.member_type.next = NULL;
  node->finaliser = My_ASTNodeMemberType_Finaliser;
  return node;
}

ASTNode *ASTNodeStrucType_New(ASTTypeMemb *first_member) {
  ASTNode *member = NULL, *node;
  assert(first_member->type != NULL);
  if (first_member != NULL)
    member = ASTNodeMemberType_New(first_member);

  node = ASTNode_New(ASTNODETYPE_STRUCTYPE);
  node->attr.struc_type.first_member = member;
  node->attr.struc_type.last_member = member;
  return node;
}

ASTNode *ASTNodeStrucType_Add_Member(ASTNode *struc_type,
                                     ASTTypeMemb *member) {
  ASTNode *this_member = NULL;
  assert(struc_type->type == ASTNODETYPE_STRUCTYPE);
  assert(member != NULL);
  assert(!(member->type == NULL && member->name == NULL));

  this_member = ASTNodeMemberType_New(member);
  if (struc_type->attr.struc_type.last_member == NULL) {
    assert(struc_type->attr.struc_type.first_member == NULL);
    struc_type->attr.struc_type.first_member = this_member;
    struc_type->attr.struc_type.last_member = this_member;
    return struc_type;

  } else {
    ASTNode *last_member = struc_type->attr.struc_type.last_member;
    last_member->attr.member_type.next = this_member;
    struc_type->attr.struc_type.last_member = this_member;
  }
  return struc_type;
}

ASTNode *ASTNodeSpecType_New(ASTNode *first_type, ASTNode *second_type) {
  ASTNode *node;
  node = ASTNode_New(ASTNODETYPE_SPECTYPE);
  node->attr.spec_type.first_member = NULL;
  node->attr.spec_type.last_member = NULL;
  node = ASTNodeSpecType_Add_Member(node, first_type);
  node = ASTNodeSpecType_Add_Member(node, second_type);
  return node;
}

ASTNode *ASTNodeSpecType_Add_Member(ASTNode *spec_type, ASTNode *type) {
  ASTNode *memb_node;
  ASTTypeMemb type_memb;

  assert(spec_type->type == ASTNODETYPE_SPECTYPE);

  type_memb.type = type;
  type_memb.name = NULL;
  memb_node = ASTNodeMemberType_New(& type_memb);
  if (spec_type->attr.spec_type.last_member == NULL) {
    assert(spec_type->attr.spec_type.first_member == NULL);
    spec_type->attr.spec_type.first_member =
      spec_type->attr.spec_type.last_member = memb_node;
    return spec_type;

  } else {
    ASTNode *last_member = spec_type->attr.struc_type.last_member;
    last_member->attr.member_type.next = memb_node;
    spec_type->attr.struc_type.last_member = memb_node;
  }

  return spec_type;
}

ASTNode *ASTNodeRaiseType_New(ASTNode *type) {
  ASTNode *node;
  node = ASTNode_New(ASTNODETYPE_RAISETYPE);
  node->attr.raise_type.type = type;
  return node;
}




#define MY_COPY_SRC(lhs, rhs)                     \
  do {BoxASTNode *src_node = (rhs);               \
      if (src_node) lhs->src = src_node->src;     \
      else BoxSrc_Init(& lhs->src);} while(0)

#define MY_PROPAGATE_SRC(container, fst, lst)           \
  do {(container)->src.begin = (fst)->src.begin;        \
      (container)->src.end = (lst)->src.end;} while(0)


void BoxAST_Init(BoxAST *ast)
{
  BoxAllocPool_Init(& ast->pool, sizeof(BoxASTNode)*16);
  ast->root = NULL;

#if 1
#define BOXASTNODE_DEF(NODE, Node)                                      \
    printf("%s size=%zu, alignment=%zu\n",                              \
           #Node, sizeof(BoxASTNode##Node),                             \
           __alignof__(BoxASTNode##Node));
#include "astnodes.h"
#undef BOXASTNODE_DEF
#endif
}

void BoxAST_Finish(BoxAST *ast)
{
  BoxAllocPool_Finish(& ast->pool);
}

/* Create a new abstract syntax tree. */
BoxAST *BoxAST_Create(void)
{
  BoxAST *ast = (BoxAST *) Box_Mem_Alloc(sizeof(BoxAST));
  if (ast)
    BoxAST_Init(ast);
  return ast;
}

/* Destroy an abstract syntax tree created with BoxAST_Create(). */
void BoxAST_Destroy(BoxAST *ast)
{
  if (ast) {
    BoxAST_Finish(ast);
    Box_Mem_Free(ast);
  }
}

/* Set the root node of the tree. */
BoxASTNode *BoxAST_Get_Root(BoxAST *ast)
{
  return ast->root;
}

/* Set the root node of the tree. */
void BoxAST_Set_Root(BoxAST *ast, BoxASTNode *root)
{
  ast->root = root;
}

/* Get size and alignment for the given node type. */
static uint32_t My_Get_Node_Size(BoxASTNodeType type, uint32_t *alignment)
{
#define BOXASTNODE_DEF(NODE, Node)               \
  case BOXASTNODETYPE_##NODE:                    \
    *alignment = __alignof__(BoxASTNode##Node);  \
    return (uint32_t) sizeof(BoxASTNode##Node);

  switch (type) {
#include "astnodes.h"
  default:
    return 0;
  }

#undef BOXASTNODE_DEF
}

/* Get a string representation of the given node type. */
const char *BoxASTNodeType_To_Str(BoxASTNodeType type)
{
#define BOXASTNODE_DEF(NODE, Node) \
  case BOXASTNODETYPE_##NODE: return #Node;

  switch (type) {
#include "astnodes.h"
  default:
    return "Unknown-AST-node";
  }

#undef BOXASTNODE_DEF
}

void *BoxAST_Create_Node(BoxAST *ast, BoxASTNodeType type)
{
  uint32_t node_alignment;
  uint32_t node_size = My_Get_Node_Size(type, & node_alignment);
  if (node_size) {
    BoxASTNode *node =
      BoxAllocPool_Alloc_Aligned(& ast->pool, node_size, node_alignment);
    if (node) {
      BoxASTNode_Set_Type(node, type);
      BoxASTNode_Set_Attr_Mask(node, BOXASTNODEATTR_DEFAULT);
      return node;
    }
  }
  return NULL;
}

/* Create a variable-size node. */
void *BoxAST_Create_Var_Node(BoxAST *ast, BoxASTNodeType type,
                             uint32_t extra_size, void **extra)
{
  uint32_t node_alignment;
  uint32_t head_size = My_Get_Node_Size(type, & node_alignment);
  if (head_size) {
    uint32_t node_size = head_size + extra_size;
    BoxASTNode *node;

    assert(head_size && extra_size);
    node = BoxAllocPool_Alloc_Aligned(& ast->pool, node_size,
                                      node_alignment);
    if (node) {
      BoxASTNode_Set_Type(node, type);
      BoxASTNode_Set_Attr_Mask(node, BOXASTNODEATTR_DEFAULT);
      if (extra)
        *extra = (void *) ((char *) node + node_size);
      return node;
    }
  }

  return NULL;
}

BoxBool BoxASTNode_Set_Src(BoxASTNode *node, BoxSrcIdx begin, BoxSrcIdx end)
{
  if (node) {
    if (begin & end) {
      node->src.begin = begin;
      node->src.end = end;
      return BOXBOOL_TRUE;
    }
    node->src.begin = 0;
  }
  return BOXBOOL_FALSE;
}

BoxASTNode *BoxAST_Create_CharImm(BoxAST *ast, BoxSrc *src, BoxChar value)
{
  BoxASTNode *node = BoxAST_Create_Node(ast, BOXASTNODETYPE_CHAR_IMM);
  if (node) {
    node->src = *src;
    ((BoxASTNodeCharImm *) node)->value = value;
  }
  return node;
}

BoxASTNode *BoxAST_Create_IntImm(BoxAST *ast, BoxSrc *src, BoxInt value)
{
  BoxASTNode *node = BoxAST_Create_Node(ast, BOXASTNODETYPE_INT_IMM);
  if (node) {
    node->src = *src;
    ((BoxASTNodeIntImm *) node)->value = value;
  }
  return node;
}

BoxASTNode *BoxAST_Create_RealImm(BoxAST *ast, BoxSrc *src, BoxReal value)
{
  BoxASTNode *node = BoxAST_Create_Node(ast, BOXASTNODETYPE_REAL_IMM);
  if (node) {
    node->src = *src;
    ((BoxASTNodeRealImm *) node)->value = value;
  }
  return node;
}

BoxASTNode *BoxAST_Create_StrImm(BoxAST *ast, BoxSrc *src,
                                 const char *str, uint32_t str_length)
{
  char *str_copy = BoxAllocPool_Str_NDup(& ast->pool, str, str_length);
  BoxASTNode *node = BoxAST_Create_Node(ast, BOXASTNODETYPE_STR_IMM);
  if (node && str_copy) {
    node->src = *src;
    ((BoxASTNodeStrImm *) node)->str = str_copy;
  }
  return node;
}

/* Create the first statement of statement list. */
BoxASTNode *BoxAST_Create_Statement(BoxAST *ast, BoxASTNode *val)
{
  BoxASTNode *node = BoxAST_Create_Node(ast, BOXASTNODETYPE_STATEMENT);
  if (node) {
    BoxASTNodeStatement *stmt = (BoxASTNodeStatement *) node;
    if (val)
      node->src = val->src;
    stmt->value = val;
    stmt->next = stmt;
    stmt->sep = 0;
  }
  return node;
}

/* Append a statement to the given one. */
BoxASTNode *BoxAST_Append_Statement(BoxAST *ast, BoxASTNode *prev_stmt_node,
                                    BoxASTSep sep, BoxASTNode *stmt_val)
{
  BoxASTNode *stmt_node = BoxAST_Create_Node(ast, BOXASTNODETYPE_STATEMENT);
  if (stmt_node) {
    BoxASTNodeStatement *stmt = (BoxASTNodeStatement *) stmt_node,
                        *prev_stmt = (BoxASTNodeStatement *) prev_stmt_node;
    MY_COPY_SRC(stmt_node, stmt_val);
    stmt->value = stmt_val;
    stmt->next = prev_stmt->next;
    stmt->sep = sep;
    prev_stmt->next = stmt;
  }
  return stmt_node;
}

/* Create a box node. */
BoxASTNode *BoxAST_Create_Box(BoxAST *ast, BoxASTNode *parent,
                              BoxASTNode *last_stmt_node)
{
  BoxASTNode *node = BoxAST_Create_Node(ast, BOXASTNODETYPE_BOX);
  if (node) {
    BoxASTNodeBox *box = (BoxASTNodeBox *) node;
    BoxASTNodeStatement *last_stmt;

    assert(BoxASTNode_Get_Type(last_stmt_node) == BOXASTNODETYPE_STATEMENT);
    last_stmt = (BoxASTNodeStatement *) last_stmt_node;
    box->parent = parent;
    box->first_stmt = last_stmt->next;
    MY_PROPAGATE_SRC(node, (parent) ? parent : (BoxASTNode *) box->first_stmt,
                     last_stmt_node);
    last_stmt->next = NULL;
  }
  return node;
}

/* Create a type identifier node. */
BoxASTNode *BoxAST_Create_TypeIdfr(BoxAST *ast, BoxSrc *src,
                                   const char *name, uint32_t name_length)
{
  uint32_t extra_size = name_length + 1;
  BoxASTNode *node;

  assert(extra_size > 1);
  node = BoxAST_Create_Var_Node(ast, BOXASTNODETYPE_TYPE_IDFR,
                                extra_size, NULL);
  if (node) {
    BoxASTNodeTypeIdfr *idfr = (BoxASTNodeTypeIdfr *) node;
    node->src = *src;
    memcpy(& idfr->name[0], name, name_length);
    idfr->name[name_length] = '\0';
  }

  return node;
}

/* Create a type/variable identifier node. */
BoxASTNode *BoxAST_Create_VarIdfr(BoxAST *ast, BoxSrc *src,
                                  const char *name, uint32_t name_length)
{
  uint32_t extra_size = name_length + 1;
  BoxASTNode *node;

  assert(extra_size > 1);
  node = BoxAST_Create_Var_Node(ast, BOXASTNODETYPE_VAR_IDFR,
                                extra_size, NULL);
  if (node) {
    BoxASTNodeVarIdfr *idfr = (BoxASTNodeVarIdfr *) node;
    node->src = *src;
    memcpy(& idfr->name[0], name, name_length);
    idfr->name[name_length] = '\0';
  }

  return node;
}

/* Create an ignore node. */
BoxASTNode *BoxAST_Create_Ignore(BoxAST *ast, BoxASTNode *value)
{
  BoxASTNode *node = BoxAST_Create_Node(ast, BOXASTNODETYPE_IGNORE);
  if (node) {
    BoxASTNodeIgnore *ignore = (BoxASTNodeIgnore *) node;
    node->src = value->src;
    ignore->value = value;
  }
  return node;
}

/* Create a new unary operation. */
BoxASTNode *BoxAST_Create_UnOp(BoxAST *ast, BoxASTUnOp op, BoxASTNode *value)
{
  BoxASTNode *node = BoxAST_Create_Node(ast, BOXASTNODETYPE_UN_OP);
  if (node) {
    BoxASTNodeUnOp *un_op = (BoxASTNodeUnOp *) node;
    un_op->value = value;
    un_op->op = op;
  }
  return node;
}

/* Create a new binary operation. */
BoxASTNode *BoxAST_Create_BinOp(BoxAST *ast, BoxASTNode *lhs,
                                BoxASTBinOp op, BoxASTNode *rhs)
{
  BoxASTNode *node = BoxAST_Create_Node(ast, BOXASTNODETYPE_BIN_OP);
  if (node) {
    BoxASTNodeBinOp *bin_op = (BoxASTNodeBinOp *) node;
    bin_op->lhs = lhs;
    bin_op->rhs = rhs;
    bin_op->op = op;
  }
  return node;
}
