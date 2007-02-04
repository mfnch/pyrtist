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

/* parser.c - settembre 2002
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "types.h"
#include "defaults.h"
#include "messages.h"
#include "array.h"
#include "str.h"
#include "virtmach.h"
#include "vmsym.h"
#include "vmsymstuff.h"
#include "compiler.h"
#include "box.h"
#include "tokenizer.h"
#include "parserh.h"
#include "builtins.h"

#define YYDEBUG 0

ParserAttr parser_attr;

/* Funzioni definite in questo file */
void yyerror(char* s);

static Task Proc_Def_Open(Expr *child_type, Expr *parent_type);
static Task Proc_Def_Close(void);
static Task Declare_Proc(Expr *child_type, Expr *parent_type, Name *name);

/* Numero della linea che e' in fase di lettura dal tokenizer */
extern UInt tok_linenum;

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
  Expression *result = Cmp_Operator_Exec(opr, a, NULL); \
  if ( result == NULL ) {parser_attr.no_syntax_err = 1; YYERROR;} \
   else rs = *result;

#define OPERATOR1R_EXEC(opr, rs, a) \
  Expression *result = Cmp_Operator_Exec(opr, NULL, a); \
  if ( result == NULL ) {parser_attr.no_syntax_err = 1; YYERROR;} \
   else rs = *result;

#define BOX_OPEN(expr) \
  if IS_FAILED( Box_Instance_Begin( expr ) ) \
    {parser_attr.no_syntax_err = 1; parser_attr.old_box = 0; YYERROR;} \
  if (expr != NULL) \
    if IS_FAILED( Prs_Procedure_Special(NULL, TYPE_OPEN, 0) ) MY_ERR

#define BOX_CLOSE(expr) \
  if (expr != NULL) \
    if IS_FAILED( Prs_Procedure_Special(NULL, TYPE_CLOSE, 0) ) MY_ERR; \
  if IS_FAILED( Box_Instance_End( expr ) ) \
    {parser_attr.no_syntax_err = 1; YYERROR;}

#define BOX_REOPEN(expr) \
  if IS_FAILED( Box_Instance_Begin( expr ) ) \
    {parser_attr.no_syntax_err = 1; parser_attr.old_box = 1; YYERROR;}

#define BOX_RECLOSE(expr) \
  if IS_FAILED( Box_Instance_End( expr ) ) \
    {parser_attr.no_syntax_err = 1; YYERROR;}

#define MY_ERR {parser_attr.no_syntax_err = 1; YYERROR;}
%}

/* Union che contiene tutti i tipi di valore semantico
 * che possono avere i token
 */
%union {
  Expression  Ex; /* Espressione generica */
  Name        Nm; /* Nome */
  Intg        Sf; /* Suffisso che specifica lo scoping per le variabili */
  Intg        Ty; /* Tipo */
}

/* Lista dei token senza valore semantico
 */
%token TOK_END

%token TOK_ERR

%token TOK_NEWLINE
%token TOK_ERRSEP

%token TMP_TOK_AGAIN
%token TMP_TOK_EXIT

/* Lista dei token aventi valore semantico
 */
%token <Ex> TOK_EXPR
%token <Nm> TOK_LNAME
%token <Nm> TOK_UNAME
%token <Nm> TOK_LMEMBER
%token <Nm> TOK_UMEMBER
%token <Nm> TOK_STRING

/* Lista delle espressioni aventi valore semantico
 */
%type <Ex> name.type
%type <Ex> prim.type
%type <Ex> type
%type <Ex> asgn.type
%type <Ex> type.list
%type <Ex> type.species
%type <Ex> simple.expr
%type <Ex> array.expr
%type <Ex> expr
%type <Ex> prim.expr
%type <Ex> parent
%type <Ex> parent.opt
%type <Nm> prim.suffix
%type <Sf> suffix
%type <Sf> suffix.opt
%type <Nm> string

/* Lista dei token affetti da regole di precedenza
 */
%right '=' TOK_APLUS TOK_AMINUS TOK_ATIMES TOK_ADIV TOK_AREM TOK_ABAND TOK_ABXOR TOK_ABOR TOK_ASHL TOK_ASHR
%left TOK_LOR
%left TOK_LAND
%left '|'
%left '^'
%left '&'
%left TOK_EQ TOK_NE
%left '<' TOK_LE '>' TOK_GE
%left TOK_SHL TOK_SHR
%left '+' '-'
%left '*' '/' '%'
%left TOK_NEG
%right TOK_POW
%left '!' '~' TOK_INC TOK_DEC
%left TOK_LMEMBER TOK_UMEMBER

/* Regola di partenza
 */
%start program

%%

/*****************************************************************************
 *                                 Separators                                *
 *****************************************************************************/
sep:
   ','                 { }
 | ';'                 { if IS_FAILED( Prs_Procedure_Special(NULL, TYPE_PAUSE, 1) ) MY_ERR }
 | TOK_NEWLINE         { Cmp_Assemble(ASM_LINE_Iimm, CAT_IMM, ++tok_linenum); }
 ;

/*****************************************************************************
 *       Grammatica relativa ai suffissi del tipo ::: o :tipo1:::tipo2::     *
 *****************************************************************************/
prim.suffix:
   ':'                 { $$ = *Name_Empty(); }
 | ':' TOK_UNAME       { $$ = *Name_Dup(& $2); if ($$.text == NULL ) MY_ERR }
 ;

suffix:
   prim.suffix         { if IS_FAILED( Prs_Suffix(& $$, -1, & $1) ) MY_ERR }
 | suffix prim.suffix  { if IS_FAILED( Prs_Suffix(& $$, $1, & $2) ) MY_ERR }
 ;

suffix.opt:
                       { $$ = -1; }
| suffix               { $$ = $1; }
 ;

/*****************************************************************************
 *                                  Types                                    *
 *****************************************************************************/
name.type:
  TOK_UNAME suffix.opt      { if ( Prs_Name_To_Expr(& $1, & $$, $2) ) MY_ERR }
| TOK_UMEMBER suffix.opt    { if ( Prs_Member_To_Expr(& $1, & $$, $2) ) MY_ERR }
 ;

prim.type:
  name.type                 { $$ = $1; }
| '(' type ')'              { $$ = $2; }
| '(' type.list ')'         { $$ = $2; }
| '(' type.species ')'      { $$ = $2; }
 ;

type:
  prim.type                 { $$ = $1; }
| '(' expr ')' type         { if ( Prs_Array_Of_X(& $$, & $2, & $4) ) MY_ERR }
| '(' ')' type              { if ( Prs_Array_Of_X(& $$, NULL, & $3) ) MY_ERR }
 ;

asgn.type:
  type                      { $$ = $1; }
| name.type '=' asgn.type   { if ( Prs_Rule_Typed_Eq_Typed(& $$, & $1, & $3) ) MY_ERR }
| expr '=' asgn.type        { if (Prs_Rule_Valued_Eq_Typed(& $$, & $1, & $3) ) MY_ERR }
 ;

type.list:
  type ',' type             { if ( Prs_Struct_New(& $$, & $1, & $3) ) MY_ERR }
| type.list ',' type        { if ( Prs_Struct_Add(& $$, & $1, & $3) ) MY_ERR }
 ;

type.species:
  type '<' type             { if ( Prs_Species_New(& $$, & $1, & $3) ) MY_ERR }
| type.species '<' type     { if ( Prs_Species_Add(& $$, & $1, & $3) ) MY_ERR }
 ;

/*****************************************************************************
 *                                 Strings                                   *
 *****************************************************************************/
string:
  TOK_STRING           { $$ = $1; }
| string TOK_STRING    { if IS_FAILED( Name_Cat_And_Free(& $$, & $1, & $2) ) MY_ERR }
 ;

/*****************************************************************************
 *                                  Lists                                    *
 *****************************************************************************/
expr.list:
  expr ',' expr       { if ( Cmp_Structure_Begin()   ) MY_ERR
                        if ( Cmp_Structure_Add(& $1) ) MY_ERR
                        if ( Cmp_Structure_Add(& $3) ) MY_ERR }
| expr.list ',' expr  { if ( Cmp_Structure_Add(& $3) ) MY_ERR }
 ;

/*****************************************************************************
 *                           Primary Expressions                             *
 *****************************************************************************/
simple.expr:
  TOK_LNAME suffix.opt    { Prs_Name_To_Expr( & $1, & $$, $2); }
| TOK_EXPR                { $$ = $1; }
| string                  { if IS_FAILED( Cmp_String_New_And_Free(& $$, & $1) ) MY_ERR }
 ;

array.expr:
  simple.expr             { $$ = $1; }
| array.expr '(' expr ')' { }
 ;

prim.expr:
   array.expr          { $$ = $1; }

 | '(' expr ')'        { $$ = $2; $$.is.ignore = 0; }

 | '(' expr.list ')'   { if ( Cmp_Structure_End(& $$) ) MY_ERR }

 | prim.expr
   '['                 { BOX_REOPEN( & $1 ); }
   statement.list
   ']'                 { BOX_RECLOSE( & $1 );      }

 | prim.type
   '['                 { BOX_OPEN( & $1 ); }
   statement.list
   ']'                 { BOX_CLOSE( & $1 );      }
 ;

/* Espressioni secondarie */
expr:
   prim.expr           { $$ = $1; }

/* | prim.expr TOK_UMEMBER    {} */

 | TOK_LMEMBER {
    Expression e_box, *result;
    Box *b;
    if IS_FAILED( Box_Get(& b, 0) ) {YYERROR;}
    if ( b->value.resolved == TYPE_VOID ) {
      MSG_WARNING("La box di tipo [...] non puo' possedere alcun membro!");
      YYERROR;
    }
    e_box = b->value;
    result = Cmp_Member_Get(& e_box, & $1);
    if ( result == NULL ) {
      parser_attr.no_syntax_err = 1;
      YYERROR;
    }
    result->is.release = 0;
    $$ = *result;
  }

 | expr TOK_LMEMBER {
    Expression *result = Cmp_Member_Get(& $1, & $2);
    if ( result == NULL ) {
      parser_attr.no_syntax_err = 1;
      YYERROR;
    } else $$ = *result;
  }

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
   expr { if IS_FAILED( Cmp_Procedure( NULL, & $1, -1, /* auto_define */ 1) ) MY_ERR }
;

/******************************************************************************/
/*               DEFINIZIONE DELLA SINTASSI DELL'ISTRUZIONE END               */
/******************************************************************************/
end.statement:
  TOK_END { Cmp_Assemble(ASM_RET); }
;

again.statement:
  TMP_TOK_AGAIN {
    Box *b;
    if IS_FAILED( Box_Get(& b, 0) ) {YYERROR;}
    VM_Label_Jump(cmp_vm, b->label_begin, 0);
  }
;

exit.statement:
  TMP_TOK_EXIT {
    Box *b;
    if IS_FAILED( Box_Get(& b, 0) ) {YYERROR;}
    VM_Label_Jump(cmp_vm, b->label_end, 0);
  }
;

parent:
   type                 {$$ = $1;}
 | type '@' parent      {$$ = $1;}
;

parent.opt:
                        {}
 | parent               {$$ = $1;}
;

proc.def:
   '@' parent
   '['
   statement.list
   ']'

 | type '@' parent.opt
   '['                  {DO(Proc_Def_Open(& $1, & $3))}
   statement.list
   ']'                  {DO(Proc_Def_Close())}

 | type '@' parent.opt
   '?' TOK_LNAME        {DO(Declare_Proc(& $1, & $3, & $5))}
;

 /*************DEFINIZIONE DELLA STRUTTURA GENERICA DEI PROGRAMMI**************/
 /* Cio' che resta descrive la sintassi delle righe e del corpo del programma */
statement:
 | type.statement
 | expr.statement
 | end.statement
 | again.statement
 | exit.statement
 | compound.statement
 | proc.def
 | error sep {
  if (! parser_attr.no_syntax_err ) {
    MSG_ERROR("Syntax error.");
  }
  parser_attr.no_syntax_err = 0;
    Tok_Unput(',');
    /*yyclearin;*/
    /*YYBACKUP(',', yylval);*/
  yyerrok;
  }
;

statement.list:
   statement
 | statement.list sep statement
 ;

compound.statement:
 '['               {BOX_OPEN( NULL );}
 statement.list
 ']'               {BOX_CLOSE( NULL );}
 ;

program:
   statement.list
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
Task Parser_Init(UInt maxinc, char *f) {

  /* Leggo comunque dallo stdin (che puo' essere stato rediretto)! */
  yyrestart( stdin );

  /* Inizializzo il tokenizer */
  TASK( Tok_Init( maxinc, f ) );

  parser_attr.no_syntax_err = 0;

#if YYDEBUG == 1
  yydebug = 1;
#endif

  return Success;
}

/* Completa il parsing del file di input.
 */
Task Parser_Finish(void) {
  return Success;
}

static Task Declare_Proc(Expr *child_type, Expr *parent_type, Name *name) {
  UInt sym_num;
  Int proc;
  TASK( VM_Sym_New_Call(cmp_vm, & sym_num) );
  TASK( VM_Sym_Name_Set(cmp_vm, sym_num, name) );
  proc = Tym_Def_Procedure(child_type->type, 0, parent_type->type, sym_num);
  return (proc == TYPE_NONE) ? Failed : Success;
}

static Task Proc_Def_Open(Expr *child_type, Expr *parent_type) {
  UInt sym_num;
  Int proc_type;
  TASK( VM_Sym_New_Call(cmp_vm, & sym_num) );
  proc_type = Tym_Def_Procedure(child_type->type,0,parent_type->type,sym_num);
  if (proc_type == TYPE_NONE) return Failed;
  return Box_Def_Begin(proc_type);
}

static Task Proc_Def_Close(void) {
  return Box_Def_End();
}

#if 0
enum {EXPR_TYPE, EXPR_VALUE};

Task Prs_Want(Expression *e, UInt want) {
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
                      Expression *rs, Expression *new_e, Expression *e) {
  Symbol *s;
  Expression *result, *target;

  if ( ! e->is.typed ) {
    MSG_ERROR("L'espressione alla destra di '%s' deve avere tipo!", opr->name);
    return Failed;
  }

  assert( (!new_e->is.typed) && (opr->can_define) && (e->is.value) );

  {
    Task t = Sym_Explicit_New(& s, & new_e->value.nm, new_e->addr);
    Cmp_Expr_Destroy_Tmp(new_e);
    TASK( t );
  }

  target = & (s->value);
  if ( e->is.target ) {
    TASK( Cmp_Expr_Create(target, e->type, /* temporary = */ 0) );
    result = Cmp_Operator_Exec(opr, target, e);
    if ( result == NULL ) return Failed;
    *rs = *result;
    return Success;

  } else {
    TASK( Cmp_Expr_Container_New(target, e->type, CONTAINER_LVAR_AUTO) );
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
Task Prs_Operator(Operator *opr, Expression *rs, Expression *a, Expression *b) {
  if ( a->is.typed && b->is.typed ) {
    Expression *result = Cmp_Operator_Exec(opr, a, b);
    if ( result == NULL ) return Failed;
    *rs = *result;
    return Success;

  } else {
    if ( opr->can_define ) {
      return Prs_Def_Operator(opr, rs, a, b);
    } else {
      MSG_ERROR("L'espressione deve avere tipo!");
      return Failed;
    }
  }
}

/* If name:suffix is the name of an already defined symbol
 * this function puts the corresponding expression into *e, otherwise
 * it transforms the name *nm into an untyped expression *e.
 */
Task Prs_Name_To_Expr(Name *nm, Expr *e, Intg suffix) {
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

/* If name:suffix is the name of an already defined symbol
 * this function puts the corresponding expression into *e, otherwise
 * it transforms the name *nm into an untyped expression *e.
 */
Task Prs_Member_To_Expr(Name *nm, Expression *e, Intg suffix) {
  Symbol *s;
  Box *b;

  TASK( Box_Get(& b, (suffix < 0) ? 0 : suffix) );

  if IS_FAILED( Sym_Implicit_Find(& s, b->type, nm) ) {
    MSG_ERROR("'%s' non e' membro di '%s'.",
     Name_To_Str(nm), Tym_Type_Name(b->type));
    return Failed;
  }

  /* Il nome corrisponde ad un simbolo gia' definito:
  * restituisco l'espressione (typed) corrispondente!
  */
  *e = s->value;
  return Success;
}

/* DESCRIPTION: Every explicit symbol can be followed by a suffix,
 *  which specifies which opened box contains the symbol.
 *  For example: variable::box1:box2
 *  The compiler interprets the string '::box1:box2' as the specification
 *  of the box which contains the variable 'v'.
 *  The association string --> box-level is obtained by the following
 *  function.
 */
Task Prs_Suffix(Intg *rs, Intg suffix, Name *nm) {
  if ( nm->length > 0 ) {
    Symbol *s;

    /* Cerco il simbolo a profondita' suffix o superiori */
    s = Sym_Explicit_Find(nm, suffix, NO_EXACT_DEPTH);
    if ( s == NULL ) {
      MSG_ERROR( "'%s' <-- tipo astratto inesistente!",
       Name_To_Str(nm) );
      return Failed;
    }

    /* Controllo che sia un tipo, ossia un simbolo con tipo e senza valore. */
    assert( s->value.is.typed );
    if ( s->value.is.value ) {
      MSG_ERROR( "'%s' deve essere un tipo astratto!",
       Name_To_Str(nm) );
      return Failed;
    }

    suffix = (suffix < 0) ? 0 : suffix;
    if ( (*rs = Box_Search_Opened(s->value.type, suffix + 1)) >= 0 )
      return Success;

    MSG_ERROR( "'%s' <-- nessuna box aperta e' di questo tipo!",
      Name_To_Str(nm) );
    return Failed;

  } else {
    if ( suffix < 0 ) {
      *rs = 0;
      return Success;
    } else {
      if ( (*rs = suffix + 1) <= Box_Depth() ) {
        return Success;
      } else {
        MSG_ERROR("Massima profondita' di box superata.");
        return Failed;
      }
    }
  }
}

/* DESCRIPTION: This function build an array of *num objects of type *x.
 * NOTE: *num is an integer expression. The new type will be put into *array.
 */
Task Prs_Array_Of_X(Expression *array, Expression *num, Expression *x) {
  register Intg t, n;

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
    MSG_ERROR("Impossibile creare un'array di oggetti di tipo non definito!");
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
Task Prs_Alias_Of_X(Expression *alias, Expression *x) {
  Symbol *s;
  Expression *target;
  register Intg t;

  assert( !alias->is.typed );

  if ( ! x->is.typed ) {
    MSG_ERROR("L'espressione alla destra di '=' deve avere tipo!");
    return Failed;
  }

  TASK( Sym_Explicit_New(& s, & alias->value.nm, alias->addr) );

  target = & (s->value);
  target->is.typed = 1;
  target->is.value = 0;
  target->type = t = Tym_Def_Alias_Of(& alias->value.nm, x->type);
  TASK( Cmp_Expr_Destroy_Tmp(alias) );
  *alias = *target;
  if ( t != TYPE_NONE ) return Success;
  return Failed;
}

#define CHECK_TYPE(t)                        \
  if ( ! t->is.typed ) {                     \
     MSG_ERROR("'%s' <-- Tipo non definito", \
      Name_To_Str(& t->value.nm));           \
     return Failed;                          \
  }                                          \
  assert(! t->is.value)

/* This function creates the new specie of types (first < second).
 */
Task Prs_Species_New(
 Expression *species, Expression *first, Expression *second) {
  Intg new_species;

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
Task Prs_Species_Add(Expression *species, Expression *old, Expression *type) {
  Intg old_species;

  CHECK_TYPE(old); CHECK_TYPE(type);

  old_species = old->type;
  assert( old_species != TYPE_NONE );
  TASK( Tym_Def_Specie( & old_species, type->type) );
  *species = *old;
  return Success;
}

/* This function creates the new structure of types (first, second).
 */
Task Prs_Struct_New(
 Expression *strc, Expression *first, Expression *second) {
  Intg new_strc;

  CHECK_TYPE(first); CHECK_TYPE(second);

  new_strc = TYPE_NONE;
  TASK( Tym_Def_Structure( & new_strc,  first->type) );
  TASK( Tym_Def_Structure( & new_strc, second->type) );

  strc->is.typed = 1;
  strc->is.value = 0;
  strc->type = new_strc;
  return Success;
}

/* This function adds a new type to an already existing structure.
 */
Task Prs_Struct_Add(Expression *strc, Expression *old, Expression *type) {
  Intg old_strc;

  CHECK_TYPE(old); CHECK_TYPE(type);

  old_strc = old->type;
  assert( old_strc != TYPE_NONE );
  TASK( Tym_Def_Structure( & old_strc, type->type) );
  *strc = *old;
  return Success;
}

/* This function calls a procedure without value, as (;), (<) or (>).
 */
Task Prs_Procedure_Special(int *found, int type, int auto_define) {
  Expression e;
  int dummy = 0;
  if ( found == NULL ) found = & dummy;
  Expr_New_Type(& e, type);
  return Cmp_Procedure(found, & e, 0, auto_define);
}

/* This function handles the rule: Type = Type
 */
Task Prs_Rule_Typed_Eq_Typed(Expression *rs,
 Expression *typed1, Expression *typed2) {

  if ( typed1->is.typed ) {
    /* Assertion: 'typed2' must be equal to 'typed1'. */
    *rs = *typed2;
    if ( ! typed2->is.typed ) {
      MSG_ERROR("Il tipo alla destra di '=' non e' ancora stato definito!");
      return Failed;
    }
    if ( Tym_Compare_Types(typed1->type, typed2->type, NULL) == 1 )
      return Success;
    MSG_ERROR("Incongruenza fra i tipi!");
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
Task Prs_Rule_Valued_Eq_Typed(Expression *rs,
 Expression *valued, Expression *typed) {

  *rs = *typed;
  rs->is.ignore = 1;
  if ( ! typed->is.typed ) {
    MSG_ERROR("Il tipo alla destra di '=' non e' ancora stato definito!");
    (void) Cmp_Expr_Destroy_Tmp(valued);
    return Failed;
  }

  if ( valued->is.typed ) {
    /* Assertion: the type of 'valued' should be 'typed'. */
    Intg t = valued->type;
    TASK( Cmp_Expr_Destroy_Tmp(valued) );

    if ( Tym_Compare_Types(t, typed->type, NULL) == 1 ) return Success;
    MSG_ERROR("Incongruenza fra i tipi!");
    return Failed;

  } else {
    /* Definition: creating (into 'valued') a new object of type 'typed'. */
    Symbol *s;
    Expression *target;

    {
      Task t = Sym_Explicit_New(& s, & valued->value.nm, valued->addr);
      TASK( Cmp_Expr_Destroy_Tmp(valued) );
      TASK( t );
    }

    target = & (s->value);
    TASK( Cmp_Expr_Create(target, typed->type, /* temporary = */ 0 ) );
    return Success;
  }
}

/* This function handles the rule: Type = value
 */
Task prs_rule_typed_eq_valued(Expression *typed, Expression *valued) {
  return Success;
}

/* This function handles the rule: value = value
 */
Task Prs_Rule_Valued_Eq_Valued(Expression *rs,
 Expression *valued1, Expression *valued2) {

  *rs = *valued2;
  rs->is.ignore = 1;
  if ( ! valued2->is.typed ) {
    MSG_ERROR("L'oggetto alla destra di '=' non ha tipo definito!");
    (void) Cmp_Expr_Destroy_Tmp(valued1);
    return Failed;
  }

  if ( valued1->is.typed ) {
    /* Assignment: assigns 'valued2' to 'valued1'. */
    Expression *result = Cmp_Operator_Exec(cmp_opr.assign, valued1, valued2);
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
