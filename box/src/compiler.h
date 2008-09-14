/***************************************************************************
 *   Copyright (C) 2008 by Matteo Franchin                                 *
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

/* compiler.h - dicembre 2004
 *
 * Questo file contiene le dichiarazioni per typeman.c, symbol.c, compiler.c
 */

#ifndef _COMPILER_H
#  define _COMPILER_H

#  include "virtmach.h"
#  include "expr.h"
#  include "typesys.h"

struct Operation {
  struct {
    unsigned int intrinsic   : 1; /* E' una operazione intrinseca? */
    unsigned int commutative : 1; /* E' commutativa? */
    unsigned int immediate   : 1; /* Ammette l'operazione immediata? */
    unsigned int assignment  : 1; /* E' una operazione di assegnazione? */
    unsigned int link        : 1; /* E' una operazione di tipo "link"? */
  } is;
  Int type1, type2, type_rs;   /* Tipi dei due argomenti */
  union {
    UInt asm_code;  /* Codice assembly dell'istruzione associata */
    Int module;    /* Modulo caricato nella VM per eseguire l'operazione */
  }; /* <-- questa e' una union senza nome! Non e' ISO C purtroppo! */
  struct Operation *next;     /* Prossima operazione dello stesso operatore */
};

typedef struct Operation Operation;

struct Operator {
  unsigned int can_define : 1; /* E' un operatore di definizione? */
  char *name; /* Token che rappresenta l'operatore */
  /* Operazioni privilegiate, cioe' con collegamenti diretti */
  Operation *opn[3][CMP_PRIVILEGED];
  /* Operazioni non privilegiate, cioe' con collegamenti in catena */
  Operation *opn_chain;
};

typedef struct Operator Operator;

/* "Collezione" di tutti gli operatori */
struct cmp_opr_struct {
  /* Operatore usato per la conversione fra tipi diversi */
  Operator *converter;

  /* Operatori di assegnazione */
  Operator *assign;
  Operator *aplus;
  Operator *aminus;
  Operator *atimes;
  Operator *adiv;
  Operator *arem;
  Operator *aband;
  Operator *abxor;
  Operator *abor;
  Operator *ashl;
  Operator *ashr;

  /* Operatori di incremento/decremento */
  Operator *inc;
  Operator *dec;

  /* Operatori aritmetici convenzionali */
  Operator *pow;
  Operator *plus;
  Operator *minus;
  Operator *times;
  Operator *div;
  Operator *rem;

  /* Operatori bit-bit */
  Operator *bor;
  Operator *bxor;
  Operator *band;
  Operator *bnot;

  /* Operatori di shift */
  Operator *shl;
  Operator *shr;

  /* Operatori di confronto */
  Operator *eq;
  Operator *ne;
  Operator *gt;
  Operator *ge;
  Operator *lt;
  Operator *le;

  /* Operatori logici */
  Operator *lor;
  Operator *land;
  Operator *lnot;
};

/* Tipi possibili per un simbolo */
typedef enum { CONSTANT, VARIABLE, FUNCTION, BOX } SymbolType;

/* Questa struttura descrive tutte le proprieta' di un simbolo
 * NOTA: brother serve a costruire la catena dei simboli di una box.
 *  Quando una box viene chiusa tale catena viene utilizzata per cancellarne
 *  i simboli.
 */
struct symbol {
  /* Quantita' di competenza delle funzioni del tipo Sym_Symbol_... */
  char *name;              /* Nome del simbolo */
  UInt leng;               /* Lunghezza del nome */
  struct symbol *next;     /* Simbolo seguente nella hash-table */
  struct symbol *previous; /* Simbolo precedente nella hash-table */

  /* Quantita' che descrivono il simbolo e lo relazionano agli altri */
  struct {
    unsigned int is_explicit : 1;
  } symattr;              /* Attributi del simbolo */
  SymbolType symtype;     /* Tipo di simbolo */
  Expr value;             /* Valore dell'simbolo */
  struct symbol *child;   /* Catena dei simboli figli */
  struct symbol *brother; /* Prossimo simbolo nella catena (vedi sopra) */

  /* Questa union distingue i dati di simboli espliciti dagli impliciti */
  union {
    UInt exp;             /* Caso esplicito: il genitore e' un esempio di box */
    Int imp;             /* Caso implicito: il genitore e' un tipo di box */
  } parent;
};

typedef struct symbol Symbol;

/* Definisco il tipo SymbolAction. Se dichiaro:
 *  SymbolAction action;
 * action sara' una funzione in grado di operare o analizzare su un simbolo
 * (tipo Symbol). Funzioni di questo tipo vengono chiamate da quelle funzioni
 * (come ad esempio Sym_Symbol_Find) che scorrono la lista dei simboli,
 * in modo da poter eseguire un'operazione o controllo sul simbolo trovato.
 * Il valore restituito e' un UInt che informa la funzione chiamante dell'esito
 * dell'operazione/controllo. I valori sono:
 *   0 tutto Ok, e' possibile passare al prossimo simbolo;
 *   1 tutto Ok, ma non occorre continuare e passare al prossimo simbolo;
 *   2 qualcosa non e' andato correttamente, meglio che la funzione chiamante
 *     esca immediatamente.
 */
typedef UInt SymbolAction(Symbol *s);

/* Tipo legato all'uso di Cmp_Operation_Find */
typedef struct {
  unsigned int commute :1;
  unsigned int expand1 :1;
  unsigned int expand2 :1;
  Int exp_type1;
  Int exp_type2;
} OpnInfo;

typedef struct {
  VMProgram *vm;
  TS ts_obj, *ts;
  Array *allocd_oprs;
} Compiler;

/* The main compiler data structure */
extern Compiler *cmp;

#  define cmp_vm (cmp->vm)

/* We make our life simpler, since we write code always in cmp_vm */
#  define Cmp_Assemble(...) VM_Assemble(cmp->vm, __VA_ARGS__)

extern struct cmp_opr_struct cmp_opr;

#include "box.h"

/* Procedure definite in 'symbol.c'*/
UInt Sym__Hash(char *name, UInt leng);
Symbol *Sym_Symbol_Find(Name *nm, SymbolAction action);
#ifdef DEBUG
void Sym_Symbol_Diagnostic(FILE *stream);
#endif
Symbol *Sym_Symbol_New(Name *nm);
void Sym_Symbol_Delete(Symbol *s);
Task Sym_Implicit_Find(Symbol **s, Int parent, Name *nm);
Task Sym_Implicit_New(Symbol **new_sym, Int parent, Name *nm);
#define EXACT_DEPTH 0
#define NO_EXACT_DEPTH 1
Symbol *Sym_Explicit_Find(Name *nm, Int depth, int mode);

/* Procedure definite in 'compiler.c'*/
Task Cmp_Init(VMProgram *program);
void Cmp_Destroy(void);
Task Cmp_Parse(const char *file);
Operator *Cmp_Operator_New(char *token);
void Cmp_Operator_Destroy(Operator *opr);
Operation *Cmp_Operation_Add(Operator *opr, Int type1, Int type2, Int typer);
Operation *Cmp_Operation_Find(Operator *opr,
 Int type1, Int type2, Int typer, OpnInfo *oi);
Task Cmp_Conversion(Int type1, Int type2, Expr *e);
Task Cmp_Conversion_Exec(Expr *e, Int type_dest, Operation *c_opn);
Expr *Cmp_Operator_Exec(Operator *opr, Expr *e1, Expr *e2);
Expr *Cmp_Operation_Exec(Operation *opn, Expr *e1, Expr *e2);
Task Cmp_Expr_Unvalued(Expr *e, Int type);
Task Cmp_Expr_LReg(Expr *e, Int t, int zero);
Task Cmp_Free(Expr *expr);
Task Cmp_Expr_To_X(Expr *expr, AsmArg categ, Int reg, int and_free);
Task Cmp__Expr_To_LReg(Expr *expr, int force);
Task Cmp_Expr_To_Ptr(Expr *expr, AsmArg categ, Int reg, int and_free);
Task Cmp_Expr_Container_Change(Expr *e, Container *c);
Task Cmp_Expr_Destroy(Expr *e, int destroy_target);
Task Cmp_Expr_Copy(Expr *e_dest, Expr *e_src);
Task Cmp_Expr_Move(Expr *e_dest, Expr *e_src);

Expr *Cmp_Expr_Reg0_To_LReg(Int t);
Task Cmp_Expr_O_To_OReg(Expr *e);
Task Cmp_Complete_Ptr_1(Expr *e);
Task Cmp_Complete_Ptr_2(Expr *e1, Expr *e2);
Task Cmp_Define_Builtins();
Task Cmp_Data_Init(void);
Int Cmp_Data_Add(Int type, void *data, Int size);
void Cmp_Data_Destroy(void);
Task Cmp_Data_Prepare(void);
Task Cmp_Data_Display(FILE *stream);
Task Cmp_Imm_Init(void);
Int Cmp_Imm_Add(Int type, void *data, Int size);
void Cmp_Imm_Destroy(void);
Task Cmp_String_New(Expr *e, Name *str, int free_str);
#define Cmp_String_New_And_Free(e, str) Cmp_String_New(e, str, 1)
Task Cmp_Procedure_Search(int *found, Int procedure, Int suffix,
                          Box **box, Int *prototype, UInt *sym_num,
                          int auto_define);
Task Cmp_Procedure(int *found, Expr *e, Int suffix, int auto_define);
Task Cmp_Structure_Begin(void);
Task Cmp_Structure_Add(Expr *e);
Task Cmp_Structure_End(Expr *new_struct);
Task Cmp_Structure_Get(Expr *member, int *n);
Task Cmp_Expr_Expand(Int species, Expr *e);
Task Cmp_Convert(Int type, Expr *e);

/* Used to define new C procedures for the builtin modules. */
Task Cmp_Builtin_Proc_Def(Int procedure, int when_should_call, Int of_type,
                          Task (*C_func)(VMProgram *));
Task Cmp_Builtin_CFunc_Def(UInt *sym_num, UInt *call_num,
                           const char *name, VMFunc c_func);
Task Cmp_Builtin_Conv_Def(char *name, Type src, Type dest, VMFunc c_func);


#define Cmp_Expr_Destroy_Tmp(e) Cmp_Expr_Destroy(e, 0)
#define Cmp_Expr_To_LReg(expr) Cmp__Expr_To_LReg(expr, 0)
#define Cmp_Expr_Force_To_LReg(expr) Cmp__Expr_To_LReg(expr, 1)
#define Cmp_Expr_To_Reg0(expr) Cmp__Expr_To_LReg(expr, 0, 1)
#define Cmp_Expr_Force_To_Reg0(expr) Cmp__Expr_To_LReg(expr, 1, 1)
#define Cmp_Operation_Find2(o, t1, t2) Cmp_Operation_Find(o, t1, t2, TYPE_NONE)
#define Cmp_Expr_Reg0(e, t) Cmp_Expr_LReg(e, t, 1)

#define BOX_CREATION 1
#define BOX_MODIFICATION 2

#endif
