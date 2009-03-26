%{
/***************************************************************************
 *   Copyright (C) 2006 by Matteo Franchin                                 *
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

/* $Id$ */

/* parser.c - settembre 2002 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "types.h"
#include "defaults.h"
#include "mem.h"
#include "ast.h"
#include "messages.h"

void yyerror(char *s);

#if 0
#include "array.h"
#include "str.h"
#include "virtmach.h"
#include "vmsym.h"
#include "vmsymstuff.h"
#include "expr.h"
#include "compiler.h"
#include "box.h"
#include "tokenizer.h"
#include "builtins.h"

#define YYDEBUG 0

ParserAttr parser_attr;


static Task Proc_Def_Open(Expr *child_type, Int kind, Expr *parent_type);
static Task Proc_Def_Close(void);
static Task Declare_Proc(Expr *child_type, Int kind, Expr *parent_type,
                         Name *name);

/* Numero della linea che e' in fase di lettura dal tokenizer */
extern UInt tok_linenum;



/****************************************************************************
 * Here we define static function which implements the actions of some      *
 * rules. Actions are included directly into the grammar when possible,     *
 * but sometimes - to keep acceptable readability we confine the code       *
 * in these static functions that are collected here.                       *
 ****************************************************************************/

static void Sep_Newline(void) {
  Cmp_Assemble(ASM_LINE_Iimm, CAT_IMM, ++tok_linenum);
  Msg_Line_Set(tok_linenum);
}

static void Sep_Pause(void) {
  (void) Box_Procedure_Quick_Call_Void(TYPE_PAUSE, BOX_DEPTH_UPPER,
                                       BOX_MSG_SILENT);
}

static Task Type_Struc_Begin(StrucMember *sm, Expr *type, char *m) {
  TASK( Expr_Must_Have_Type(type) );
  sm->type = type->type;
  sm->name = m;
  return Success;
}

/* Construct the core of a structure type: takes two members and construct
 * a two membered structure Sm1, Sm2 --> (Sm1, Sm2).
 * The type of the new structure of types is stored inside *st.
 */
static void Type_Struc_All_1(Struc *s, StrucMember *sm1, StrucMember *sm2) {
  Type st;
  TS_Structure_Begin(cmp->ts, & st);
  assert(sm1->type != TYPE_NONE);
  TS_Structure_Add(cmp->ts, st, sm1->type, sm1->name);
  free(sm1->name);
  sm1->name = (char *) NULL;
  if (sm2->type == TYPE_NONE) sm2->type = sm1->type;
  TS_Structure_Add(cmp->ts, st, sm2->type, sm2->name);
  free(sm2->name);
  sm2->name = (char *) NULL;
  s->type = st;
  s->previous = sm2->type;
}

/* Add a new member to an existing structure type.
 * After the structure type se1 has been changed, it is copied into *se.
 */
static void Type_Struc_All_2(Struc *s, Struc *s1, StrucMember *sm2) {
  if (sm2->type == TYPE_NONE) sm2->type = s1->previous;
  TS_Structure_Add(cmp->ts, s1->type, sm2->type, sm2->name);
  free(sm2->name);
  sm2->name = (char *) NULL;
  s->type = s1->type;
  s->previous = sm2->type;
}

/* Construct a single-member structure type.
 */
static void Type_Struc_1(Expr *se, StrucMember *sm) {
  Type st;
  TS_Structure_Begin(cmp->ts, & st);
  TS_Structure_Add(cmp->ts, st, sm->type, sm->name);
  free(sm->name);
  sm->name = (char *) NULL;
  Expr_New_Type(se, st);
}

/* Just convert the final structure type into an Expr */
static Task Type_Struc_2(Expr *se, Struc *s) {
  Expr_New_Type(se, s->type);
  return Success;
}

static Task Reg_Subtype(Expr *result, Name *parent, Name *child) {
  Expr parent_expr;
  Type subtype;
  TASK( Prs_Name_To_Expr(parent, & parent_expr, 0) );
  if (!parent_expr.is.typed) {
    MSG_ERROR("Cannot refer to the subtype '%N' of the undefined type '%N'",
              child, parent);
    return Failed;
  }

  TS_Subtype_Find(cmp->ts, & subtype, parent_expr.type, child);
  if (subtype == TS_TYPE_NONE) {
    MSG_ERROR("The type '%N' has not a subtype with name '%N'", parent, child);
    return Failed;
  }

  Expr_New_Type(result, subtype);
  return Success;
}

static Task Reg_SubSubtype(Expr *result, Expr *reg_parent, Name *child) {
  Type subtype_type;
  if (!reg_parent->is.typed) {
    MSG_ERROR("Cannot refer to the subtype '%N' of the undefined type '%N'",
              child, reg_parent->value.nm);
    return Failed;
  }

  TS_Subtype_Find(cmp->ts, & subtype_type, reg_parent->type, child);
  if (subtype_type == TS_TYPE_NONE) {
    MSG_ERROR("The type '%~s' has not a subtype with name '%N'",
              TS_Name_Get(cmp->ts, reg_parent->type), child);
    return Failed;
  }

  Expr_New_Type(result, subtype_type);
  return Success;
}

static Task Unreg_Subtype(Expr *result, Name *parent, Name *child) {
  Expr parent_expr;
  Type subtype_type;
  TASK( Prs_Name_To_Expr(parent, & parent_expr, 0) );
  if (!parent_expr.is.typed) {
    MSG_ERROR("Cannot refer to the subtype '%N' of the undefined type '%N'",
              child, parent);
    return Failed;
  }
  TS_Subtype_New(cmp->ts, & subtype_type, parent_expr.type, child);
  Expr_New_Type(result, subtype_type);
  return Success;
}

static Task Unreg_SubSubtype(Expr *result, Expr *reg_parent, Name *child) {
  Type subtype_type;
  if (!reg_parent->is.typed) {
    MSG_ERROR("Cannot refer to the subtype '%N' of the undefined type '%N'",
              child, reg_parent->value.nm);
    return Failed;
  }
  assert(TS_Is_Subtype(cmp->ts, reg_parent->type));
  TS_Subtype_New(cmp->ts, & subtype_type, reg_parent->type, child);
  Expr_New_Type(result, subtype_type);
  return Success;
}

static Task Register_Subtype(Expr *result, Expr *unreg_subtype, Expr *type) {
  *result = *type;
  if (!unreg_subtype->is.typed) {
    MSG_ERROR("Register_Subtype: '%N' is not a subtype!",
              unreg_subtype->value.nm);
    return Failed;
  }
  if (!type->is.typed) {
    MSG_ERROR("Cannot define the subtype '%s': '%N' is untyped!",
              Tym_Type_Name(unreg_subtype->type), & type->value.nm);
    return Failed;
  }
  TASK( TS_Subtype_Register(cmp->ts, unreg_subtype->type, type->type) );
  return Success;
}

static Task Subtype_Create(Expr *result, Expr *parent, Name *child) {
  Expr this_parent;
  if (parent == (Expr *) NULL) {
    parent = & this_parent;
    TASK( Box_Parent_Get(parent, 0) );
  }
  TASK( Expr_Subtype_Create(result, parent, child) );
  TASK( Cmp_Expr_Destroy_Tmp(parent) );
  return Success;
}

static Task Expr_Statement(Expr *e) {
  TASK( Expr_Resolve_Subtype(e) );
  (void) Box_Procedure_Quick_Call(e, BOX_DEPTH_UPPER, BOX_MSG_VERBOSE);
  (void) Cmp_Expr_Destroy_Tmp(e);
  return Success;
}

static void Type_Detached(Expr *dst, Expr *src) {
  Type dt;
  if (Expr_Is_Error(src)) {*dst = *src; return;}
  TS_Detached_New(cmp->ts, & dt, src->type);
  Expr_New_Type(dst, dt);
}

/*****************************************************************************/

#define DO(action) \
  if IS_FAILED( action ) {parser_attr.no_syntax_err = 1; YYERROR;}

#define OPERATORA_EXEC(opr, rs, a, b) \
  if IS_FAILED( Prs_Operator(opr, & (rs), a, b) ) \
    {(rs).is.ignore = 1; parser_attr.no_syntax_err = 1; YYERROR;} \
  (rs).is.ignore = 1;

#define OPERATOR2_EXEC(opr, rs, a, b) \
  if IS_FAILED( Prs_Operator(opr, & (rs), a, b) ) \
    {parser_attr.no_syntax_err = 1; YYERROR;}

#define OPERATOR1L_EXEC(opr, rs, a) \
  Expr *result = Cmp_Operator_Exec(opr, a, NULL); \
  if ( result == NULL ) {parser_attr.no_syntax_err = 1; YYERROR;} \
   else rs = *result;

#define OPERATOR1R_EXEC(opr, rs, a) \
  Expr *result = Cmp_Operator_Exec(opr, NULL, a); \
  if ( result == NULL ) {parser_attr.no_syntax_err = 1; YYERROR;} \
   else rs = *result;

#define MY_ERR {parser_attr.no_syntax_err = 1; YYERROR;}
#endif















#if 0

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

/*****************************************************************************
 *                                  Types                                    *
 *****************************************************************************/

name.type:
  TOK_UNAME suffix.opt     { DO( Prs_Name_To_Expr(& $1, & $$, $2) ) }
 ;

prim.type:
  name.type                 { $$ = $1; }
| '(' type ')'              { $$ = $2; }
| type.struc                { $$ = $1; }
| '(' type.species ')'      { $$ = $2; }
 ;

array.type:
  prim.type                 { $$ = $1; }
| '(' expr ')' type         { if ( Prs_Array_Of_X(& $$, & $2, & $4) ) MY_ERR }
| '(' ')' type              { if ( Prs_Array_Of_X(& $$, NULL, & $3) ) MY_ERR }
 ;

detached.type:
  array.type                { $$ = $1; }
| TOK_INC array.type        { Type_Detached(& $$, & $2); }
;

type:
  array.type                { $$ = $1; }
;

asgn.type:
  detached.type             { $$ = $1; }
| name.type '=' asgn.type   { if ( Prs_Rule_Typed_Eq_Typed(& $$, & $1, & $3) ) MY_ERR }
| expr '=' asgn.type        { if (Prs_Rule_Valued_Eq_Typed(& $$, & $1, & $3) ) MY_ERR }
| unregistered.subtype '=' asgn.type          {DO(Register_Subtype(& $$, & $1, & $3));}
 ;

type.species:
  type TOK_TO type          { if ( Prs_Species_New(& $$, & $1, & $3) ) MY_ERR }
| type.species TOK_TO type  { if ( Prs_Species_Add(& $$, & $1, & $3) ) MY_ERR }
 ;

/*****************************************************************************
 *                            Structure types                                *
 *****************************************************************************/

/* Matches the first element of a structure type */
type.struc1:
  type                     {DO(Type_Struc_Begin(& $$, & $1, (char *) NULL));}
| type TOK_LNAME           {DO(Type_Struc_Begin(& $$, & $1, Name_To_Str(& $2)));}
;

/* Matches the second, third, ... element of a structure type */
type.struc234:
  type.struc1              {$$ = $1;}
| TOK_LNAME                {$$.type = TYPE_NONE; $$.name = Name_To_Str(& $1);}
;

/* Matches a structure type with at least two elements */
type.struc1234:
  type.struc1 void.seps type.struc234    {Type_Struc_All_1(& $$, & $1, & $3);}
| type.struc1234 void.seps type.struc234 {Type_Struc_All_2(& $$, & $1, & $3);}
;

/* Matches all sorts of structure types */
type.struc:
  '(' type.struc1 void.seps ')'          {Type_Struc_1(& $$, & $2);}
| '(' type.struc1234 void.seps.opt ')'   {DO(Type_Struc_2(& $$, & $2));}
;


registered.subtype:
  TOK_UNAME TOK_UMEMBER                  {DO(Reg_Subtype(& $$, & $1, & $2));}
| registered.subtype TOK_UMEMBER         {DO(Reg_SubSubtype(& $$, & $1, & $2));}
;

unregistered.subtype:
  TOK_UNAME TOK_UMEMBER                  {DO(Unreg_Subtype(& $$, & $1, & $2));}
| registered.subtype TOK_UMEMBER         {DO(Unreg_SubSubtype(& $$, & $1, & $2));}
;

/*****************************************************************************
 *                           Structure expression                            *
 *****************************************************************************/
expr.struc.all:
  expr void.seps expr           { if ( Cmp_Structure_Begin()   ) MY_ERR
                                  if ( Cmp_Structure_Add(& $1) ) MY_ERR
                                  if ( Cmp_Structure_Add(& $3) ) MY_ERR }
| expr.struc.all void.seps expr { if ( Cmp_Structure_Add(& $3) ) MY_ERR }
 ;

expr.struc:
  '(' expr void.seps ')'               { if ( Cmp_Structure_Begin()   ) MY_ERR
                                         if ( Cmp_Structure_Add(& $2) ) MY_ERR
                                         if ( Cmp_Structure_End(& $$) ) MY_ERR }
| '(' expr.struc.all void.seps.opt ')' { if ( Cmp_Structure_End(& $$) ) MY_ERR }
;

/*****************************************************************************
 *                                 Strings                                   *
 *****************************************************************************/
string:
  TOK_STRING           { $$ = $1; }
| string TOK_STRING    { if IS_FAILED( Name_Cat_And_Free(& $$, & $1, & $2) ) MY_ERR }
 ;

/*****************************************************************************
 *                           Primary Expressions                             *
 *****************************************************************************/
simple.expr:
  TOK_LNAME suffix.opt    { Prs_Name_To_Expr( & $1, & $$, $2); }
| TOK_EXPR                { $$ = $1; }
| string                  { if IS_FAILED( Cmp_String_New_And_Free(& $$, & $1) ) MY_ERR }
| TOK_NPARENT suffix.opt  { DO( Box_NParent_Get(& $$, $1, $2) ); }
 ;

array.expr:
  simple.expr             { $$ = $1; }
| array.expr '(' expr ')' { DO(Expr_Array_Member(& $$, & $1, & $3)); }
;

subtype.expr:
  prim.expr TOK_UMEMBER {DO(Subtype_Create(& $$, & $1, & $2));}
| TOK_UMEMBER           {DO(Subtype_Create(& $$, (Expr *) NULL, & $1));}
;

prim.expr:
   array.expr           { $$ = $1; }

 | '(' expr ')'         { $$ = $2; $$.is.ignore = 0; }

 | expr.struc           { $$ = $1; }

 | prim.expr
   '['                  { DO( Box_Instance_Begin(& $1, 2) ); }
   statement.list
   ']'                  { DO( Box_Instance_End(& $1) ); }

 | name.type
   '['                  { DO( Box_Instance_Begin(& $1, 1) ); }
   statement.list
   ']'                  { DO( Box_Instance_End(& $1) ); }

| subtype.expr
   '['                  { DO( Box_Instance_Begin(& $1, 1) ); }
   statement.list
   ']'                  { DO( Box_Instance_End(& $1)); }
 ;

/* Espressioni secondarie */
expr:
   prim.expr           { $$ = $1; }

 | TOK_LMEMBER {
    Expr e_parent;
    TASK( Box_Parent_Get(& e_parent, 0) );
    DO(Expr_Struc_Member(& $$, & e_parent, & $1));
    $$.is.release = 0;
  }

 | expr TOK_LMEMBER {DO(Expr_Struc_Member(& $$, & $1, & $2));}

 | expr '=' expr        { if (Prs_Rule_Valued_Eq_Valued(& $$, & $1, & $3) ) MY_ERR}

 | expr TOK_APLUS expr  { OPERATORA_EXEC(cmp_opr.aplus, $$, & $1, & $3); }
 | expr TOK_AMINUS expr { OPERATORA_EXEC(cmp_opr.aminus,$$, & $1, & $3); }
 | expr TOK_ATIMES expr { OPERATORA_EXEC(cmp_opr.atimes,$$, & $1, & $3); }
 | expr TOK_ADIV expr   { OPERATORA_EXEC(cmp_opr.adiv,  $$, & $1, & $3); }
 | expr TOK_AREM expr   { OPERATORA_EXEC(cmp_opr.arem,  $$, & $1, & $3); }
 | expr TOK_ABAND expr  { OPERATORA_EXEC(cmp_opr.aband, $$, & $1, & $3); }
 | expr TOK_ABXOR expr  { OPERATORA_EXEC(cmp_opr.abxor, $$, & $1, & $3); }
 | expr TOK_ABOR expr   { OPERATORA_EXEC(cmp_opr.abor,  $$, & $1, & $3); }
 | expr TOK_ASHL expr   { OPERATORA_EXEC(cmp_opr.ashl,  $$, & $1, & $3); }
 | expr TOK_ASHR expr   { OPERATORA_EXEC(cmp_opr.ashr,  $$, & $1, & $3); }

 | expr TOK_LOR expr    { OPERATOR2_EXEC(cmp_opr.lor,  $$, & $1, & $3); }
 | expr TOK_LAND expr   { OPERATOR2_EXEC(cmp_opr.land, $$, & $1, & $3); }

 | expr '|' expr        { OPERATOR2_EXEC(cmp_opr.bor,  $$, & $1, & $3); }
 | expr '^' expr        { OPERATOR2_EXEC(cmp_opr.bxor, $$, & $1, & $3); }
 | expr '&' expr        { OPERATOR2_EXEC(cmp_opr.band, $$, & $1, & $3); }

 | expr TOK_EQ expr     { OPERATOR2_EXEC(cmp_opr.eq, $$, & $1, & $3); }
 | expr TOK_NE expr     { OPERATOR2_EXEC(cmp_opr.ne, $$, & $1, & $3); }
 | expr '<' expr        { OPERATOR2_EXEC(cmp_opr.lt, $$, & $1, & $3); }
 | expr TOK_LE expr     { OPERATOR2_EXEC(cmp_opr.le, $$, & $1, & $3); }
 | expr '>' expr        { OPERATOR2_EXEC(cmp_opr.gt, $$, & $1, & $3); }
 | expr TOK_GE expr     { OPERATOR2_EXEC(cmp_opr.ge, $$, & $1, & $3); }

 | expr TOK_SHL expr    { OPERATOR2_EXEC(cmp_opr.shl, $$, & $1, & $3); }
 | expr TOK_SHR expr    { OPERATOR2_EXEC(cmp_opr.shr, $$, & $1, & $3); }

 | expr '+' expr        { OPERATOR2_EXEC(cmp_opr.plus, $$, & $1, & $3); }
 | expr '-' expr        { OPERATOR2_EXEC(cmp_opr.minus,$$, & $1, & $3); }
 | expr '*' expr        { OPERATOR2_EXEC(cmp_opr.times,$$, & $1, & $3); }
 | expr '/' expr        { OPERATOR2_EXEC( cmp_opr.div, $$, & $1, & $3); }
 | expr '%' expr        { OPERATOR2_EXEC( cmp_opr.rem, $$, & $1, & $3); }
 | expr TOK_POW expr    { OPERATOR2_EXEC( cmp_opr.pow, $$, & $1, & $3); }
 | '-' expr %prec TOK_NEG { OPERATOR1L_EXEC( cmp_opr.minus, $$, & $2 ); }
 | '+' expr %prec TOK_NEG { OPERATOR1L_EXEC(  cmp_opr.plus, $$, & $2 ); }

 | '!' expr             { OPERATOR1L_EXEC( cmp_opr.lnot, $$, & $2 ); }

 | '~' expr             { OPERATOR1L_EXEC( cmp_opr.bnot, $$, & $2 ); }

 | TOK_INC expr         { OPERATOR1L_EXEC( cmp_opr.inc, $$, & $2 ); }
 | TOK_DEC expr         { OPERATOR1L_EXEC( cmp_opr.dec, $$, & $2 ); }

 | expr TOK_INC         { OPERATOR1R_EXEC( cmp_opr.inc, $$, & $1 ); }
 | expr TOK_DEC         { OPERATOR1R_EXEC( cmp_opr.dec, $$, & $1 ); }
;

/******************************************************************************
 *               DEFINIZIONE DELL'ESPRESSIONE COME ISTRUZIONE                 *
 ******************************************************************************/
type.statement:
   asgn.type { }
;

/******************************************************************************
 *               DEFINIZIONE DELL'ESPRESSIONE COME ISTRUZIONE                 *
 ******************************************************************************/
expr.statement:
   expr      { DO(Expr_Statement(& $1)) }
|  '\\' expr { DO(Expr_Ignore(& $2)) }
;

/******************************************************************************/
/*               DEFINIZIONE DELLA SINTASSI DELL'ISTRUZIONE END               */
/******************************************************************************/
end.statement:
  TOK_END { Cmp_Assemble(ASM_RET); }
;

child:
   type                 {$$ = $1;}
|  TOK_PROC             {Expr_New_Type(& $$, $1);}
;

parent:
   type                 {$$ = $1;}
 | registered.subtype   {$$ = $1;}
;

parent.opt:
                        {}
 | parent               {$$ = $1;}
;

proc.def:
   TOK_AT parent
   '['
   statement.list
   ']'                  {}

 | child TOK_AT parent.opt
   '['                  {DO(Proc_Def_Open(& $1, $2, & $3))}
   statement.list
   ']'                  {DO(Proc_Def_Close())}

 | child TOK_AT parent.opt
   '?' TOK_STRING       {DO(Declare_Proc(& $1, $2, & $3, & $5))}
;

%type <Ex> name.type
%type <Ex> prim.type
%type <Ex> type
%type <Ex> asgn.type
%type <Ex> type.species
%type <Ex> simple.expr
%type <Ex> array.expr
%type <Ex> expr
%type <Ex> prim.expr
%type <Ex> child
%type <Ex> parent
%type <Ex> parent.opt
%type <Nm> prim.suffix
%type <Sf> suffix
%type <Sf> suffix.opt
%type <Nm> string


%type <Ex> expr.struc
%type <Ex> expr.struc.all

%type <SM> type.struc1
%type <SM> type.struc234
%type <Sc> type.struc1234
%type <Ex> type.struc

%type <Ex> array.type
%type <Ex> detached.type

%type <Ex> registered.subtype
%type <Ex> unregistered.subtype
%type <Ex> subtype.expr
#endif
#if 0
  Expr          Ex;   /* Espressione generica */
  Name          Nm;   /* Nome */
  Int           Sf;   /* Suffisso che specifica lo scoping per le variabili */
  Type          Ty;   /* Tipo */
  Struc         Sc;   /* Incomplete structure type */
  StrucMember   SM;   /* Member of structure type */
  Int           kind; /* Per TOK_AT */
  Int           Int;
  Type          proc; /* Per TOK_PROC */

void.seps:
  void.sep
| void.seps void.sep
;

void.seps.opt:
| void.seps
;

#endif

%}

/* Possible types for the nodes of the tree */
%union {
  char *     String;
  AstUnOp    UnaryOperator;
  AstBinOp   BinaryOperator;
  AstNodePtr Node;
}

/* Lista dei token senza valore semantico
 */
%token TOK_END

%token TOK_ERR

%token TOK_NEWLINE
%token TOK_ERRSEP

%token TOK_TO

%token <Int> TOK_NPARENT

/* Lista dei token aventi valore semantico
 */
%token <Ex> TOK_EXPR
%token <kind> TOK_AT
%token <proc> TOK_PROC

/* List of tokens simple tokens */
%token TOK_INC TOK_DEC TOK_SHL TOK_SHR
%token TOK_EQ TOK_NE TOK_LT TOK_LE TOK_GT TOK_GE
%token TOK_LOR TOK_LAND
%token TOK_APLUS TOK_AMINUS TOK_ATIMES TOK_ADIV TOK_AREM TOK_ABAND TOK_ABXOR
%token TOK_ABOR TOK_ASHL TOK_ASHR

/* List of tokens with semantical value */
%token <String> TOK_IDENTIFIER TOK_TYPE_IDENT
%token <Node> TOK_CONSTANT TOK_STRING

/* List of nodes with semantical value */
%type <UnaryOperator> un_op post_op
%type <BinaryOperator> mul_op add_op shift_op cmp_op eq_op assign_op
%type <Node> expr_sep sep_expr struc_expr
%type <Node> string_concat prim_expr postfix_expr unary_expr mul_expr add_expr
%type <Node> shift_expr cmp_expr eq_expr band_expr bxor_expr bor_expr
%type <Node> land_expr lor_expr assign_expr expr statement statement_list
%type <Node> member_type type_sep sep_type struc_type prim_type type

/* Lista dei token affetti da regole di precedenza
 */
%right TOK_POW
%left TOK_UMEMBER


/* Regola di partenza
 */
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
    void_sep
  | ';'
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
    expr void_seps               {$$ = AstNodeStruc_New(NULL, $1);}
  | sep_expr void_seps           {$$ = $1;}
  ;

sep_expr:
    void_seps expr               {$$ = AstNodeStruc_New(NULL, $2);}
  | expr_sep expr                {$$ = AstNodeStruc_Add_Member($1, NULL, $2);}
  ;

struc_expr:
    expr_sep                     {$$ = $1;}
  | sep_expr                     {$$ = $1;}
  ;

/******************************* ARITHMETICS *******************************/
string_concat:
    TOK_STRING                   {$$ = $1;}
  | string_concat TOK_STRING     {$$ = AstNodeString_Concat($1, $2);}
  ;

prim_expr:
    TOK_CONSTANT                 {$$ = $1;}
  | string_concat                {$$ = $1;}
  | TOK_IDENTIFIER               {$$ = AstNodeVar_New($1, 0); BoxMem_Free($1);}
  | '(' expr ')'                 {$$ = $2;}
  | '(' struc_expr ')'           {$$ = $2;}
  ;

postfix_expr:
    prim_expr                    {$$ = $1;}
  | postfix_expr '(' expr ')'    {$$ = AstNodeArrayGet_New($1, $3);}
  | postfix_expr
          '[' statement_list ']' {$$ = AstNodeBox_Set_Parent($3, $1);}
  | type  '[' statement_list ']' {$$ = AstNodeBox_Set_Parent($3, $1);}
  | postfix_expr
              '.' TOK_IDENTIFIER {$$ = AstNodeMemberGet_New($1, $3, 0);
                                  BoxMem_Free($3);}
  | '.' TOK_IDENTIFIER           {$$ = AstNodeMemberGet_New(NULL, $2, 0);
                                  BoxMem_Free($2);}
  | postfix_expr post_op         {$$ = AstNodeUnOp_New($2, $1);}
  ;

unary_expr:
    postfix_expr                 {$$ = $1;}
  | un_op unary_expr             {$$ = AstNodeUnOp_New($1, $2);}
  ;

mul_expr:
    unary_expr                   {$$ = $1;}
  | mul_expr mul_op unary_expr   {$$ = AstNodeBinOp_New($2, $1, $3);}
  ;

add_expr:
    mul_expr                     {$$ = $1;}
  | add_expr add_op mul_expr     {$$ = AstNodeBinOp_New($2, $1, $3);}
  ;

shift_expr:
    add_expr                     {$$ = $1;}
  | shift_expr shift_op add_expr {$$ = AstNodeBinOp_New($2, $1, $3);}
  ;

cmp_expr:
    shift_expr                   {$$ = $1;}
  | cmp_expr cmp_op shift_expr   {$$ = AstNodeBinOp_New($2, $1, $3);}
  ;

eq_expr:
    cmp_expr                     {$$ = $1;}
  | eq_expr eq_op cmp_expr       {$$ = AstNodeBinOp_New($2, $1, $3);}
  ;

band_expr:
    eq_expr                      {$$ = $1;}
  | band_expr '&' eq_expr        {$$ = AstNodeBinOp_New(ASTBINOP_BAND, $1, $3);}
  ;

bxor_expr:
    band_expr                    {$$ = $1;}
  | bxor_expr '^' band_expr      {$$ = AstNodeBinOp_New(ASTBINOP_BXOR, $1, $3);}
  ;

bor_expr:
    bxor_expr                    {$$ = $1;}
  | bor_expr '|' bxor_expr       {$$ = AstNodeBinOp_New(ASTBINOP_BOR, $1, $3);}
  ;

land_expr:
    bor_expr                     {$$ = $1;}
  | land_expr TOK_LAND bor_expr  {$$ = AstNodeBinOp_New(ASTBINOP_LAND, $1, $3);}
  ;

lor_expr:
    land_expr                    {$$ = $1;}
  | lor_expr TOK_LOR land_expr   {$$ = AstNodeBinOp_New(ASTBINOP_LOR, $1, $3);}
  ;

assign_expr:
    lor_expr                     {$$ = $1;}
  | assign_expr
              assign_op lor_expr {$$ = AstNodeBinOp_New($2, $1, $3);}
  ;

expr:
    assign_expr                  {$$ = $1;}
  ;

/***************************** TYPE ARITHMETICS ****************************/

member_type:
    type                         {}
  | type TOK_IDENTIFIER          {}
  ;

type_sep:
    member_type void_seps        {$$ = $1;}
  | sep_type void_seps           {$$ = $1;}
  ;

sep_type:
    void_seps member_type        {$$ = $2;}
  | type_sep member_type         {$$ = $1;}
  ;

struc_type:
    type_sep                     {$$ = $1;}
  | sep_type                     {$$ = $1;}
  ;

prim_type:
    TOK_TYPE_IDENT               {$$ = AstNodeTypeName_New($1, 0);
                                  BoxMem_Free($1);}
  | '(' type ')'                 {$$ = $2;}
  | '(' struc_type ')'           {$$ = $2;}
  ;

type:
    prim_type                    {$$ = $1;}
  ;

/************************ STATEMENT LISTS AND BOXES ************************/
/* Cio' che resta descrive la sintassi delle righe e del corpo del programma */
statement:
                                 {$$ = NULL;}
  | expr                         {$$ = AstNodeStatement_New($1);}
  | '\\' expr                    {$$ = AstNodeStatement_New($2);}
  | error sep                    {MSG_ERROR("Syntax error.");
                                  $$ = AstNodeStatement_New(AstNodeError_New());
                                  Tok_Unput(',');
                                  yyerrok;}
  ;

statement_list:
    statement                    {$$ = AstNodeBox_New(NULL, $1);}
  | statement_list sep statement {$$ = AstNodeBox_Add_Statement($1, $3);}
  ;

program:
    statement_list               {AstNode_Print(stdout, $1);}
  ;

%%

/* error function */
void yyerror(char* s)
{
 /* MSG_ERROR(s);*/
  return;
}

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
  TASK( Tok_Init(TOK_MAX_INCLUDE, f) );

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


#if 0
static Task Declare_Proc(Expr *child_type, Int kind, Expr *parent_type,
                         Name *name) {
  BoxVMSymID sym_num;
  Int proc;
  sym_num = VM_Sym_New_Call(cmp_vm);
  TASK( BoxVMSym_Name_Set(cmp_vm, sym_num, name) );
  proc = TS_Procedure_Def(child_type->type, kind, parent_type->type, sym_num);
  return (proc == TYPE_NONE) ? Failed : Success;
}

static Task Proc_Def_Open(Expr *child_type, Int kind, Expr *parent_type) {
  return Box_Def_Begin(parent_type->type, child_type->type, kind);
}

static Task Proc_Def_Close(void) {
  return Box_Def_End();
}

#if 0
enum {EXPR_TYPE, EXPR_VALUE};

Task Prs_Want(Expr *e, UInt want) {
  switch(want) {
  case EXPR_TYPE:
    break;
  case EXPR_VALUE:
    break;
  }
}
#endif

/* This function assigns the expression *e, to the untyped
 * expression *new_e: if *e is a target, *new_e will be a copy of *e,
 * otherwise *e will be simply copied into *new_e.
 */
Task Prs_Def_Operator(Operator *opr,
                      Expr *rs, Expr *new_e, Expr *e) {
  Symbol *s;
  Expr *result, *target;
  Container *container;

  if ( ! e->is.typed ) {
    MSG_ERROR("The expression on the right of '%s' must have a type!",
              opr->name);
    return Failed;
  }

  assert( (!new_e->is.typed) && (opr->can_define) && (e->is.value) );

  {
    Task t = Sym_Explicit_New(& s, & new_e->value.nm, new_e->addr);
    Cmp_Expr_Destroy_Tmp(new_e);
    TASK( t );
  }

  container = (Box_Def_Num() == 0) ?
              CONTAINER_GVAR_AUTO : CONTAINER_LVAR_AUTO;
  target = & (s->value);
  if ( e->is.target ) {
    Expr_Container_New(target, e->type, container);
    Expr_Alloc(target);
    result = Cmp_Operator_Exec(opr, target, e);
    if ( result == NULL ) return Failed;
    *rs = *result;
    return Success;

  } else {
    Expr_Container_New(target, e->type, container);
    TASK( Cmp_Expr_To_X(e, target->categ, target->value.i, 0) );
    target->is.allocd = e->is.allocd;
    e->is.allocd = 0;
    TASK( Cmp_Expr_Destroy_Tmp(e) );
    *rs = *target;
    return Success;
  }
}

/* DESCRIPTION: This function compiles the binary operation opr.
 *  a and b are the two arguments, rs is the result.
 */
Task Prs_Operator(Operator *opr, Expr *rs, Expr *a, Expr *b) {
  if ( a->is.typed && b->is.typed ) {
    Expr *result = Cmp_Operator_Exec(opr, a, b);
    if ( result == NULL ) return Failed;
    *rs = *result;
    return Success;

  } else {
    if ( opr->can_define ) {
      return Prs_Def_Operator(opr, rs, a, b);
    } else {
      MSG_ERROR("The expression should have type!");
      return Failed;
    }
  }
}

/* If name:suffix is the name of an already defined symbol
 * this function puts the corresponding expression into *e, otherwise
 * it transforms the name *nm into an untyped expression *e.
 */
Task Prs_Name_To_Expr(Name *nm, Expr *e, Int suffix) {
  Symbol *s;

  if ( suffix < 0 ) {
    /* Non e' stata specificata la profondita' di scatola! */
    s = Sym_Explicit_Find(nm, 0, NO_EXACT_DEPTH);
    suffix = 0;
  } else {
    /* E' stata specificata la profondita' di scatola! */
    s = Sym_Explicit_Find(nm, suffix, EXACT_DEPTH);
  }

  if ( s == NULL ) {
    /* Il nome non corrisponde ad un simbolo gia' definito:
     * restituisco una espressione (untyped) corrispondente.
     */
    e->is.typed = 0;
    e->is.ignore = 0;
    e->addr = suffix;
    e->value.nm = *Name_Dup(nm);
    if ( e->value.nm.text == NULL ) return Failed;
    return Success;

  } else {
   /* Il nome corrisponde ad un simbolo gia' definito:
    * restituisco l'espressione (typed) corrispondente!
    */
    *e = s->value;
    return Success;
  }
}

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

/* DESCRIPTION: This function build an alias of the type *x.
 * NOTE: The new type will be put into *alias.
 */
Task Prs_Alias_Of_X(Expr *alias, Expr *x) {
  Symbol *s;
  Expr *target;
  register Int t;

  assert( !alias->is.typed );

  if ( ! x->is.typed ) {
    MSG_ERROR("L'espressione alla destra di '=' deve avere tipo!");
    return Failed;
  }

  TASK( Sym_Explicit_New(& s, & alias->value.nm, alias->addr) );

  target = & (s->value);
  target->is.typed = 1;
  target->is.value = 0;
  target->is.imm = target->is.target = 0;
  target->type = t = Tym_Def_Alias_Of(& alias->value.nm, x->type);
  TASK( Cmp_Expr_Destroy_Tmp(alias) );
  *alias = *target;
  if ( t != TYPE_NONE ) return Success;
  return Failed;
}

#define CHECK_TYPE(t)                   \
  if ( ! t->is.typed ) {                \
     MSG_ERROR("'%s' <-- Unknown type", \
      Name_To_Str(& t->value.nm));      \
     return Failed;                     \
  }                                     \
  assert(! t->is.value)

/* This function creates the new specie of types (first < second).
 */
Task Prs_Species_New(Expr *species, Expr *first, Expr *second) {
  Int new_species;

  CHECK_TYPE(first); CHECK_TYPE(second);

  new_species = TYPE_NONE;
  TASK( Tym_Def_Specie( & new_species,  first->type) );
  TASK( Tym_Def_Specie( & new_species, second->type) );

  species->is.typed = 1;
  species->is.value = 0;
  species->type = new_species;
  return Success;
}

/* This function adds a new type to an already existing species.
 */
Task Prs_Species_Add(Expr *species, Expr *old, Expr *type) {
  Int old_species;

  CHECK_TYPE(old); CHECK_TYPE(type);

  old_species = old->type;
  assert( old_species != TYPE_NONE );
  TASK( Tym_Def_Specie( & old_species, type->type) );
  *species = *old;
  return Success;
}

/* This function handles the rule: Type = Type
 */
Task Prs_Rule_Typed_Eq_Typed(Expr *rs, Expr *typed1, Expr *typed2) {

  if ( typed1->is.typed ) {
    /* Assertion: 'typed2' must be equal to 'typed1'. */
    *rs = *typed2;
    if ( ! typed2->is.typed ) {
      MSG_ERROR("The type on the right hand side of '=' "
                "has not been defined yet!");
      return Failed;
    }
    if ( Tym_Compare_Types(typed1->type, typed2->type, NULL) == 1 )
      return Success;
    MSG_ERROR("Type mismatch!");
    return Failed;

  } else {
    /* Definition: Creation (into 'typed1') of an alias of 'typed2'. */
    TASK( Prs_Alias_Of_X(typed1, typed2) );
    *rs = *typed1;
    return Success;
  }
}

/* This function handles the rule: value = Type
 */
Task Prs_Rule_Valued_Eq_Typed(Expr *rs, Expr *valued, Expr *typed) {
  Container *container;
  *rs = *typed;
  rs->is.ignore = 1;
  if ( ! typed->is.typed ) {
    MSG_ERROR("Undefined type on the right of '='.");
    (void) Cmp_Expr_Destroy_Tmp(valued);
    return Failed;
  }

  if ( valued->is.typed ) {
    /* Assertion: the type of 'valued' should be 'typed'. */
    Int t = valued->type;
    TASK( Cmp_Expr_Destroy_Tmp(valued) );

    if ( Tym_Compare_Types(t, typed->type, NULL) == 1 ) return Success;
    MSG_ERROR("Type mismatch!");
    return Failed;

  } else {
    /* Definition: creating (into 'valued') a new object of type 'typed'. */
    Symbol *s;
    Expr *target;

    {
      Task t = Sym_Explicit_New(& s, & valued->value.nm, valued->addr);
      TASK( Cmp_Expr_Destroy_Tmp(valued) );
      TASK( t );
    }

    target = & (s->value);
    container = (Box_Def_Num() == 0) ?
                CONTAINER_GVAR_AUTO : CONTAINER_LVAR_AUTO;
    Expr_Container_New(target, typed->type, container);
    Expr_Alloc(target);
    return Success;
  }
}

/* This function handles the rule: Type = value
 */
Task prs_rule_typed_eq_valued(Expr *typed, Expr *valued) {
  return Success;
}

/* This function handles the rule: value = value
 */
Task Prs_Rule_Valued_Eq_Valued(Expr *rs,
                               Expr *valued1, Expr *valued2) {

  *rs = *valued2;
  rs->is.ignore = 1;
  if ( ! valued2->is.typed ) {
    MSG_ERROR("The expression on the right of '=' "
              "has not a well defined type!");
    (void) Cmp_Expr_Destroy_Tmp(valued1);
    return Failed;
  }

  if ( valued1->is.typed ) {
    /* Assignment: assigns 'valued2' to 'valued1'. */
    Expr *result = Cmp_Operator_Exec(cmp_opr.assign, valued1, valued2);
    if ( result == NULL ) return Failed;
    *rs = *result;
    rs->is.ignore = 1;
    return Success;

  } else {
    /* Definition & assignment: define 'valued1' as an object with the same
     *  type of 'valued2', and assigns 'valued2' to 'valued1'.
     */
    TASK( Prs_Def_Operator(cmp_opr.assign, rs, valued1, valued2) );
    rs->is.ignore = 1;
    return Success;
  }
}
#endif
