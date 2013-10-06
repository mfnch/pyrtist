%{
/***************************************************************************
 *   Copyright (C) 2006-2013 by Matteo Franchin                            *
 *   fnch@libero.it                                                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "types.h"
#include "defaults.h"
#include "mem.h"
#include "messages.h"
#include "tokenizer.h"
#include "srcpos.h"
#include "ast.h"
#include "parserh.h"
#include "paths.h"

static int yyparse(BoxLex *box_lexer, ASTNode **program_node);
static void yyerror(BoxLex *box_lexer, ASTNode **program_node, char *s);
static void My_Syntax_Error();

#define SRC(ast_node, node_src) \
  do {(ast_node)->src = (node_src);} while(0)

/* Trick to induce yyparse to call BoxLex_Next_Token rather than yylex. */
#define yylex BoxLex_Next_Token

%}

%parse-param {BoxLex *bl}
%lex-param   {BoxLex *bl}
%parse-param {ASTNode **program_node}

/* Possible types for the nodes of the tree */
%union {
  char *        String;
  BoxTypeId     TTag;
  BoxCombType   Combine;
  ASTScope      Scope;
  ASTUnOp       UnaryOperator;
  ASTBinOp      BinaryOperator;
  ASTNodePtr    Node;
  ASTTypeMemb   TypeMemb;
  ASTSep        Sep;
  ASTSelfLevel  SelfLevel;
}

%code requires {
  /** Redefine YYLTYPE so that it contains an extra member, file_name */
  #define YYLTYPE BoxSrc

  #define YYLLOC_DEFAULT(Current, Rhs, N)                       \
    do {                                                        \
      if (N > 0) {                                              \
        (Current).begin = YYRHSLOC(Rhs, 1).begin;               \
        (Current).end   = YYRHSLOC(Rhs, N).end;                 \
      } else                                                    \
        (Current).begin = (Current).end = YYRHSLOC(Rhs, 0).end; \
    } while (0)
}

%token TOK_NEWLINE
%token TOK_ERRSEP

%token TOK_TO TOK_MAPTO

%token <SelfLevel> TOK_SELF

/* List of tokens that have semantical values */
%token <Combine> TOK_COMBINE
%token <TTag> TOK_TTAG

/* List of simple tokens */
%token TOK_INC TOK_DEC TOK_SHL TOK_SHR
%token TOK_EQ TOK_NE TOK_LT TOK_LE TOK_GT TOK_GE
%token TOK_LOR TOK_LAND
%token TOK_APLUS TOK_AMINUS TOK_ATIMES TOK_ADIV TOK_AREM TOK_ABAND TOK_ABXOR
%token TOK_ABOR TOK_ASHL TOK_ASHR
%token TOK_POW
%token TOK_ERR

/* List of tokens with semantical value */
%token <String> TOK_IDENTIFIER TOK_TYPE_IDENT TOK_KEYWORD
%token <Node> TOK_CONSTANT TOK_STRING

/* List of nodes with semantical value */
%type <Sep> sep
%type <UnaryOperator> un_op post_op
%type <BinaryOperator> mul_op add_op shift_op cmp_op eq_op assign_op
%type <Node> expr_list expr_list_memb opt_ident
%type <Node> string_concat prim_expr postfix_expr opt_postfix_expr
%type <Node> unary_expr pow_expr mul_expr add_expr
%type <Node> shift_expr cmp_expr eq_expr band_expr bxor_expr bor_expr
%type <Node> land_expr lor_expr assign_expr expr statement statement_list
%type <Node> at_left procedure opt_c_name procedure_decl
%type <Node> actions opt_actions restrs opt_restrs

/* Starting rule */
%start program

%%

/******************************** SEPARATORS *******************************/
void_sep:
    ','
  | TOK_NEWLINE
  ;

void_seps:
    void_sep
  | void_seps void_sep
  ;

sep:
    void_sep                  {$$ = ASTSEP_VOID;}
  | ';'                       {$$ = ASTSEP_PAUSE;}
  ;

/******************************** OPERATORS ********************************/
un_op:
    '+'                       {$$ = ASTUNOP_PLUS;}
  | '-'                       {$$ = ASTUNOP_NEG;}
  | '!'                       {$$ = ASTUNOP_NOT;}
  | TOK_INC                   {$$ = ASTUNOP_LINC;}
  | TOK_DEC                   {$$ = ASTUNOP_LDEC;}
  | '~'                       {$$ = ASTUNOP_BNOT;}
  ;

post_op:
    TOK_INC                   {$$ = ASTUNOP_RINC;}
  | TOK_DEC                   {$$ = ASTUNOP_RDEC;}
  ;

add_op:
    '+'                       {$$ = ASTBINOP_ADD;}
  | '-'                       {$$ = ASTBINOP_SUB;}
  ;

mul_op:
    '*'                       {$$ = ASTBINOP_MUL;}
  | '/'                       {$$ = ASTBINOP_DIV;}
  | '%'                       {$$ = ASTBINOP_REM;}
  ;

shift_op:
    TOK_SHL                   {$$ = ASTBINOP_SHL;}
  | TOK_SHR                   {$$ = ASTBINOP_SHR;}
  ;

eq_op:
    TOK_EQ                    {$$ = ASTBINOP_EQ;}
  | TOK_NE                    {$$ = ASTBINOP_NE;}
  ;

cmp_op:
    '<'                       {$$ = ASTBINOP_LT;}
  | TOK_LE                    {$$ = ASTBINOP_LE;}
  | '>'                       {$$ = ASTBINOP_GT;}
  | TOK_GE                    {$$ = ASTBINOP_GE;}
  ;

assign_op:
    '='                       {$$ = ASTBINOP_ASSIGN;}
  | TOK_APLUS                 {$$ = ASTBINOP_APLUS;}
  | TOK_AMINUS                {$$ = ASTBINOP_AMINUS;}
  | TOK_ATIMES                {$$ = ASTBINOP_ATIMES;}
  | TOK_AREM                  {$$ = ASTBINOP_AREM;}
  | TOK_ADIV                  {$$ = ASTBINOP_ADIV;}
  | TOK_ASHL                  {$$ = ASTBINOP_ASHL;}
  | TOK_ASHR                  {$$ = ASTBINOP_ASHR;}
  | TOK_ABAND                 {$$ = ASTBINOP_ABAND;}
  | TOK_ABXOR                 {$$ = ASTBINOP_ABXOR;}
  | TOK_ABOR                  {$$ = ASTBINOP_ABOR;}
  ;

/******************************* STRUCTURES ********************************/

opt_ident:
                              {$$ = NULL;}
  | TOK_IDENTIFIER            {$$ = $1;}
  ;

expr_list_memb:
                               {$$ = NULL;}
  |  expr opt_ident            {$$ = $1;}
  ;

expr_list:
    expr_list_memb             {$$ = NULL;}
  | expr_list void_sep
                expr_list_memb {$$ = $3;}
  ;

/******************************* ARITHMETICS *******************************/
string_concat:
    TOK_STRING                   {$$ = $1;}
  | string_concat TOK_STRING     {$$ = ASTNodeString_Concat($1, $2);
                                  SRC($$, @$);}
  ;

prim_expr:
    TOK_CONSTANT                 {$$ = $1;}
  | TOK_KEYWORD                  {$$ = ASTNodeInstance_New(
                                         ASTNodeTypeName_New($1, 0));
                                  Box_Mem_Free($1); SRC($$, @$);}
  | string_concat                {$$ = $1;}
  | TOK_IDENTIFIER               {$$ = ASTNodeVar_New($1, 0);
                                  Box_Mem_Free($1); SRC($$, @$);}
  | TOK_TYPE_IDENT               {$$ = NULL;}
  | '?'                          {$$ = NULL;}
  | TOK_SELF                     {$$ = ASTNodeSelfGet_New($1); SRC($$, @$);}
  | '(' expr_list ')'            {$$ = $2;}
  ;

postfix_expr:
    prim_expr                    {$$ = $1;}
  | postfix_expr '(' expr ')'    {$$ = ASTNodeArrayGet_New($1, $3); SRC($$, @$);}
  | procedure_decl               {$$ = $1;}
  | postfix_expr
          '[' statement_list ']' {$$ = ASTNodeBox_Set_Parent($3, $1); SRC($$, @$);}
  | opt_postfix_expr '.' TOK_IDENTIFIER
                                 {$$ = ASTNodeMemberGet_New($1, $3, 0);
                                  Box_Mem_Free($3); SRC($$, @$);}
  | postfix_expr post_op         {$$ = ASTNodeUnOp_New($2, $1); SRC($$, @$);}
  ;

opt_postfix_expr:
                                 {$$ = NULL;}
  | postfix_expr                 {$$ = $1;}
  ;

unary_expr:
    postfix_expr                 {$$ = $1;}
  | un_op unary_expr             {$$ = ASTNodeUnOp_New($1, $2); SRC($$, @$);}
  | '^' unary_expr               {$$ = ASTNodeRaise_New($2); SRC($$, @$);}
;

pow_expr:
    unary_expr                   {$$ = $1;}
  | pow_expr TOK_POW unary_expr  {$$ = ASTNodeBinOp_New(ASTBINOP_POW, $1, $3);
                                  SRC($$, @$);}
  ;

mul_expr:
    pow_expr                     {$$ = $1;}
  | mul_expr mul_op pow_expr     {$$ = ASTNodeBinOp_New($2, $1, $3); SRC($$, @$);}
  ;

add_expr:
    mul_expr                     {$$ = $1;}
  | add_expr add_op mul_expr     {$$ = ASTNodeBinOp_New($2, $1, $3); SRC($$, @$);}
  ;

shift_expr:
    add_expr                     {$$ = $1;}
  | shift_expr shift_op add_expr {$$ = ASTNodeBinOp_New($2, $1, $3); SRC($$, @$);}
  ;

cmp_expr:
    shift_expr                   {$$ = $1;}
  | cmp_expr cmp_op shift_expr   {$$ = ASTNodeBinOp_New($2, $1, $3); SRC($$, @$);}
  ;

eq_expr:
    cmp_expr                     {$$ = $1;}
  | eq_expr eq_op cmp_expr       {$$ = ASTNodeBinOp_New($2, $1, $3); SRC($$, @$);}
  ;

band_expr:
    eq_expr                      {$$ = $1;}
  | band_expr '&' eq_expr        {$$ = ASTNodeBinOp_New(ASTBINOP_BAND, $1, $3); SRC($$, @$);}
  ;

bxor_expr:
    band_expr                    {$$ = $1;}
  | bxor_expr '^' band_expr      {$$ = ASTNodeBinOp_New(ASTBINOP_BXOR, $1, $3); SRC($$, @$);}
  ;

bor_expr:
    bxor_expr                    {$$ = $1;}
  | bor_expr '|' bxor_expr       {$$ = ASTNodeBinOp_New(ASTBINOP_BOR, $1, $3); SRC($$, @$);}
  ;

land_expr:
    bor_expr                     {$$ = $1;}
  | land_expr TOK_LAND bor_expr  {$$ = ASTNodeBinOp_New(ASTBINOP_LAND, $1, $3); SRC($$, @$);}
  ;

lor_expr:
    land_expr                    {$$ = $1;}
  | lor_expr TOK_LOR land_expr   {$$ = ASTNodeBinOp_New(ASTBINOP_LOR, $1, $3); SRC($$, @$);}
  ;

assign_expr:
    lor_expr                     {$$ = $1;}
  | lor_expr assign_op
                     assign_expr {$$ = ASTNodeBinOp_New($2, $1, $3); SRC($$, @$);}
  ;

expr:
    assign_expr                  {$$ = $1;}
  ;

/****************** PROCEDURE DECLARATION AND DEFINITION *******************/
/* Definition and declaration of procedures */

actions:
    expr                         {$$ = NULL;}
  | actions void_seps expr       {$$ = NULL;}
  ;

restrs:
                                 {$$ = NULL;}
  | expr                         {$$ = NULL;}
  | restrs void_seps expr        {$$ = NULL;}
  ;

opt_actions:
                                 {$$ = NULL;}
  | TOK_MAPTO actions            {$$ = NULL;}
  ;

opt_restrs:
                                 {$$ = NULL;}
  | '(' restrs opt_actions')'    {$$ = NULL;}
  ;

 /* left side of @ */
at_left:
    TOK_TYPE_IDENT               {$$ = $1;}
  | TOK_TTAG                     {$$ = ASTNodeTypeTag_New($1); SRC($$, @$);}
  ;

procedure:
    at_left TOK_COMBINE opt_restrs TOK_TYPE_IDENT
                                 {$$ = ASTNodeProcDef_New($1, $2, $3);
                                  SRC($$, @$);}
  ;

opt_c_name:
                                 {$$ = NULL;}
  | string_concat                {$$ = $1;}
  ;

procedure_decl:
    procedure opt_c_name '?'     {$$ = ASTNodeProcDef_Set($1, $2, NULL); SRC($$, @$);}
  | procedure opt_c_name
          '[' statement_list ']' {$$ = ASTNodeProcDef_Set($1, $2, $4); SRC($$, @$);}
  ;

/************************ STATEMENT LISTS AND BOXES ************************/
/* Syntax for the body of the program */
statement:
                                 {$$ = NULL;}
  | expr                         {$$ = ASTNodeStatement_New($1); SRC($$, @$);}
  | '\\' expr                    {$$ = ASTNodeStatement_New(
                                       ASTNodeIgnore_New($2, 1)); SRC($$, @$);}
  | '[' statement_list ']'       {$$ = ASTNodeStatement_New($2); SRC($$, @$);}
  | error sep                    {$$ = ASTNodeStatement_New(ASTNodeError_New());
                                  SRC($$, @$);
                                  My_Syntax_Error(& @$, NULL);
                                  assert(yychar == YYEMPTY); yychar = (int) ',';
                                  yyerrok;}
  ;

statement_list:
    statement                    {$$ = ASTNodeBox_New(NULL, $1); SRC($$, @$);}
  | statement_list sep statement {$$ = ASTNodeBox_Add_Sep($1, $2);
                                  $$ = ASTNodeBox_Add_Statement($1, $3);
                                  SRC($$, @$);}
  ;

program:
    statement_list               {*program_node = $1;}
  ;

%%

/* error function */
static void yyerror(BoxLex *box_lexer, ASTNode **program_node, char* s) {
  /* Do nothing, as - at the moment - we report error in error action */
}

static void My_Syntax_Error(YYLTYPE *src, char *s) {
  BoxSrc *prev_src_of_err;
  prev_src_of_err = Msg_Set_Src(src);
  if (s == NULL)
    MSG_ERROR("Syntax error.");
  else
    MSG_ERROR("%s", s);
  (void) Msg_Set_Src(prev_src_of_err);
}

ASTNode *Parser_Parse(FILE *in, const char *in_name,
                      const char *auto_include, BoxPaths *paths) {
  ASTNode *program_node = NULL;
  BoxTask parse_error;
  BoxSrcName *file_names = NULL;
  BoxLex *box_lexer;

  box_lexer = BoxLex_Create(paths);
  assert(box_lexer != NULL);

  in_name = (in_name != NULL) ? in_name : "<stdin>";
  in = (in != NULL) ? in : stdin;
  parse_error = BoxLex_Begin_Include_FILE(box_lexer, in, in_name);

  if (auto_include != NULL && parse_error == BOXTASK_OK)
    parse_error = BoxLex_Begin_Include(box_lexer, auto_include);

  if (parse_error == BOXTASK_OK)
    parse_error = yyparse(box_lexer, & program_node);

  file_names = BoxLex_Destroy(box_lexer);

  if (parse_error == BOXTASK_OK) {
    assert(program_node->type == ASTNODETYPE_BOX);
    program_node->attr.box.file_names = file_names;
    return program_node;

  } else {
    if (parse_error == BOXTASK_FAILURE)
      MSG_ERROR("Parse error at end of input.");

    ASTNode_Destroy(program_node);
    BoxSrcName_Destroy(file_names);
    return NULL;
  }
}
