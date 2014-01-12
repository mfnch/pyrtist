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


/**
 * @brief Separator between statements.
 */
typedef enum {
  BOXASTSEP_NONE=0,
  BOXASTSEP_VOID,
  BOXASTSEP_PAUSE,
  BOXASTSEP_ARROW,
} BoxASTSep;

/**
 * @brief Unary operators.
 */
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

/**
 * @brief Binary operators.
 */
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

/**
 * @brief Integer indicating the self level: 0 --> $, 1 --> $$, 2 --> $$$, etc.
 */
typedef BoxInt ASTSelfLevel;

/**
 * @brief Return the string representation of the given unary operator.
 */
BOXEXPORT const char *
BoxASTUnOp_To_String(BoxASTUnOp op);

/**
 * @brief Return the string representation of the given binary operator.
 */
BOXEXPORT const char *
BoxASTBinOp_To_String(BoxASTBinOp op);

/**
 * @brief Return whether the given operator is a "right" unary operator.
 *
 * meaning that it returns its operand before modifying it.
 */
BOXEXPORT BoxBool
BoxASTUnOp_Is_Right(BoxASTUnOp op);

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
BoxAST_Close_Compound(BoxAST *ast, BoxASTNode *compound_node);

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
