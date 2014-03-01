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
  BoxTypeId     TTag;
  BoxCombType   Combine;
  BoxASTUnOp    UnaryOperator;
  BoxASTBinOp   BinaryOperator;
  BoxASTNodePtr Node;
  BoxASTNodePtr NewNode;
  BoxASTSep     Sep;
  uint32_t      SelfLevel;
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
%token TOK_EQ TOK_NE TOK_LT TOK_LE TOK_GT TOK_GE
%token TOK_APLUS TOK_AMINUS TOK_ATIMES TOK_ADIV TOK_AREM TOK_ABAND TOK_ABXOR
%token TOK_ABOR TOK_ASHL TOK_ASHR TOK_DEFINE
%token TOK_POW
%token TOK_ERR

/* List of tokens with semantical value */
%token <Node> TOK_TYPE_IDFR TOK_VAR_IDFR TOK_KEYWORD
%token <Node> TOK_CONSTANT TOK_STRING
%token <BinaryOperator> TOK_LAND TOK_LOR

/* List of nodes with semantical value */
%type <Sep> sep compound_sep
%type <UnaryOperator> un_op post_op
%type <BinaryOperator> mul_op add_op shift_op cmp_op eq_op assign_op
%type <Node> compound compound_memb opt_ident
%type <Node> string_concat type name prim_expr postfix_expr opt_postfix_expr
%type <Node> unary_expr pow_expr mul_expr add_expr
%type <Node> shift_expr cmp_expr eq_expr band_expr bxor_expr bor_expr
%type <Node> land_expr lor_expr assign_expr comb_expr expr
%type <Node> actions opt_actions restrs opt_restrs opt_comb_def
%type <Node> opt_c_name comb_parent
%type <Node> statement statement_list


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
    '+'                       {$$ = BOXASTUNOP_PLUS;}
  | '-'                       {$$ = BOXASTUNOP_NEG;}
  | '!'                       {$$ = BOXASTUNOP_NOT;}
  | TOK_INC                   {$$ = BOXASTUNOP_LINC;}
  | TOK_DEC                   {$$ = BOXASTUNOP_LDEC;}
  | '~'                       {$$ = BOXASTUNOP_BNOT;}
  | '^'                       {$$ = BOXASTUNOP_RAISE;}
  ;

post_op:
    TOK_INC                   {$$ = BOXASTUNOP_RINC;}
  | TOK_DEC                   {$$ = BOXASTUNOP_RDEC;}
  ;

add_op:
    '+'                       {$$ = BOXASTBINOP_ADD;}
  | '-'                       {$$ = BOXASTBINOP_SUB;}
  ;

mul_op:
    '*'                       {$$ = BOXASTBINOP_MUL;}
  | '/'                       {$$ = BOXASTBINOP_DIV;}
  | '%'                       {$$ = BOXASTBINOP_REM;}
  ;

shift_op:
    TOK_SHL                   {$$ = BOXASTBINOP_SHL;}
  | TOK_SHR                   {$$ = BOXASTBINOP_SHR;}
  ;

eq_op:
    TOK_EQ                    {$$ = BOXASTBINOP_EQ;}
  | TOK_NE                    {$$ = BOXASTBINOP_NE;}
  ;

cmp_op:
    '<'                       {$$ = BOXASTBINOP_LT;}
  | TOK_LE                    {$$ = BOXASTBINOP_LE;}
  | '>'                       {$$ = BOXASTBINOP_GT;}
  | TOK_GE                    {$$ = BOXASTBINOP_GE;}
  ;

assign_op:
    '='                       {$$ = BOXASTBINOP_ASSIGN;}
  | TOK_DEFINE                {$$ = BOXASTBINOP_ASSIGN;}
  | TOK_APLUS                 {$$ = BOXASTBINOP_APLUS;}
  | TOK_AMINUS                {$$ = BOXASTBINOP_AMINUS;}
  | TOK_ATIMES                {$$ = BOXASTBINOP_ATIMES;}
  | TOK_AREM                  {$$ = BOXASTBINOP_AREM;}
  | TOK_ADIV                  {$$ = BOXASTBINOP_ADIV;}
  | TOK_ASHL                  {$$ = BOXASTBINOP_ASHL;}
  | TOK_ASHR                  {$$ = BOXASTBINOP_ASHR;}
  | TOK_ABAND                 {$$ = BOXASTBINOP_ABAND;}
  | TOK_ABXOR                 {$$ = BOXASTBINOP_ABXOR;}
  | TOK_ABOR                  {$$ = BOXASTBINOP_ABOR;}
  ;

/******************************* STRUCTURES ********************************/

compound_sep:
    ','                          {$$ = BOXASTSEP_VOID;}
  | TOK_NEWLINE                  {$$ = BOXASTSEP_VOID;}
  | TOK_TO                       {$$ = BOXASTSEP_ARROW;}
  ;

opt_ident:
                                 {$$ = NULL;}
  | TOK_VAR_IDFR                 {$$ = $1;}
  ;

compound_memb:
                                 {$$ = NULL;}
  |  expr opt_ident              {$$ = BoxAST_Create_Member(ast, $1, $2);}
  ;

compound:
    compound_memb                {$$ = BoxAST_Create_Compound(ast, $1);}
  | compound compound_sep compound_memb
                               {$$ = BoxAST_Append_Member(ast, $1, $2, 0, $3);}
  ;

/******************************* ARITHMETICS *******************************/

string_concat:
    TOK_STRING                   {$$ = $1;}
  | string_concat TOK_STRING     {$$ = NULL;}
  ;

type:
    TOK_TYPE_IDFR                {$$ = $1;}
  | type ':' TOK_TYPE_IDFR       {$$ = NULL;}
  ;

name:
    type                         {$$ = $1;}
  | TOK_VAR_IDFR                 {$$ = $1;}
  | type ':' TOK_VAR_IDFR        {$$ = NULL;}
  ;

prim_expr:
    TOK_CONSTANT                 {$$ = $1;}
  | TOK_TTAG                     {$$ = BoxAST_Create_TypeTag(ast, $1);}
  | TOK_KEYWORD                  {$$ = BoxAST_Create_Keyword(ast, $1);}
  | string_concat                {$$ = $1;}
  | name                         {$$ = $1;}
  | ':' name                     {$$ = NULL;}
  | '?'                          {$$ = NULL;}
  | TOK_SELF                     {$$ = BoxAST_Create_ArgGet(ast, $1);}
  | '(' compound ')'             {$$ = BoxAST_Close_Compound(ast, $2);}
  ;

postfix_expr:
    prim_expr                    {$$ = $1;}
  | postfix_expr '(' expr ')'    {$$ = NULL;}
  | postfix_expr '[' statement_list ']'
                                 {$$ = BoxAST_Create_Box(ast, $1, $3);}
  | opt_postfix_expr '.' TOK_VAR_IDFR
                                 {$$ = BoxAST_Create_Get(ast, $1, $3);}
  | opt_postfix_expr '.' TOK_TYPE_IDFR
                                 {$$ = BoxAST_Create_Subtype(ast, $1, $3);}
  | postfix_expr post_op         {$$ = BoxAST_Create_UnOp(ast, $2, $1);}
  ;

opt_postfix_expr:
                                 {$$ = NULL;}
  | postfix_expr                 {$$ = $1;}
  ;

unary_expr:
    postfix_expr                 {$$ = $1;}
  | un_op unary_expr             {$$ = BoxAST_Create_UnOp(ast, $1, $2);}
;

pow_expr:
    unary_expr                   {$$ = $1;}
  | pow_expr TOK_POW unary_expr  {$$ = BoxAST_Create_BinOp(ast, $1,
                                    BOXASTBINOP_POW, $3);}
  ;

mul_expr:
    pow_expr                     {$$ = $1;}
  | mul_expr mul_op pow_expr     {$$ = BoxAST_Create_BinOp(ast, $1, $2, $3);}
  ;

add_expr:
    mul_expr                     {$$ = $1;}
  | add_expr add_op mul_expr     {$$ = BoxAST_Create_BinOp(ast, $1, $2, $3);}
  ;

shift_expr:
    add_expr                     {$$ = $1;}
  | shift_expr shift_op add_expr {$$ = BoxAST_Create_BinOp(ast, $1, $2, $3);}
  ;

cmp_expr:
    shift_expr                   {$$ = $1;}
  | cmp_expr cmp_op shift_expr   {$$ = BoxAST_Create_BinOp(ast, $1, $2, $3);}
  ;

eq_expr:
    cmp_expr                     {$$ = $1;}
  | eq_expr eq_op cmp_expr       {$$ = BoxAST_Create_BinOp(ast, $1, $2, $3);}
  ;

band_expr:
    eq_expr                      {$$ = $1;}
  | band_expr '&' eq_expr
                                 {$$ = BoxAST_Create_BinOp(ast, $1,
                                    BOXASTBINOP_BAND, $3);}
  ;

bxor_expr:
    band_expr                    {$$ = $1;}
  | bxor_expr '^' band_expr      {$$ = BoxAST_Create_BinOp(ast, $1,
                                    BOXASTBINOP_BXOR, $3);}
  ;

bor_expr:
    bxor_expr                    {$$ = $1;}
  | bor_expr '|' bxor_expr       {$$ = BoxAST_Create_BinOp(ast, $1,
                                    BOXASTBINOP_BOR, $3);}
  ;

land_expr:
    bor_expr                     {$$ = $1;}
  | land_expr TOK_LAND bor_expr  {$$ = BoxAST_Create_BinOp(ast, $1, $2, $3);}
  ;

lor_expr:
    land_expr                    {$$ = $1;}
  | lor_expr TOK_LOR land_expr   {$$ = BoxAST_Create_BinOp(ast, $1, $2, $3);}
  ;

assign_expr:
    lor_expr                     {$$ = $1;}
  | lor_expr assign_op assign_expr
                                 {$$ = BoxAST_Create_BinOp(ast, $1, $2, $3);}
  ;

comb_expr:
    assign_expr                  {$$ = $1;}
  | assign_expr TOK_COMBINE opt_restrs comb_parent opt_c_name opt_comb_def
                         {$$ = BoxAST_Create_CombDef(ast, $1, $2, $4, $5, $6);}
   ;

expr:
    comb_expr                    {$$ = $1;}
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
  | '(' restrs opt_actions ')'   {$$ = NULL;}
  ;

opt_c_name:
                                 {$$ = NULL;}
  | string_concat                {$$ = $1;}
  ;

opt_comb_def:
                                 {$$ = NULL;}
  | '?'                          {$$ = NULL;}
  | '[' statement_list ']'       {$$ = BoxAST_Create_Box(ast, NULL, $2);}
  ;

comb_parent:
    TOK_TYPE_IDFR                {$$ = $1;}
  | comb_parent '.' TOK_TYPE_IDFR
                                 {$$ = BoxAST_Create_Subtype(ast, $1, $3);}
  ;

/************************ STATEMENT LISTS AND BOXES ************************/
/* Syntax for the body of the program */
statement:
                                 {$$ = NULL;}
  | expr                         {$$ = $1;}
  | '\\' expr                    {$$ = BoxAST_Create_Ignore(ast, $2);}
  | '[' statement_list ']'       {$$ = BoxAST_Create_Box(ast, NULL, $2);;}
  | error sep                    {$$ = NULL;
                                  BoxAST_Log(ast, NULL, BOXLOGLEVEL_ERROR,
                                             "Syntax error!");
                                  assert(yychar == YYEMPTY); yychar = (int) ',';
                                  yyerrok;}
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
