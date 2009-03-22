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

#  include "types.h"

#  define AST_MAX_NUM_SUBNODES 3

typedef struct __AstNode AstNode;

/** Possible types of nodes in the AST */
typedef enum {
  ASTNODETYPE_TYPENAME,
  ASTNODETYPE_BOX,
  ASTNODETYPE_STATEMENT,
  ASTNODETYPE_CONST,
  ASTNODETYPE_STRING,
  ASTNODETYPE_VAR,
  ASTNODETYPE_UNOP,
  ASTNODETYPE_BINOP,
  ASTNODETYPE_MEMBER,
  ASTNODETYPE_STRUC
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

typedef enum {
  ASTUNOP_NEG,
  ASTUNOP_LINC,
  ASTUNOP_LDEC,
  ASTUNOP_RINC,
  ASTUNOP_RDEC
} AstUnOp;

typedef enum {
  ASTBINOP_ADD,
  ASTBINOP_SUB,
  ASTBINOP_MUL,
  ASTBINOP_DIV
} AstBinOp;

/* Each of the following structures corresponds to a possible node in the AST
   and contains the characteristics attribute for that node type.
 */

/** Node for a type name */
typedef struct {
  char    *name;
  AstNode *scope;
} AstNodeTypeName;

/** Node for a Box: which is a list of statements */
typedef struct {
  AstNode *type;
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

/** Node finaliser (used only for few nodes) */
typedef void (*AstNodeFinaliser)(AstNode *node);

/** Structure describing one node in the AST */
struct __AstNode {
  AstNodeType        type;
  AstNodeFinaliser   finaliser;
  union {
    AstNodeTypeName  typenm;
    AstNodeBox       box;
    AstNodeStatement statement;
    AstNodeConst     constant;
    AstNodeString    string;
    AstNodeUnOp      un_op;
    AstNodeBinOp     bin_op;
    AstNodeVar       var;
    AstNodeMember    member;
    AstNodeStruc     struc;
  } attr;
};

#endif /* _AST_H */
