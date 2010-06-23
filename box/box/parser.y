%{
/***************************************************************************
 *   Copyright (C) 2006-2010 by Matteo Franchin                            *
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
#include "ast.h"

static ASTNode *program_node = NULL;

int yyparse(void);
int yylex(void);
void yyerror(char *s);

#if 0
#define YYDEBUG 0

/*****************************************************************************
 *       Grammatica relativa ai suffissi del tipo ::: o :tipo1:::tipo2::     *
 *****************************************************************************/
prim.suffix:
   ':'                 { $$ = *Name_Empty(); }
 | ':' TOK_UNAME       { $$ = *Name_Dup(& $2); if ($$.text == NULL ) MY_ERR }
 ;

suffix:
   prim.suffix         { DO( Prs_Suffix(& $$, -1, & $1) ) }
 | suffix prim.suffix  { DO( Prs_Suffix(& $$, $1, & $2) ) }
 ;

suffix.opt:
                       { $$ = -1; }
 | suffix              { $$ = $1; }
 ;

#endif

%}

/* Possible types for the nodes of the tree */
%union {
  char *        String;
  BoxType       TTag;
  ASTUnOp       UnaryOperator;
  ASTBinOp      BinaryOperator;
  ASTNodePtr    Node;
  ASTTypeMemb   TypeMemb;
  ASTSep        Sep;
  ASTSelfLevel  SelfLevel;
}

%token TOK_NEWLINE
%token TOK_ERRSEP

%token TOK_TO TOK_MAPTO

%token <SelfLevel> TOK_SELF

/* List of tokens that have semantical values */
%token TOK_AT
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
%token <String> TOK_IDENTIFIER TOK_TYPE_IDENT
%token <Node> TOK_CONSTANT TOK_STRING

/* List of nodes with semantical value */
%type <Sep> sep
%type <UnaryOperator> un_op post_op
%type <BinaryOperator> mul_op add_op shift_op cmp_op eq_op assign_op
%type <Node> expr_sep sep_expr struc_expr
%type <Node> string_concat prim_expr postfix_expr unary_expr pow_expr
%type <Node> mul_expr add_expr
%type <Node> shift_expr cmp_expr eq_expr band_expr bxor_expr bor_expr
%type <Node> land_expr lor_expr assign_expr expr statement statement_list
%type <Node> type_sep sep_type struc_type species_type func_type
%type <Node> named_type prim_type array_type type inc_type assign_type
%type <Node> at_left procedure opt_c_name procedure_decl
%type <TypeMemb> struc_type_1st struc_type_2nd

/* Starting rule */
%start program

%%

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
expr_sep:
    expr void_seps               {$$ = ASTNodeStruc_New(NULL, $1);}
  | sep_expr void_seps           {$$ = $1;}
  ;

sep_expr:
    void_seps expr               {$$ = ASTNodeStruc_New(NULL, $2);}
  | expr_sep expr                {$$ = ASTNodeStruc_Add_Member($1, NULL, $2);}
  ;

struc_expr:
    expr_sep                     {$$ = $1;}
  | sep_expr                     {$$ = $1;}
  ;

/******************************* ARITHMETICS *******************************/
string_concat:
    TOK_STRING                   {$$ = $1;}
  | string_concat TOK_STRING     {$$ = ASTNodeString_Concat($1, $2);}
  ;

prim_expr:
    TOK_CONSTANT                 {$$ = $1;}
  | string_concat                {$$ = $1;}
  | TOK_IDENTIFIER               {$$ = ASTNodeVar_New($1, 0); BoxMem_Free($1);}
  | TOK_SELF                     {$$ = ASTNodeSelfGet_New($1);}
  | '(' expr ')'                 {$$ = ASTNodeIgnore_New($2, 0);}
  | '(' struc_expr ')'           {$$ = $2;}
  ;

postfix_expr:
    prim_expr                    {$$ = $1;}
  | postfix_expr '(' expr ')'    {$$ = ASTNodeArrayGet_New($1, $3);}
  | procedure_decl               {$$ = $1;}
  | postfix_expr
          '[' statement_list ']' {$$ = ASTNodeBox_Set_Parent($3, $1);}
  | type  '[' statement_list ']' {$$ = ASTNodeBox_Set_Parent($3, $1);}
  | postfix_expr
              '.' TOK_IDENTIFIER {$$ = ASTNodeMemberGet_New($1, $3, 0);
                                  BoxMem_Free($3);}
  | postfix_expr '.' TOK_TYPE_IDENT '[' statement_list ']'
                                 {$$ = ASTNodeBox_Set_Parent($5,
                                               ASTNodeSubtype_Build($1, $3));
                                  BoxMem_Free($3);}
  | '.' TOK_TYPE_IDENT '[' statement_list ']'
                                 {$$ = ASTNodeBox_Set_Parent($4,
                                             ASTNodeSubtype_Build(NULL, $2));
                                  BoxMem_Free($2);}
  | '.' TOK_IDENTIFIER           {$$ = ASTNodeMemberGet_New(NULL, $2, 0);
                                  BoxMem_Free($2);}
  | postfix_expr post_op         {$$ = ASTNodeUnOp_New($2, $1);}
  ;

unary_expr:
    postfix_expr                 {$$ = $1;}
  | un_op unary_expr             {$$ = ASTNodeUnOp_New($1, $2);}
  ;

pow_expr:
    unary_expr                   {$$ = $1;}
  | pow_expr TOK_POW unary_expr  {$$ = ASTNodeBinOp_New(ASTBINOP_POW, $1, $3);}
  ;

mul_expr:
    pow_expr                     {$$ = $1;}
  | mul_expr mul_op pow_expr     {$$ = ASTNodeBinOp_New($2, $1, $3);}
  ;

add_expr:
    mul_expr                     {$$ = $1;}
  | add_expr add_op mul_expr     {$$ = ASTNodeBinOp_New($2, $1, $3);}
  ;

shift_expr:
    add_expr                     {$$ = $1;}
  | shift_expr shift_op add_expr {$$ = ASTNodeBinOp_New($2, $1, $3);}
  ;

cmp_expr:
    shift_expr                   {$$ = $1;}
  | cmp_expr cmp_op shift_expr   {$$ = ASTNodeBinOp_New($2, $1, $3);}
  ;

eq_expr:
    cmp_expr                     {$$ = $1;}
  | eq_expr eq_op cmp_expr       {$$ = ASTNodeBinOp_New($2, $1, $3);}
  ;

band_expr:
    eq_expr                      {$$ = $1;}
  | band_expr '&' eq_expr        {$$ = ASTNodeBinOp_New(ASTBINOP_BAND, $1, $3);}
  ;

bxor_expr:
    band_expr                    {$$ = $1;}
  | bxor_expr '^' band_expr      {$$ = ASTNodeBinOp_New(ASTBINOP_BXOR, $1, $3);}
  ;

bor_expr:
    bxor_expr                    {$$ = $1;}
  | bor_expr '|' bxor_expr       {$$ = ASTNodeBinOp_New(ASTBINOP_BOR, $1, $3);}
  ;

land_expr:
    bor_expr                     {$$ = $1;}
  | land_expr TOK_LAND bor_expr  {$$ = ASTNodeBinOp_New(ASTBINOP_LAND, $1, $3);}
  ;

lor_expr:
    land_expr                    {$$ = $1;}
  | lor_expr TOK_LOR land_expr   {$$ = ASTNodeBinOp_New(ASTBINOP_LOR, $1, $3);}
  ;

assign_expr:
    lor_expr                     {$$ = $1;}
  | lor_expr assign_op
                     assign_expr {$$ = ASTNodeBinOp_New($2, $1, $3);}
  ;

expr:
    assign_expr                  {$$ = $1;}
  ;

/***************************** TYPE ARITHMETICS ****************************/

/* STRUCTURE TYPES */
struc_type_1st:
    type                         {$$.type = $1; $$.name = NULL;}
  | type TOK_IDENTIFIER          {$$.type = $1; $$.name = $2;}
  ;

struc_type_2nd:
    type                         {$$.type = $1; $$.name = NULL;}
  | TOK_IDENTIFIER               {$$.type = NULL; $$.name = $1;}
  | type TOK_IDENTIFIER          {$$.type = $1; $$.name = $2;}
  ;

type_sep:
    struc_type_1st void_seps     {$$ = ASTNodeStrucType_New(& $1);
                                  BoxMem_Free($1.name);}
  | sep_type void_seps           {$$ = $1;}
  ;

sep_type:
    void_seps struc_type_1st     {$$ = ASTNodeStrucType_New(& $2);
                                  BoxMem_Free($2.name);}
  | type_sep struc_type_2nd      {$$ = ASTNodeStrucType_Add_Member($1, & $2);
                                  BoxMem_Free($2.name);}
  ;

struc_type:
    type_sep                     {$$ = $1;}
  | sep_type                     {$$ = $1;}
  ;

/* SPECIES TYPES */
species_type:
    type TOK_TO type             {$$ = ASTNodeSpecType_New($1, $3);}
  | species_type TOK_TO type     {$$ = ASTNodeSpecType_Add_Member($1, $3);}
  ;

/* PRIMARY TYPES */

named_type:
    TOK_TYPE_IDENT               {$$ = ASTNodeTypeName_New($1, 0);
                                  BoxMem_Free($1);}
  | named_type '.' TOK_TYPE_IDENT
                                 {$$ = ASTNodeSubtype_New($1, $3);
                                  BoxMem_Free($3);}
  ;

prim_type:
    named_type                   {$$ = $1;}
  | '(' type ')'                 {$$ = $2;}
  | '(' struc_type ')'           {$$ = $2;}
  | '(' species_type ')'         {$$ = $2;}
  ;

array_type:
    prim_type                    {$$ = $1;}
  | array_type '(' expr ')'      {}
  ;

func_type:
    array_type                   {$$ = $1;}
  | array_type TOK_MAPTO func_type
                                 {}
  ;

type:
    func_type                    {$$ = $1;}
  ;

inc_type:
    type                         {$$ = $1;}
  | TOK_INC type                 {$$ = ASTNodeIncType_New($2);}
  ;

assign_type:
    named_type '=' inc_type      {$$ = ASTNodeTypeDef_New($1, $3);;}
  | named_type '=' assign_type   {$$ = ASTNodeTypeDef_New($1, $3);}
  ;

/****************** PROCEDURE DECLARATION AND DEFINITION *******************/
/* Definition and declaration of procedures */

 /* left side of @ */
at_left:
    type                         {$$ = $1;}
  | TOK_TTAG                     {$$ = ASTNodeTypeTag_New($1);}
  ;

procedure:
    at_left TOK_AT named_type    {$$ = ASTNodeProcDef_New($1, $3);}
  ;

opt_c_name:
                                 {$$ = NULL;}
  | string_concat                {$$ = $1;}
  ;

procedure_decl:
    procedure opt_c_name '?'     {$$ = ASTNodeProcDef_Set($1, $2, NULL);}
  | procedure opt_c_name
          '[' statement_list ']' {$$ = ASTNodeProcDef_Set($1, $2, $4);}
  ;

/************************ STATEMENT LISTS AND BOXES ************************/
/* Syntax for the body of the program */
statement:
                                 {$$ = NULL;}
  | assign_type                  {$$ = ASTNodeStatement_New($1);}
  | expr                         {$$ = ASTNodeStatement_New($1);}
  | '\\' expr                    {$$ = ASTNodeStatement_New(
                                         ASTNodeIgnore_New($2, 1));}
  | '[' statement_list ']'       {$$ = ASTNodeStatement_New($2);}
  | error sep                    {$$ = ASTNodeStatement_New(ASTNodeError_New());
                                  Tok_Unput(($2 == ASTSEP_PAUSE) ? ';' : ',');
                                  yyerrok;}
  ;

statement_list:
    statement                    {$$ = ASTNodeBox_New(NULL, $1);}
  | statement_list sep statement {$$ = ASTNodeBox_Add_Sep($1, $2);
                                  $$ = ASTNodeBox_Add_Statement($1, $3);}
  ;

program:
    statement_list               {program_node = $1;}
  ;

%%

/* error function */
void yyerror(char* s) {
#if 0
  MSG_ERROR("%s", s);
#else
  MSG_ERROR("(%~s) %s", ASTSrc_To_Str(tok_src), s);
#endif
}

#if 0
/* Inizializza il parser. Se f != NULL il compilatore partira'
 * come se la prima istruzione del programma fosse una "include nomefile"
 * dove nomefile e' la stringa a cui punta f.
 * maxinc indica il numero massimo di file di include che possono essere
 * aperti contemporaneamente.
 */
Task Parser_Init(const char *f) {

  /* Leggo comunque dallo stdin (che puo' essere stato rediretto)! */
  yyrestart(stdin);

  /* Inizializzo il tokenizer */
  TASK( Tok_Init(NULL, f) );

  //parser_attr.no_syntax_err = 0;

#if YYDEBUG == 1
  yydebug = 1;
#endif

  return Success;
}

/* Completa il parsing del file di input.
 */
Task Parser_Finish(void) {
  Tok_Finish();
  return Success;
}
#endif

ASTNode *Parser_Parse(FILE *in, const char *auto_include) {
  ASTNode *program;
  int parse_status;

  assert(program_node == NULL);

  if (Tok_Init(in, auto_include) != BOXTASK_OK)
    return NULL;
  parse_status = yyparse();
  Tok_Finish();

  if (parse_status) {
    ASTNode_Destroy(program_node);
    program_node = NULL;
    return NULL;
  }

  program = program_node;
  program_node = NULL;
  return program;
}



#if 0
Task Parse() {
  struct yy_buffer_state *bufState = NULL;
  yyin = stream_in;
  bufState = yy_create_buffer( /* FILE *fh */ NULL, YY_BUF_SIZE );
  if (! bufState) goto error;
  yy_switch_to_buffer( bufState );

  /* Parse, building the AST */
  if (yyparse() != 0)
      goto error;
  yy_delete_buffer( bufState );
      PRIVATE(yyin) = NULL;
}
#endif


#if 0

/* Every explicit symbol can be followed by a suffix,
 * which specifies which opened box contains the symbol.
 * For example: variable::box1:box2
 * The compiler interprets the string '::box1:box2' as the specification
 * of the box which contains the variable 'v'.
 * The association string --> box-level is obtained by the following
 * function.
 */
Task Prs_Suffix(Int *rs, Int suffix, Name *nm) {
  if ( nm->length > 0 ) {
    Symbol *s;

    /* Cerco il simbolo a profondita' suffix o superiori */
    s = Sym_Explicit_Find(nm, suffix, NO_EXACT_DEPTH);
    if (s == NULL) {
      MSG_ERROR("'%N' <-- type not found!", nm);
      return Failed;
    }

    /* Controllo che sia un tipo, ossia un simbolo con tipo e senza valore. */
    assert( s->value.is.typed );
    if ( s->value.is.value ) {
      MSG_ERROR("'%N' has to be a type!", nm);
      return Failed;
    }

    suffix = (suffix < 0) ? 0 : suffix;
    if ( (*rs = Box_Search_Opened(s->value.type, suffix + 1)) >= 0 )
      return Success;

    MSG_ERROR("'%N' <-- none of the opened box have this type!", nm);
    return Failed;

  } else {
    if ( suffix < 0 ) {
      *rs = 0;
      return Success;
    } else {
      if ( (*rs = suffix + 1) <= Box_Num() ) {
        return Success;
      } else {
        MSG_ERROR("Maximum box depth exceeded.");
        return Failed;
      }
    }
  }
}

/* DESCRIPTION: This function build an array of *num objects of type *x.
 * NOTE: *num is an integer expression. The new type will be put into *array.
 */
Task Prs_Array_Of_X(Expr *array, Expr *num, Expr *x) {
  register Int t, n;

  /* Checks on *num */
  if ( num != NULL ) {
    if ( ! num->is.typed ) {
      MSG_ERROR("L'indice dell'array e' una espressione senza tipo.");
      return Failed;
    }

    if ( ! Tym_Compare_Types(TYPE_INTG, num->type, NULL) ) {
      MSG_ERROR("L'indice dell'array non e' di tipo intero.");
      return Failed;
    }

    if ( (! num->is.value) || (num->categ != CAT_IMM) ) {
      MSG_ERROR(
       "L'indice dell'array deve essere una costante intera positiva.");
      return Failed;
    }

    n = num->value.i;
    if ( n < 1 ) {
      MSG_ERROR("L'indice dell'array e' non positivo!");
      return Failed;
    }

  } else {
    n = -1;
  }

  /* Checks on *x */
  if ( ! x->is.typed ) {
    MSG_ERROR("Cannot create an array of objects with undefined type!");
    return Failed;
  }

  array->is.typed = 1;
  array->is.value = 0;
  array->type = t = Tym_Def_Array_Of(n, x->type);
  if ( t != TYPE_NONE ) return Success;
  return Failed;
}

#endif
