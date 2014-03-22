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
#include <stdarg.h>

#include <box/mem.h>
#include <box/print.h>
#include <box/srcpos.h>

#include <box/ast_priv.h>


#define MY_COPY_SRC(lhs, rhs)                             \
  do {BoxASTNode *src_node = (rhs);                       \
    if (src_node) lhs->head.src = src_node->head.src;     \
    else BoxSrc_Init(& lhs->head.src);} while(0)

#define MY_PROPAGATE_SRC(container, fst, lst)                     \
  do {(container)->head.src.begin = (fst)->head.src.begin;        \
      (container)->head.src.end = (lst)->head.src.end;} while(0)

#define MY_PROPAGATE_TYPE(dst, src)                             \
  BoxASTNode_Set_Attr((dst), 0,                                 \
    (BoxASTNode_Get_Attr_Mask(src) & BOXASTNODEATTR_TYPE))


const char *
BoxASTUnOp_To_String(BoxASTUnOp op)
{
  switch(op) {
  case BOXASTUNOP_PLUS: return "+";
  case BOXASTUNOP_NEG: return "-";
  case BOXASTUNOP_LINC: return "++";
  case BOXASTUNOP_LDEC: return "--";
  case BOXASTUNOP_RINC: return "++";
  case BOXASTUNOP_RDEC: return "--";
  case BOXASTUNOP_NOT: return "!";
  case BOXASTUNOP_BNOT: return "~";
  case BOXASTUNOP_RAISE: return "^";
  case BOXASTUNOP_REF: return "&";
  case BOXASTUNOP_DEREF: return "*";
  default: break;
  }
  return "?";
}

BoxBool
BoxASTUnOp_Is_Right(BoxASTUnOp op)
{
 switch(op) {
 case BOXASTUNOP_RINC:
 case BOXASTUNOP_RDEC:
   return BOXBOOL_TRUE;
 default:
   return BOXBOOL_FALSE;
 }
  return BOXBOOL_FALSE;
}

const char *
BoxASTBinOp_To_String(BoxASTBinOp op)
{
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

void
BoxAST_Init(BoxAST *ast, BoxLogger *logger)
{
  BoxAllocPool_Init(& ast->pool, sizeof(BoxASTNode)*16);
  ast->root = NULL;
  ast->logger = logger;
  ast->src_map = BoxSrcMap_Create();
  ast->src_map_ok = BOXBOOL_TRUE;
  ast->is_sane = BOXBOOL_TRUE;
  BoxIndex_Init(& ast->src_names, 50);

#if 0
#define BOXASTNODE_DEF(NODE, Node)                                      \
    printf("%s size=%zu, alignment=%zu\n",                              \
           #Node, sizeof(BoxASTNode##Node),                             \
           __alignof__(BoxASTNode##Node));
#include "astnodes.h"
#undef BOXASTNODE_DEF
#endif
}

void
BoxAST_Finish(BoxAST *ast)
{
  BoxSrcMap_Destroy(ast->src_map);
  BoxIndex_Finish(& ast->src_names);
  BoxAllocPool_Finish(& ast->pool);
}

/* Create a new abstract syntax tree. */
BoxAST *
BoxAST_Create(BoxLogger *logger)
{
  BoxAST *ast = (BoxAST *) Box_Mem_Alloc(sizeof(BoxAST));
  if (ast)
    BoxAST_Init(ast, logger);
  return ast;
}

/* Destroy an abstract syntax tree created with BoxAST_Create(). */
void
BoxAST_Destroy(BoxAST *ast)
{
  if (ast) {
    BoxAST_Finish(ast);
    Box_Mem_Free(ast);
  }
}

BoxBool
BoxAST_Is_Sane(BoxAST *ast)
{
  return ast->is_sane;
}

/* Message logging. */
void
BoxAST_Log_VA(BoxAST *ast, BoxSrc *src, BoxLogLevel lev,
              const char *fmt, va_list ap)
{
  BoxSrcFullPos begin_fp, end_fp;

  if (lev >= BOXLOGLEVEL_ERROR)
    ast->is_sane = BOXBOOL_FALSE;

  if (src && BoxAST_Get_Src_Map(ast, src->begin, & begin_fp)
      && BoxAST_Get_Src_Map(ast, src->end, & end_fp)) {
    BoxLogPos begin_lp, end_lp;

    begin_lp.file_name = BoxAST_Get_File_Name(ast, begin_fp.file_num);
    begin_lp.line = begin_fp.line;
    begin_lp.col = begin_fp.col;

    end_lp.file_name = (end_fp.file_num == begin_fp.file_num ?
                        begin_lp.file_name :
                        BoxAST_Get_File_Name(ast, end_fp.file_num));
    end_lp.line = end_fp.line;
    end_lp.col = end_fp.col;
    BoxLogger_Log_VA(ast->logger, & begin_lp, & end_lp, lev, fmt, ap);
    return;
  }

  BoxLogger_Log_VA(ast->logger, NULL, NULL, lev, fmt, ap);
}

/* Generic error reporting for an AST. */
void
BoxAST_Log(BoxAST *ast, BoxSrc *src, BoxLogLevel lev, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  BoxAST_Log_VA(ast, src, lev, fmt, ap);
  va_end(ap);
}

/* Error reporting for groups of AST nodes. */
void
BoxASTNode_Log(BoxAST *ast, BoxASTNode *node, BoxLogLevel lev,
               const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  BoxAST_Log_VA(ast, & node->head.src, lev, fmt, ap);
  va_end(ap);
}

/* Provide a correspondence between linear and full source positions. */
void
BoxAST_Store_Src_Map(BoxAST *ast, BoxSrcLinPos lp, BoxSrcFullPos *fp)
{
  if (!BoxSrcMap_Store_FP(ast->src_map, lp, fp)) {
    if (ast->src_map_ok) {
      ast->src_map_ok = BOXBOOL_FALSE;
      BoxAST_Log(ast, NULL, BOXLOGLEVEL_WARNING,
                 "Cannot store source mapping. Positions in source file may "
                 "be inaccurate or wrong.");
    }
  }
}

/* Map from linear to full source positions. */
BoxBool
BoxAST_Get_Src_Map(BoxAST *ast, BoxSrcLinPos lp, BoxSrcFullPos *fp)
{
  if (BoxSrcMap_Map(ast->src_map, lp,
                    & fp->file_num, & fp->line, & fp->col))
    return BOXBOOL_TRUE;

  ast->src_map_ok = BOXBOOL_FALSE;
  BoxAST_Log(ast, NULL, BOXLOGLEVEL_WARNING,
             "Cannot read source mapping. Positions in source file may "
             "be inaccurate or wrong.");
  return BOXBOOL_FALSE;
}

uint32_t BoxAST_Get_File_Num(BoxAST *ast, const char *name)
{
  return BoxIndex_Get_Idx_From_Name(& ast->src_names, name);
}

const char *
BoxAST_Get_File_Name(BoxAST *ast, uint32_t num)
{
  return BoxIndex_Get_Name_From_Idx(& ast->src_names, num);
}

/* Set the root node of the tree. */
BoxASTNode *
BoxAST_Get_Root(BoxAST *ast)
{
  return ast->root;
}

/* Set the root node of the tree. */
void
BoxAST_Set_Root(BoxAST *ast, BoxASTNode *root)
{
  ast->root = root;
}

/* Get size and alignment for the given node type. */
static uint32_t
My_Get_Node_Size(BoxASTNodeType type, uint32_t *alignment)
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
const char *
BoxASTNodeType_To_Str(BoxASTNodeType type)
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

void *
BoxAST_Create_Node(BoxAST *ast, BoxASTNodeType type)
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
void *
BoxAST_Create_Var_Node(BoxAST *ast, BoxASTNodeType type,
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

BoxBool
BoxASTNode_Set_Src(BoxASTNode *node, BoxSrcIdx begin, BoxSrcIdx end)
{
  if (node) {
    if (begin & end) {
      node->head.src.begin = begin;
      node->head.src.end = end;
      return BOXBOOL_TRUE;
    }
    node->head.src.begin = 0;
  }
  return BOXBOOL_FALSE;
}

BoxASTNode *
BoxAST_Create_CharImm(BoxAST *ast, BoxSrc *src, BoxChar value)
{
  BoxASTNode *node = BoxAST_Create_Node(ast, BOXASTNODETYPE_CHAR_IMM);
  if (node) {
    node->head.src = *src;
    ((BoxASTNodeCharImm *) node)->value = value;
  }
  return node;
}

BoxASTNode *
BoxAST_Create_IntImm(BoxAST *ast, BoxSrc *src, BoxInt value)
{
  BoxASTNode *node = BoxAST_Create_Node(ast, BOXASTNODETYPE_INT_IMM);
  if (node) {
    node->head.src = *src;
    ((BoxASTNodeIntImm *) node)->value = value;
  }
  return node;
}

BoxASTNode *
BoxAST_Create_RealImm(BoxAST *ast, BoxSrc *src, BoxReal value)
{
  BoxASTNode *node = BoxAST_Create_Node(ast, BOXASTNODETYPE_REAL_IMM);
  if (node) {
    node->head.src = *src;
    ((BoxASTNodeRealImm *) node)->value = value;
  }
  return node;
}

BoxASTNode *
BoxAST_Create_StrImm(BoxAST *ast, BoxSrc *src,
                     const char *str, uint32_t str_length)
{
  char *str_copy = BoxAllocPool_Str_NDup(& ast->pool, str, str_length);
  BoxASTNode *node = BoxAST_Create_Node(ast, BOXASTNODETYPE_STR_IMM);
  if (node && str_copy) {
    node->head.src = *src;
    ((BoxASTNodeStrImm *) node)->str = str_copy;
  }
  return node;
}

/* Create the first statement of statement list. */
BoxASTNode *
BoxAST_Create_Statement(BoxAST *ast, BoxASTNode *val)
{
  BoxASTNode *node = BoxAST_Create_Node(ast, BOXASTNODETYPE_STATEMENT);
  if (node) {
    BoxASTNodeStatement *stmt = (BoxASTNodeStatement *) node;
    if (val)
      node->head.src = val->head.src;
    stmt->value = val;
    stmt->next = stmt;
    stmt->sep = 0;
  }
  return node;
}

/* Append a statement to the given one. */
BoxASTNode *
BoxAST_Append_Statement(BoxAST *ast, BoxASTNode *prev_stmt_node,
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
BoxASTNode *
BoxAST_Create_Box(BoxAST *ast, BoxASTNode *parent, BoxASTNode *last_stmt_node)
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
BoxASTNode *
BoxAST_Create_TypeIdfr(BoxAST *ast, BoxSrc *src,
                       const char *name, uint32_t name_length)
{
  uint32_t extra_size = name_length + 1;
  BoxASTNode *node;

  assert(extra_size > 1);
  node = BoxAST_Create_Var_Node(ast, BOXASTNODETYPE_TYPE_IDFR,
                                extra_size, NULL);
  if (node) {
    BoxASTNodeTypeIdfr *idfr = (BoxASTNodeTypeIdfr *) node;
    node->head.src = *src;
    memcpy(& idfr->name[0], name, name_length);
    idfr->name[name_length] = '\0';
  }

  return node;
}

/* Create a type/variable identifier node. */
BoxASTNode *
BoxAST_Create_VarIdfr(BoxAST *ast, BoxSrc *src,
                      const char *name, uint32_t name_length)
{
  uint32_t extra_size = name_length + 1;
  BoxASTNode *node;

  assert(extra_size > 1);
  node = BoxAST_Create_Var_Node(ast, BOXASTNODETYPE_VAR_IDFR,
                                extra_size, NULL);
  if (node) {
    BoxASTNodeVarIdfr *idfr = (BoxASTNodeVarIdfr *) node;
    node->head.src = *src;
    memcpy(& idfr->name[0], name, name_length);
    idfr->name[name_length] = '\0';
  }

  return node;
}

/* Create an ignore node. */
BoxASTNode *
BoxAST_Create_Ignore(BoxAST *ast, BoxASTNode *value)
{
  BoxASTNode *node = BoxAST_Create_Node(ast, BOXASTNODETYPE_IGNORE);
  if (node) {
    BoxASTNodeIgnore *ignore = (BoxASTNodeIgnore *) node;
    node->head.src = value->head.src;
    ignore->value = value;
  }
  return node;
}

/* Create a new unary operation. */
BoxASTNode *
BoxAST_Create_UnOp(BoxAST *ast, BoxASTUnOp op, BoxASTNode *value)
{
  BoxASTNodeType node_type = (BoxASTNode_Is_Type(value)) ?
    BOXASTNODETYPE_UN_TYPE_OP : BOXASTNODETYPE_UN_OP;
  BoxASTNode *node = BoxAST_Create_Node(ast, node_type);
  MY_PROPAGATE_TYPE(node, value);
  if (node) {
    BoxASTNodeUnOp *un_op = (BoxASTNodeUnOp *) node;
    MY_COPY_SRC(node, value);
    un_op->value = value;
    un_op->op = op;
  }
  return node;
}

/* Create a new binary operation. */
BoxASTNode *
BoxAST_Create_BinOp(BoxAST *ast, BoxASTNode *lhs,
                    BoxASTBinOp op, BoxASTNode *rhs)
{
  BoxASTNode *node = BoxAST_Create_Node(ast, BOXASTNODETYPE_BIN_OP);
  if (node) {
    BoxASTNodeBinOp *bin_op = (BoxASTNodeBinOp *) node;
    MY_PROPAGATE_SRC(bin_op, lhs, rhs);
    bin_op->lhs = lhs;
    bin_op->rhs = rhs;
    bin_op->op = op;
  }
  return node;
}

/* Create a new structure/species.
 * NOTE: a compound expression is able to represent structure values/types and
 *   species types. Having one single AST node to represent such a variety of
 *   language constructs does shift the burden of detecting illegal usage of
 *   these construct here below. The functions below are indeed in charge of
 *   carrying out some checks that normally would be done during parsing. This
 *   is the price to pay for extra flexibility in the grammar...
 */
BoxASTNode *
BoxAST_Create_Compound(BoxAST *ast, BoxASTNode *memb)
{
  BoxASTNode *node = BoxAST_Create_Node(ast, BOXASTNODETYPE_COMPOUND);
  if (node) {
    BoxASTNodeCompound *spec = (BoxASTNodeCompound *) node;
    MY_COPY_SRC(node, memb);
    spec->kind = BOXASTCOMPOUNDKIND_UNDET;
    spec->sep = BOXASTSEP_NONE;
    spec->memb = NULL;
    if (memb)
      return BoxAST_Append_Member(ast, node, BOXASTSEP_NONE, NULL, memb);
  }
  return node;
}

/* Create a new structure/species member. */
BoxASTNode *
BoxAST_Create_Member(BoxAST *ast, BoxASTNode *expr, BoxASTNode *name)
{
  BoxASTNode *node = BoxAST_Create_Node(ast, BOXASTNODETYPE_MEMBER);
  if (node) {
    MY_COPY_SRC(node, name);
    ((BoxASTNodeMember *) node)->expr = expr;
    ((BoxASTNodeMember *) node)->name = name;
  }
  return node;
}

#define BoxASTNode_Mark_Err(stuff)

/* Append a member to an expression list. */
BoxASTNode *
BoxAST_Append_Member(BoxAST *ast, BoxASTNode *compound_node, BoxASTSep sep,
                     BoxSrc *sep_src, BoxASTNode *memb_node)
{
  BoxASTNodeCompound *compound;
  BoxASTNodeMember *memb;

  if (!compound_node)
    return NULL;

  assert(BoxASTNode_Get_Type(compound_node) == BOXASTNODETYPE_COMPOUND);
  compound = (BoxASTNodeCompound *) compound_node;

  /* The code below contracts a list of consecutive separators. */
  if (sep == BOXASTSEP_VOID) {
    if (compound->kind == BOXASTCOMPOUNDKIND_IDENTITY
        && compound->sep == BOXASTSEP_NONE) {
      compound->kind = BOXASTCOMPOUNDKIND_STRUCT;
      compound->sep = sep;
    }
  } else if (sep > BOXASTSEP_VOID) {
    assert(sep == BOXASTSEP_ARROW);
    if (compound->sep > BOXASTSEP_VOID)
      BoxAST_Log(ast, & compound->sep_src, BOXLOGLEVEL_ERROR,
                 "Duplicate separator in compound expression");
    compound->sep = sep;
    compound->sep_src = (sep_src) ? *sep_src : ((BoxSrc) {0, 0});
  }

  if (!memb_node)
    return compound_node;

  MY_PROPAGATE_SRC(compound_node, compound_node, memb_node);

  assert(BoxASTNode_Get_Type(memb_node) == BOXASTNODETYPE_MEMBER);
  memb = (BoxASTNodeMember *) memb_node;

  /* Change the "parse" state and check for errors with separators. */
  switch (compound->kind) {
  case BOXASTCOMPOUNDKIND_UNDET:
    compound->kind = BOXASTCOMPOUNDKIND_IDENTITY;
    break;
  case BOXASTCOMPOUNDKIND_IDENTITY:
    compound->kind = ((compound->sep > BOXASTSEP_VOID) ?
                      BOXASTCOMPOUNDKIND_SPECIES : BOXASTCOMPOUNDKIND_STRUCT);
    break;
  case BOXASTCOMPOUNDKIND_SPECIES:
    if (compound->sep != BOXASTSEP_ARROW) {
      BoxAST_Log(ast, & compound->sep_src, BOXLOGLEVEL_ERROR,
                 "Invalid separator in species definition");
      //BoxASTNode_Mark_Err(compound_node);
    }
    break;
  case BOXASTCOMPOUNDKIND_STRUCT:
    if (compound->sep > BOXASTSEP_VOID) {
      BoxAST_Log(ast, & compound->sep_src, BOXLOGLEVEL_ERROR,
                 "Invalid separator in structure");
      //BoxASTNode_Mark_Err(compound_node);
    }
    break;
  default:
    abort();
  }

  /* Add the member to the compound. Here compound->memb (when != NULL) points
   * to the last member of the compound and compound->memb->next points to the
   * first member.
   */
  if (compound->memb) {
    memb->next = compound->memb->next;
    compound->memb->next = memb;
    compound->memb = memb;
  } else {
    compound->memb = memb;
    memb->next = memb;
  }

  compound->sep = BOXASTSEP_NONE;
  return compound_node;
}

/* Perform final checks on a compound node. */
BoxASTNode *
BoxAST_Close_Compound(BoxAST *ast, BoxASTNode *compound_node)
{
  BoxASTNodeCompound *compound;
  BoxASTNodeMember *memb;

  if (!compound_node)
    return NULL;

  assert(BoxASTNode_Get_Type(compound_node) == BOXASTNODETYPE_COMPOUND);
  compound = (BoxASTNodeCompound *) compound_node;

  memb = compound->memb;
  assert(!memb
         || BoxASTNode_Get_Type((BoxASTNode *) memb) == BOXASTNODETYPE_MEMBER);

  switch (compound->kind) {
  case BOXASTCOMPOUNDKIND_UNDET:
    BoxAST_Log(ast, NULL, BOXLOGLEVEL_ERROR, "Empty parentheses");
    break;
  case BOXASTCOMPOUNDKIND_IDENTITY:
    if (memb->name)
      BoxASTNode_Log(ast, memb->name, BOXLOGLEVEL_ERROR,
                     "Spurious member name, but this is not a structure. "
                     "Hint: Structures must contain at least one separator");
    MY_PROPAGATE_TYPE(compound_node, memb->expr);
    break;
  case BOXASTCOMPOUNDKIND_SPECIES:
    assert(compound->memb);
    compound->memb = memb->next; /* First member. */
    memb->next = NULL;           /* NULL terminate. */
    for (memb = compound->memb; memb; memb = memb->next) {
      if (memb->name)
        BoxASTNode_Log(ast, memb->name, BOXLOGLEVEL_ERROR,
                       "Unexpected member name in species");
      if (!BoxASTNode_Is_Type(memb->expr))
        BoxASTNode_Log(ast, memb->expr, BOXLOGLEVEL_ERROR,
                       "Unexpected expression in species");
    }
    BoxASTNode_Set_Attr(compound_node, 0, BOXASTNODEATTR_TYPE);
    break;
  case BOXASTCOMPOUNDKIND_STRUCT:
    compound->memb = memb->next; /* First member. */
    memb->next = NULL;           /* NULL terminate. */
    MY_PROPAGATE_TYPE(compound_node, compound->memb->expr);
    if (BoxASTNode_Is_Type((BoxASTNode *) compound->memb->expr)) {
      for (memb = compound->memb; memb; memb = memb->next) {
        if (memb->name) {
          if (!BoxASTNode_Is_Type(memb->expr))
            BoxASTNode_Log(ast, memb->expr, BOXLOGLEVEL_ERROR,
                           "Invalid expression in structure");
        } else if (!BoxASTNode_Is_Type(memb->expr)) {
          if (BoxASTNode_Get_Type(memb->expr) == BOXASTNODETYPE_VAR_IDFR) {
            memb->name = memb->expr;
            memb->expr = NULL;
          } else
            BoxASTNode_Log(ast, memb->expr, BOXLOGLEVEL_ERROR,
                           "Invalid expression in structure");
        }
      }
    } else {
      for (memb = compound->memb; memb; memb = memb->next) {
        if (memb->name)
          BoxASTNode_Log(ast, memb->name, BOXLOGLEVEL_ERROR,
                         "Spurious member name in structure");
        if (BoxASTNode_Is_Type(memb->expr))
          BoxASTNode_Log(ast, memb->expr, BOXLOGLEVEL_ERROR,
                         "Unexpected type expression in structure");
      }
    }
    break;
  }

  return compound_node;
}

/* Create a new binary operation. */
BoxASTNode *
BoxAST_Create_Get(BoxAST *ast, BoxASTNode *parent, BoxASTNode *member_name)
{
  BoxASTNode *node = BoxAST_Create_Node(ast, BOXASTNODETYPE_GET);
  if (node) {
    assert(member_name
           && BoxASTNode_Get_Type(member_name) == BOXASTNODETYPE_VAR_IDFR);
    MY_PROPAGATE_SRC(node, (parent) ? parent : member_name, member_name);
    ((BoxASTNodeGet *) node)->parent = parent;
    ((BoxASTNodeGet *) node)->name = (BoxASTNodeVarIdfr *) member_name;
  }
  return node;
}

/* Create a combination definition node. */
BoxASTNode *
BoxAST_Create_CombDef(BoxAST *ast, BoxASTNode *child, BoxCombType comb_type,
                      BoxASTNode *parent, BoxASTNode *c_name,
                      BoxASTNode *implem)
{
  BoxASTNode *node = BoxAST_Create_Node(ast, BOXASTNODETYPE_COMB_DEF);
  if (node) {
    BoxASTNodeCombDef *spec = (BoxASTNodeCombDef *) node;
    BoxASTNode *rightmost = (implem) ? implem : ((c_name) ? c_name : parent);
    assert(rightmost);
    MY_PROPAGATE_SRC(node, child, rightmost);
    spec->child = child;
    spec->comb_type = (uint8_t) comb_type;
    spec->parent = parent;
    spec->c_name = c_name;
    spec->implem = implem;
  }
  return node;
}

/* Create an argument node. */
BoxASTNode *
BoxAST_Create_ArgGet(BoxAST *ast, BoxSrc *src, uint32_t depth)
{
  BoxASTNode *node = BoxAST_Create_Node(ast, BOXASTNODETYPE_ARG_GET);
  if (node) {
    node->head.src = *src;
    ((BoxASTNodeArgGet *) node)->depth = depth;
  }
  return node;
}

/* Create a type tag. */
BoxASTNode *
BoxAST_Create_TypeTag(BoxAST *ast, BoxSrc *src, BoxTypeId type_id)
{
  BoxASTNode *node = BoxAST_Create_Node(ast, BOXASTNODETYPE_TYPE_TAG);
  if (node) {
    node->head.src = *src;
    ((BoxASTNodeTypeTag *) node)->type_id = (uint32_t) type_id;
  }
  return node;
}

/* Create a subtype. */
BoxASTNode *
BoxAST_Create_Subtype(BoxAST *ast, BoxASTNode *parent, BoxASTNode *member_name)
{
  BoxASTNode *node = BoxAST_Create_Node(ast, BOXASTNODETYPE_SUBTYPE);
  if (node) {
    assert(BoxASTNode_Get_Type(member_name) == BOXASTNODETYPE_TYPE_IDFR);
    MY_PROPAGATE_SRC(node, (parent) ? parent : member_name, member_name);
    ((BoxASTNodeSubtype *) node)->parent = parent;
    ((BoxASTNodeSubtype *) node)->name = (BoxASTNodeTypeIdfr *) member_name;
    if (parent)
      MY_PROPAGATE_TYPE(node, parent);
  }
  return node;
}

/* Create a keyword. */
BoxASTNode *
BoxAST_Create_Keyword(BoxAST *ast, BoxASTNode *type)
{
  BoxASTNode *node = BoxAST_Create_Node(ast, BOXASTNODETYPE_KEYWORD);
  if (node) {
    assert(BoxASTNode_Get_Type(type) == BOXASTNODETYPE_TYPE_IDFR);
    MY_COPY_SRC(node, type);
    ((BoxASTNodeKeyword *) node)->type = (BoxASTNodeTypeIdfr *) type;
    BoxASTNode_Set_Attr(node, 0, BOXASTNODEATTR_TYPE);
  }
  return node;
}
