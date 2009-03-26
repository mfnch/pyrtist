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

/** @file ast.h
 * @brief Abstract syntax tree related functionality.
 *
 * A nice description...
 */

#ifndef _AST_H
#  define _AST_H

#  include <stdio.h>

#  include "types.h"

#  define AST_MAX_NUM_SUBNODES 3

typedef struct __AstNode AstNode;

/** Possible types of nodes in the AST */
typedef enum {
  ASTNODETYPE_ERROR,
  ASTNODETYPE_TYPENAME,
  ASTNODETYPE_SUBTYPE,
  ASTNODETYPE_BOX,
  ASTNODETYPE_STATEMENT,
  ASTNODETYPE_CONST,
  ASTNODETYPE_STRING,
  ASTNODETYPE_VAR,
  ASTNODETYPE_UNOP,
  ASTNODETYPE_BINOP,
  ASTNODETYPE_MEMBER,
  ASTNODETYPE_STRUC,
  ASTNODETYPE_ARRAYGET,
  ASTNODETYPE_MEMBERGET
} AstNodeType;

/** Type of constants */
typedef enum {
  ASTCONSTTYPE_CHAR,
  ASTCONSTTYPE_INT,
  ASTCONSTTYPE_REAL
} AstConstType;

/** Union large enough to contain a value of any constant type */
typedef union {
  Char c;
  Int  i;
  Real r;
} AstConst;

/** Unary operators */
typedef enum {
  ASTUNOP_PLUS,
  ASTUNOP_NEG,
  ASTUNOP_LINC,
  ASTUNOP_LDEC,
  ASTUNOP_RINC,
  ASTUNOP_RDEC,
  ASTUNOP_NOT,
  ASTUNOP_BNOT
} AstUnOp;

/** Binary operators */
typedef enum {
  ASTBINOP_ADD,
  ASTBINOP_SUB,
  ASTBINOP_MUL,
  ASTBINOP_DIV,
  ASTBINOP_REM,
  ASTBINOP_SHL,
  ASTBINOP_SHR,
  ASTBINOP_EQ,
  ASTBINOP_NE,
  ASTBINOP_LT,
  ASTBINOP_LE,
  ASTBINOP_GT,
  ASTBINOP_GE,
  ASTBINOP_BAND,
  ASTBINOP_BXOR,
  ASTBINOP_BOR,
  ASTBINOP_LAND,
  ASTBINOP_LOR,
  ASTBINOP_ASSIGN,
  ASTBINOP_APLUS,
  ASTBINOP_AMINUS,
  ASTBINOP_ATIMES,
  ASTBINOP_AREM,
  ASTBINOP_ADIV,
  ASTBINOP_ASHL,
  ASTBINOP_ASHR,
  ASTBINOP_ABAND,
  ASTBINOP_ABXOR,
  ASTBINOP_ABOR
} AstBinOp;

/* Each of the following structures corresponds to a possible node in the AST
   and contains the characteristics attribute for that node type.
 */

/** Node for a type name */
typedef struct {
  char    *name;
  AstNode *scope;
} AstNodeTypeName;

/** Node for a subtype */
typedef struct {
  char    *name;
  AstNode *scope;
} AstNodeSubtype;

/** Node for a Box: which is a list of statements */
typedef struct {
  AstNode *parent;
  AstNode *first_statement;
  AstNode *last_statement;
} AstNodeBox;

/** Node for a statement */
typedef struct {
  AstNode *expression;
  AstNode *next_statement;
} AstNodeStatement;

/** Node for a constant expression */
typedef struct {
  AstConstType type;
  AstConst     value;
} AstNodeConst;

/** Node for a string */
typedef struct {
  char    *str;
} AstNodeString;

/** Node for a variable */
typedef struct {
  char    *name;
  AstNode *scope;
} AstNodeVar;

/** Node for a unary operation */
typedef struct {
  AstUnOp  operation;
  AstNode  *expr;
} AstNodeUnOp;

/** Node for a binary operation */
typedef struct {
  AstBinOp operation;
  AstNode  *left;
  AstNode  *right;
} AstNodeBinOp;

/** Node for one structure member */
typedef struct {
  AstNode  *name,
           *expr,
           *next;
} AstNodeMember;

/** Node for a structure */
typedef struct {
  AstNode  *first_member;
  AstNode  *last_member;
} AstNodeStruc;

/** Node for an array */
typedef struct {
  AstNode  *array;
  AstNode  *index;
} AstNodeArrayGet;

/** Node for an array */
typedef struct {
  AstNode  *struc;
  char     *member;
} AstNodeMemberGet;

/** Node finaliser (used only for few nodes) */
typedef void (*AstNodeFinaliser)(AstNode *node);

/** Structure describing one node in the AST */
struct __AstNode {
  AstNodeType        type;
  AstNodeFinaliser   finaliser;
  union {
    AstNodeTypeName  typenm;
    AstNodeSubtype   subtype;
    AstNodeBox       box;
    AstNodeStatement statement;
    AstNodeConst     constant;
    AstNodeString    string;
    AstNodeUnOp      un_op;
    AstNodeBinOp     bin_op;
    AstNodeVar       var;
    AstNodeMember    member;
    AstNodeStruc     struc;
    AstNodeArrayGet  array_get;
    AstNodeMemberGet member_get;
  } attr;
};

typedef AstNode *AstNodePtr;

int AstNode_Get_Subnodes(AstNode *node,
                         AstNode **subnodes[AST_MAX_NUM_SUBNODES]);
const char *AstNodeType_To_Str(AstNodeType t);
AstNode *AstNode_New(AstNodeType t);
void AstNode_Destroy(AstNode *node);
void AstNode_Set_Error(AstNode *node);
void AstNode_Print(FILE *out, AstNode *node);

AstNode *AstNodeError_New(void);
AstNode *AstNodeTypeName_New(const char *name, size_t name_len);
AstNode *AstNodeSubtype_New(const char *name, size_t name_len);
AstNode *AstNodeStatement_New(AstNode *expr);
AstNode *AstNodeBox_New(AstNode *type, AstNode *first_statement);
AstNode *AstNodeBox_Add_Statement(AstNode *box, AstNode *statement);
AstNode *AstNodeBox_Set_Parent(AstNode *box, AstNode *parent);
AstNode *AstNodeConst_New(AstConstType t, AstConst c);
AstNode *AstNodeString_New(const char *str, size_t str_len);
AstNode *AstNodeVar_New(const char *name, size_t name_len);
AstNode *AstNodeUnOp_New(AstUnOp op, AstNode *expr);
AstNode *AstNodeBinOp_New(AstBinOp op, AstNode *left, AstNode *right);
AstNode *AstNodeMember_New(AstNode *name, AstNode *expr);
AstNode *AstNodeStruc_New(AstNode *first_name, AstNode *first_expr);
AstNode *AstNodeStruc_Add_Member(AstNode *struc,
                                 AstNode *this_name, AstNode *this_expr);
AstNode *AstNodeArrayGet_New(AstNode *array, AstNode *index);
AstNode *AstNodeMemberGet_New(AstNode *struc,
                              const char *member, int member_len);

#endif /* _AST_H */

