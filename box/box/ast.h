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

/**
 * @file ast.h
 * @brief Abstract syntax tree related functionality.
 *
 * A nice description...
 */

#ifndef _AST_H
#  define _AST_H

#  include <stdio.h>
#  include <stdlib.h>

#  include <box/types.h>
#  include <box/srcpos.h>

#  define AST_MAX_NUM_SUBNODES 4

typedef struct __ASTNode ASTNode;

/** Possible types of nodes in the AST */
typedef enum {
  ASTNODETYPE_ERROR,
  ASTNODETYPE_TYPENAME,
  ASTNODETYPE_TYPETAG,
  ASTNODETYPE_SUBTYPE,
  ASTNODETYPE_INSTANCE,
  ASTNODETYPE_BOX,
  ASTNODETYPE_STATEMENT,
  ASTNODETYPE_CONST,
  ASTNODETYPE_STRING,
  ASTNODETYPE_VAR,
  ASTNODETYPE_IGNORE,
  ASTNODETYPE_UNOP,
  ASTNODETYPE_BINOP,
  ASTNODETYPE_MEMBER,
  ASTNODETYPE_STRUC,
  ASTNODETYPE_ARRAYGET,
  ASTNODETYPE_MEMBERGET,
  ASTNODETYPE_RAISE,
  ASTNODETYPE_SELFGET,
  ASTNODETYPE_SUBTYPEBLD,
  ASTNODETYPE_PROCDEF,
  ASTNODETYPE_TYPEDEF,
  ASTNODETYPE_STRUCTYPE,
  ASTNODETYPE_MEMBERTYPE,
  ASTNODETYPE_RAISETYPE,
  ASTNODETYPE_SPECTYPE

} ASTNodeType;

/**Separator between statements */
typedef enum {ASTSEP_VOID, ASTSEP_PAUSE} ASTSep;

/** Integer number which identifies the scope of a variable or a type */
typedef int ASTScope;

/** Type of constants */
typedef enum {
  ASTCONSTTYPE_CHAR,
  ASTCONSTTYPE_INT,
  ASTCONSTTYPE_REAL
} ASTConstType;

/** Union large enough to contain a value of any constant type */
typedef union {
  BoxChar c;
  BoxInt  i;
  BoxReal r;
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
  ASTBINOP_POW,
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

/** Integer indicating the self level: 0 --> $, 1 --> $$, 2 --> $$$, etc. */
typedef BoxInt ASTSelfLevel;

/* Each of the following structures corresponds to a possible node in the AST
   and contains the characteristics attribute for that node type.
 */

/** Node for a type name */
typedef struct {
  char    *name;
  ASTNode *scope;
} ASTNodeTypeName;

/** Node for a special type */
typedef struct {
  BoxTypeId  type;
} ASTNodeTypeTag;

/** Node for a subtype */
typedef struct {
  char    *name;
  ASTNode *parent;
} ASTNodeSubtype;

/** Node for instantiation. E.g. in ``x = Type'' this node captures has
 * ``Type'' as a child and indicates that a new instance of that type should
 * be created.
 */
typedef struct {
  ASTNode    *type;
} ASTNodeInstance;

/** Node for a Box: which is a list of statements */
typedef struct {
  ASTNode    *parent;
  ASTNode    *first_statement;
  ASTNode    *last_statement;
  BoxSrcName *file_names;     /**< Names of all the files involved */
} ASTNodeBox;

/** Node for a statement */
typedef struct {
  ASTNode *target;
  ASTNode *next_statement;
  ASTSep  sep;
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

/** Node for removal of ignore attribute */
typedef struct {
  ASTNode *expr;
  int     ignore;
} ASTNodeIgnore;

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
  char     *name;
  ASTNode  *expr,
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

/** Node for the raise operation */
typedef struct {
  ASTNode  *expr;
} ASTNodeRaise;

/** Node for subtype building */
typedef struct {
  ASTNode  *parent;
  char     *subtype;
} ASTNodeSubtypeBld;

/** Node for $, $$, $$$, etc. */
typedef struct {
  ASTSelfLevel
           level;
} ASTNodeSelfGet;

/** Node for procedure definition/declaration */
typedef struct {
  ASTNode     *child_type,
              *parent_type,
              *c_name,
              *implem;
  BoxCombType combine;
} ASTNodeProcDef;

/** Node for type definition */
typedef struct {
  ASTNode  *name,
           *src_type;
} ASTNodeTypeDef;

/** Node for structure type definition */
typedef struct {
  ASTNode  *first_member,
           *last_member;
} ASTNodeStrucType;

/** Node for member definition in structure types */
typedef struct {
  char     *name;
  ASTNode  *type,
           *next;
} ASTNodeMemberType;

/** Node for type raising */
typedef struct {
  ASTNode  *type;
} ASTNodeRaiseType;

/** Node for species type definition */
typedef struct {
  ASTNode  *first_member,
           *last_member;
} ASTNodeSpecType;

/** Node finaliser (used only for few nodes) */
typedef void (*ASTNodeFinaliser)(ASTNode *node);

/** Structure describing one node in the AST */
struct __ASTNode {
  ASTNodeType         type;      /**< Node type */
  ASTNodeFinaliser    finaliser; /**< Destructor for node */
  BoxSrc              src;       /**< Position in the source*/
  /* ^^^ this last entry doubles the size of ASTNode
   *     (from 24 bytes to 48 on 32 bit machine)
   *     We should use linear positions!
   */
  union {
    ASTNodeTypeName   typenm;
    ASTNodeTypeTag    typetag;
    ASTNodeSubtype    subtype;
    ASTNodeInstance   instance;
    ASTNodeBox        box;
    ASTNodeStatement  statement;
    ASTNodeConst      constant;
    ASTNodeString     string;
    ASTNodeVar        var;
    ASTNodeIgnore     ignore;
    ASTNodeUnOp       un_op;
    ASTNodeBinOp      bin_op;
    ASTNodeMember     member;
    ASTNodeStruc      struc;
    ASTNodeArrayGet   array_get;
    ASTNodeMemberGet  member_get;
    ASTNodeRaise      raise;
    ASTNodeSubtypeBld subtype_bld;
    ASTNodeSelfGet    self_get;
    ASTNodeProcDef    proc_def;
    ASTNodeTypeDef    type_def;
    ASTNodeStrucType  struc_type;
    ASTNodeMemberType member_type;
    ASTNodeRaiseType  raise_type;
    ASTNodeSpecType   spec_type;
  } attr;
};

typedef ASTNode *ASTNodePtr;

/** Used to define members of structure and species types */
typedef struct {
  ASTNode *type; /**< Type of the member (NULL, if not present) */
  char    *name; /**< Name of the member (NULL, if not present) */
} ASTTypeMemb;

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

/** Returns 1 if the given operator is a "right" unary operator,
 * meaning that it returs its operand before modifying it.
 */
int ASTUnOp_Is_Right(ASTUnOp op);

ASTNode *ASTNode_New(ASTNodeType t);
void ASTNode_Destroy(ASTNode *node);
void ASTNode_Set_Error(ASTNode *node);
void ASTNode_Print(FILE *out, ASTNode *node);

ASTNode *ASTNodeError_New(void);
ASTNode *ASTNodeTypeName_New(const char *name, size_t name_len);
ASTNode *ASTNodeTypeTag_New(BoxTypeId value);
ASTNode *ASTNodeSubtype_New(ASTNode *parent_type, const char *name);
ASTNode *ASTNodeStatement_New(ASTNode *target);
ASTNode *ASTNodeSep_New(ASTSep sep);
ASTNode *ASTNodeInstance_New(ASTNode *type);
ASTNode *ASTNodeBox_New(ASTNode *type, ASTNode *first_statement);
ASTNode *ASTNodeBox_Add_Statement(ASTNode *box, ASTNode *statement);
ASTNode *ASTNodeBox_Add_Sep(ASTNode *box, ASTSep sep);
ASTNode *ASTNodeBox_Set_Parent(ASTNode *box, ASTNode *parent);
ASTNode *ASTNodeConst_New(ASTConstType t, ASTConst c);
ASTNode *ASTNodeString_New(const char *str, size_t str_len);
ASTNode *ASTNodeString_Concat(ASTNode *str1, ASTNode *str2);
ASTNode *ASTNodeVar_New(const char *name, size_t name_len);
ASTNode *ASTNodeIgnore_New(ASTNode *expr, int do_ignore);
ASTNode *ASTNodeUnOp_New(ASTUnOp op, ASTNode *expr);
ASTNode *ASTNodeBinOp_New(ASTBinOp op, ASTNode *left, ASTNode *right);
ASTNode *ASTNodeMember_New(const char *name, ASTNode *expr);
ASTNode *ASTNodeStruc_New(const char *first_name, ASTNode *first_expr);
ASTNode *ASTNodeStruc_Add_Member(ASTNode *struc,
                                 const char *this_name, ASTNode *this_expr);
ASTNode *ASTNodeArrayGet_New(ASTNode *array, ASTNode *index);
ASTNode *ASTNodeMemberGet_New(ASTNode *struc,
                              const char *member, int member_len);
ASTNode *ASTNodeRaise_New(ASTNode *expr);
ASTNode *ASTNodeSubtype_Build(ASTNode *parent, const char *subtype);
ASTNode *ASTNodeSelfGet_New(BoxInt level);
ASTNode *ASTNodeProcDef_New(ASTNode *child_type, BoxCombType combine,
                            ASTNode *parent_type);
ASTNode *ASTNodeProcDef_Set(ASTNode *proc_def, ASTNode *c_name,
                            ASTNode *implem);
ASTNode *ASTNodeTypeDef_New(ASTNode *name, ASTNode *src_type);
ASTNode *ASTNodeStrucType_New(ASTTypeMemb *first_member);
ASTNode *ASTNodeStrucType_Add_Member(ASTNode *struc_type,
                                     ASTTypeMemb *member);
ASTNode *ASTNodeSpecType_New(ASTNode *first_type, ASTNode *second_type);
ASTNode *ASTNodeSpecType_Add_Member(ASTNode *species, ASTNode *memb);
ASTNode *ASTNodeRaiseType_New(ASTNode *type);

#endif /* _AST_H */
