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

typedef struct __ASTNode ASTNode;

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
} ASTNodeType;

/** Type of constants */
typedef enum {
  ASTCONSTTYPE_CHAR,
  ASTCONSTTYPE_INT,
  ASTCONSTTYPE_REAL
} ASTConstType;

/** Union large enough to contain a value of any constant type */
typedef union {
  Char c;
  Int  i;
  Real r;
} ASTConst;

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
} ASTUnOp;

#define ASTUNOP__NUM_OPS (ASTUNOP_BNOT + 1)

/** Binary operators */
typedef enum {
  ASTBINOP_ADD=0,
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
} ASTBinOp;

#define ASTBINOP__NUM_OPS (ASTBINOP_ABOR + 1)

/* Each of the following structures corresponds to a possible node in the AST
   and contains the characteristics attribute for that node type.
 */

/** Node for a type name */
typedef struct {
  char    *name;
  ASTNode *scope;
} ASTNodeTypeName;

/** Node for a subtype */
typedef struct {
  char    *name;
  ASTNode *scope;
} ASTNodeSubtype;

/** Node for a Box: which is a list of statements */
typedef struct {
  ASTNode *parent;
  ASTNode *first_statement;
  ASTNode *last_statement;
} ASTNodeBox;

/** Node for a statement */
typedef struct {
  ASTNode *target;
  ASTNode *next_statement;
} ASTNodeStatement;

/** Node for a constant expression */
typedef struct {
  ASTConstType type;
  ASTConst     value;
} ASTNodeConst;

/** Node for a string */
typedef struct {
  char    *str;
} ASTNodeString;

/** Node for a variable */
typedef struct {
  char    *name;
  ASTNode *scope;
} ASTNodeVar;

/** Node for a unary operation */
typedef struct {
  ASTUnOp  operation;
  ASTNode  *expr;
} ASTNodeUnOp;

/** Node for a binary operation */
typedef struct {
  ASTBinOp operation;
  ASTNode  *left;
  ASTNode  *right;
} ASTNodeBinOp;

/** Node for one structure member */
typedef struct {
  ASTNode  *name,
           *expr,
           *next;
} ASTNodeMember;

/** Node for a structure */
typedef struct {
  ASTNode  *first_member;
  ASTNode  *last_member;
} ASTNodeStruc;

/** Node for an array */
typedef struct {
  ASTNode  *array;
  ASTNode  *index;
} ASTNodeArrayGet;

/** Node for an array */
typedef struct {
  ASTNode  *struc;
  char     *member;
} ASTNodeMemberGet;

/** Node finaliser (used only for few nodes) */
typedef void (*ASTNodeFinaliser)(ASTNode *node);

/** Structure describing one node in the AST */
struct __ASTNode {
  ASTNodeType        type;
  ASTNodeFinaliser   finaliser;
  union {
    ASTNodeTypeName  typenm;
    ASTNodeSubtype   subtype;
    ASTNodeBox       box;
    ASTNodeStatement statement;
    ASTNodeConst     constant;
    ASTNodeString    string;
    ASTNodeUnOp      un_op;
    ASTNodeBinOp     bin_op;
    ASTNodeVar       var;
    ASTNodeMember    member;
    ASTNodeStruc     struc;
    ASTNodeArrayGet  array_get;
    ASTNodeMemberGet member_get;
  } attr;
};

typedef ASTNode *ASTNodePtr;

int ASTNode_Get_Subnodes(ASTNode *node,
                         ASTNode **subnodes[AST_MAX_NUM_SUBNODES]);
const char *ASTNodeType_To_Str(ASTNodeType t);

/** Return the string representation of the unary operator passed as
 * argument.
 */
const char *ASTUnOp_To_String(ASTUnOp op);

/** Return the string representation of the binary operator passed as
 * argument.
 */
const char *ASTBinOp_To_String(ASTBinOp op);

ASTNode *ASTNode_New(ASTNodeType t);
void ASTNode_Destroy(ASTNode *node);
void ASTNode_Set_Error(ASTNode *node);
void ASTNode_Print(FILE *out, ASTNode *node);

ASTNode *ASTNodeError_New(void);
ASTNode *ASTNodeTypeName_New(const char *name, size_t name_len);
ASTNode *ASTNodeSubtype_New(const char *name, size_t name_len);
ASTNode *ASTNodeStatement_New(ASTNode *target);
ASTNode *ASTNodeBox_New(ASTNode *type, ASTNode *first_statement);
ASTNode *ASTNodeBox_Add_Statement(ASTNode *box, ASTNode *statement);
ASTNode *ASTNodeBox_Set_Parent(ASTNode *box, ASTNode *parent);
ASTNode *ASTNodeConst_New(ASTConstType t, ASTConst c);
ASTNode *ASTNodeString_New(const char *str, size_t str_len);
ASTNode *ASTNodeString_Concat(ASTNode *str1, ASTNode *str2);
ASTNode *ASTNodeVar_New(const char *name, size_t name_len);
ASTNode *ASTNodeUnOp_New(ASTUnOp op, ASTNode *expr);
ASTNode *ASTNodeBinOp_New(ASTBinOp op, ASTNode *left, ASTNode *right);
ASTNode *ASTNodeMember_New(ASTNode *name, ASTNode *expr);
ASTNode *ASTNodeStruc_New(ASTNode *first_name, ASTNode *first_expr);
ASTNode *ASTNodeStruc_Add_Member(ASTNode *struc,
                                 ASTNode *this_name, ASTNode *this_expr);
ASTNode *ASTNodeArrayGet_New(ASTNode *array, ASTNode *index);
ASTNode *ASTNodeMemberGet_New(ASTNode *struc,
                              const char *member, int member_len);

#endif /* _AST_H */
