%{
/* parser.c- Autore: Franchin Matteo - settembre 2002
 *
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "types.h"
#include "messages.h"
#include "array.h"
#include "str.h"
#include "symbols.h"
#include "defaults.h"

//#include "registers.h"

#include "tokenizer.h"
#include "parser.h"

/* Funzioni definite in questo file */
void yyerror(char* s);

%}

/* Union che contiene tutti i tipi di valore semantico
 * che possono avere i token
 */
%union {
	Value	val;	/* Numero o registro */
	Name	nm;		/* Nome di variabile(non ancora definita) o stringa */
	Symbol 	*sym;	/* Simbolo */
}

/* Lista dei token senza valore semantico
 */
%token TOK_NEWLINE
%token TOK_AAGAIN
%token TOK_ERR
%token TOK_END

/* Lista dei token aventi valore semantico
 */
%token <val> TOK_NUM

%token <nm> TOK_STRING

%token <val> TOK_IVAR
%token <val> TOK_FVAR
%token <val> TOK_PVAR
%token <val> TOK_OVAR

%token <nm> TOK_NAME
                  	
%token <fn> TOK_SUB
%token <fn> TOK_IFN
%token <fn> TOK_FFN
%token <fn> TOK_PFN
%token <fn> TOK_OFN

%token <ofn> TOK_OBJSUB
%token <ofn> TOK_IOBJFN
%token <ofn> TOK_FOBJFN
%token <ofn> TOK_POBJFN
%token <ofn> TOK_OOBJFN

%token <val> TOK_INSTRUC

%token <proc> TOK_IPROC

%token <fn> TOK_IPAR
%token <fn> TOK_FPAR
%token <fn> TOK_PPAR

/* Lista delle espressioni aventi valore semantico
 */
%type <val> num.prm
%type <val> num.exp
%type <val> num.prm.reg
%type <val> num.reg

%type <val> point.prm
%type <val> point.exp
%type <val> point.prm.reg
%type <val> point.reg

%type <val> object.prm

%type <nm> string.prm

/* Lista dei token affetti da regole di precedenza
 */
%left '-' '+'
%left '*' '/'
%left TOK_NEG
%right TOK_PROJX TOK_PROJY

/* Regola di partenza
 */
%start program

/******************************************************************************/
/******************************************************************************/
/* num.prm --->
| TOK_IVAR	{
	$$.type = TVAL_REGINTG;
	if ( ($$.num.reg = reg_occupy(TVAL_REGINTG)) == 0 ) {YYERROR;}
	op_ins2r(ins_movi, $$.num.reg, $1.num.reg);
  }

| TOK_FVAR {
	$$.type = TVAL_REGFLT;
	if ( ($$.num.reg = reg_occupy(TVAL_REGFLT)) == 0 ) {YYERROR;}
	op_ins2r(ins_movf, $$.num.reg, $1.num.reg);
  }

| TOK_IFN {op_ins0r(ins_ast);} ifps.arg.list ']'	{
	op_call($1->fnnum);
	op_ins0r(ins_aclr);
	
	$$.type = TVAL_REGINTG;
	if ( ($$.num.reg = reg_occupy(TVAL_REGINTG)) == 0 ) {YYERROR;}
	
	op_ins2r(ins_movi, $$.num.reg, 0);
  }

| TOK_FFN {op_ins0r(ins_ast);} ifps.arg.list ']'	{
	op_call($1->fnnum);
	op_ins0r(ins_aclr);
	
	$$.type = TVAL_REGFLT;
	if ( ($$.num.reg = reg_occupy(TVAL_REGFLT)) == 0 ) {YYERROR;}
	
	op_ins2r(ins_movf, $$.num.reg, 0);
  }

| TOK_IOBJFN {
	op_ins0r(ins_ast);
	op_ins2r(ins_movo, 0, $1.reg);
	op_call($1.fdesc->fnnum);
	op_ins0r(ins_aclr);
	
	$$.type = TVAL_REGINTG;
	if ( ($$.num.reg = reg_occupy(TVAL_REGINTG)) == 0 ) {YYERROR;}
	
	op_ins2r(ins_movi, $$.num.reg, 0);
  }

| TOK_IOBJFN {op_ins0r(ins_ast);} '[' ifps.arg.list ']'	{
	op_ins2r(ins_movo, 0, $1.reg);
	op_call($1.fdesc->fnnum);
	op_ins0r(ins_aclr);
	
	$$.type = TVAL_REGINTG;
	if ( ($$.num.reg = reg_occupy(TVAL_REGINTG)) == 0 ) {YYERROR;}
	
	op_ins2r(ins_movi, $$.num.reg, 0);
  }

| TOK_FOBJFN {
	op_ins0r(ins_ast);
	op_ins2r(ins_movo, 0, $1.reg);
	op_call($1.fdesc->fnnum);
	op_ins0r(ins_aclr);
	
	$$.type = TVAL_REGFLT;
	if ( ($$.num.reg = reg_occupy(TVAL_REGFLT)) == 0 ) {YYERROR;}
	
	op_ins2r(ins_movf, $$.num.reg, 0);
  }

| TOK_FOBJFN {op_ins0r(ins_ast);} '[' ifps.arg.list ']' {
	op_ins2r(ins_movo, 0, $1.reg);
	op_call($1.fdesc->fnnum);
	op_ins0r(ins_aclr);
	
	$$.type = TVAL_REGFLT;
	if ( ($$.num.reg = reg_occupy(TVAL_REGFLT)) == 0 ) {YYERROR;}
	
	op_ins2r(ins_movf, $$.num.reg, 0);
  }

| TOK_IPAR {
	op_ins0r(ins_ast);
	op_call($1->fnnum);
	op_ins0r(ins_aclr);
	
	$$.type = TVAL_REGINTG;
	if ( ($$.num.reg = reg_occupy(TVAL_REGINTG)) == 0 ) {YYERROR;}
	
	op_ins2r(ins_movi, $$.num.reg, 0);
  }

| TOK_IPAR {op_ins0r(ins_ast);} '[' ifps.arg.list ']' {
	op_call($1->fnnum);
	op_ins0r(ins_aclr);
	
	$$.type = TVAL_REGINTG;
	if ( ($$.num.reg = reg_occupy(TVAL_REGINTG)) == 0 ) {YYERROR;}
	
	op_ins2r(ins_movi, $$.num.reg, 0);
  }

| TOK_FPAR {
	op_ins0r(ins_ast);
	op_call($1->fnnum);
	op_ins0r(ins_aclr);
	
	$$.type = TVAL_REGFLT;
	if ( ($$.num.reg = reg_occupy(TVAL_REGFLT)) == 0 ) {YYERROR;}
	
	op_ins2r(ins_movf, $$.num.reg, 0);
  }

| TOK_FPAR {op_ins0r(ins_ast);} '[' ifps.arg.list ']' {
	op_call($1->fnnum);
	op_ins0r(ins_aclr);
	
	$$.type = TVAL_REGFLT;
	if ( ($$.num.reg = reg_occupy(TVAL_REGFLT)) == 0 ) {YYERROR;}
	
	op_ins2r(ins_movf, $$.num.reg, 0);
  }
*/
/******************************************************************************/
/******************************************************************************/
/* num.exp ---->
| TOK_IOBJFN {op_ins0r(ins_ast);} ifps.prm.arg	{
	op_ins2r(ins_movo, 0, $1.reg);
	op_call($1.fdesc->fnnum);
	op_ins0r(ins_aclr);
	
	$$.type = TVAL_REGINTG;
	if ( ($$.num.reg = reg_occupy(TVAL_REGINTG)) == 0 ) {YYERROR;}
	
	op_ins2r(ins_movi, $$.num.reg, 0);
  }

| TOK_FOBJFN {op_ins0r(ins_ast);} ifps.prm.arg	{
	op_ins2r(ins_movo, 0, $1.reg);
	op_call($1.fdesc->fnnum);
	op_ins0r(ins_aclr);
	
	$$.type = TVAL_REGFLT;
	if ( ($$.num.reg = reg_occupy(TVAL_REGFLT)) == 0 ) {YYERROR;}
	
	op_ins2r(ins_movf, $$.num.reg, 0);
  }

| TOK_IPAR {op_ins0r(ins_ast);} ifps.prm.arg	{
	op_call($1->fnnum);
	op_ins0r(ins_aclr);
	
	$$.type = TVAL_REGINTG;
	if ( ($$.num.reg = reg_occupy(TVAL_REGINTG)) == 0 ) {YYERROR;}
	
	op_ins2r(ins_movi, $$.num.reg, 0);
  }

| TOK_FPAR {op_ins0r(ins_ast);} ifps.prm.arg	{
	op_call($1->fnnum);
	op_ins0r(ins_aclr);
	
	$$.type = TVAL_REGFLT;
	if ( ($$.num.reg = reg_occupy(TVAL_REGFLT)) == 0 ) {YYERROR;}
	
	op_ins2r(ins_movf, $$.num.reg, 0);
  }
*/
/******************************************************************************/
/******************************************************************************/
/* point.prm ---->
| TOK_PVAR	{
	$$.type = TVAL_REGPNT;
	if ( ($$.num.reg = reg_occupy(TVAL_REGPNT)) == 0 ) {YYERROR;}
	op_ins2r(ins_movp, $$.num.reg, $1.num.reg);
  }

| TOK_PFN {op_ins0r(ins_ast);} ifps.arg.list ']'	{
	op_call($1->fnnum);
	op_ins0r(ins_aclr);
	
	$$.type = TVAL_REGPNT;
	if ( ($$.num.reg = reg_occupy(TVAL_REGPNT)) == 0 ) {YYERROR;}
	
	op_ins2r(ins_movp, $$.num.reg, 0);
  }

| TOK_POBJFN {
	op_ins0r(ins_ast);
	op_ins2r(ins_movo, 0, $1.reg);
	op_call($1.fdesc->fnnum);
	op_ins0r(ins_aclr);
	
	$$.type = TVAL_REGPNT;
	if ( ($$.num.reg = reg_occupy(TVAL_REGPNT)) == 0 ) {YYERROR;}
	
	op_ins2r(ins_movp, $$.num.reg, 0);
  }

| TOK_POBJFN {op_ins0r(ins_ast);} '[' ifps.arg.list ']'	{
	op_ins2r(ins_movo, 0, $1.reg);
	op_call($1.fdesc->fnnum);
	op_ins0r(ins_aclr);
	
	$$.type = TVAL_REGPNT;
	if ( ($$.num.reg = reg_occupy(TVAL_REGPNT)) == 0 ) {YYERROR;}
	
	op_ins2r(ins_movp, $$.num.reg, 0);
  }

| TOK_PPAR {
	op_ins0r(ins_ast);
	op_call($1->fnnum);
	op_ins0r(ins_aclr);
	
	$$.type = TVAL_REGPNT;
	if ( ($$.num.reg = reg_occupy(TVAL_REGPNT)) == 0 ) {YYERROR;}
	
	op_ins2r(ins_movp, $$.num.reg, 0);
  }

| TOK_PPAR {op_ins0r(ins_ast);} '[' ifps.arg.list ']' {
	op_call($1->fnnum);
	op_ins0r(ins_aclr);
	
	$$.type = TVAL_REGPNT;
	if ( ($$.num.reg = reg_occupy(TVAL_REGPNT)) == 0 ) {YYERROR;}
	
	op_ins2r(ins_movp, $$.num.reg, 0);
  }
*/

/******************************************************************************/
/******************************************************************************/
/* point.exp ---->
| TOK_POBJFN {op_ins0r(ins_ast);} ifps.prm.arg	{
	op_ins2r(ins_movo, 0, $1.reg);
	op_call($1.fdesc->fnnum);
	op_ins0r(ins_aclr);
	
	$$.type = TVAL_REGPNT;
	if ( ($$.num.reg = reg_occupy(TVAL_REGPNT)) == 0 ) {YYERROR;}
	
	op_ins2r(ins_movp, $$.num.reg, 0);
  }

| TOK_PPAR {op_ins0r(ins_ast);} ifps.prm.arg	{
	op_call($1->fnnum);
	op_ins0r(ins_aclr);
	
	$$.type = TVAL_REGPNT;
	if ( ($$.num.reg = reg_occupy(TVAL_REGPNT)) == 0 ) {YYERROR;}
	
	op_ins2r(ins_movp, $$.num.reg, 0);
  }
*/

%%

/******************************************************************************/
/*                  ESPRESSIONI NUMERICHE(INTERE E FLOATING)                  */
/******************************************************************************/
num.prm:
  TOK_NUM	{ $$ = $1; }

| '(' num.exp ')'	{ $$ = $2; }
;

num.exp:
  num.prm	{ $$ = $1; }

| num.exp '+' num.exp	{
	if (!pars_numop(& $1, & $3, ins_addi, ins_addf)) {YYERROR;}
	$$ = $1;
  }

| num.exp '-' num.exp	{
	if (!pars_numop(& $1, & $3, ins_subi, ins_subf)) {YYERROR;}
	$$ = $1;
  }

| num.exp '*' num.exp	{
	if (!pars_numop(& $1, & $3, ins_muli, ins_mulf)) {YYERROR;}
	$$ = $1;
  }

| num.exp '/' num.exp	{
	if (!pars_numop(& $1, & $3, ins_divi, ins_divf)) {YYERROR;}
	$$ = $1;
  }

| '-' num.exp %prec TOK_NEG	{
	if (($2.type & 0x80) != 0) {
		if ($2.type == TVAL_IMMINTG) {
			if (!op_imm1r(ins_negi, & $2.num.intg)) {YYERROR;}
		} else {
			if (!op_imm1r(ins_negf, & $2.num.intg)) {YYERROR;}
		}
		$$ = $2;

	} else {
		if ($2.type == TVAL_REGINTG) {
			op_ins1r(ins_negi, $2.num.reg);
		} else {
			op_ins1r(ins_negf, $2.num.reg);
		}
		$$ = $2;
	}
  }

| point.exp TOK_PROJX	{
	if ($1.type == TVAL_IMMPNT) {
		$$.type = TVAL_IMMFLT;
		if (!op_imm2r(ins_pxtof, & $$.num.flt, & $1.num.pnt)) {YYERROR;}
		
	} else {
		$$.type = TVAL_REGFLT;
		if ( ($$.num.reg = reg_occupy(TVAL_REGFLT)) == 0 ) {YYERROR;}
		op_ins2r(ins_pxtof, $$.num.reg, $1.num.reg);
		if (!reg_release($1.type, $1.num.reg)) {YYERROR;}
	}
  }

| point.exp TOK_PROJY	{
	if ($1.type == TVAL_IMMPNT) {
		$$.type = TVAL_IMMFLT;
		if (!op_imm2r(ins_pytof, & $$.num.flt, & $1.num.pnt)) {YYERROR;}
		
	} else {
		$$.type = TVAL_REGFLT;
		if ( ($$.num.reg = reg_occupy(TVAL_REGFLT)) == 0 ) {YYERROR;}
		op_ins2r(ins_pytof, $$.num.reg, $1.num.reg);
		if (!reg_release($1.type, $1.num.reg)) {YYERROR;}
	}
  }

;

/******************************************************************************/
/*                          ESPRESSIONI TIPO PUNTO                            */
/******************************************************************************/
point.prm:
  '(' num.exp ',' num.exp ')' {
	if (!pars_2ntop( & $$, & $2, & $4)) {YYERROR;}
  }

| '(' point.exp ')'	{ $$ = $2; }

;

point.exp:
  point.prm		{ $$ = $1; }

| point.exp '+' point.exp {
	if (!pars_pntop(& $1, & $3, ins_addp)) {YYERROR;}
	$$ = $1;
  }

| point.exp '-' point.exp {
	if (!pars_pntop(& $1, & $3, ins_subp)) {YYERROR;}
	$$ = $1;
  }

| point.exp '*' num.exp {
	if (!pars_popf(& $1, & $3, ins_pmulf)) {YYERROR;}
	$$ = $1;
  }

| num.exp '*' point.exp {
	if (!pars_popf(& $3, & $1, ins_pmulf)) {YYERROR;}
	$$ = $3;
  }

| point.exp '/' num.exp {
	if (!pars_popf(& $1, & $3, ins_pdivf)) {YYERROR;}
	$$ = $1;
  }

| '-' point.exp %prec TOK_NEG	{
	if ($2.type == TVAL_IMMPNT) {
		if (!op_imm1r(ins_negp, & $2.num.pnt)) {YYERROR;}
		$$ = $2;

	} else {
		op_ins1r(ins_negp, $2.num.reg);
		$$ = $2;
	}
  }

;

/******************************************************************************/
/*                         ESPRESSIONI TIPO OGGETTO                           */
/******************************************************************************/
object.prm:
  TOK_OVAR {
	$$.type = TVAL_REGOBJ;
	if ( ($$.num.reg = reg_occupy(TVAL_REGOBJ)) == 0 ) {YYERROR;}
	op_ins2r(ins_movo, $$.num.reg, $1.num.reg);
  }
;

/******************************************************************************/
/*                         ESPRESSIONI TIPO STRINGA                           */
/******************************************************************************/
string.prm:
  TOK_STRING	{$$ = $1;}

| string.prm TOK_STRING {
	$$.leng = $1.leng + $2.leng;
	$$.ptr = (char *) malloc($$.leng + 1);
	strncpy($$.ptr, $1.ptr, $1.leng);
	strncpy($$.ptr+$1.leng, $2.ptr, $2.leng + 1);
	free($1.ptr);
	free($2.ptr);
  }
;

/******************************************************************************/
/* Queste le uso per convertire un espressione immediata/registro in registro */

num.prm.reg:
  num.prm	{
	if (!pars_toreg(& $1)) {YYERROR;}
	$$ = $1;
  }
;

point.prm.reg:
  point.prm	{
	if (!pars_toreg(& $1)) {YYERROR;}
	$$ = $1;
  }
;

ifps.prm.arg:
  num.prm.reg	{
	if ($1.type == TVAL_REGINTG)
		op_ins1r(ins_argi, $1.num.reg);
	else
		op_ins1r(ins_argf, $1.num.reg);

	if (!reg_release($1.type, $1.num.reg)) {YYERROR;}
  }

| point.prm.reg	{
	op_ins1r(ins_argp, $1.num.reg);

	if (!reg_release($1.type, $1.num.reg)) {YYERROR;}
  }

| string.prm {
	op_lds(0, $1.ptr, $1.leng);
	op_ins1r(ins_args, 0);
	free($1.ptr);
  }
;

num.reg:
  num.exp	{
	if (!pars_toreg(& $1)) {YYERROR;}
	$$ = $1;
  }
;

point.reg:
  point.exp	{
	if (!pars_toreg(& $1)) {YYERROR;}
	$$ = $1;
  }
;

ifps.arg:
  num.reg	{
	if ($1.type == TVAL_REGINTG)
		op_ins1r(ins_argi, $1.num.reg);
	else
		op_ins1r(ins_argf, $1.num.reg);

	if (!reg_release($1.type, $1.num.reg)) {YYERROR;}
  }

| point.reg	{
	op_ins1r(ins_argp, $1.num.reg);

	if (!reg_release($1.type, $1.num.reg)) {YYERROR;}
  }

| string.prm {
	op_lds(0, $1.ptr, $1.leng);
	op_ins1r(ins_args, 0);
	free($1.ptr);
  }
;

ifps.arg.list:
  ifps.arg
| ifps.arg.list ',' ifps.arg
;

/******************************************************************************/
/*           DEFINIZIONE DELLA SINTASSI DELLE ISTRUZIONI D'OGGETTO            */
/******************************************************************************/
instruction.arg:
  num.reg	{
	
	lib_iproc *proc;

	if ($1.type == TVAL_REGINTG) {
		/* Il numero e' intero: controllo se l'istruzione accetta interi */
		proc = lib_findiproc(comp_ins, "_integer", 8);
		if (proc != NULL) {
			op_ins0r(ins_ast);
			op_ins1r(ins_argi, $1.num.reg);
			op_call(proc->num);
			op_ins0r(ins_aclr);
		} else {
			LERRORMSG(tok_linenum, "L'istruzione non ammette argomenti di tipo intero");
			YYERROR;
		}
	} else {
		/* Il numero e' un float: controllo se l'istruzione accetta float */
		proc = lib_findiproc(comp_ins, "_float", 6);
		if (proc != NULL) {
			op_ins0r(ins_ast);
			op_ins1r(ins_argf, $1.num.reg);
			op_call(proc->num);
			op_ins0r(ins_aclr);
		} else {
			LERRORMSG(tok_linenum, "L'istruzione non ammette argomenti di tipo float");
			YYERROR;
		}
	}

	if (!reg_release($1.type, $1.num.reg)) {YYERROR;}
  }

| point.reg {
	
	lib_iproc *proc;
	
	proc = lib_findiproc(comp_ins, "_point", 6);
	if (proc != NULL) {
		op_ins0r(ins_ast);
		op_ins1r(ins_argp, $1.num.reg);
		op_call(proc->num);
		op_ins0r(ins_aclr);
	} else {
		LERRORMSG(tok_linenum, "L'istruzione non ammette argomenti di tipo punto");
		YYERROR;
	}

	if (!reg_release($1.type, $1.num.reg)) {YYERROR;}
  }

| object.prm {
	
	lib_iproc *proc;
	
	proc = lib_findiproc(comp_ins, "_object", 7);
	if (proc != NULL) {
		op_ins0r(ins_ast);
		op_ins1r(ins_argo, $1.num.reg);
		op_call(proc->num);
		op_ins0r(ins_aclr);
	} else {
		LERRORMSG(tok_linenum, "L'istruzione non ammette argomenti di tipo oggetto");
		YYERROR;
	}

	if (!reg_release($1.type, $1.num.reg)) {YYERROR;}
  }

| string.prm {
	
	lib_iproc *proc;
	
	proc = lib_findiproc(comp_ins, "_string", 7);
	if (proc != NULL) {
		op_ins0r(ins_ast);
		op_lds(0, $1.ptr, $1.leng);
		op_ins1r(ins_args, 0);
		op_call(proc->num);
		op_ins0r(ins_aclr);
		free($1.ptr);
	} else {
		LERRORMSG(tok_linenum, "L'istruzione non ammette argomenti di tipo stringa");
		YYERROR;
	}
  }

| TOK_IPROC {op_ins0r(ins_ast); op_call($1->num); op_ins0r(ins_aclr);}

| TOK_IPROC {op_ins0r(ins_ast);} ifps.arg {op_call($1->num); op_ins0r(ins_aclr);}

| TOK_IPROC {op_ins0r(ins_ast);} '[' ifps.arg.list ']' {op_call($1->num); op_ins0r(ins_aclr);}
;

instruction.sep:
  ','
| ';' {
	lib_iproc *proc;
	
	proc = lib_findiproc(comp_ins, "_again", 6);
	if (proc != NULL) {
		op_call(proc->num);
	} else {
		LERRORMSG(tok_linenum, "L'istruzione non ammette il separatore ;");
		YYERROR;
	}
  }
| TOK_AAGAIN { /* TOK_AAGAIN e' il separatore ;; */
	lib_iproc *proc;
	
	proc = lib_findiproc(comp_ins, "_aagain", 7);
	if (proc != NULL) {
		op_call(proc->num);
	} else {
		LERRORMSG(tok_linenum, "L'istruzione non ammette il separatore ;;");
		YYERROR;
	}
  }
;

instruction.arg.list:
  instruction.arg
| instruction.arg.list instruction.sep instruction.arg
;

instruction.statement:
  TOK_INSTRUC {

	lib_iproc *proc_begin;
	
	/* Se l'istruzione possiede la procedura _begin, allora la eseguo */
	proc_begin = lib_findiproc(comp_ins, "_begin", 6);
	if (proc_begin != NULL)
		op_call(proc_begin->num);

  } instruction.arg.list {

	lib_iproc *proc_end;
	
	/* Se l'istruzione restituisce una variabile, termino con _endwithobj
	 * altrimenti termino con _end
	 */
	if ($1.type == TVAL_NONE) {
		proc_end = lib_findiproc(comp_ins, "_end", 4);
		if (proc_end != NULL)
			op_call(proc_end->num);

	} else {
		proc_end = lib_findiproc(comp_ins, "_endwithobj", 11);
		if (proc_end != NULL)
			op_call(proc_end->num);
		/* Ora trasferisco l'oggetto nella variabile specificata nell'istruzione */
		switch ($1.type) {
		 case TVAL_INTG:
			op_ins2r(ins_movi, $1.num.reg, 0); break;
		 case TVAL_FLT:
			op_ins2r(ins_movf, $1.num.reg, 0); break;
		 case TVAL_PNT:
			op_ins2r(ins_movp, $1.num.reg, 0); break;
		 case TVAL_OBJ:
			op_ins2r(ins_movo, $1.num.reg, 0); break;
		}
	}

	comp_ins_flag = 0;
  }
	;

/******************************************************************************/
/*               DEFINIZIONE DELLA SINTASSI DELL'ISTRUZIONE END               */
/******************************************************************************/
end.statement:
  TOK_END	{op_ins0r(ins_end);}
;

/******************************************************************************/
/*        DEFINIZIONE DELLA SINTASSI DELLE ISTRUZIONI DI ASSEGNAZIONE         */
/******************************************************************************/
assignment.statement:
  TOK_IVAR '=' num.exp {
	if (!pars_ntoivar(& $1, & $3)) {YYERROR;}
  }

| TOK_FVAR '=' num.exp {
	if (!pars_ntofvar(& $1, & $3)) {YYERROR;}
  }

| TOK_PVAR '=' point.exp {
	if ($3.type == TVAL_IMMPNT) {
		op_ldp($1.num.reg, $3.num.pnt.x, $3.num.pnt.y);
		
	} else {
		op_ins2r(ins_movp, $1.num.reg, $3.num.reg);
		if (!reg_release($3.type, $3.num.reg)) {YYERROR;}
	}
  }

| TOK_NAME '=' num.exp	{
	if (!pars_ntovar(& $1, & $3)) {YYERROR;}
  }

| TOK_NAME '=' point.exp {

	long vnum;

	vnum = var_define($1.ptr, $1.leng, TVAL_PNT, TVAL_NOSUBTYPE);
	if (vnum == 0) {YYERROR;}

	if ($3.type == TVAL_IMMPNT) {
		op_ldp(vnum, $3.num.pnt.x, $3.num.pnt.y);
		
	} else {
		op_ins2r(ins_movp, vnum, $3.num.reg);
		if (!reg_release($3.type, $3.num.reg)) {YYERROR;}
	}	
  }

| TOK_NAME '=' string.prm {

	printf("In fase di implementazione!\n");
	/*long vnum;

	vnum = var_define($1.ptr, $1.leng, TVAL_PNT, TVAL_NOSUBTYPE);
	if (vnum == 0) {YYERROR;}

	if ($3.type == TVAL_IMMPNT) {
		op_ldp(vnum, $3.num.pnt.x, $3.num.pnt.y);
		
	} else {
		op_ins2r(ins_movp, vnum, $3.num.reg);
		if (!reg_release($3.type, $3.num.reg)) {YYERROR;}
	}*/	
  }
;

/******************************************************************************/
/*                    SINTASSI DELLA CHIAMATA A SUBROUTINE                    */
/******************************************************************************/
subroutine.statement:
  TOK_SUB {
	op_ins0r(ins_ast);
	op_call($1->fnnum);
	op_ins0r(ins_aclr);
  }

| TOK_SUB {op_ins0r(ins_ast);} ifps.arg	{
	op_call($1->fnnum);
	op_ins0r(ins_aclr);
  }

| TOK_SUB {op_ins0r(ins_ast);} '[' ifps.arg.list ']'	{
	op_call($1->fnnum);
	op_ins0r(ins_aclr);
  }

| TOK_OBJSUB {
	op_ins0r(ins_ast);
	op_ins2r(ins_movo, 0, $1.reg);
	op_call($1.fdesc->fnnum);
	op_ins0r(ins_aclr);
  }

| TOK_OBJSUB {op_ins0r(ins_ast);} ifps.arg	{
	op_ins2r(ins_movo, 0, $1.reg);
	op_call($1.fdesc->fnnum);
	op_ins0r(ins_aclr);
  }

| TOK_OBJSUB {op_ins0r(ins_ast);} '[' ifps.arg.list ']'	{
	op_ins2r(ins_movo, 0, $1.reg);
	op_call($1.fdesc->fnnum);
	op_ins0r(ins_aclr);
  }
;

/*************DEFINIZIONE DELLA STRUTTURA GENERICA DEI PROGRAMMI**************/
/* Cio' che resta descrive la sintassi delle righe e del corpo del programma
 */
statement:
  assignment.statement
| instruction.statement
| subroutine.statement
| end.statement
;

statement.opt:
 | statement
;

statement.line:
  statement.opt TOK_NEWLINE {
	op_line(++tok_linenum);
  }

 | error TOK_NEWLINE {
	comp_ins_flag = 0;
	err_prnclr(stdout);
	op_line(++tok_linenum);
	yyerrok;
  }
;

statement.list:
		statement.line
	|	statement.list statement.line
	;

program:
	|	statement.list
	;

%%

#include "lex.yy.c"

/* error function */
void yyerror(char* s)
{
  LERRORMSG(tok_linenum, "Errore di sintassi");
  return;
}

