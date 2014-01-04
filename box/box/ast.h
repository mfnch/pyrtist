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

#ifndef _BOX_AST_H
#  define _BOX_AST_H

#  include <stdio.h>
#  include <stdlib.h>
#  include <stdint.h>

#  include <box/types.h>
#  include <box/allocpool.h>
#  include <box/srcpos.h>

#  define AST_MAX_NUM_SUBNODES 4

typedef struct ASTNode_struct ASTNode;

/** Possible types of nodes in the AST */
typedef enum {
  ASTNODETYPE_ERROR,
  ASTNODETYPE_TYPENAME,  //
  ASTNODETYPE_TYPETAG,   //
  ASTNODETYPE_SUBTYPE,   //
  ASTNODETYPE_INSTANCE,
  ASTNODETYPE_BOX,       //
  ASTNODETYPE_STATEMENT, //
  ASTNODETYPE_CONST,     //
  ASTNODETYPE_STRING,    //
  ASTNODETYPE_VAR,       //
  ASTNODETYPE_IGNORE,    //
  ASTNODETYPE_UNOP,      //
  ASTNODETYPE_BINOP,     //
  ASTNODETYPE_MEMBER,    //
  ASTNODETYPE_STRUC,     //
  ASTNODETYPE_ARRAYGET,
  ASTNODETYPE_MEMBERGET, //
  ASTNODETYPE_RAISE,     //
  ASTNODETYPE_SELFGET,   //
  ASTNODETYPE_SUBTYPEBLD,//
  ASTNODETYPE_PROCDEF,   //
  ASTNODETYPE_TYPEDEF,   //
  ASTNODETYPE_STRUCTYPE, //
  ASTNODETYPE_MEMBERTYPE,
  ASTNODETYPE_RAISETYPE, //
  ASTNODETYPE_SPECTYPE   //
} ASTNodeType;

/**
 * @brief Separator between statements.
 */
typedef enum {
  BOXASTSEP_NONE=0,
  BOXASTSEP_VOID,
  BOXASTSEP_PAUSE,
  BOXASTSEP_ARROW,
  ASTSEP_VOID=BOXASTSEP_VOID,
  ASTSEP_PAUSE=BOXASTSEP_PAUSE
} BoxASTSep;

/** Integer number which identifies the scope of a variable or a type */
typedef int ASTScope;

/**
 * @brief Union of all the immediate types.
 */
typedef union {
  BoxChar c;
  BoxInt  i;
  BoxReal r;
} BoxASTImm;

/**
 * @brief Type of immediate.
 */
typedef enum {
  BOXASTIMMTYPE_CHAR,
  BOXASTIMMTYPE_INT,
  BOXASTIMMTYPE_REAL
} BoxASTImmType;

/** Type of constants */
typedef enum {
  ASTCONSTTYPE_CHAR,
  ASTCONSTTYPE_INT,
  ASTCONSTTYPE_REAL
} ASTConstType;

/** Unary operators */
typedef enum {
  BOXASTUNOP_PLUS,
  BOXASTUNOP_NEG,
  BOXASTUNOP_LINC,
  BOXASTUNOP_LDEC,
  BOXASTUNOP_RINC,
  BOXASTUNOP_RDEC,
  BOXASTUNOP_NOT,
  BOXASTUNOP_BNOT,
  BOXASTUNOP_NUM_OPS
} BoxASTUnOp;

/** Binary operators */
typedef enum {
  BOXASTBINOP_ADD=0,
  BOXASTBINOP_SUB,
  BOXASTBINOP_MUL,
  BOXASTBINOP_DIV,
  BOXASTBINOP_REM,
  BOXASTBINOP_POW,
  BOXASTBINOP_SHL,
  BOXASTBINOP_SHR,
  BOXASTBINOP_EQ,
  BOXASTBINOP_NE,
  BOXASTBINOP_LT,
  BOXASTBINOP_LE,
  BOXASTBINOP_GT,
  BOXASTBINOP_GE,
  BOXASTBINOP_BAND,
  BOXASTBINOP_BXOR,
  BOXASTBINOP_BOR,
  BOXASTBINOP_LAND,
  BOXASTBINOP_LOR,
  BOXASTBINOP_ASSIGN,
  BOXASTBINOP_APLUS,
  BOXASTBINOP_AMINUS,
  BOXASTBINOP_ATIMES,
  BOXASTBINOP_AREM,
  BOXASTBINOP_ADIV,
  BOXASTBINOP_ASHL,
  BOXASTBINOP_ASHR,
  BOXASTBINOP_ABAND,
  BOXASTBINOP_ABXOR,
  BOXASTBINOP_ABOR,
  BOXASTBINOP_NUM_OPS
} BoxASTBinOp;

/* TEMPORARY: for compatibility. */
#define ASTConst BoxASTImm
#define ASTSep BoxASTSep
#define ASTBinOp BoxASTBinOp
#define ASTUnOp BoxASTUnOp

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
  ASTSelfLevel level;
} ASTNodeSelfGet;

/** Node for procedure definition/declaration */
typedef struct {
  BoxCombType combine;
  ASTNode     *child_type,
              *parent_type,
              *c_name,
              *implem;
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
typedef void (*ASTNodeFinalizer)(ASTNode *node);

/** Structure describing one node in the AST */
struct ASTNode_struct {
  ASTNodeFinalizer    finaliser; /**< Destructor for node */
  BoxSrc              src;       /**< Position in the source*/
  ASTNodeType         type;      /**< Node type */

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



/**
 * @brief Common attribute for AST nodes.
 */
enum BoxASTNodeAttr {
  BOXASTNODEATTR_ERR     = 0x1, /**< The node contains an error. */
  BOXASTNODEATTR_TYPE    = 0x2, /**< The node represents a type. */
  BOXASTNODEATTR_ALL     = 0x3, /**< All attributes or-ed together. */
  BOXASTNODEATTR_DEFAULT = 0x0, /**< Init value for the attributes. */
};

/**
 * @brief Possible types of nodes in the AST.
 */
typedef enum {
#  define BOXASTNODE_DEF(NODE, Node) BOXASTNODETYPE_##NODE,
#  include <box/astnodes.h>
#  undef BOXASTNODE_DEF
} BoxASTNodeType;

/**
 * @brief Generic AST node.
 *
 * AST nodes have type names starting with BoxASTNode (e.g. #BoxASTNodeIntImm)
 * and can all be safely cast to a generic AST node.
 * All AST nodes do indeed begin with the same common header structure, which
 * tells the type of the node and the piece of source code the node is
 * representing. See #BOXASTNODEHEAD for more information.
 */
typedef struct BoxASTNode_struct {
  BoxSrc         src;
  uint8_t        type;
  uint8_t        attr;
} BoxASTNode;

/**
 * @brief Get the attributes mask of an AST node.
 */
#define BoxASTNode_Get_Attr_Mask(ast) ((ast)->attr)

/**
 * @brief Set the attributes mask of an AST node (does an or operation).
 */
#define BoxASTNode_Set_Attr_Mask(ast, val) do{(ast)->attr = (val);} while(0)

/**
 * @brief Set the attributes mask of an AST node (does an or operation).
 */
#define BoxASTNode_Set_Attr(ast, clr, set) \
  do {(ast)->attr = ((ast)->attr & ~((uint8_t) clr)) \
                  | (uint8_t) (set);} while(0)

/**
 * @brief Whether the node is a type expression.
 */
#define BoxASTNode_Is_Type(ast) \
  ((BoxASTNode_Get_Attr_Mask((ast)) & BOXASTNODEATTR_TYPE) != 0)

/**
 * @brief Get the type of an AST node.
 */
#define BoxASTNode_Get_Type(ast) ((ast)->type)

/**
 * @brief Set the type of an AST node.
 */
#define BoxASTNode_Set_Type(ast, val) do{(ast)->type = (val);} while(0)

/**
 * @brief Beginning of every AST node.
 *
 * The C standard guarantees that the pointer to a structure is always the
 * pointer to its first member. We therefore demand that every AST node is
 * declared like this:
 * @code
 * typedef struct BoxASTNodeXXX {
 *   BOXASTNODEHEAD
 *   // More stuff here ...
 * }
 * @endcode
 * This guarantees that every AST nodes can be safely cast to a generic
 * #BoxASTNode node.
 */
#define BOXASTNODEHEAD \
  BoxASTNode head;

/**
 * @brief Pointer to an AST node.
 */
typedef BoxASTNode *BoxASTNodePtr;


/* Define all nodes types. */
#  include <box/astnodes.h>

/**
 * @brief Box Compiler's Abstract Syntax Tree object.
 */
typedef struct BoxAST_struct BoxAST;

/**
 * @brief Create a new abstract syntax tree.
 */
BOXEXPORT BoxAST *
BoxAST_Create(void);

/**
 * @brief Destroy an abstract syntax tree created with BoxAST_Create().
 */
BOXEXPORT void
BoxAST_Destroy(BoxAST *ast);

/**
 * @brief Get the root node of the tree.
 */
BOXEXPORT BoxASTNode *
BoxAST_Get_Root(BoxAST *ast);

/**
 * @brief Set the root node of the tree.
 */
BOXEXPORT void
BoxAST_Set_Root(BoxAST *ast, BoxASTNode *root);

/**
 * @brief Create an char immediate node.
 */
BOXEXPORT BoxASTNode *
BoxAST_Create_CharImm(BoxAST *ast, BoxSrc *src, BoxChar value);

/**
 * @brief Create an integer immediate node.
 */
BOXEXPORT BoxASTNode *
BoxAST_Create_IntImm(BoxAST *ast, BoxSrc *src, BoxInt value);

/**
 * @brief Create an real immediate node.
 */
BOXEXPORT BoxASTNode *
BoxAST_Create_RealImm(BoxAST *ast, BoxSrc *src, BoxReal value);

/**
 * @brief Create an string immediate node.
 */
BOXEXPORT BoxASTNode *
BoxAST_Create_StrImm(BoxAST *ast, BoxSrc *src,
                     const char *str, uint32_t str_length);

/**
 * @brief  Get a string representation of the given node type.
 */
BOXEXPORT const char *
BoxASTNodeType_To_Str(BoxASTNodeType type);

/**
 * @brief Create the first statement of statement list.
 *
 * @param ast The parent AST object.
 * @param val The value of the statement.
 * @return Return the new statement object.
 */
BOXEXPORT BoxASTNode *
BoxAST_Create_Statement(BoxAST *ast, BoxASTNode *val);

/**
 * @brief Append a statement to the given one.
 *
 * @param ast The parent AST object.
 * @param prev_stmt The statement target of the append operation.
 * @param sep The statement separator.
 * @param val The value of the statement to be appended.
 * @return The new statement object.
 */
BOXEXPORT BoxASTNode *
BoxAST_Append_Statement(BoxAST *ast, BoxASTNode *prev_stmt, BoxASTSep sep,
                        BoxASTNode *val);

/**
 * @brief Create a box node.
 *
 * @param ast The parent AST object.
 * @param parent The value to which the box refers.
 * @param last_stmt The last statement for the box.
 * @return The new box node.
 */
BOXEXPORT BoxASTNode *
BoxAST_Create_Box(BoxAST *ast, BoxASTNode *parent, BoxASTNode *last_stmt);

/**
 * @brief Create a type identifier node.
 */
BOXEXPORT BoxASTNode *
BoxAST_Create_TypeIdfr(BoxAST *ast, BoxSrc *src,
                       const char *name, uint32_t name_length);

/**
 * @brief Create a type identifier node.
 */
BOXEXPORT BoxASTNode *
BoxAST_Create_VarIdfr(BoxAST *ast, BoxSrc *src,
                      const char *name, uint32_t name_length);

/**
 * @brief Create an ignore node.
 */
BOXEXPORT BoxASTNode *
BoxAST_Create_Ignore(BoxAST *ast, BoxASTNode *value);

/**
 * @brief Create a raise node.
 */
BOXEXPORT BoxASTNode *
BoxAST_Create_Raise(BoxAST *ast, BoxASTNode *value);

/**
 * @brief Create a new unary operation.
 */
BOXEXPORT BoxASTNode *
BoxAST_Create_UnOp(BoxAST *ast, BoxASTUnOp op, BoxASTNode *operand);

/**
 * @brief Create a new binary operation.
 */
BOXEXPORT BoxASTNode *
BoxAST_Create_BinOp(BoxAST *ast,
                    BoxASTNode *left, BoxASTBinOp op, BoxASTNode *right);

/**
 * @brief Create a new member (to be appended to a compound).
 */
BOXEXPORT BoxASTNode *
BoxAST_Create_Member(BoxAST *ast, BoxASTNode *expr, BoxASTNode *name);

/**
 * @brief Kinds of compounds.
 */
enum BoxASTCompoundKind_enum {
  BOXASTCOMPOUNDKIND_UNDET,
  BOXASTCOMPOUNDKIND_IDENTITY,
  BOXASTCOMPOUNDKIND_SPECIES,
  BOXASTCOMPOUNDKIND_STRUCT
};

/**
 * @brief Create a new structure/species.
 */
BOXEXPORT BoxASTNode *
BoxAST_Create_Compound(BoxAST *ast, BoxASTNode *memb);

/**
 * @brief Append a member to a structure/species.
 */
BOXEXPORT BoxASTNode *
BoxAST_Append_Member(BoxAST *ast, BoxASTNode *list, BoxASTSep sep,
                     BoxSrc *sep_src, BoxASTNode *memb);

/**
 * @brief Perform final checks and adjustments on a compound node.
 */
BOXEXPORT BoxASTNode *
BoxAST_Close_Compound(BoxASTNode *compound_node);

/**
 * @brief Get a member from a structure value.
 */
BOXEXPORT BoxASTNode *
BoxAST_Create_Get(BoxAST *ast, BoxASTNode *parent, BoxASTNode *member_name);

/**
 * @brief Create a combination definition node.
 */
BOXEXPORT BoxASTNode *
BoxAST_Create_CombDef(BoxAST *ast, BoxASTNode *child, BoxCombType comb_type,
                      BoxASTNode *parent, BoxASTNode *c_name,
                      BoxASTNode *implem);

/**
 * @brief Create an argument node.
 */
BOXEXPORT BoxASTNode *
BoxAST_Create_ArgGet(BoxAST *ast, uint32_t depth);

/**
 * @brief Create a type tag.
 */
BOXEXPORT BoxASTNode *
BoxAST_Create_TypeTag(BoxAST *ast, BoxTypeId type_id);

/**
 * @brief Create a subtype.
 */
BOXEXPORT BoxASTNode *
BoxAST_Create_Subtype(BoxAST *ast, BoxASTNode *parent,
                      BoxASTNode *member_name);

/**
 * @brief Create a keyword.
 */
BOXEXPORT BoxASTNode *
BoxAST_Create_Keyword(BoxAST *ast, BoxASTNode *type);

#endif /* _BOX_AST_H */
