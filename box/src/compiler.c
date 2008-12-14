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

/*
 * Questo file contiene l'implementazione del compilatore.
 */

#define USE_PRIVILEGED

/*#define DEBUG*/
/* #define DEBUG_CONTAINER_HANDLING */
/*#define DEBUG_SPECIES_EXPANSION*/
/*#define DEBUG_STRUCT_EXPANSION*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "types.h"
#include "defaults.h"
#include "messages.h"
#include "mem.h"
#include "array.h"
#include "str.h"
#include "virtmach.h"
#include "vmproc.h"
#include "vmsym.h"
#include "vmsymstuff.h"
#include "registers.h"
#include "compiler.h"
#include "box.h"
#include "builtins.h"
#include "parserh.h"
#include "typesys.h"
#include "autogen.h"
#include "expr.h"
#include "container.h"

/******************************************************************************/

/* The main compiler data structure */
Compiler *cmp;

/* "Collezione" di tutti gli operatori */
struct cmp_opr_struct cmp_opr;

/*****************************************************************************/

static void Opr_Destructor(void *opr_ptr) {
  Operator *opr = *((Operator **) opr_ptr);
  Cmp_Operator_Destroy(opr);
}

/* Gets ready to start the compilation */
Task Cmp_Init(VMProgram *program) {
  /* Initialization of the code which writes the bytecode program for the VM */
  cmp = Mem_Alloc(sizeof(Compiler));
  cmp->vm = program;
  cmp->ts = & cmp->ts_obj;

  BoxArr_Init(& cmp->allocd_oprs, sizeof(Operator *), 64);
  BoxArr_Set_Finalizer(& cmp->allocd_oprs, Opr_Destructor);
  TASK( TS_Init(cmp->ts) );

  /* Initialization of the lists which hold the occupation status
   * for registers and variables.
   */
  TASK( Reg_Init() );
  Msg_Line_Set((Int) 1);
  return Success;
}


Task Cmp_Parse(const char *file) {
  int parse_status;
  TASK( Box_Init() ); /* Init the box stack */
  TASK( Box_Main_Begin() ); /* Create the main box */

  /* Inizializzo il segmento che contiene gli oggetti con valore immediato */
  BoxArr_Init(& cmp->imm_segment, sizeof(char), CMP_TYPICAL_IMM_SIZE);

  TASK( Builtins_Init() ); /* Add builtin stuff */
  TASK( Parser_Init(file) ); /* Prepare the parser */
  parse_status = yyparse(); /* Parse the file */
  Parser_Finish(); /* Finalize parsing */
  Box_Main_End(); /* Close the main box */
  Box_Destroy(); /* Destroy the stack of boxes */
  Builtins_Destroy(); /* Finalises builtins stuff */
  Msg_Line_Set(MSG_UNDEF_LINE); /* Remove line numbers from error messages */
  return (parse_status != 0) ? Failed : Success;
}

void Cmp_Destroy(void) {
  TS_Destroy(cmp->ts);
  Reg_Destroy();

  BoxArr_Finish(& cmp->imm_segment);
  BoxArr_Finish(& cmp->allocd_oprs);
  Mem_Free(cmp);
}

/******************************************************************************/

/* Crea un nuovo operatore.
 */
Operator *Cmp_Operator_New(char *name) {
  Operator *opr;
  int i, j;

  opr = (Operator *) Mem_Alloc(sizeof(Operator));
  if ( opr == NULL ) {
    MSG_FATAL("Memory request failed.");
    return NULL;
  }

  /* Just to remember what gets allocated, so we can destroy it later! */
  BoxArr_Push(& cmp->allocd_oprs, & opr);

  opr->name = name;
  opr->can_define = 0;

  /* Pulisco i collegamenti alle operazioni */
  opr->opn_chain = NULL;
  for (j = 0; j < 3; j++ ) {
    for (i = 0; i < CMP_PRIVILEGED; i++)
      opr->opn[j][i] = NULL;
  }

  return opr;
}

void Cmp_Operator_Destroy(Operator *opr) {
  Operation *opn = opr->opn_chain;
  for(; opn != (Operation *) NULL;) {
    Operation *next = opn->next;
    Mem_Free(opn);
    opn = next;
  }

  Mem_Free(opr);
}

/* Aggiunge una nuova operazione di tipo type1 opr type2
 * all'operatore *opr. Se type1 o type2 sono uguali a TYPE_NONE si tratta
 * di un'operazione unaria (sinistra o destra rispettivamente).
 */
Operation *Cmp_Operation_Add(Operator *opr, Int type1, Int type2, Int typer) {
  Operation *opn;
  Int aa, t, t1, t2;
  int is_privileged;

#if 0
  printf("Adding operation (%s, %s) to operator '%s'\n",
   Tym_Type_Names(type1), Tym_Type_Names(type2), opr->name);
#endif
  opn = (Operation *) Mem_Alloc(sizeof(Operation));
  if ( opn == NULL ) {
    MSG_ERROR("Memory request failed.");
    return NULL;
  }

  /* Is it a privileged operation or not? */
  aa = (type1 == TYPE_NONE) + ((type2 == TYPE_NONE) << 1);
  switch (aa) {
  case 0:
    t = t1 = Tym_Type_Resolve_All(type1);
    t2     = Tym_Type_Resolve_All(type2);
    is_privileged = ( (t1 == t2) && (t <= CMP_PRIVILEGED) );
    break;
  case 1:
    t = Tym_Type_Resolve_All(type2);
    is_privileged = (t <= CMP_PRIVILEGED);
    break;
  case 2:
    t = Tym_Type_Resolve_All(type1);
    is_privileged = (t <= CMP_PRIVILEGED);
    break;
  default:
    MSG_ERROR("Operazione fra tipi nulli!");
    return NULL;
    break;
  }

#ifdef USE_PRIVILEGED
  /* Aggiungo l'operazione all'operatore: un'operazione privilegiata
   * viene inserita non solo nella catena delle operazioni, ma pure
   * in un'array che ne permette il ritrovamento in maniera immediata.
   */
  if ( is_privileged ) {
    /* In tal caso l'operazione e' privilegiata: i collegamenti
     * all'operatore sono diretti e quindi piu' veloci!
     */
    opr->opn[aa][t] = opn;
  }
#endif

  /* Creo un collegamento di tipo "a catena"
   */
  opn->next = opr->opn_chain;
  opr->opn_chain = opn;

  opn->type1 = type1;
  opn->type2 = type2;
  opn->type_rs = typer;
  return opn;
}

/* DESCRIPTION: Finds the unary or binary operation associated with
 *  the operator *opr.
 *  If type1 and type2 are both different from TYPE_NONE, then this function
 *  will search for a binary operation of the following kind:
 *                               type1 opr type2
 *  If type1 = TYPE_NONE, then a left-unary operation will be searched:
 *                                  opr type2
 *  If type2 = TYPE_NONE, then a right-unary operation will be searched:
 *                                  type1 opr
 *  If typer != TYPE_NONE also the type of the result will be checked
 *  during the search.
 * NOTE: it should not happen that type1 = type2 = TYPE_NONE.
 */
Operation *Cmp_Operation_Find(Operator *opr,
                              Type type1, Type type2, Type typer,
                              OpnInfo *oi)
{

  Int type;
  int no_check_arg1, no_check_arg2, check_rs, unary;
  int ne1, ne2;
  Operation *opn;

#if 0
  printf("Cmp_Operation_Find: Cerco %s OP %s\n",
   Tym_Type_Names(type1), Tym_Type_Names(type2));
#endif

  no_check_arg1 = (type1 == TYPE_NONE);
  no_check_arg2 = (type2 == TYPE_NONE);
  check_rs      = (typer != TYPE_NONE);
  unary = no_check_arg1 || no_check_arg2;

  /* Is it a privileged operation or not? */
  if ( ! check_rs ) {
    Int aa;
    int is_privileged;

    aa = no_check_arg1 | (no_check_arg2 << 1);
    switch (aa) {
    case 0:
      type = type1 = Tym_Type_Resolve_All(type1);
      type2 = Tym_Type_Resolve_All(type2);
      is_privileged = ( (type1 == type2) && (type <= CMP_PRIVILEGED) );
      break;
    case 1:
      type = Tym_Type_Resolve_All(type2);
      is_privileged = (type <= CMP_PRIVILEGED);
      break;
    case 2:
      type = Tym_Type_Resolve_All(type1);
      is_privileged = (type <= CMP_PRIVILEGED);
      break;
    default:
      MSG_ERROR("Operazione fra tipi nulli!");
      return NULL;
      break;
    }

#ifdef USE_PRIVILEGED
    if ( is_privileged ) {
      opn = opr->opn[aa][type];
      if ( opn != NULL ) {
        if ( oi == NULL ) return opn;
        oi->commute = 0;
        oi->expand1 = 0;
        oi->expand2 = 0;
        return opn;
      }
    }
#endif
  }

  for (opn = opr->opn_chain; opn != NULL; opn = opn->next ) {
    register Int t1 = opn->type1, t2 = opn->type2;
    int ok_1, ok_2, ok_rs = 1;

    ok_1 = (t1 == type1);
    ok_2 = (t2 == type2);
    if ( ! (ok_1 || no_check_arg1) ) ok_1 = Tym_Compare_Types(t1, type1, & ne1);
    if ( ! (ok_2 || no_check_arg2) ) ok_2 = Tym_Compare_Types(t2, type2, & ne2);

    if ( check_rs ) {
      ok_rs = Tym_Compare_Types(typer, opn->type_rs, NULL);
    }

    if ( ok_rs ) {
      if ( ok_1 && ok_2 ) {
        if ( oi == NULL ) return opn;
        oi->commute = 0;
        oi->expand1 = ne1;
        oi->expand2 = ne2;
        oi->exp_type1 = t1;
        oi->exp_type2 = t2;
        return opn;
      }

      if ( !unary && opn->is.commutative ) {
        ok_1 = (t1 == type2);
        ok_2 = (t2 == type1);
        if ( ! ok_1 ) ok_1 = Tym_Compare_Types(t1, type2, & ne2);
        if ( ! ok_2 ) ok_2 = Tym_Compare_Types(t2, type1, & ne1);
        if ( ok_1 && ok_2 ) {
          if ( oi == NULL ) return opn;
          oi->commute = 1;
          oi->expand1 = ne1;
          oi->expand2 = ne2;
          oi->exp_type1 = t2;
          oi->exp_type2 = t1;
          return opn;
        }
      }
    }
  }
  return NULL;
}

/* DESCRIPTION: Search for existance of the operation of conversion
 *  from type1 to type2. If e != NULL (e->type must be equal to type1)
 *  the function will try to convert the expression e into a new expression
 *  of type type2: the appropriate assembly code will be generated.
 */
Task Cmp_Conversion(Int type1, Int type2, Expr *e) {
  int do_it = 0;
  Int t1, t2;
  Operation *opn;

#ifdef DEBUG_SPECIES_EXPANSION
  printf("INIZIO LA CONVERSIONE - Converto: '%s' in '%s'\n",
   Tym_Type_Names(type1), Tym_Type_Names(type2));
#endif

  if ( e != NULL ) {
    assert( e->type == type1 );
    do_it = 1;
  }

  if ( type1 == type2 ) return Success;
  t1 = TS_Resolve(cmp->ts, type1, TS_KS_ALIAS | TS_KS_SPECIES);
  t2 = TS_Resolve(cmp->ts, type2, TS_KS_ALIAS | TS_KS_SPECIES);
  if ( t1 == t2 ) return Success;

#ifdef DEBUG_SPECIES_EXPANSION
  printf("Converto: '%s' in '%s'\n",
   Tym_Type_Names(t1), Tym_Type_Names(t2));
#endif
  /* Cerco se la conversione esiste */
  opn = Cmp_Operation_Find(cmp_opr.converter, t1, TYPE_NONE, t2, NULL);

  if ( opn == NULL ) {
    if ( ! do_it ) return Failed;
    MSG_ERROR("Conversion from '%s' to '%s' is undefined!",
              Tym_Type_Names(type1), Tym_Type_Names(type2));
    return Failed;
  }

  if ( do_it ) {
    /* Eseguo l'operazione di conversione */
    TASK( Cmp_Conversion_Exec(e, t2, opn) );
  }
#ifdef DEBUG_SPECIES_EXPANSION
  printf("FINISCO LA CONVERSIONE\n");
#endif
  return Success;
}

/* DESCRIPTION: Converte l'espressione *e in una di tipo type_dest,
 *  attraverso l'operazione di conversione *c_opn.
 * NOTE: The result of the conversion is returned into *e
 *  (the original expression is destroyed after the conversion).
 *  The final expression is associated with the register number 0
 *  (you may want to put it into a normal register with Cmp_Expr_Reg0_To_LReg:
 *  remember that the register number 0 is often used for special purposes
 *  and shouldn't be left occupied for long periods).
 */
Task Cmp_Conversion_Exec(Expr *e, Int type_dest, Operation *c_opn) {
  if ( c_opn->is.intrinsic ) {
    if ( (e->categ == CAT_LREG) && (e->value.i == 0) ) {
      /* Si tratta del registro r0 (ri0, rr0, ...) */
      Cmp_Assemble( c_opn->asm_code, CAT_LREG, (Int) 0 );
      e->type = type_dest;
      e->resolved = Tym_Type_Resolve_All(type_dest);

    } else {
      /* Metto l'espressione nel registro 0 */
      TASK( Cmp_Expr_To_LReg(e) );
      TASK( Cmp_Complete_Ptr_1(e) );
      Cmp_Assemble( c_opn->asm_code, e->categ, e->value.i );
      TASK( Cmp_Expr_Destroy_Tmp(e) );
      TASK( Cmp_Expr_Reg0(e, type_dest) );
    }

    *e = *Cmp_Expr_Reg0_To_LReg(type_dest);
    if ( e == NULL ) return Failed;
    assert(e->is.release);
    return Success;

  } else {
    /* Operazioni di conversione user-defined */
    Expr new_e;

  /* mov gro2, expr_src */
    TASK( Cmp_Expr_To_Ptr(e, CAT_GREG, (Int) 2, 0) );
  /* mov gro1, expr_dest */
    Expr_Container_New(& new_e, type_dest, CONTAINER_LREG_AUTO);
    Expr_Alloc(& new_e);
    TASK( Cmp_Expr_To_Ptr(& new_e, CAT_GREG, (Int) 1, 0) );
  /* call conv_func */
    TASK( VM_Sym_Call(cmp_vm, c_opn->module) );
    TASK( Cmp_Expr_Destroy_Tmp(e) );
    *e = new_e;
    return Success;
  }
}

/******************************************************************************/

/* DESCRIZIONE: Cerca di dare significato all'espressione *e1 opr *e2 e la
 *  "realizza", cioe' restituisce l'espressione risultante, compilando
 *  il codice eventualmente necessario. Si preoccupa anche di "liberare"
 *  le espressioni e1 ed e2.
 */
Expr *Cmp_Operator_Exec(Operator *opr, Expr *e1, Expr *e2) {
  int e1valued = 1, e2valued = 1;
  Int e1type, e2type, num_arg = 2;
  Operation *opn;
  OpnInfo oi;

#if 0
  printf("Cmp_Operator_Exec: Cerco %s OP %s\n",
   Tym_Type_Names(e1->type), Tym_Type_Names(e2->type));
#endif

  if ( e1 == NULL ) {
    e1type = TYPE_NONE;
    num_arg = 1;

  } else {
    Expr_Resolve_Subtype(e1);
    e1type = e1->resolved;
    e1valued = e1->is.value;
  }

  if ( e2 == NULL ) {
    e2type = TYPE_NONE;
    num_arg = 1;
  } else {
    Expr_Resolve_Subtype(e2);
    e2type = e2->resolved;
    e2valued = e2->is.value;
  }

  if ( !e1valued || !e2valued ) {
    if ( !e1valued ) {
      MSG_ERROR("The expression on the left of '%s' "
                "must have a definite value!", opr->name);
    } else {
      MSG_ERROR("The expression on the right of '%s' "
                "must have a definite value!", opr->name);
    }
    return NULL;
  }

  /* Prima cerco se esiste un'operazione fra i tipi di e1 e e2 */
  opn = Cmp_Operation_Find(opr, e1type, e2type, TYPE_NONE, & oi);
  if (opn != NULL) {
    /* Ora eseguo le espansioni, se necessario */
    if ( oi.expand1 ) {
      if IS_FAILED( Cmp_Expr_Expand(oi.exp_type1, e1) ) return NULL;
    }
    if ( oi.expand2 ) {
      if IS_FAILED( Cmp_Expr_Expand(oi.exp_type2, e2) ) return NULL;
    }

    /* manca la valutazione della commutativita'! */

    return Cmp_Operation_Exec(opn, e1, e2);

  } else {
    if (strcmp(opr->name, "=") != 0)
      goto Exec_Opn_Error;

    if IS_FAILED( Cmp_Expr_Expand(e1->type, e2) )
      return NULL;

    if IS_FAILED(Expr_Move(e1, e2))
      return NULL;

    return e1;
  }

Exec_Opn_Error:
  if ( num_arg == 1 ) {
    if ( e1type != TYPE_NONE ) {
        MSG_ERROR("%s %s <-- Operation has not been defined!",
         opr->name, Tym_Type_Name(e1->type) );
    } else {
        MSG_ERROR("%s %s <-- Operation has not been defined!",
         Tym_Type_Name(e2->type), opr->name );
    }
    return NULL;

  } else {
    MSG_ERROR("%s %s %s <-- Operation has not been defined!",
     Tym_Type_Names(e1->type), opr->name, Tym_Type_Names(e2->type) );
    return NULL;
  }
}



/******************************************************************************/

/* DESCRIZIONE: Questa funzione gestisce la generazione del codice
 *  corrispondente ad un gran numero di operazioni intrinseche.
 * NOTA: Viene chiamata da Cmp_Operation_Exec.
 */
static Expr *Opn_Exec_Intrinsic(Operation *opn, Expr *e1, Expr *e2) {

  struct {unsigned int unary :1, right : 1, strange :1, immediate :1;} opn_is;
  Int rs_resolved = Tym_Type_Resolve_All(opn->type_rs);

  /* Di solito argomenti e risultato sono tutti dello stesso tipo
    * In questi casi strange_case = 0, negli altri invece strange_case = 1!
    */
  switch( (e1 == NULL) + ((e2 == NULL) << 1) ) {
   case 0: /* Operazione a 2 argomenti */
    opn_is.strange = (rs_resolved != e1->resolved)
      || (rs_resolved != e2->resolved);
    opn_is.immediate = (e1->categ == CAT_IMM) && (e2->categ == CAT_IMM);
    opn_is.unary = 0;
    break;
   case 1: /* Operazione a 1 argomento (destro, cioe' e2) */
    opn_is.strange = (e2->resolved != rs_resolved);
    opn_is.immediate = (e2->categ == CAT_IMM);
    opn_is.unary = 1;
    opn_is.right = 1;
    e1 = e2;
    break;
   case 2: /* Operazione a 1 argomento (sinistro, cioe' e1) */
    opn_is.strange = (e1->resolved != rs_resolved);
    opn_is.immediate = (e1->categ == CAT_IMM);
    opn_is.unary = 1;
    opn_is.right = 0;
    e2 = e1;
    break;
   default:
    MSG_ERROR("Internal error: operation has no arguments!");
    return NULL;
  }

  if ( opn->is.assignment ) { /***********************************ASSIGNMENT*/
    if ( opn_is.strange ) {
      MSG_ERROR("Errore interno: operazione di assegnazione anomala!");
      return NULL;
    }

    /* Mi assicuro che il primo argomento possa fungere da target
      * dell'assegazione!
      */
    if ( ! e1->is.target ) {
      MSG_ERROR("This expression of type %~s cannot be modified!",
                TS_Name_Get(cmp->ts, e1->type));
      return NULL;
    }

    if ( opn_is.unary ) {
      /* Ora compilo l'operazione */
      if ( opn_is.right ) {
        Expr old_e2 = *e2;
        /*e2value = e2->value.i, e2categ = e2->categ;*/
        if IS_FAILED( Cmp_Expr_Force_To_LReg(e2) ) return NULL;
        if IS_FAILED( Cmp_Complete_Ptr_1(& old_e2) ) return NULL;
        Cmp_Assemble(opn->asm_code, old_e2.categ, old_e2.value.i);
        return e2;
      } else {
        if IS_FAILED( Cmp_Complete_Ptr_1(e1) ) return NULL;
        Cmp_Assemble(opn->asm_code, e1->categ, e1->value.i);
        return e1;
      }
    } else {
      /* Se necessario, converto e2 in registro locale. */
      if IS_FAILED( Cmp_Expr_To_LReg(e2) ) return NULL;

      /* Ora compilo l'operazione */
      if IS_FAILED( Cmp_Complete_Ptr_2(e1, e2) ) return NULL;
      Cmp_Assemble(opn->asm_code,
       e1->categ, e1->value.i, e2->categ, e2->value.i);

      /* Ora libero il registro che non contiene il risultato! */
      Cmp_Expr_Destroy_Tmp(e2);
      return e1;
    }

  } else { /**************************************************NOT ASSIGNMENT*/
    struct {unsigned int e1 : 1, e2 : 1;} result_in;

    /* Determino se e1 puo' contenere il risultato dell'operazione
    * (il quale e' una espressione temporanea).
    * A tale scopo bisogna che e1 abbia lo stesso tipo del risultato,
    * che sia un registro locale e che non sia una variabile (cioe' un
    * target per l'operazione di assegnazione).
    */
    result_in.e1 = (e1->resolved == rs_resolved)
     && (e1->categ == CAT_LREG) && (! e1->is.target);

    if ( opn_is.strange ) { /**************************************STRANGE*/
      /* Le operazioni strane sono, ad esempio: il confronto fra numeri
      * (operatori <, >, ==, ...), moltiplicazione fra un punto e
      * un reale, etc.
      */
      if ( opn_is.unary ) {
        MSG_ERROR("Still not implemented!");
        return NULL;

      } else {
        /* Se l'operazione e' binaria ho 3 casi a seconda dei tipi
        * delle 3 quantita' coinvolte dall'operazione (2 argomenti +
        * 1 risultato):
        */
        struct {unsigned int er_e1 : 1, er_e2 : 1, e1_e2 : 1;} eq;
        eq.e1_e2 = ( e1->resolved == e2->resolved );
        eq.er_e1 = ( rs_resolved == e1->resolved );
        eq.er_e2 = ( rs_resolved == e2->resolved );
        if ( eq.e1_e2 ) {
          /* caso 1: argomenti dello stesso tipo, ma risultato
          *  di tipo diverso (caso di <, <=, >, >=, ==, !=, ...)
          */
          if ( (Cmp_Expr_To_LReg(e1) == Failed)
            || (Cmp_Expr_To_LReg(e2) == Failed) ) return NULL;
          if IS_FAILED( Cmp_Complete_Ptr_2(e1, e2) ) return NULL;
          Cmp_Assemble(opn->asm_code,
           e1->categ, e1->value.i, e2->categ, e2->value.i);
          Cmp_Expr_Destroy_Tmp(e1);
          Cmp_Expr_Destroy_Tmp(e2);
          return Cmp_Expr_Reg0_To_LReg(opn->type_rs);

        } else {
          if ( eq.er_e2 ) {
            Expr *tmp; tmp = e1; e1 = e2; e2 = tmp;
            goto er_equal_e1;
          }

          if ( eq.er_e1 ) { /* caso 2: un argomento e' dello stesso tipo
                                del risultato */
er_equal_e1:
            if IS_FAILED( Cmp_Expr_Force_To_LReg(e1) ) return NULL;
            if IS_FAILED( Cmp_Expr_To_X(e2, CAT_LREG, 0, 1) ) return NULL;
            if IS_FAILED( Cmp_Complete_Ptr_1(e1) ) return NULL;
            Cmp_Assemble(opn->asm_code, e1->categ, e1->value.i);
            return e1;

          } else { /* caso 3: tipi tutti diversi */
            MSG_ERROR("Still not implemented!");
            return NULL;
          }
        }
      }
      return NULL;

    } else { /*************************************************NOT STRANGE*/
      if ( opn_is.unary ) {
        if ( opn_is.right ) e1 = e2;
        if IS_FAILED( Cmp_Expr_Force_To_LReg(e1) ) return NULL;
        if IS_FAILED( Cmp_Complete_Ptr_1(e1) ) return NULL;
        Cmp_Assemble(opn->asm_code, e1->categ, e1->value.i);
        return e1;

      } else {
        result_in.e2 = (e2->resolved == rs_resolved)
         && (e2->categ == CAT_LREG) && (! e2->is.target);

        /* Se l'operazione e' commutativa, posso usare e2 per contenere
        * il risultato dell'operazione!
        */
        if ( result_in.e2 && !result_in.e1 && opn->is.commutative ) {
          register Expr *tmp = e2;
          e2 = e1; e1 = tmp;
          result_in.e1 = 1;
          result_in.e2 = 0;
        }

        /* Se non posso mettere il risultato in e1, allora devo
        * convertire e1 in un registro locale (che puo' contenerlo!)
        */
        if ( ! result_in.e1 ) {
          if IS_FAILED( Cmp_Expr_Force_To_LReg(e1) ) return NULL;
        }

        /* Se necessario, converto e2 in registro locale. */
        if IS_FAILED( Cmp_Expr_To_LReg(e2) ) return NULL;

        /* Ora compilo l'operazione */
        if IS_FAILED( Cmp_Complete_Ptr_2(e1, e2) ) return NULL;
        Cmp_Assemble(opn->asm_code,
         e1->categ, e1->value.i, e2->categ, e2->value.i);

        /* Ora libero il registro che non contiene il risultato! */
        Cmp_Expr_Destroy_Tmp(e2);
        return e1;
      }
    }
  }
}

/* DESCRIZIONE:
 *  NULL, e --> operazione unaria destra come e++
 *  e, NULL --> operazione unaria sinistra come ++e
 */
Expr *Cmp_Operation_Exec(Operation *opn, Expr *e1, Expr *e2) {
  int strange_case, result_in_e1;

  if ( opn->is.intrinsic )
    return Opn_Exec_Intrinsic(opn, e1, e2);
  else {
    MSG_ERROR("Operator overloading is still not implemented!");
    return NULL;
  }

/*****************************************************************************/
        if ( opn->is.immediate ) {
                if ( e1->is.imm && e2->is.imm ) {
                        /*return ...;*/
                }
        }

        /* Di solito argomenti e risultato sono tutti dello stesso tipo
         * (in tal caso strange_case = 0)
         */
        strange_case = (e1->type != e2->type) || (e1->type != opn->type_rs);

        /* Controllo se uno dei due argomenti puo' essere usato per contenere
         * il risultato dell'operazione.
         */
        if ( (e1->type == opn->type_rs) && (e1->categ == CAT_LREG) ) {
                /* Il tipo dell'argomento 1 coincide col tipo del risultato, inoltre
                 * l'argomento 1 e' un registro locale: e1 conterra' il risultato
                 * dell'operazione!
                 */
                result_in_e1 = 1;

        } else if (opn->is.commutative) {
                if ( (e2->type == opn->type_rs) && (e2->categ == CAT_LREG) ) {
                        /* Il tipo dell'argomento 2 coincide col tipo del risultato, inoltre
                         * l'argomento 2 e' un registro locale e l'operazione e' commutativa:
                         * scambio e2 con e1 cosicche' e1 conterra' il risultato dell'oper.!
                         */
                        register Expr *tmp = e2;
                        e2 = e1; e1 = tmp;
                        result_in_e1 = 1;

                } else
                        result_in_e1 = 0;

        } else
                result_in_e1 = 0;

        if ( ! strange_case ) {
                /* Se non posso mettere il risultato in e1 o e2, allora devo convertire
                 * e1 in un registro locale (che puo' contenerlo!)
                 */
                if ( ! result_in_e1 ) {
                  if IS_FAILED( Cmp_Expr_Force_To_LReg(e1) ) return NULL;
                }

                /* Se necessario, converto e2 in registro locale. */
                if IS_FAILED( Cmp_Expr_To_LReg(e2) ) return NULL;

                /* Ora compilo l'operazione */
                if IS_FAILED( Cmp_Complete_Ptr_2(e1, e2) ) return NULL;
                Cmp_Assemble(opn->asm_code,
                 e1->categ, e1->value.i, e2->categ, e2->value.i);

                /* Ora libero il registro che non contiene il risultato! */
                Cmp_Expr_Destroy_Tmp(e2);
                return e1;

        } else {
        }

        printf("Esecuzione terminata!\n");
        return NULL;
}

/******************************************************************************
 * Le procedure che seguono servono gestire i registri: occuparli, liberarli, *
 * convertirli, etc.                                                          *
 ******************************************************************************/

/* DESCRIZIONE: Inizializzo la struttura expr in modo che contenga un
 *  registro locale libero. Se zero == 0, tale registro viene occupato,
 *  fino a quando non sara' "liberato" con Cmp_LReg_Free.
 *  Se zero == 1, viene utilizzato il registro numero zero (ad esempio:
 *  ri0, rr0, ...), il quale viene utilizzato dal compilatore
 *  (per convenzione) per il transito temporaneo dei dati.

 -----------------------> OBSLOLETE <------------------------
 */
Task Cmp_Expr_LReg(Expr *e, Int type, int zero) {
  e->is.imm = 0;
  e->is.value = 1;
  e->is.ignore = 0;
  e->is.target = 0;
  e->is.typed = 1;
  e->is.release = 1;
  e->is.allocd = 0;

  e->categ = CAT_LREG;
  e->type = type;
  e->resolved = Tym_Type_Resolve_All(type);

  if (TS_Is_Fast(cmp->ts, type)) {
    if ( zero ) { e->value.i = 0; return Success; }
    if ( (e->value.i = Reg_Occupy(type)) < 1 ) return Failed;
    return Success;

  } else {
    Int s = Tym_Type_Size(type);
    if ( s > 0 ) {
      Int at;
      e->is.allocd = 1;
      if ( zero ) { e->value.i = 0; }
        else { if ( (e->value.i = Reg_Occupy(TYPE_OBJ)) < 1 ) return Failed; }

      at = Expr_Allocation_Type(e);
      Cmp_Assemble(ASM_MALLOC_I, CAT_IMM, s, CAT_IMM, at);
      Cmp_Assemble(ASM_MOV_OO, e->categ, e->value.i, CAT_LREG, 0);
    }
    return Success;
  }
}

static Int asm_lea[NUM_INTRINSICS] = {
  ASM_LEA_C, ASM_LEA_I, ASM_LEA_R, ASM_LEA_P
};

static Int asm_mov[NUM_INTRINSICS] = {
  ASM_MOV_CC, ASM_MOV_II, ASM_MOV_RR, ASM_MOV_PP
};

/* This function generates the assembly code which
 * puts the expression expr into a precise register.
 * NOTE: categ can be CAT_LREG or CAT_GREG.
 */
Task Cmp_Expr_To_X(Expr *expr, AsmArg categ, Int reg, int and_free) {
#define EXIT_FUNCTION if ( !and_free ) return Success; return Cmp_Expr_Destroy_Tmp(expr)
  int is_integer;

  assert(expr != NULL);
  assert( (expr->is.typed) && (expr->is.value) );
  assert( (categ == CAT_LREG) || (categ == CAT_GREG) );

  if ( expr->resolved < TYPE_OBJ ) {
    /********** THIS PART OF THE FUNCTIONS HANDLES INTRINSIC TYPES ***********/
    is_integer = (expr->resolved==TYPE_CHAR) || (expr->resolved==TYPE_INT);
    if ( (expr->categ == CAT_IMM) && (! is_integer) ) {
      switch ( expr->resolved ) {
       case TYPE_REAL:
        Cmp_Assemble(ASM_MOV_Rimm, categ, reg, CAT_IMM, expr->value.r); break;
       case TYPE_POINT:
        Cmp_Assemble(ASM_MOV_Pimm, categ, reg, CAT_IMM, expr->value.p); break;
       default:
        MSG_ERROR( "Errore interno: non sono ammessi valori immediati"
         " per il tipo '%s'.", Tym_Type_Name(expr->type) );
        return Failed;
      }
      EXIT_FUNCTION;

    } else {
      register Int t = expr->resolved;
      if ( expr->categ == CAT_PTR ) {
        /* L'espressione e' un puntatore, allora devo settare il puntatore
        * di riferimento, in modo da realizzare una cosa simile a:
        *   mov ro0, ro1         <-- setto il puntatore di riferimento (ro0)
        *   mov rr1, real[ro0+8] <-- prelevo il valore
        */
        Int addr_categ = ( expr->is.gaddr ) ? CAT_GREG : CAT_LREG;
        Cmp_Assemble( ASM_MOV_OO, CAT_LREG, (Int) 0,
         addr_categ, expr->addr );
      }

      if (t < 0 || t >= NUM_INTRINSICS) {
        MSG_ERROR("Errore interno in Cmp_Expr_To_X");
        return Failed;
      }

      Cmp_Assemble(asm_mov[t], categ, reg, expr->categ, expr->value.i);
      EXIT_FUNCTION;
    }

  } else {/***** THIS PART OF THE FUNCTIONS HANDLES NON-INTRINSIC TYPES ******/
    assert( expr->categ != CAT_IMM );

    if ( expr->categ == CAT_PTR ) {
      register Int addr_categ = ( expr->is.gaddr ) ? CAT_GREG : CAT_LREG;
      Cmp_Assemble( ASM_MOV_OO, CAT_LREG, (Int) 0, addr_categ, expr->addr );
      Cmp_Assemble( ASM_LEA_OO, categ, reg, CAT_PTR, expr->value.i );
      EXIT_FUNCTION;

    } else { /* expr->categ == CAT_LREG, CAT_GREG */
      Cmp_Assemble(ASM_MOV_OO, categ, reg, expr->categ, expr->value.i);
      EXIT_FUNCTION;
    }
  }

#undef EXIT_FUNCTION
}

/* DESCRIPTION: Converte l'espressione expr in un'espressione contenuta
 *  in un registro locale. Se force = 1, tale operazione viene eseguita
 *  in tutti i casi, se invece force = 0, l'operazione viene eseguita
 *  solo per espressioni immediate di tipo TYPE_REAL e TYPE_POINT.
 */
Task Cmp__Expr_To_LReg(Expr *expr, int force) {
  Expr lreg;

  /* Se l'espressione e' gia' un registro locale, allora esco! */
  if ( expr->categ == CAT_LREG )
    if ( expr->value.reg > 0 ) return Success;

  if ( ! force ) {
    int is_integer =
     (expr->resolved == TYPE_CHAR) || (expr->resolved == TYPE_INT);
    if (is_integer || (expr->categ != CAT_IMM)) return Success;
  }

  Expr_Container_New(& lreg, expr->type, CONTAINER_LREG_AUTO);
  Expr_Alloc(& lreg);
  TASK( Cmp_Expr_To_X(expr, lreg.categ, lreg.value.reg, 1) );
  *expr = lreg;
  return Success;
}

/* This function generates the assembly code which
 * puts the address of the expression expr into a precise register.
 * NOTE:
 * - categ can be CAT_LREG or CAT_GREG.
 * - can't be used to pass more than one argument per time:
 *   For example: suppose you use it with two immediate values:
 *     mov ri0, 1 <-- first value
 *     lea ri0
 *     mov ro1, ro0
 *     mov ri0, 2 <-- second value
 *     lea ri0
 *     mov ro2, ro0
 *   ro1 and ro2 will be pointers to the same value (2 which overwrited 1).
 */
Task Cmp_Expr_To_Ptr(Expr *expr, AsmArg categ, Int reg, int and_free) {
  assert(expr != NULL);
  assert( (expr->is.typed) && (expr->is.value) );
  assert( (categ == CAT_LREG) || (categ == CAT_GREG) );

  if ( expr->categ == CAT_IMM ) {
    register Int t = expr->resolved;

    TASK( Cmp_Expr_To_X(expr, CAT_LREG, (Int) 0, and_free) );
    assert(t >= 0 && t < NUM_INTRINSICS);
    Cmp_Assemble(asm_lea[t], CAT_LREG, (Int) 0);
    Cmp_Assemble(ASM_MOV_OO, categ, reg, CAT_LREG, (Int) 0);
    return Success;

  } else {
    register Int t = expr->resolved;

    assert(t >= 0);
    if (t < NUM_INTRINSICS) {
      if ( expr->categ == CAT_PTR ) {
        /* L'espressione e' un puntatore, allora devo settare il puntatore
        * di riferimento, in modo da realizzare una cosa simile a:
        *   mov ro0, ro1 <-- setto il puntatore di riferimento (ro0)
        *   lea p[ro0+8] <-- prelevo il valore
        *   mov ..., ro0 <-- metto l'indirizzo dove richiesto
        */
        register Int addr_categ = ( expr->is.gaddr ) ? CAT_GREG : CAT_LREG;
        Cmp_Assemble( ASM_MOV_OO, CAT_LREG, (Int) 0, addr_categ, expr->addr );
      }

      Cmp_Assemble(asm_lea[t], expr->categ, expr->value.i);
      Cmp_Assemble(ASM_MOV_OO, categ, reg, CAT_LREG, (Int) 0);
      if ( !and_free ) return Success;
      return Cmp_Expr_Destroy_Tmp(expr);

    } else {
      if ( expr->categ == CAT_PTR ) {
        register Int addr_categ = ( expr->is.gaddr ) ? CAT_GREG : CAT_LREG;
        Cmp_Assemble( ASM_MOV_OO, CAT_LREG, (Int) 0, addr_categ, expr->addr );
        Cmp_Assemble( ASM_LEA_OO, categ, reg, CAT_PTR, expr->value.i );
        if ( !and_free ) return Success;
        return Cmp_Expr_Destroy_Tmp(expr);

      } else { /* expr->categ == CAT_LREG, CAT_GREG */
        Cmp_Assemble(ASM_MOV_OO, categ, reg, expr->categ, expr->value.i);
        if ( !and_free ) return Success;
        return Cmp_Expr_Destroy_Tmp(expr);
      }
    }
  }
}

/* This function generates the assembly code which
 * puts the address of the expression expr into a precise register.
 * NOTE:
 * - categ can be CAT_LREG or CAT_GREG.
 * - can't be used to pass more than one argument per time:
 *   For example: suppose you use it with two immediate values:
 *     mov ri0, 1 <-- first value
 *     lea ri0
 *     mov ro1, ro0
 *     mov ri0, 2 <-- second value
 *     lea ri0
 *     mov ro2, ro0
 *   ro1 and ro2 will be pointers to the same value (2 which overwrote 1).
 */
Task Cmp_Expr_Container_Change(Expr *e, Container *c) {
  assert(e != NULL && c != NULL);
  assert(e->is.typed && e->is.value);

  switch( c->type_of_container ) {
  case CONTAINER_TYPE_ARG: case CONTAINER_TYPE_STACK: {
    int is_arg = (c->type_of_container == CONTAINER_TYPE_ARG);

    if ( e->categ == CAT_IMM ) {
      Int t = e->resolved;
      assert(t >= 0 && t < NUM_INTRINSICS);
      TASK( Cmp_Expr_To_X(e, CAT_LREG, (Int) 0, 0) );
      Cmp_Assemble(asm_lea[t], CAT_LREG, (Int) 0);
      if ( is_arg ) { /* c->type_of_container == CONTAINER_TYPE_ARG */
        Cmp_Assemble(ASM_MOV_OO, CAT_GREG, c->which_one, CAT_LREG, (Int) 0);
        return Success;
      } else { /* c->type_of_container == CONTAINER_TYPE_STACK */
        Cmp_Assemble(ASM_PUSH_O, CAT_LREG, (Int) 0);
        return Success;
      }
    } else {
      register Int t = e->resolved;
      assert(t >= 0);

      if (t < NUM_INTRINSICS) {
        if ( e->categ == CAT_PTR ) {
          /* L'espressione e' un puntatore, allora devo settare il puntatore
          * di riferimento, in modo da realizzare una cosa simile a:
          *   mov ro0, ro1 <-- setto il puntatore di riferimento (ro0)
          *   lea p[ro0+8] <-- prelevo il valore
          *   mov ..., ro0 <-- metto l'indirizzo dove richiesto
          */
          register Int addr_categ = ( e->is.gaddr ) ? CAT_GREG : CAT_LREG;
          Cmp_Assemble( ASM_MOV_OO, CAT_LREG, (Int) 0, addr_categ, e->addr );
        }

        Cmp_Assemble(asm_lea[t], e->categ, e->value.i);
        if ( is_arg ) { /* c->type_of_container == CONTAINER_TYPE_ARG */
          Cmp_Assemble(ASM_MOV_OO, CAT_GREG, c->which_one, CAT_LREG, (Int) 0);
          return Success;
        } else { /* c->type_of_container == CONTAINER_TYPE_STACK */
          Cmp_Assemble(ASM_PUSH_O, CAT_LREG, (Int) 0);
          return Success;
        }

      } else {
        if ( e->categ == CAT_PTR ) {
          if ( is_arg ) {
            register Int addr_categ = ( e->is.gaddr ) ? CAT_GREG : CAT_LREG;
            Cmp_Assemble( ASM_MOV_OO, CAT_LREG, (Int) 0, addr_categ, e->addr );
            Cmp_Assemble( ASM_LEA_OO, CAT_GREG, c->which_one,
              CAT_PTR, e->value.i );
            return Success;
          } else {
            register Int addr_categ = ( e->is.gaddr ) ? CAT_GREG : CAT_LREG;
            Cmp_Assemble( ASM_MOV_OO, CAT_LREG, (Int) 0, addr_categ, e->addr );
            Cmp_Assemble( ASM_LEA_OO, CAT_LREG, 0, CAT_PTR, e->value.i );
            Cmp_Assemble( ASM_PUSH_O, CAT_LREG, 0);
          }
        } else { /* e->categ == CAT_LREG, CAT_GREG */
          if ( is_arg ) {
            Cmp_Assemble(ASM_MOV_OO, CAT_GREG, c->which_one,
             e->categ, e->value.i);
            return Success;
          } else {
            Cmp_Assemble(ASM_PUSH_O, e->categ, e->value.i);
            return Success;
          }
        }
      }
    }
  }
  default:
    MSG_ERROR("Cmp_Expr_Container_Change: wrong type of container!");
  }
  return Failed;
}

/* DESCRIPTION:
 */
Task Cmp_Expr_Destroy(Expr *e, int destroy_target) {
#ifdef DEBUG_CONTAINER_HANDLING
  printf( "Cmp_Expr_Destroy: destroying " );
  Expr_Print(e, stdout);
#endif

  if ( ! e->is.typed ) {
    Name_Free(& e->value.nm);
    return Success;

  } else {
    register int intrinsic;
    Int type_of_register, resolved;;

    assert( ! (e->is.target && e->is.imm) );
    if ( !e->is.value ) return Success;

    resolved = e->resolved;
    intrinsic = (resolved < NUM_INTRINSICS);
    type_of_register = (intrinsic) ? resolved : TYPE_OBJ;

    switch ( e->categ ) {
     case CAT_IMM: return Success;

     case CAT_PTR:
      if ( ! e->is.release ) break;
      /* L'address e' un registro o variabile? */
      /* Se e' un registro, va liberato! */
      if ( e->addr > 0 ) return Reg_Release(TYPE_OBJ, e->addr);
      break;

     case CAT_LREG:
      if ( (! e->is.release) || (e->value.i <= 0) ) break;
      TASK( Reg_Release(type_of_register, e->value.i) );
      break;

     default: break;
    }

    /* Call the finaliser */


    if (!intrinsic && e->is.allocd && (!e->is.target || destroy_target)) {
      Auto_Acknowledge_Call(e->type, TYPE_DESTROY, 1);

      /* NOTE: Should I use  Cmp_Complete_Ptr_1?
       * I think I shouldn't, since it makes no sense to call munln
       * on the members of a structure or array!
       */
      Cmp_Assemble(ASM_MUNLN_O, e->categ, e->value.i);
    }
    return Success;
  }
}

/* DESCRIZIONE: Prende il valore dal registro zero per metterlo in un registro
 *  locale temporaneo (che viene occupato).
 */
Expr *Cmp_Expr_Reg0_To_LReg(Int t) {
  static Expr lreg;

  /* Metto in lreg un nuovo registro locale */
  Expr_Container_New(& lreg, t, CONTAINER_LREG_AUTO);

  switch (t) {
   case TYPE_NONE:
    MSG_ERROR("Internal error in Cmp_Expr_Reg0_To_LReg");
    return NULL; break;
   case TYPE_CHAR:
    Cmp_Assemble(ASM_MOV_CC, lreg.categ, lreg.value.i, CAT_LREG, 0);
    return &lreg; break;
   case TYPE_INT:
    Cmp_Assemble(ASM_MOV_II, lreg.categ, lreg.value.i, CAT_LREG, 0);
    return &lreg; break;
   case TYPE_REAL:
    Cmp_Assemble(ASM_MOV_RR, lreg.categ, lreg.value.i, CAT_LREG, 0);
    return &lreg; break;
   case TYPE_POINT:
    Cmp_Assemble(ASM_MOV_PP, lreg.categ, lreg.value.i, CAT_LREG, 0);
    return &lreg; break;
   default:
    Cmp_Assemble(ASM_MOV_OO, lreg.categ, lreg.value.i, CAT_LREG, 0);
    return &lreg; break;
  }
  return NULL;
}

#if 0
/* Not used */

/* DESCRIZIONE: Questa funzione controlla che una espressione di tipo oggetto
 *  sia di categoria "registro locale" (CAT_LREG) o "registro globale"
 *  (CAT_GREG). In caso contrario (cioe' per categoria "puntatore a "
 *  (CAT_PTR) ) provvede a mettere l'espressione in un registro locale.
 * NOTA: Tutte queste operazioni sono eseguite su *e.
 */
Task Cmp_Expr_O_To_OReg(Expr *e) {
  assert ( e->type >= NUM_INTRINSICS );

  switch ( e->categ ) {
   case CAT_GREG: case CAT_LREG:
    return Success;
   case CAT_PTR: {
      Int ro_num = e->addr;
      Int ro_categ = ( e->is.gaddr ) ? CAT_GREG : CAT_LREG;

      if ( ro_num > 0 ) {
        /* Il puntatore di riferimento e' contenuto in un registro:
          * quindi posso riutilizzare tale registro nel modo seguente
          * (se il registro in questione e' ro4):
          *  mov ro0, ro4
          *  mov ro4, Obj[ro0+64]
          */
        Cmp_Assemble( ASM_MOV_OO, CAT_LREG, (Int) 0, ro_categ, ro_num );
        Cmp_Assemble( ASM_MOV_OO, ro_categ, ro_num, CAT_PTR, e->value.i );
        e->resolved = e->type = TYPE_OBJ;
        e->categ = ro_categ;
        e->value.i = ro_num;
        e->is.imm = e->is.ignore = 0;
        e->is.value = e->is.target = e->is.typed = 1;
        return Success;

      } else {
        /* Il puntatore di riferimento e' contenuto in una variabile:
          * devo realizzare qualcosa del tipo:
          *  mov ro0, vo1
          *  mov ro1, Obj[ro0+8]
          */
        Int new_reg;

        Cmp_Assemble( ASM_MOV_OO, CAT_LREG, (Int) 0, ro_categ, ro_num );
        new_reg = Reg_Occupy(TYPE_OBJ);
        if ( new_reg < 1 ) return Failed;
        Cmp_Assemble( ASM_MOV_OO, CAT_LREG, new_reg, CAT_PTR, e->value.i );
        e->type = TYPE_OBJ;
        e->categ = CAT_LREG;
        e->value.i = new_reg;
        e->is.imm = e->is.ignore = 0;
        e->is.value = e->is.target = e->is.typed = 1;
        return Success;
      }
    }
   default:
    MSG_ERROR("Errore interno in Cmp_Expr_O_To_OReg.");
    return Failed;
  }
  return Failed;
}
#endif

/* DESCRIZIONE: Quando faccio riferimento ad un membro di una struttura
 *  il compilatore genera del codice tipo:
 *   mov ro0, ro5        \___ accede all'intero descritto da (ro5, 8)
 *   mov ri1, i[ro0+8]   /
 *  Quindi bisogna caricare in ro0 il registro che fa da puntatore base.
 *  Questa funzione fa proprio questo.
 */
Task Cmp_Complete_Ptr_2(Expr *e1, Expr *e2) {
  Int addr_categ;

  switch ( (e1->categ == CAT_PTR) | (e2->categ == CAT_PTR) << 1) {
   case 1:
    addr_categ = ( e1->is.gaddr ) ? CAT_GREG : CAT_LREG;
    Cmp_Assemble(ASM_MOV_OO, CAT_LREG, (Int) 0, addr_categ, e1->addr);
    return Success;
   case 2:
    addr_categ = ( e2->is.gaddr ) ? CAT_GREG : CAT_LREG;
    Cmp_Assemble(ASM_MOV_OO, CAT_LREG, (Int) 0, addr_categ, e2->addr);
    return Success;
   case 3:
    if IS_FAILED( Cmp_Expr_Force_To_LReg(e2) ) return Failed;
    addr_categ = ( e1->is.gaddr ) ? CAT_GREG : CAT_LREG;
    Cmp_Assemble(ASM_MOV_OO, CAT_LREG, (Int) 0, addr_categ, e1->addr);
    return Success;
   default:
    return Success;
  }
}

/* DESCRIZIONE: Analoga alla precedente, ma questa funzione deve essere chiamata
 *  nel caso di istruzioni con 1 solo argomento.
 */
Task Cmp_Complete_Ptr_1(Expr *e) {
  if ( e->categ == CAT_PTR ) {
    register Int addr_categ = ( e->is.gaddr ) ? CAT_GREG : CAT_LREG;
    Cmp_Assemble(ASM_MOV_OO, CAT_LREG, (Int) 0, addr_categ, e->addr);
  }
  return Success;
}

/*****************************************************************************
 * The following functions are needed to handle the data segment and the     *
 * segment of immediate values. The first is necessary to store strings and  *
 * the values of other kind of objects. The second is used to keep the value *
 * of objects which can be handled immediately at compile-time.              *
 * EXAMPLE (to understand what is an immediate): in the following statement: *
 *                                    a = 1 + 2                              *
 *  1 and 2 are immediate values: the compiler can calculate immediately the *
 *  result of any operation involving only 1 and 2, so it can produce the    *
 *  same output of the statement 'a = 3'.                                    *
 *****************************************************************************/

typedef struct {
  Int type;
  Int size;
} DataItem;

/* DESCRIPTION: This function adds a new piece of data to the segment
 *  of immediate objects.
 * NOTE: It returns the address of the data item with respect to the beginning
 *  of the segment.
 */
Int Cmp_Imm_Add(Compiler *cmp, Int type, void *data, Int size) {
  Int addr;
  DataItem di;

  /* Now we insert the data descriptor */
  di.type = type;
  di.size = size;
  BoxArr_MPush(& cmp->imm_segment, & di, sizeof(di));

  /* And now we insert the piece of data */
  addr = Arr_NumItem(& cmp->imm_segment);
  BoxArr_MPush(& cmp->imm_segment, data, size);
  return addr;
}

/*****************************************************************************
 * The following functions are needed to handle strings.                     *
 *****************************************************************************/

Task Cmp_String_New(Expr *e, Name *str, int free_str) {
  Int ts, addr;
  Int length = str->length;

  /* Creo il tipo di dato corrispondente alla stringa */
  ts = Tym_Def_Array_Of(length, TYPE_CHAR);
  if ( ts < 0 ) return Failed;

  /* Copio la stringa nell'area dati */
  addr = BoxVM_Data_Add(cmp->vm, str->text, length, ts);
  if (addr < 0) return Failed;

  /* Libero la stringa se devo! */
  if (free_str) Name_Free(str);

  /* Costruisco l'espressione corrispondente alla stringa */
  e->is.imm = 0;
  e->is.value = 1;
  e->is.ignore = 0;
  e->is.target = 0;
  e->is.typed = 1;
  e->is.gaddr = 1;
  e->is.allocd = 0;
  e->is.release = 0;

  e->addr = 0;
  e->resolved = e->type = ts;
  e->categ = CAT_PTR;

  e->value.i = addr;
  return Success;
}

/*****************************************************************************
 * The following functions are needed to define builtin procedures.          *
 *****************************************************************************/

Task Cmp_Builtin_Proc_Def(Int procedure, int when_should_call, Int of_type,
                          Task (*C_func)(VMProgram *)) {
  UInt proc = 0, sym_num, call_num;

  /* We then create the symbol associated with this name */
  TASK( VM_Sym_New_Call(cmp_vm, & sym_num) );

  /* We tell to the compiler that some procedures are associated with it */
  if ( when_should_call & BOX_CREATION ) {
    proc = Tym_Def_Procedure(procedure, 0, of_type, sym_num);
    if ( proc == TYPE_NONE ) return Failed;
  }

  if ( when_should_call & BOX_MODIFICATION ) {
    proc = Tym_Def_Procedure(procedure, 1, of_type, sym_num);
    if ( proc == TYPE_NONE ) return Failed;
  }

  if (!proc) {
    MSG_FATAL("Cmp_Def_C_Procedure: Should not happen!");
    return Failed;
  }

  /* We finally install the code (a C function) for the procedure */
  TASK( VM_Proc_Install_CCode(cmp_vm, & call_num, C_func,
                              "(noname)", Tym_Type_Name(proc)) );
  /* And define the symbol */
  TASK( VM_Sym_Def_Call(cmp_vm, sym_num, call_num) );
  return Success;
}

Task Cmp_Builtin_CFunc_Def(UInt *sym_num, UInt *call_num,
                           const char *name, VMFunc c_func) {
  UInt dummy_sym_num, dummy_call_num;
  if (sym_num == (UInt *) NULL) sym_num = & dummy_sym_num;
  if (call_num == (UInt *) NULL) call_num = & dummy_call_num;
  TASK( VM_Sym_New_Call(cmp->vm, sym_num) );
  TASK( VM_Proc_Install_CCode(cmp->vm, call_num, c_func, "(noname)", name) );
  TASK( VM_Sym_Def_Call(cmp->vm, *sym_num, *call_num) );
  return Success;
}

Task Cmp_Builtin_Conv_Def(char *name, Type src, Type dest, VMFunc c_func) {
  UInt sym_num;
  Operation *opn;

  TASK( Cmp_Builtin_CFunc_Def(& sym_num, (UInt *) NULL, name, c_func) );
  opn = Cmp_Operation_Add(cmp_opr.converter, src, TYPE_NONE, dest);
  if ( opn == NULL ) return Failed;
  opn->is.commutative = 0;
  opn->is.intrinsic = 0;
  opn->is.assignment = 0;
  opn->module = sym_num;
  return Success;
}

#include "structure.c"
