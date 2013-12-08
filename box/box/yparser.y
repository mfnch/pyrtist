%{
/***************************************************************************
 *   Copyright (C) 2006-2013 by Matteo Franchin                            *
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
#include "srcpos.h"
#include "ast.h"
#include "parser.h"
#include "paths.h"


static int yyparse(BoxParser *parser, BoxAST *ast);
static void yyerror(BoxParser *parser, BoxAST *ast, char *s);
static void My_Syntax_Error();

/* Trick to induce yyparse to call BoxParser_Get_Next_Token rather than
 * yylex.
 */
#define yylex BoxParser_Get_Next_Token

%}

%parse-param {BoxParser *parser}
%lex-param   {BoxParser *parser}
%parse-param {BoxAST *ast}

/* Possible types for the nodes of the tree */
%union {
  char *        String;
  BoxTypeId     TTag;
  BoxCombType   Combine;
  ASTScope      Scope;
  ASTUnOp       UnaryOperator;
  ASTBinOp      BinaryOperator;
  ASTNodePtr    Node;
  BoxASTNodePtr NewNode;
  ASTTypeMemb   TypeMemb;
  BoxASTSep     Sep;
  ASTSelfLevel  SelfLevel;
}

%code requires {
  /* Redefine YYLTYPE so that we use our - more efficient - representation
   * for positions (serial positions rather than file-name/line/column).
   */
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
%token TOK_EQ TOK_NE TOK_LT TOK_LE TOK_GT TOK_GE TOK_LOR TOK_LAND
%token TOK_APLUS TOK_AMINUS TOK_ATIMES TOK_ADIV TOK_AREM TOK_ABAND TOK_ABXOR
%token TOK_ABOR TOK_ASHL TOK_ASHR TOK_DEFINE
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
%type <Node> string_concat type name prim_expr postfix_expr opt_postfix_expr
%type <Node> unary_expr pow_expr mul_expr add_expr
%type <Node> shift_expr cmp_expr eq_expr band_expr bxor_expr bor_expr
%type <Node> land_expr lor_expr assign_expr comb_expr expr
%type <Node> actions opt_actions restrs opt_restrs opt_comb_def
%type <Node> opt_c_name comb_parent
%type <NewNode> statement statement_list


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
    void_sep                  {$$ = BOXASTSEP_VOID;}
  | ';'                       {$$ = BOXASTSEP_PAUSE;}
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
  | TOK_DEFINE                {$$ = ASTBINOP_ASSIGN;}
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

expr_list_sep:
    ','
  | TOK_NEWLINE
  | TOK_TO
  ;

opt_ident:
                                 {$$ = NULL;}
  | TOK_IDENTIFIER               {$$ = NULL;}
  ;

expr_list_memb:
                                 {$$ = NULL;}
  |  expr opt_ident              {$$ = $1;}
  ;

expr_list:
    expr_list_memb               {$$ = NULL;}
  | expr_list expr_list_sep expr_list_memb
                                 {$$ = $3;}
  ;

/******************************* ARITHMETICS *******************************/

string_concat:
    TOK_STRING                   {$$ = $1;}
  | string_concat TOK_STRING     {$$ = NULL;}
  ;

type:
    TOK_TYPE_IDENT               {$$ = NULL;}
  | type ':' TOK_TYPE_IDENT      {$$ = NULL;}
  ;

name:
    type                         {$$ = NULL;}
  | TOK_IDENTIFIER               {$$ = NULL;}
  | type ':' TOK_IDENTIFIER      {$$ = NULL;}
  ;

prim_expr:
    TOK_CONSTANT                 {$$ = $1;}
  | TOK_TTAG                     {$$ = NULL;}
  | TOK_KEYWORD                  {$$ = NULL;}
  | string_concat                {$$ = $1;}
  | name                         {$$ = NULL;}
  | ':' name                     {$$ = NULL;}
  | '?'                          {$$ = NULL;}
  | TOK_SELF                     {$$ = NULL;}
  | '(' expr_list ')'            {$$ = $2;}
  ;

postfix_expr:
    prim_expr                    {$$ = $1;}
  | postfix_expr '(' expr ')'    {$$ = NULL;}
  | postfix_expr '[' statement_list ']'
                                 {$$ = NULL;}
  | opt_postfix_expr '.' TOK_IDENTIFIER
                                 {$$ = NULL;}
  | opt_postfix_expr '.' TOK_TYPE_IDENT
                                 {$$ = NULL;}
  | postfix_expr post_op         {$$ = NULL;}
  ;

opt_postfix_expr:
                                 {$$ = NULL;}
  | postfix_expr                 {$$ = $1;}
  ;

unary_expr:
    postfix_expr                 {$$ = $1;}
  | un_op unary_expr             {$$ = NULL;}
  | '^' unary_expr               {$$ = NULL;}
;

pow_expr:
    unary_expr                   {$$ = $1;}
  | pow_expr TOK_POW unary_expr  {$$ = NULL;}
  ;

mul_expr:
    pow_expr                     {$$ = $1;}
  | mul_expr mul_op pow_expr     {$$ = NULL;}
  ;

add_expr:
    mul_expr                     {$$ = $1;}
  | add_expr add_op mul_expr     {$$ = NULL;}
  ;

shift_expr:
    add_expr                     {$$ = $1;}
  | shift_expr shift_op add_expr {$$ = NULL;}
  ;

cmp_expr:
    shift_expr                   {$$ = $1;}
  | cmp_expr cmp_op shift_expr   {$$ = NULL;}
  ;

eq_expr:
    cmp_expr                     {$$ = $1;}
  | eq_expr eq_op cmp_expr       {$$ = NULL;}
  ;

band_expr:
    eq_expr                      {$$ = $1;}
  | band_expr '&' eq_expr        {$$ = NULL;}
  ;

bxor_expr:
    band_expr                    {$$ = $1;}
  | bxor_expr '^' band_expr      {$$ = NULL;}
  ;

bor_expr:
    bxor_expr                    {$$ = $1;}
  | bor_expr '|' bxor_expr       {$$ = NULL;}
  ;

land_expr:
    bor_expr                     {$$ = $1;}
  | land_expr TOK_LAND bor_expr  {$$ = NULL;}
  ;

lor_expr:
    land_expr                    {$$ = $1;}
  | lor_expr TOK_LOR land_expr   {$$ = NULL;}
  ;

assign_expr:
    lor_expr                     {$$ = $1;}
  | lor_expr assign_op  assign_expr
                                 {$$ = NULL;}
  ;

comb_expr:
    assign_expr                  {$$ = $1;}
  | assign_expr TOK_COMBINE opt_restrs comb_parent opt_c_name opt_comb_def
                                 {$$ = NULL;}
   ;

expr:
    comb_expr             {$$ = $1;}
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

opt_c_name:
                                 {$$ = NULL;}
  | string_concat                {$$ = $1;}
  ;

opt_comb_def:
                                 {$$ = NULL;}
  | '?'                          {$$ = NULL;}
  | '[' statement_list ']'       {$$ = NULL;}
  ;

comb_parent:
    TOK_TYPE_IDENT               {$$ = NULL;}
  | comb_parent '.' TOK_TYPE_IDENT
                                 {$$ = NULL;}
  ;

/************************ STATEMENT LISTS AND BOXES ************************/
/* Syntax for the body of the program */
statement:
                                 {$$ = NULL;}
  | expr                         {$$ = (BoxASTNode *) $1;}
  | '\\' expr                    {$$ = NULL;}
  | '[' statement_list ']'       {$$ = NULL;}
  | error sep                    {$$ = NULL;
                                  /*ASTNodeStatement_New(ASTNodeError_New());
                                  My_Syntax_Error(& @$, NULL);
                                  assert(yychar == YYEMPTY); yychar = (int) ',';
                                  yyerrok;*/}
  ;

statement_list:
    statement                    {$$ = BoxAST_Create_Statement(ast, $1);}
  | statement_list sep statement
                                {$$ = BoxAST_Append_Statement(ast, $1, $2, $3);}
  ;

program:
    statement_list
                       {BoxAST_Set_Root(ast, BoxAST_Create_Box(ast, NULL, $1));}
  ;

%%

/* error function */
static void yyerror(BoxParser *bp, BoxAST *ast, char* s)
{
  /* Do nothing, as - at the moment - we report error in error action */
}

static void My_Syntax_Error(YYLTYPE *src, char *s)
{
  BoxSrc *prev_src_of_err;
  prev_src_of_err = Msg_Set_Src(src);
  if (s)
    MSG_ERROR("%s", s);
  else
    MSG_ERROR("Syntax error.");
  (void) Msg_Set_Src(prev_src_of_err);
}

BoxAST *Box_Parse_FILE(FILE *in, const char *in_name,
                       const char *auto_include, BoxPaths *paths)
{
  BoxAST *ast = NULL;
  BoxBool no_errors = BOXBOOL_TRUE;
  BoxParser *parser;

  parser = BoxParser_Create(paths);
  assert(parser);

  in_name = (in_name) ? in_name : "<stdin>";
  in = (in) ? in : stdin;
  no_errors = BoxParser_Begin_Include_FILE(parser, in, in_name);

  if (auto_include && no_errors)
    no_errors = BoxParser_Begin_Include(parser, auto_include);

  if (no_errors)
    no_errors = !yyparse(parser, BoxParser_Get_AST(parser));

  ast = BoxParser_Destroy(parser);
  if (no_errors && ast)
    return ast;

  if (!no_errors)
    MSG_ERROR("Parse error at end of input.");

  BoxAST_Destroy(ast);
  return NULL;
}
