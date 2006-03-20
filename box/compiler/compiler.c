/*
 * Copyright (C) 2005 Matteo Franchin
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

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
#include "array.h"
#include "str.h"
#include "virtmach.h"
#include "registers.h"
#include "compiler.h"

/******************************************************************************/

/* "Collezione" di tutti gli operatori */
struct cmp_opr_struct cmp_opr;

/******************************************************************************/

/* DESCRIZIONE: Crea un nuovo operatore.
 */
Operator *Cmp_Operator_New(char *name) {
  Operator *opr;
  int i, j;

  MSG_LOCATION("Cmp_Operator_New");

  opr = (Operator *) malloc( sizeof(Operator) );
  if ( opr == NULL ) {
    MSG_ERROR("Memoria esaurita!");
    return NULL;
  }

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

/* DESCRIZIONE: Aggiunge una nuova operazione di tipo type1 opr type2
 *  all'operatore *opr. Se type1 o type2 sono uguali a TYPE_NONE si tratta di
 *  un'operazione unaria (sinistra o destra rispettivamente).
 */
Operation *Cmp_Operation_Add(
 Operator *opr, Intg type1, Intg type2, Intg typer) {
  Operation *opn;
  Intg aa, t, t1, t2;
  int is_privileged;

  MSG_LOCATION("Cmp_Operation_Add");
#if 0
  printf("Adding operation (%s, %s) to operator '%s'\n",
   Tym_Type_Names(type1), Tym_Type_Names(type2), opr->name);
#endif
  opn = (Operation *) malloc( sizeof(Operation) );
  if ( opn == NULL ) {
    MSG_ERROR("Memoria esaurita!");
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
 Intg type1, Intg type2, Intg typer, OpnInfo *oi) {

  Intg type;
  int no_check_arg1, no_check_arg2, check_rs;
  int ne1, ne2;
  Operation *opn;

  MSG_LOCATION("Cmp_Operation_Find");

#if 0
  printf("Cmp_Operation_Find: Cerco %s OP %s\n",
   Tym_Type_Names(type1), Tym_Type_Names(type2));
#endif

  no_check_arg1 = (type1 == TYPE_NONE);
  no_check_arg2 = (type2 == TYPE_NONE);
  check_rs      = (typer != TYPE_NONE);

  /* Is it a privileged operation or not? */
  if ( ! check_rs ) {
    Intg aa;
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
    register Intg t1 = opn->type1, t2 = opn->type2;
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

      if ( opn->is.commutative ) {
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
Task Cmp_Conversion(Intg type1, Intg type2, Expression *e) {
  int do_it = 0;
  Intg t1, t2;
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
  t1 = Tym_Type_Resolve_All(type1);
  t2 = Tym_Type_Resolve_Alias(type2);
  if ( t1 == t2 ) return Success;

#ifdef DEBUG_SPECIES_EXPANSION
  printf("Converto: '%s' in '%s'\n",
   Tym_Type_Names(t1), Tym_Type_Names(t2));
#endif
  /* Cerco se la conversione esiste */
  opn = Cmp_Operation_Find(cmp_opr.converter, t1, TYPE_NONE, t2, NULL);

  if ( opn == NULL ) {
    if ( ! do_it ) return Failed;
    MSG_ERROR("Non so come convertire '%s' in '%s'!",
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
Task Cmp_Conversion_Exec(Expression *e, Intg type_dest, Operation *c_opn) {
  if ( c_opn->is.intrinsic ) {
    if ( (e->categ == CAT_LREG) && (e->value.i == 0) ) {
      /* Si tratta del registro r0 (ri0, rr0, ...) */
      VM_Assemble( c_opn->asm_code, CAT_LREG, (Intg) 0 );
      e->type = type_dest;
      e->resolved = Tym_Type_Resolve_All(type_dest);

    } else {
      /* Metto l'espressione nel registro 0 */
      TASK( Cmp_Expr_To_LReg(e) );
      TASK( Cmp_Complete_Ptr_1(e) );
      VM_Assemble( c_opn->asm_code, e->categ, e->value.i );
      TASK( Cmp_Expr_Destroy(e) );
      TASK( Cmp_Expr_Reg0(e, type_dest) );
    }

    *e = *Cmp_Expr_Reg0_To_LReg(type_dest);
    if ( e == NULL ) return Failed;
    assert(e->is.release);
    return Success;

  } else {
    /* Operazioni di conversione user-defined */
    Expression new_e;

  /* mov gro2, expr_src */
    TASK( Cmp_Expr_To_Ptr(e, CAT_GREG, (Intg) 2, 0) );
  /* mov gro1, expr_dest */
    TASK( Cmp_Expr_Create(& new_e, type_dest, /* temporary = */ 1) );
    TASK( Cmp_Expr_To_Ptr(& new_e, CAT_GREG, (Intg) 1, 0) );
  /* call conv_func */
    VM_Assemble(ASM_CALL_I, CAT_IMM, c_opn->module);
    TASK( Cmp_Expr_Destroy(e) );
    *e = new_e;
    return Success;
  }
}

/******************************************************************************/

Task Cmp_Member_Intrinsic(Expression *e, Name *m) {
  switch( e->type ) {
   case TYPE_POINT:
    if ( m->length == 1 ) {
      Intg addr;

      switch( tolower(*(m->text)) ) {
       case 'x':
        TASK( Cmp_Expr_To_LReg(e) );
        TASK( Cmp_Complete_Ptr_1(e) );
        VM_Assemble(ASM_PPTRX_P, e->categ, e->value.i);
        break;
       case 'y':
        TASK( Cmp_Expr_To_LReg(e) );
        TASK( Cmp_Complete_Ptr_1(e) );
        VM_Assemble(ASM_PPTRY_P, e->categ, e->value.i);
        break;
       default:
        goto cmp_memb_intr_err;
        break;
      }
      TASK( Cmp_Expr_Destroy(e) );

      /* ATTENZIONE: devo mettere ro0 in un registro locale temporaneo */

      if ( (addr = Reg_Occupy(TYPE_OBJ)) < 1 ) return Failed;
      VM_Assemble(ASM_MOV_OO, CAT_LREG, addr, CAT_LREG, 0);

      e->resolved = e->type = TYPE_REAL;
      e->categ = CAT_PTR;
      e->is.gaddr = 0;
      e->addr = addr;
      e->value.i = 0;
      e->is.imm = e->is.ignore = 0;
      e->is.value = e->is.target = 1;
      e->is.allocd = 0;
      e->is.release = 1;
      return Success;
    }
    goto cmp_memb_intr_err;
    break;

   default:
    goto cmp_memb_intr_err;
    break;
  }
  return Failed;

cmp_memb_intr_err:
  MSG_ERROR( "'%s' non e' un membro del tipo intrinseco '%s'!",
   Name_To_Str(m), Tym_Type_Name(e->type) );
  return Failed;
}

static Expression *Cmp__Member_Of_Ptr(Expression *e, Name *nm) {
  MSG_ERROR("Non ancora implementato (Cmp__Member_Of_Ptr)!");
  return NULL;
}

/* DESCRIPTION: This is a particular case of the following function
 *  (Cmp_Member_Get). It deals with members of instance of an object.
 */
static Expression *Cmp__Member_Of_Instance(Expression *e, Name *nm) {
  Symbol *s;
  Intg t = e->resolved;

  /* Determino il simbolo associato all'espressione *e */
  /*sym_struct = Cmp_Type_Symbol(e->type);
  if ( sym_struct == NULL ) return Failed;*/

  /* Cerco *nm fra i membri del tipo t */
  s = Sym_Implicit_Find(t, nm);
  if ( s == NULL ) {
    MSG_ERROR( "'%s' non e' un membro del tipo '%s'!",
     Name_To_Str(nm), Tym_Type_Name(t) );
    return NULL;
  }

  switch ( e->categ ) {
   case CAT_GREG:
   case CAT_LREG:
    /* *e e' un registro (oppure una variabile) locale o globale! */
    e->is.gaddr = ( e->categ == CAT_GREG ) ? 1 : 0;
    e->addr = e->value.i;
    e->categ = CAT_PTR;
    e->value.i = s->value.addr;
    e->type = s->value.type;
    e->resolved = s->value.resolved;
    e->is.imm = e->is.ignore = 0;
    e->is.value = e->is.target = 1;
    e->is.allocd = 0; e->is.release = 1;
    return e;

   case CAT_PTR:
    e->value.i += s->value.addr;
    e->type = s->value.type;
    e->resolved = s->value.resolved;
    e->is.imm = e->is.ignore = 0;
    e->is.value = e->is.target = 1;
    return e;

   default:
    MSG_ERROR("Errore interno in Cmp__Member_Of_Instance!");
  }

  return NULL;
}

/* DESCRIZIONE: Restituisce l'espressione corrispondente al membro di *e,
 *  il cui nome e' specificato in *m (NULL in caso d'errore, tipo:
 *  membro non trovato, espressione invalida o qualsiasi altro).
 */
Expression *Cmp_Member_Get(Expression *e, Name *nm) {
  Intg t = e->resolved;
  TypeDesc *td;

  if ( ! e->is.value ) {
    MSG_ERROR("Richiesta del membro '%s' di un'espressione senza valore!",
     Name_To_Str(nm));
    return NULL;
  }

  /* Determino se si tratta di un oggetto intrinseco */
  if ( t < NUM_INTRINSICS ) {
    if IS_FAILED( Cmp_Member_Intrinsic(e, nm) ) return NULL;
    return e;
  }

  td = Tym_Type_Get(t);
  if ( td == NULL ) return NULL;
  switch ( td->tot ) {
   case TOT_INSTANCE: /* in C questo e' il caso: espressione.membro */
    return Cmp__Member_Of_Instance(e, nm);
   case TOT_PTR_TO:   /* in C questo e' il caso: espressione->membro */
    return Cmp__Member_Of_Ptr(e, nm);
   default:
    MSG_ERROR("Il tipo '%s' non ha membri!", Tym_Type_Name(t));
  }
  return NULL;
}

/******************************************************************************/

/* DESCRIZIONE: Cerca di dare significato all'espressione *e1 opr *e2 e la
 *  "realizza", cioe' restituisce l'espressione risultante, compilando
 *  il codice eventualmente necessario. Si preoccupa anche di "liberare"
 *  le espressioni e1 ed e2.
 */
Expression *Cmp_Operator_Exec(Operator *opr, Expression *e1, Expression *e2) {
  int e1valued = 1, e2valued = 1;
  Intg e1type, e2type, num_arg = 2;
  Operation *opn;
  OpnInfo oi;

  MSG_LOCATION("Cmp_Operator_Exec");

#if 0
  printf("Cmp_Operator_Exec: Cerco %s OP %s\n",
   Tym_Type_Names(e1->type), Tym_Type_Names(e2->type));
#endif

  if ( e1 == NULL ) {e1type = TYPE_NONE; num_arg = 1;}
    else {e1type = e1->resolved; e1valued = e1->is.value;}
  if ( e2 == NULL ) {e2type = TYPE_NONE; num_arg = 1;}
    else {e2type = e2->resolved; e2valued = e2->is.value;}

  if ( !e1valued || !e2valued ) {
    if ( !e1valued ) {
      MSG_ERROR("L'espressione alla sinistra di '%s' deve avere valore!",
       opr->name);
    } else {
      MSG_ERROR("L'espressione alla destra di '%s' deve avere valore!",
       opr->name);
    }
    return NULL;
  }

  /* Prima cerco se esiste un'operazione fra i tipi di e1 e e2 */
  opn = Cmp_Operation_Find(opr, e1type, e2type, TYPE_NONE, & oi);
  if ( opn == NULL ) goto Exec_Opn_Error;

  /* Ora eseguo le espansioni, se necessario */
  if ( oi.expand1 ) {
    if IS_FAILED( Cmp_Expr_Expand(oi.exp_type1, e1) ) return NULL;
  }
  if ( oi.expand2 ) {
    if IS_FAILED( Cmp_Expr_Expand(oi.exp_type2, e2) ) return NULL;
  }

  /*
    manca la valutazione della commutativita'!
   */


  return Cmp_Operation_Exec(opn, e1, e2);

Exec_Opn_Error:
  if ( num_arg == 1 ) {
    if ( e1type != TYPE_NONE ) {
        MSG_ERROR("%s %s <-- Operazione non definita!",
         opr->name, Tym_Type_Name(e1->type) );
    } else {
        MSG_ERROR("%s %s <-- Operazione non definita!",
         Tym_Type_Name(e2->type), opr->name );
    }
    return NULL;

  } else {
    MSG_ERROR("%s %s %s <-- Operazione non definita!",
     Tym_Type_Names(e1->type), opr->name, Tym_Type_Names(e2->type) );
    return NULL;
  }
}



/******************************************************************************/

/* DESCRIZIONE: Questa funzione gestisce la generazione del codice
 *  corrispondente ad un gran numero di operazioni intrinseche.
 * NOTA: Viene chiamata da Cmp_Operation_Exec.
 */
static Expression *Opn_Exec_Intrinsic(
 Operation *opn, Expression *e1, Expression *e2) {

  struct {unsigned int unary :1, right : 1, strange :1, immediate :1;} opn_is;
  Intg rs_resolved = Tym_Type_Resolve_All(opn->type_rs);

  MSG_LOCATION("Opn_Exec_Intrinsic");

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
    MSG_ERROR("Errore interno: operazione priva di argomenti!");
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
      MSG_ERROR("Impossibile modificare il valore dell'espressione!");
      return NULL;
    }

    if ( opn_is.unary ) {
      /* Ora compilo l'operazione */
      if ( opn_is.right ) {
        Expression old_e2 = *e2;
        /*e2value = e2->value.i, e2categ = e2->categ;*/
        if IS_FAILED( Cmp_Expr_Force_To_LReg(e2) ) return NULL;
        if IS_FAILED( Cmp_Complete_Ptr_1(& old_e2) ) return NULL;
        VM_Assemble(opn->asm_code, old_e2.categ, old_e2.value.i);
        return e2;
      } else {
        if IS_FAILED( Cmp_Complete_Ptr_1(e1) ) return NULL;
        VM_Assemble(opn->asm_code, e1->categ, e1->value.i);
        return e1;
      }
    } else {
      /* Se necessario, converto e2 in registro locale. */
      if IS_FAILED( Cmp_Expr_To_LReg(e2) ) return NULL;

      /* Ora compilo l'operazione */
      if IS_FAILED( Cmp_Complete_Ptr_2(e1, e2) ) return NULL;
      VM_Assemble(opn->asm_code,
       e1->categ, e1->value.i, e2->categ, e2->value.i);

      /* Ora libero il registro che non contiene il risultato! */
      Cmp_Expr_Destroy(e2);
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
        MSG_ERROR("Non ancora implementato!");
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
          VM_Assemble(opn->asm_code,
           e1->categ, e1->value.i, e2->categ, e2->value.i);
          Cmp_Expr_Destroy(e1);
          Cmp_Expr_Destroy(e2);
          return Cmp_Expr_Reg0_To_LReg(opn->type_rs);

        } else {
          if ( eq.er_e2 ) {
            Expression *tmp; tmp = e1; e1 = e2; e2 = tmp;
            goto er_equal_e1;
          }

          if ( eq.er_e1 ) { /* caso 2: un argomento e' dello stesso tipo
                                del risultato */
er_equal_e1:
            if IS_FAILED( Cmp_Expr_Force_To_LReg(e1) ) return NULL;
            if IS_FAILED( Cmp_Expr_To_X(e2, CAT_LREG, 0, 1) ) return NULL;
            if IS_FAILED( Cmp_Complete_Ptr_1(e1) ) return NULL;
            VM_Assemble(opn->asm_code, e1->categ, e1->value.i);
            return e1;

          } else { /* caso 3: tipi tutti diversi */
            MSG_ERROR("Non ancora implementato!");
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
        VM_Assemble(opn->asm_code, e1->categ, e1->value.i);
        return e1;

      } else {
        result_in.e2 = (e2->resolved == rs_resolved)
         && (e2->categ == CAT_LREG) && (! e2->is.target);

        /* Se l'operazione e' commutativa, posso usare e2 per contenere
        * il risultato dell'operazione!
        */
        if ( result_in.e2 && !result_in.e1 && opn->is.commutative ) {
          register Expression *tmp = e2;
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
        VM_Assemble(opn->asm_code,
         e1->categ, e1->value.i, e2->categ, e2->value.i);

        /* Ora libero il registro che non contiene il risultato! */
        Cmp_Expr_Destroy(e2);
        return e1;
      }
    }
  }
}

/* DESCRIZIONE:
 *  NULL, e --> operazione unaria destra come e++
 *  e, NULL --> operazione unaria sinistra come ++e
 */
Expression *Cmp_Operation_Exec(
 Operation *opn, Expression *e1, Expression *e2) {

  int strange_case, result_in_e1;

  MSG_LOCATION("Cmp_Operation_Exec");

  if ( opn->is.intrinsic )
    return Opn_Exec_Intrinsic(opn, e1, e2);
  else {
    MSG_ERROR("L'overloading degli operatori e' per ora impossibile!");
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
                        register Expression *tmp = e2;
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
                VM_Assemble(opn->asm_code,
                 e1->categ, e1->value.i, e2->categ, e2->value.i);

                /* Ora libero il registro che non contiene il risultato! */
                Cmp_Expr_Destroy(e2);
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

/* Prints the details about the specified expression *e.
 */
void Cmp_Expr_Print(FILE *out, Expression *e) {
  char buffer[128], *value = buffer;

  if ( ! e->is.typed ) {
    fprintf(out, "Name(name=\"%s\")\n", e->value.nm.text);
    return;
  }

  if ( e->categ == CAT_IMM ) {
    switch(e->resolved) {
    case TYPE_CHAR:
      sprintf(buffer, "INT("SChar")", (Char) e->value.i);
      break;
    case TYPE_INTG:
      sprintf(buffer, "INT("SIntg")", e->value.i);
      break;
    case TYPE_REAL:
      sprintf(buffer, "REAL("SReal")", e->value.r);
      break;
    default:
      value = "UNKNOWN";
      break;
    }

  } else {
    sprintf(buffer, "INT("SIntg")", e->value.i);
  }

  fprintf(out,
    "Expression(type="SIntg"=\"%s\", resolved="SIntg"=\"%s\", "
    "categ=%d=\"%s\", %s, imm=%c, value=%c, typed=%c, ignore=%c, target=%c,"
    "gaddr=%c, allocd=%c, release=%c)\n",
    e->type, Tym_Type_Names(e->type),
    e->resolved, Tym_Type_Names(e->resolved),
    e->categ,
    (e->categ >= 0) && (e->categ < 4) ? asm_arg_str[e->categ] : "ERROR!",
    value,
    e->is.imm ? '1' : '0',
    e->is.value ? '1' : '0',
    e->is.typed ? '1' : '0',
    e->is.ignore ? '1' : '0',
    e->is.target ? '1' : '0',
    e->is.gaddr ? '1' : '0',
    e->is.allocd ? '1' : '0',
    e->is.release ? '1' : '0'
  );
}

/* Create a new empty container.
 * NOTE: At the end this should substitute Cmp_Expr_LReg and Cmp_Expr_Create.
 */
Task Cmp_Expr_Container_New(Expression *e, Intg type, Container *c) {
  int intrinsic;
  Intg type_of_register, resolved;

  e->is.typed = 1;
  e->is.value = 1;
  e->is.ignore = 0;
  e->is.allocd = 0;
  e->type = type;
  e->resolved = resolved = Tym_Type_Resolve_All(type);
  intrinsic = (resolved < NUM_INTRINSICS);
  type_of_register = (intrinsic) ? resolved : TYPE_OBJ;

  e->is.imm = 0;
  e->is.target = 0;
  e->is.release = 0;

  switch( c->type_of_container ) {
  case 0:
    e->is.imm = 1;
    e->categ = CAT_IMM;
    return Success;
    break;

  case 1:
    e->categ = CAT_LREG;
    if ( c->which_one < 0 ) {
      /* Automatically choses the local register */
      if ( (e->value.i = Reg_Occupy(type_of_register)) < 1 ) return Failed;
      e->is.release = 1;
      return Success;

    } else {
      /* The user wants a particolar register to be chosen */
      e->value.i = c->which_one;
      return Success;
    }
    break;

  case 2:
    e->categ = CAT_LREG;
    e->is.target = 1;
    if ( c->which_one < 0 ) {
      /* Automatically choses the local variables */
      if ( (e->value.i = -Var_Occupy(type_of_register, cmp_box_level)) >= 0 ) return Failed;
      return Success;

    } else {
      /* The user wants a particolar variable to be chosen */
      e->value.i = c->which_one;
      return Success;
    }
    break;

  case 3:
    e->categ = CAT_GREG;
    e->value.i = c->which_one;
    return Success;
    break;

  case 4:
    e->categ = CAT_GREG;
    e->value.i = -(c->which_one);
    return Success;
    break;

  default:
    MSG_ERROR("Internal error: wrong type of container!");
  }
  return Failed;
}

/* DESCRIZIONE: Inizializzo la struttura expr in modo che contenga un
 *  registro locale libero. Se zero == 0, tale registro viene occupato,
 *  fino a quando non sara' "liberato" con Cmp_LReg_Free.
 *  Se zero == 1, viene utilizzato il registro numero zero (ad esempio:
 *  ri0, rr0, ...), il quale viene utilizzato dal compilatore
 *  (per convenzione) per il transito temporaneo dei dati.

 -----------------------> OBSLOLETE <------------------------
 */
Task Cmp_Expr_LReg(Expression *e, Intg type, int zero) {
  MSG_LOCATION("Cmp_Expr_LReg");

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

  if (type < CMP_PRIVILEGED) {
    if ( zero ) { e->value.i = 0; return Success; }
    if ( (e->value.i = Reg_Occupy(type)) < 1 ) return Failed;
    return Success;

  } else {
    TypeDesc *td;

    td = Tym_Type_Get(type);
    if ( td == NULL ) return Failed;

    {
      register Intg s = td->size;
      if ( s > 0 ) {
        e->is.allocd = 1;
        if ( zero ) { e->value.i = 0; }
         else { if ( (e->value.i = Reg_Occupy(TYPE_OBJ)) < 1 ) return Failed; }

        VM_Assemble(ASM_MALLOC_I, CAT_IMM, s);
        VM_Assemble(ASM_MOV_OO, e->categ, e->value.i, CAT_LREG, 0);
      }
    }
    return Success;
  }
}

static Intg asm_lea[NUM_INTRINSICS] = {
  ASM_LEA_C, ASM_LEA_I, ASM_LEA_R, ASM_LEA_P
};

static Intg asm_mov[NUM_INTRINSICS] = {
  ASM_MOV_CC, ASM_MOV_II, ASM_MOV_RR, ASM_MOV_PP
};

/* DESCRIPTION: This function generates the assembly code which
 *  puts the expression expr into a precise register.
 * NOTE: categ can be CAT_LREG or CAT_GREG.
 */
Task Cmp_Expr_To_X(Expression *expr, AsmArg categ, Intg reg, int and_free) {
#define EXIT_FUNCTION if ( !and_free ) return Success; return Cmp_Expr_Destroy(expr)
  int is_integer;
  MSG_LOCATION("Cmp_Expr_To_X");

  assert(expr != NULL);
  assert( (expr->is.typed) && (expr->is.value) );
  assert( (categ == CAT_LREG) || (categ == CAT_GREG) );

  if ( expr->resolved < TYPE_OBJ ) {
    /********** THIS PART OF THE FUNCTIONS HANDLES INTRINSIC TYPES ***********/
    is_integer = (expr->resolved==TYPE_CHAR) || (expr->resolved==TYPE_INTG);
    if ( (expr->categ == CAT_IMM) && (! is_integer) ) {
      switch ( expr->resolved ) {
       case TYPE_REAL:
        VM_Assemble(ASM_MOV_Rimm, categ, reg, CAT_IMM, expr->value.r); break;
       case TYPE_POINT:
        VM_Assemble(ASM_MOV_Pimm, categ, reg, CAT_IMM, expr->value.p); break;
       default:
        MSG_ERROR( "Errore interno: non sono ammessi valori immediati"
         " per il tipo '%s'.", Tym_Type_Name(expr->type) );
        return Failed;
      }
      EXIT_FUNCTION;

    } else {
      register Intg t = expr->resolved;
      if ( expr->categ == CAT_PTR ) {
        /* L'espressione e' un puntatore, allora devo settare il puntatore
        * di riferimento, in modo da realizzare una cosa simile a:
        *   mov ro0, ro1         <-- setto il puntatore di riferimento (ro0)
        *   mov rr1, real[ro0+8] <-- prelevo il valore
        */
        Intg addr_categ = ( expr->is.gaddr ) ? CAT_GREG : CAT_LREG;
        VM_Assemble( ASM_MOV_OO, CAT_LREG, (Intg) 0,
         addr_categ, expr->addr );
      }

      if ( (t < 0) || (t >= NUM_INTRINSICS) ) {
        MSG_ERROR("Errore interno in Cmp_Expr_To_X");
        return Failed;
      }

      VM_Assemble(asm_mov[t], categ, reg, expr->categ, expr->value.i);
      EXIT_FUNCTION;
    }

  } else {/***** THIS PART OF THE FUNCTIONS HANDLES NON-INTRINSIC TYPES ******/
    assert( expr->categ != CAT_IMM );

    if ( expr->categ == CAT_PTR ) {
      register Intg addr_categ = ( expr->is.gaddr ) ? CAT_GREG : CAT_LREG;
      VM_Assemble( ASM_MOV_OO, CAT_LREG, (Intg) 0, addr_categ, expr->addr );
      VM_Assemble( ASM_LEA_OO, categ, reg, CAT_PTR, expr->value.i );
      EXIT_FUNCTION;

    } else { /* expr->categ == CAT_LREG, CAT_GREG */
      VM_Assemble(ASM_MOV_OO, categ, reg, expr->categ, expr->value.i);
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
Task Cmp__Expr_To_LReg(Expression *expr, int force) {
  Expression lreg;
  MSG_LOCATION("Cmp__Expr_To_LReg");

  /* Se l'espressione e' gia' un registro locale, allora esco! */
  if ( expr->categ == CAT_LREG )
    if ( expr->value.reg > 0 ) return Success;

  if ( ! force ) {
    register int is_integer =
     (expr->resolved == TYPE_CHAR) || (expr->resolved == TYPE_INTG);
    if ( is_integer || (expr->categ != CAT_IMM) ) return Success;
  }

  TASK( Cmp_Expr_Create(& lreg, expr->type, 1 /* temporary */) );
  TASK( Cmp_Expr_To_X(expr, lreg.categ, lreg.value.reg, 1) );
  *expr = lreg;
  return Success;
}

/* This function generates the assembly code which
 * puts the address of the expression expr into a precise register.
 * NOTE: categ can be CAT_LREG or CAT_GREG.
 */
Task Cmp_Expr_To_Ptr(Expression *expr, AsmArg categ, Intg reg, int and_free) {
  MSG_LOCATION("Cmp_Expr_To_Ptr");

  assert(expr != NULL);
  assert( (expr->is.typed) && (expr->is.value) );
  assert( (categ == CAT_LREG) || (categ == CAT_GREG) );

  if ( expr->categ == CAT_IMM ) {
    register Intg t = expr->resolved;

    TASK( Cmp_Expr_To_X(expr, CAT_LREG, (Intg) 0, and_free) );
    assert( (t >= 0) && (t < NUM_INTRINSICS) );
    VM_Assemble(asm_lea[t], CAT_LREG, (Intg) 0);
    VM_Assemble(ASM_MOV_OO, categ, reg, CAT_LREG, (Intg) 0);
    return Success;

  } else {
    register Intg t = expr->resolved;

    assert(t >= 0);
    if ( t < NUM_INTRINSICS ) {
      if ( expr->categ == CAT_PTR ) {
        /* L'espressione e' un puntatore, allora devo settare il puntatore
        * di riferimento, in modo da realizzare una cosa simile a:
        *   mov ro0, ro1 <-- setto il puntatore di riferimento (ro0)
        *   lea p[ro0+8] <-- prelevo il valore
        *   mov ..., ro0 <-- metto l'indirizzo dove richiesto
        */
        register Intg addr_categ = ( expr->is.gaddr ) ? CAT_GREG : CAT_LREG;
        VM_Assemble( ASM_MOV_OO, CAT_LREG, (Intg) 0, addr_categ, expr->addr );
      }

      VM_Assemble(asm_lea[t], expr->categ, expr->value.i);
      VM_Assemble(ASM_MOV_OO, categ, reg, CAT_LREG, (Intg) 0);
      if ( !and_free ) return Success;
      return Cmp_Expr_Destroy(expr);

    } else {
      if ( expr->categ == CAT_PTR ) {
        register Intg addr_categ = ( expr->is.gaddr ) ? CAT_GREG : CAT_LREG;
        VM_Assemble( ASM_MOV_OO, CAT_LREG, (Intg) 0, addr_categ, expr->addr );
        VM_Assemble( ASM_LEA_OO, categ, reg, CAT_PTR, expr->value.i );
        if ( !and_free ) return Success;
        return Cmp_Expr_Destroy(expr);

      } else { /* expr->categ == CAT_LREG, CAT_GREG */
        VM_Assemble(ASM_MOV_OO, categ, reg, expr->categ, expr->value.i);
      }
    }
  }

  if ( !and_free ) return Success;
  return Cmp_Expr_Destroy(expr);
}

/* DESCRIPTION: This function creates (in *e) a new expression of type 'type'.
 *  If temporary == 1, the new expression is a non-target (an intermediate
 *  expression, which is automatically generated by the compiler and cannot
 *  store any definitive value), if temporary == 0 the expression corresponds
 *  to a particular variable.
 */
Task Cmp_Expr_Create(Expression *e, Intg type, int temporary) {
  register int intrinsic;
  Intg type_of_register, resolved;
  TypeDesc *td;

  assert( type >= 0 );

#ifdef DEBUG_CONTAINER_HANDLING
  printf( "Creating a new %s of type %s\n",
   (temporary ? "temporary expression" : "variable"),
   Tym_Type_Name(type) );
#endif

  e->is.typed = 1;
  e->is.value = 0;
  e->is.imm = 0;
  e->is.ignore = 0;
  e->is.target = 0;
  e->is.release = 0;
  e->is.allocd = 0;
  e->categ = CAT_LREG;
  e->type = type;
  e->resolved = resolved = Tym_Type_Resolve_All(type);
  intrinsic = (resolved < NUM_INTRINSICS);
  type_of_register = (intrinsic) ? resolved : TYPE_OBJ;

  td = Tym_Type_Get(type);
  if ( td == NULL ) return Failed;
  if ( td->size == 0 ) return Success;

  e->is.value = 1;
  if ( temporary ) {
    e->is.release = 1;
    if ( (e->value.i = Reg_Occupy(type_of_register)) < 1 ) return Failed;

  } else {
    e->is.target = 1;
    if ( (e->value.i = -Var_Occupy(type_of_register, cmp_box_level))
         >= 0 ) return Failed;
  }

  /* If the object is of an intrinsic type, we can return now! */
  if ( intrinsic ) return Success;

  /* If the object is of a user defined type, we must allocate it! */
  VM_Assemble(ASM_MALLOC_I, CAT_IMM, td->size);
  VM_Assemble(ASM_MOV_OO, e->categ, e->value.i, CAT_LREG, 0);
  e->is.allocd = 1;
  return Success;
}

/* DESCRIPTION:
 */
Task Cmp_Expr_Destroy(Expression *e) {
  MSG_LOCATION("Cmp_Expr_Destroy");

#ifdef DEBUG_CONTAINER_HANDLING
  printf( "Cmp_Expr_Destroy: destroying " );
  Cmp_Expr_Print(stdout, e);
#endif

  if ( ! e->is.typed ) {
    Name_Free(& e->value.nm);
    return Success;

  } else {
    register int intrinsic;
    Intg type_of_register, resolved;;

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

    if ( (! intrinsic) && (e->is.allocd) && (! e->is.target) ) {
      // ??? dovrei usare Cmp_Complete_Ptr_1?
      VM_Assemble(ASM_MFREE_O, e->categ, e->value.i);
    }
    return Success;
  }
}

/* DESCRIPTION:
 */
Task Cmp_Expr_Copy(Expression *e_dest, Expression *e_src) {
  MSG_WARNING("Cmp_Expr_Copy non ancora implementata!");
  return Cmp_Expr_Move(e_dest, e_src);
}

/* This function can moves intrinsic and non-intrinsic objects.
 * What is moved is the data contained inside the object.
 * Memory is not allocated nor freed!
 */
Task Cmp_Expr_Move(Expression *e_dest, Expression *e_src) {
  register Intg t, c;

  assert( (e_dest != NULL) && (e_src != NULL) );
  assert( e_dest->resolved == e_src->resolved );
  t = e_dest->resolved;
  c = e_dest->categ;
  assert( t >= 0 );
  assert((e_src->is.typed) && (e_src->is.value) && (c != CAT_IMM));

  /* Qui devo controllare se il tipo ammette un mover user-defined! */

  if ( t < NUM_INTRINSICS ) {
    /* Sposto una quantita' intrinseca */
    register int is_integer;

    is_integer = (e_src->resolved == TYPE_CHAR)
              || (e_src->resolved == TYPE_INTG);
    if ( (e_src->categ == CAT_IMM) && (! is_integer) ) {
      TASK( Cmp_Complete_Ptr_1(e_dest) );
      switch ( t ) {
       case TYPE_REAL:
        VM_Assemble(ASM_MOV_Rimm,
         c, e_dest->value.i, CAT_IMM, e_src->value.r);
        break;
       case TYPE_POINT:
        VM_Assemble(ASM_MOV_Pimm,
         c, e_dest->value.i, CAT_IMM, e_src->value.p);
        break;
       default:
        MSG_ERROR("Internal error in Cmp_Expr_Move");
        return Failed;
      }
      return Cmp_Expr_Destroy(e_src);

    } else {
      TASK( Cmp_Complete_Ptr_2(e_dest, e_src) );
      VM_Assemble( asm_mov[t],
       e_dest->categ, e_dest->value.i, e_src->categ, e_src->value.i );
      return Cmp_Expr_Destroy(e_src);
    }

  } else {
    /* Sposto un oggetto user-defined */
    MSG_ERROR("Internal error in Cmp_Expr_Move: still not implemented!");
    fprintf(stderr, "e_src = ");  Cmp_Expr_Print(stderr, e_src);
    fprintf(stderr, "e_dest = "); Cmp_Expr_Print(stderr, e_dest);
    return Failed;
  }
}

/* DESCRIZIONE: Prende il valore dal registro zero per metterlo in un registro
 *  locale temporaneo (che viene occupato).
 */
Expression *Cmp_Expr_Reg0_To_LReg(Intg t) {
  static Expression lreg;

  /* Metto in lreg un nuovo registro locale */
  if IS_FAILED( Cmp_Expr_Container_New(& lreg, t, CONTAINER_LREG_AUTO) )
    return NULL;

  switch ( t ) {
   case TYPE_NONE:
    MSG_ERROR("Errore interno in Cmp_Expr_Reg0_To_LReg");
    return NULL; break;
   case TYPE_CHAR:
    VM_Assemble(ASM_MOV_CC, lreg.categ, lreg.value.i, CAT_LREG, 0);
    return &lreg; break;
   case TYPE_INTG:
    VM_Assemble(ASM_MOV_II, lreg.categ, lreg.value.i, CAT_LREG, 0);
    return &lreg; break;
   case TYPE_REAL:
    VM_Assemble(ASM_MOV_RR, lreg.categ, lreg.value.i, CAT_LREG, 0);
    return &lreg; break;
   case TYPE_POINT:
    VM_Assemble(ASM_MOV_PP, lreg.categ, lreg.value.i, CAT_LREG, 0);
    return &lreg; break;
   default:
    VM_Assemble(ASM_MOV_OO, lreg.categ, lreg.value.i, CAT_LREG, 0);
    return &lreg; break;
  }
  return NULL;
}

/* DESCRIZIONE: Questa funzione controlla che una espressione di tipo oggetto
 *  sia di categoria "registro locale" (CAT_LREG) o "registro globale"
 *  (CAT_GREG). In caso contrario (cioe' per categoria "puntatore a "
 *  (CAT_PTR) ) provvede a mettere l'espressione in un registro locale.
 * NOTA: Tutte queste operazioni sono eseguite su *e.
 */
Task Cmp_Expr_O_To_OReg(Expression *e) {
  assert ( e->type >= NUM_INTRINSICS );

  switch ( e->categ ) {
   case CAT_GREG: case CAT_LREG:
    return Success;
   case CAT_PTR: {
      Intg ro_num = e->addr;
      Intg ro_categ = ( e->is.gaddr ) ? CAT_GREG : CAT_LREG;

      if ( ro_num > 0 ) {
        /* Il puntatore di riferimento e' contenuto in un registro:
          * quindi posso riutilizzare tale registro nel modo seguente
          * (se il registro in questione e' ro4):
          *  mov ro0, ro4
          *  mov ro4, Obj[ro0+64]
          */
        VM_Assemble( ASM_MOV_OO, CAT_LREG, (Intg) 0, ro_categ, ro_num );
        VM_Assemble( ASM_MOV_OO, ro_categ, ro_num, CAT_PTR, e->value.i );
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
        Intg new_reg;

        VM_Assemble( ASM_MOV_OO, CAT_LREG, (Intg) 0, ro_categ, ro_num );
        new_reg = Reg_Occupy(TYPE_OBJ);
        if ( new_reg < 1 ) return Failed;
        VM_Assemble( ASM_MOV_OO, CAT_LREG, new_reg, CAT_PTR, e->value.i );
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

/* DESCRIZIONE: Quando faccio riferimento ad un membro di una struttura
 *  il compilatore genera del codice tipo:
 *   mov ro0, ro5        \___ accede all'intero descritto da (ro5, 8)
 *   mov ri1, i[ro0+8]   /
 *  Quindi bisogna caricare in ro0 il registro che fa da puntatore base.
 *  Questa funzione fa proprio questo.
 */
Task Cmp_Complete_Ptr_2(Expression *e1, Expression *e2) {
  Intg addr_categ;

  switch ( (e1->categ == CAT_PTR) | (e2->categ == CAT_PTR) << 1) {
   case 1:
    addr_categ = ( e1->is.gaddr ) ? CAT_GREG : CAT_LREG;
    VM_Assemble(ASM_MOV_OO, CAT_LREG, (Intg) 0, addr_categ, e1->addr);
    return Success;
   case 2:
    addr_categ = ( e2->is.gaddr ) ? CAT_GREG : CAT_LREG;
    VM_Assemble(ASM_MOV_OO, CAT_LREG, (Intg) 0, addr_categ, e2->addr);
    return Success;
   case 3:
    if IS_FAILED( Cmp_Expr_Force_To_LReg(e2) ) return Failed;
    addr_categ = ( e1->is.gaddr ) ? CAT_GREG : CAT_LREG;
    VM_Assemble(ASM_MOV_OO, CAT_LREG, (Intg) 0, addr_categ, e1->addr);
    return Success;
   default:
    return Success;
  }
}

/* DESCRIZIONE: Analoga alla precedente, ma questa funzione deve essere chiamata
 *  nel caso di istruzioni con 1 solo argomento.
 */
Task Cmp_Complete_Ptr_1(Expression *e) {
  if ( e->categ == CAT_PTR ) {
    register Intg addr_categ = ( e->is.gaddr ) ? CAT_GREG : CAT_LREG;
    VM_Assemble(ASM_MOV_OO, CAT_LREG, (Intg) 0, addr_categ, e->addr);
  }
  return Success;
}

/******************************************************************************
 * Le procedure che seguono servono a inizializzare/resettare il compilatore. *
 ******************************************************************************/

/* Destinazione attuale del codice compilato dal compilatore */
AsmOut *Cmp_Curr_Output;

/* Con queste macro abbrevio la definizione delle operazioni */
#define NEW_OPERATOR(o, str) \
  status |= ( (cmp_opr.o = Cmp_Operator_New(str)) == NULL );

#define ADD_OPERATION(o, a1, a2, rs, asm, attr_c, attr_i, attr_a) \
  opn = Cmp_Operation_Add(cmp_opr.o, a1, a2, rs); \
  status |= ( opn == NULL ); \
  opn->is.commutative = attr_c; \
  opn->is.intrinsic = attr_i; \
  opn->is.assignment = attr_a; \
  opn->is.link = 0; \
  opn->asm_code = asm;

/* DESCRIZIONE: Esegue l'inizializzazione del compilatore:
 *  1) Crea gli operatori fondamentali e le operazioni ad essi associate
 *     (quelle fra i tipi intrinseci).
 *  2) Setta l'output di compilazione.
 */
Task Cmp_Define_Builtins()
{
  Operation *opn;
  int status;
  static UInt typl_nreg[NUM_TYPES] = REG_OCC_TYP_SIZE;
  static UInt typl_nvar[NUM_TYPES] = VAR_OCC_TYP_SIZE;
  Name intrinsic_type[6] = {
    {4, "CHAR"},
    {3, "INT"},
    {4, "REAL"},
    {5, "POINT"},
    {6, "OBJECT"},
    {4, "VOID"}
  };

  MSG_LOCATION("Cmp_Define_Builtins");

  /* Inizializzo le liste di occupazione di registri e variabili */
  TASK( Reg_Init(typl_nreg) );
  TASK( Var_Init(typl_nvar) );

  /* Setto l'output della compilazione */
  Cmp_Curr_Output = VM_Asm_Out_New(-1);
  TASK( VM_Asm_Out_Set(Cmp_Curr_Output) );
  TASK( Cmp_Box_Instance_Begin( NULL ) );

  /* Definisco i tipi intrinseci */
  if (
     ( Tym_Intrinsic_New( intrinsic_type+0, sizeof(Char)  ) != TYPE_CHAR  )
  || ( Tym_Intrinsic_New( intrinsic_type+1, sizeof(Intg)  ) != TYPE_INTG  )
  || ( Tym_Intrinsic_New( intrinsic_type+2, sizeof(Real)  ) != TYPE_REAL  )
  || ( Tym_Intrinsic_New( intrinsic_type+3, sizeof(Point) ) != TYPE_POINT )
  || ( Tym_Intrinsic_New( intrinsic_type+4, 0 ) != TYPE_OBJ )
  || ( Tym_Intrinsic_New( intrinsic_type+5, 0 ) != TYPE_VOID )
  ) return Failed;

  /* Inizializzo il segmento-dati */
  TASK( Cmp_Data_Init() );

  /* Inizializzo il segmento che contiene gli oggetti con valore immediato */
  TASK( Cmp_Imm_Init() );

  /* Creo gli operatori */
  status = 0; /* Se qualcosa va male trovero' status = 1, alla fine! */

  /* Operatore usato per la conversione fra tipi diversi */
  NEW_OPERATOR(converter, "of conversion");
  /* Operatori di assegnazione */
  NEW_OPERATOR(assign,"=");
  NEW_OPERATOR(aplus, "+=");
  NEW_OPERATOR(aminus,"-=");
  NEW_OPERATOR(atimes,"*=");
  NEW_OPERATOR(adiv,  "/=");
  NEW_OPERATOR(arem,  "%=");
  NEW_OPERATOR(aband, "&=");
  NEW_OPERATOR(abxor, "^=");
  NEW_OPERATOR(abor,  "|=");
  NEW_OPERATOR(ashl,  "<<=");
  NEW_OPERATOR(ashr,  ">>=");
  /* Operatori di incremento/decremento */
  NEW_OPERATOR(inc, "++");
  NEW_OPERATOR(dec, "--");
  /* Operatori aritmetici convenzionali */
  NEW_OPERATOR(plus, "+");
  NEW_OPERATOR(minus,"-");
  NEW_OPERATOR(times,"*");
  NEW_OPERATOR(div,  "/");
  NEW_OPERATOR(rem,  "%");
  /* Operatori bit-bit */
  NEW_OPERATOR(bor,  "|");
  NEW_OPERATOR(bxor, "^");
  NEW_OPERATOR(band, "&");
  NEW_OPERATOR(bnot, "~");
  /* Operatori di shift */
  NEW_OPERATOR(shl, "<<");
  NEW_OPERATOR(shr, ">>");
  /* Operatori di confronto */
  NEW_OPERATOR(eq, "==");
  NEW_OPERATOR(ne, "!=");
  NEW_OPERATOR(gt, ">");
  NEW_OPERATOR(ge, ">=");
  NEW_OPERATOR(lt, "<");
  NEW_OPERATOR(le, "<=");
  /* Operatori logici */
  NEW_OPERATOR(lnot, "!");
  NEW_OPERATOR(land, "&&");
  NEW_OPERATOR(lor,  "||");

  /* Esco con errore se qualcuna delle precedenti creazioni e' fallita! */
  if ( status == 1 ) return Failed;

  /* T E M P O R A N E O ! ! ! */
  TASK( Lib_Define_All() );

  status = 0; /* Se qualcosa va male trovero' status = 1, alla fine! */
  /* § OPERATORE DI ASSEGNAZIONE */
  /* §§ ASSEGNAZIONE DI PUNTI */
  ADD_OPERATION(assign,TYPE_POINT, type_Point, TYPE_POINT, ASM_MOV_PP, 0, 1, 1);
  /* §§ ASSEGNAZIONE DI REALI */
  ADD_OPERATION(assign, TYPE_REAL,type_RealNum,TYPE_REAL, ASM_MOV_RR, 0, 1, 1);
  /* §§ ASSEGNAZIONE DI INTERI */
  ADD_OPERATION(assign, TYPE_INTG,type_IntgNum,TYPE_INTG, ASM_MOV_II, 0, 1, 1);
  /* §§ ASSEGNAZIONE DI CARATTERI */
  ADD_OPERATION(assign, TYPE_CHAR, TYPE_CHAR,  TYPE_CHAR, ASM_MOV_CC, 0, 1, 1);

  /* § OPPOSTO BIT-PER-BIT DI UN INTERO */
  ADD_OPERATION(bnot, TYPE_INTG, TYPE_NONE,  TYPE_INTG, ASM_BNOT_I, 0, 1, 0);
  /* § OPERATORE DI AND BIT-PER-BIT FRA INTERI */
  ADD_OPERATION(band, TYPE_INTG, TYPE_INTG, TYPE_INTG, ASM_BAND_II, 1, 1, 0);
  /* § OPERATORE DI XOR BIT-PER-BIT FRA INTERI */
  ADD_OPERATION(bxor, TYPE_INTG, TYPE_INTG, TYPE_INTG, ASM_BXOR_II, 1, 1, 0);
  /* § OPERATORE DI OR BIT-PER-BIT FRA INTERI */
  ADD_OPERATION(bor, TYPE_INTG, TYPE_INTG, TYPE_INTG, ASM_BOR_II, 1, 1, 0);
  /* § OPERATORE DI SHIFT-LEFT */
  ADD_OPERATION(shl, TYPE_INTG, TYPE_INTG,  TYPE_INTG,  ASM_SHL_II, 0, 1, 0);
  /* § OPERATORE DI SHIFT-RIGHT */
  ADD_OPERATION(shr, TYPE_INTG, TYPE_INTG,  TYPE_INTG,  ASM_SHR_II, 0, 1, 0);
  /* § OPERATORE DI INCREMENTO */
  /* §§ INCREMENTO(SINISTRO) DI UN INTERO */
  ADD_OPERATION(inc, TYPE_INTG, TYPE_NONE,  TYPE_INTG,  ASM_INC_I, 0, 1, 1);
  /* §§ INCREMENTO(DESTRO) DI UN INTERO */
  ADD_OPERATION(inc, TYPE_NONE, TYPE_INTG,  TYPE_INTG,  ASM_INC_I, 0, 1, 1);
  /* §§ INCREMENTO(SINISTRO) DI UN REALE */
  ADD_OPERATION(inc, TYPE_REAL, TYPE_NONE,  TYPE_REAL,  ASM_INC_R, 0, 1, 1);
  /* §§ INCREMENTO(DESTRO) DI UN REALE */
  ADD_OPERATION(inc, TYPE_NONE, TYPE_REAL,  TYPE_REAL,  ASM_INC_R, 0, 1, 1);
  /* § OPERATORE DI DECREMENTO */
  /* §§ DECREMENTO(SINISTRO) DI UN INTERO */
  ADD_OPERATION(dec, TYPE_INTG, TYPE_NONE,  TYPE_INTG,  ASM_DEC_I, 0, 1, 1);
  /* §§ DECREMENTO(DESTRO) DI UN INTERO */
  ADD_OPERATION(dec, TYPE_NONE, TYPE_INTG,  TYPE_INTG,  ASM_DEC_I, 0, 1, 1);
  /* §§ DECREMENTO(SINISTRO) DI UN REALE */
  ADD_OPERATION(dec, TYPE_REAL, TYPE_NONE,  TYPE_REAL,  ASM_DEC_R, 0, 1, 1);
  /* §§ DECREMENTO(DESTRO) DI UN REALE */
  ADD_OPERATION(dec, TYPE_NONE, TYPE_REAL,  TYPE_REAL,  ASM_DEC_R, 0, 1, 1);
  /* § OPERATORE DI SOMMA */
  /* §§ SOMMA FRA INTERI */
  //ADD_OPERATION(plus,   TYPE_INTG, TYPE_INTG,  TYPE_INTG,  ASM_ADD_II, 1, 1, 0);

  /* §§ SOMMA FRA REALI E INTERI */
  ADD_OPERATION(plus, type_RealNum, type_RealNum, type_RealNum, ASM_ADD_RR, 1, 1, 0);
  ADD_OPERATION(plus, type_IntgNum, type_IntgNum, type_IntgNum, ASM_ADD_II, 1, 1, 0);
  /* §§ SOMMA FRA PUNTI */
  ADD_OPERATION(plus,  type_Point,type_Point, TYPE_POINT,  ASM_ADD_PP, 1, 1, 0);

  /* § OPERATORE DI DIFFERENZA */
  /* §§ OPPOSTO DI UN INTERO */
  ADD_OPERATION(minus,  TYPE_INTG, TYPE_NONE,  TYPE_INTG,   ASM_NEG_I, 0, 1, 0);
  /* §§ OPPOSTO DI UN REALE */
  ADD_OPERATION(minus,  TYPE_REAL, TYPE_NONE,  TYPE_REAL,   ASM_NEG_R, 0, 1, 0);
  /* §§ OPPOSTO DI UN PUNTO */
  ADD_OPERATION(minus, type_Point, TYPE_NONE, TYPE_POINT,   ASM_NEG_P, 0, 1, 0);

  /* §§ DIFFERENZA FRA REALI E INTERI */
  ADD_OPERATION(minus, type_RealNum, type_RealNum, type_RealNum, ASM_SUB_RR, 0, 1, 0);
  ADD_OPERATION(minus, type_IntgNum, type_IntgNum, type_IntgNum, ASM_SUB_II, 0, 1, 0);
  /* §§ DIFFERENZA FRA PUNTI */
  ADD_OPERATION(minus, type_Point,type_Point, TYPE_POINT,  ASM_SUB_PP, 0, 1, 0);

  /* § OPERATORE DI PRODOTTO */
  /* §§ PRODOTTO FRA REALI E INTERI */
  ADD_OPERATION(times, type_RealNum, type_RealNum, type_RealNum, ASM_MUL_RR, 1, 1, 0);
  ADD_OPERATION(times, type_IntgNum, type_IntgNum, type_IntgNum, ASM_MUL_II, 1, 1, 0);
  /* §§ PRODOTTO REALE * PUNTO O VICEVERSA */
  ADD_OPERATION(times, type_Point, type_RealNum, TYPE_POINT, ASM_PMULR_P, 1, 1, 0);

  /* § OPERATORE DI DIVISIONE */
  /* §§ DIVISIONE FRA REALI E INTERI */
  ADD_OPERATION(div, type_RealNum, type_RealNum, type_RealNum, ASM_DIV_RR, 0, 1, 0);
  ADD_OPERATION(div, type_IntgNum, type_IntgNum, type_IntgNum, ASM_DIV_II, 0, 1, 0);
  /* §§ DIVISIONE PUNTO / REALE */
  ADD_OPERATION(div, type_Point, type_RealNum, TYPE_POINT, ASM_PDIVR_P, 0, 1, 0);

  /* § RESTO DELLA DIVISIONE FRA INTERI */
  ADD_OPERATION(rem,  TYPE_INTG, TYPE_INTG, TYPE_INTG, ASM_REM_II,  0, 1, 0);

  /* § OPERATORE DI UGUAGLIANZA */
  /* §§ UGUAGLIANZA FRA INTERI */
  ADD_OPERATION(eq,   TYPE_INTG, TYPE_INTG, TYPE_INTG, ASM_EQ_II,   1, 1, 0);
  /* §§ UGUAGLIANZA FRA REALI */
  ADD_OPERATION(eq,   TYPE_REAL, TYPE_REAL, TYPE_INTG, ASM_EQ_RR,   1, 1, 0);
  /* §§ UGUAGLIANZA FRA PUNTI */
  ADD_OPERATION(eq,  type_Point,type_Point, TYPE_INTG, ASM_EQ_PP,   1, 1, 0);
  /* § OPERATORE DI DISUGUAGLIANZA */
  /* §§ DISUGUAGLIANZA FRA INTERI */
  ADD_OPERATION(ne,   TYPE_INTG, TYPE_INTG, TYPE_INTG, ASM_NE_II,   1, 1, 0);
  /* §§ DISUGUAGLIANZA FRA REALI */
  ADD_OPERATION(ne,   TYPE_REAL, TYPE_REAL, TYPE_INTG, ASM_NE_RR,   1, 1, 0);
  /* §§ DISUGUAGLIANZA FRA PUNTI */
  ADD_OPERATION(ne,  type_Point,type_Point, TYPE_INTG, ASM_NE_PP,   1, 1, 0);
  /* § OPERATORE DI MINORE */
  /* §§ MINORE FRA INTERI */
  ADD_OPERATION(lt,   TYPE_INTG, TYPE_INTG, TYPE_INTG, ASM_LT_II,   0, 1, 0);
  /* §§ MINORE FRA REALI */
  ADD_OPERATION(lt,   TYPE_REAL, TYPE_REAL, TYPE_INTG, ASM_LT_RR,   0, 1, 0);
  /* § OPERATORE DI MINORE-UGUALE */
  /* §§ MINORE-UGUALE FRA INTERI */
  ADD_OPERATION(le,   TYPE_INTG, TYPE_INTG, TYPE_INTG, ASM_LE_II,   0, 1, 0);
  /* §§ MINORE-UGUALE FRA REALI */
  ADD_OPERATION(le,   TYPE_REAL, TYPE_REAL, TYPE_INTG, ASM_LE_RR,   0, 1, 0);
  /* § OPERATORE DI MAGGIORE */
  /* §§ MAGGIORE FRA INTERI */
  ADD_OPERATION(gt,   TYPE_INTG, TYPE_INTG, TYPE_INTG, ASM_GT_II,   0, 1, 0);
  /* §§ MAGGIORE FRA REALI */
  ADD_OPERATION(gt,   TYPE_REAL, TYPE_REAL, TYPE_INTG, ASM_GT_RR,   0, 1, 0);
  /* § OPERATORE DI MAGGIORE-UGUALE */
  /* §§ MAGGIORE-UGUALE FRA INTERI */
  ADD_OPERATION(ge,   TYPE_INTG, TYPE_INTG, TYPE_INTG, ASM_GE_II,   0, 1, 0);
  /* §§ MAGGIORE-UGUALE FRA REALI */
  ADD_OPERATION(ge,   TYPE_REAL, TYPE_REAL, TYPE_INTG, ASM_GE_RR,   0, 1, 0);
  /* § OPERATORE DI NOT LOGICO DI UN INTERO */
  ADD_OPERATION(lnot, TYPE_INTG, TYPE_NONE, TYPE_INTG, ASM_LNOT_I,  0, 1, 0);
  /* § OPERATORE DI AND LOGICO FRA INTERI */
  ADD_OPERATION(land, TYPE_INTG, TYPE_INTG, TYPE_INTG, ASM_LAND_II, 1, 1, 0);
  /* § OPERATORE DI OR LOGICO FRA INTERI */
  ADD_OPERATION(lor,  TYPE_INTG, TYPE_INTG, TYPE_INTG, ASM_LOR_II,  1, 1, 0);

  /*******OPERATORI DI ASSEGNAZIONE DEL TIPO x= DOVE x E' UN OPERATORE*******/
  /* § OPERATORE &= FRA INTERI */
  ADD_OPERATION(aband, TYPE_INTG, TYPE_INTG, TYPE_INTG, ASM_BAND_II, 0, 1, 1);
  /* § OPERATORE ^= FRA INTERI */
  ADD_OPERATION(abxor, TYPE_INTG, TYPE_INTG, TYPE_INTG, ASM_BXOR_II, 0, 1, 1);
  /* § OPERATORE |= FRA INTERI */
  ADD_OPERATION(abor, TYPE_INTG, TYPE_INTG, TYPE_INTG, ASM_BOR_II, 0, 1, 1);
  /* § OPERATORE <<= FRA INTERI */
  ADD_OPERATION(ashl, TYPE_INTG, TYPE_INTG,  TYPE_INTG,  ASM_SHL_II, 0, 1, 1);
  /* § OPERATORE >>= FRA INTERI */
  ADD_OPERATION(ashr, TYPE_INTG, TYPE_INTG,  TYPE_INTG,  ASM_SHR_II, 0, 1, 1);
  /* § OPERATORE += */
  /* §§ += FRA INTERI */
  ADD_OPERATION(aplus,  TYPE_INTG, TYPE_INTG,  TYPE_INTG,  ASM_ADD_II, 0, 1, 1);
  /* §§ += FRA REALI */
  ADD_OPERATION(aplus,  TYPE_REAL, TYPE_REAL,  TYPE_REAL,  ASM_ADD_RR, 0, 1, 1);
  /* §§ += FRA PUNTI */
  ADD_OPERATION(aplus, TYPE_POINT,type_Point, TYPE_POINT,  ASM_ADD_PP, 0, 1, 1);
  /* § OPERATORE -= */
  /* §§ -= FRA INTERI */
  ADD_OPERATION(aminus, TYPE_INTG, TYPE_INTG,  TYPE_INTG,  ASM_SUB_II, 0, 1, 1);
  /* §§ -= FRA REALI */
  ADD_OPERATION(aminus, TYPE_REAL, TYPE_REAL,  TYPE_REAL,  ASM_SUB_RR, 0, 1, 1);
  /* §§ -= FRA PUNTI */
  ADD_OPERATION(aminus,TYPE_POINT,type_Point, TYPE_POINT,  ASM_SUB_PP, 0, 1, 1);
  /* § OPERATORE *= */
  /* §§ *= FRA INTERI */
  ADD_OPERATION(atimes, TYPE_INTG, TYPE_INTG,  TYPE_INTG,  ASM_MUL_II, 0, 1, 1);
  /* §§ *= FRA REALI */
  ADD_OPERATION(atimes, TYPE_REAL, TYPE_REAL,  TYPE_REAL,  ASM_MUL_RR, 0, 1, 1);
  /* § OPERATORE /= */
  /* §§ /= FRA INTERI */
  ADD_OPERATION(adiv,   TYPE_INTG, TYPE_INTG,  TYPE_INTG,  ASM_DIV_II, 0, 1, 1);
  /* §§ /= FRA REALI */
  ADD_OPERATION(adiv,   TYPE_REAL, TYPE_REAL,  TYPE_REAL,  ASM_DIV_RR, 0, 1, 1);
  /* § OPERATORE %= FRA INTERI */
  ADD_OPERATION(arem,   TYPE_INTG, TYPE_INTG,  TYPE_INTG,  ASM_REM_II, 0, 1, 1);

  /* OPERATORI CHE DEFINISCONO */
  cmp_opr.assign->can_define = 1;

  opn = Cmp_Operation_Add(cmp_opr.converter, TYPE_INTG, TYPE_NONE, TYPE_REAL);
  status |= ( opn == NULL );
  opn->is.commutative = 0;
  opn->is.intrinsic = 1;
  opn->is.assignment = 0;
  opn->is.link = 0;
  opn->asm_code = ASM_REAL_I;

  opn = Cmp_Operation_Add(cmp_opr.converter, TYPE_CHAR, TYPE_NONE, TYPE_REAL);
  status |= ( opn == NULL );
  opn->is.commutative = 0;
  opn->is.intrinsic = 1;
  opn->is.assignment = 0;
  opn->is.link = 0;
  opn->asm_code = ASM_REAL_C;

  /* Se il caricamento degli operatori non e' perfettamente riuscito esco! */
  if ( status == 1 ) return Failed;

  return Success;
}

#if 0
void Cmp_Finish() {
  Task status;
  Intg main;
  Intg num_var[NUM_TYPES], num_reg[NUM_TYPES];

  RegVar_Get_Nums(num_var, num_reg);
  if IS_FAILED( VM_Asm_Prepare(num_var, num_reg) ) return Failed;
  main = VM_Module_Undefined("main");
  if ( main < 1 ) return Failed;
  if IS_FAILED( VM_Asm_Install(main, Cmp_Curr_Output) ) return Failed;
  status = VM_Module_Execute(main);
}
#endif

/* DESCRIZIONE: Mette in *e un espressione immediata di tipo intero.
 */
void Cmp_Expr_New_Imm_Char(Expression *e, Char c) {
  MSG_LOCATION("Cmp_Expr_New_Imm_Char");
  Cmp_Expr_Container_New(e, TYPE_CHAR, CONTAINER_IMM);
  e->value.i = (Intg) c;
}

/* DESCRIZIONE: Mette in *e un espressione immediata di tipo intero.
 */
void Cmp_Expr_New_Imm_Intg(Expression *e, Intg i) {
  MSG_LOCATION("Cmp_Expr_New_Imm_Intg");
  Cmp_Expr_Container_New(e, TYPE_INTG, CONTAINER_IMM);
  e->value.i = i;
}

/* DESCRIZIONE: Mette in *e un espressione immediata di tipo reale.
 */
void Cmp_Expr_New_Imm_Real(Expression *e, Real r) {
  MSG_LOCATION("Cmp_Expr_New_Imm_Real");
  Cmp_Expr_Container_New(e, TYPE_REAL, CONTAINER_IMM);
  e->value.r = r;
}

/* DESCRIZIONE: Mette in *e un espressione immediata di tipo reale.
 */
void Cmp_Expr_New_Imm_Point(Expression *e, Point *p) {
  MSG_LOCATION("Cmp_Expr_New_Imm_Point");
  Cmp_Expr_Container_New(e, TYPE_POINT, CONTAINER_IMM);
  e->value.p = *p;
}

/* DESCRIZIONE: Crea una espressione di tipo "target" locale, cioe' che puo'
 *  essere il target di un'operazione di assegnazione. Quindi a tale espressione
 *  corrisponde una variabile locale di tipo type.
 */
Task Cmp_Expr_Target_New(Expression *e, Intg type) {
  MSG_LOCATION("Cmp_Expr_New_Target");
  assert( (type >= 0) || (type < NUM_TYPES) );

  if (type < CMP_PRIVILEGED) {
    e->is.imm = 0;
    e->is.value = 1;
    e->is.ignore = 0;
    e->is.target = 1;
    e->is.typed = 1;
    e->is.allocd = 0;
    e->is.release = 0;

    e->resolved = e->type = type;
    e->categ = CAT_LREG;

    if ( (e->value.i = -Var_Occupy(type, cmp_box_level)) < 0 )
      return Success;
    return Failed;

  } else {
    TypeDesc *td;

    e->is.imm = 0;
    e->is.value = 1;
    e->is.ignore = 0;
    e->is.target = 1;
    e->is.typed = 1;
    e->is.allocd = 1;
    e->is.release = 0;

    e->resolved = e->type = type;
    e->categ = CAT_LREG;
    if ( (e->value.i = -Var_Occupy(TYPE_OBJ, cmp_box_level)) >= 0 )
      return Failed;

    td = Tym_Type_Get(type);
    if ( td == NULL ) return Failed;

    VM_Assemble(ASM_MALLOC_I, CAT_IMM, td->size);
    VM_Assemble(ASM_MOV_OO, e->categ, e->value.i, CAT_LREG, 0);
    return Success;
  }
}

Task Cmp_Expr_Target_Delete(Expression *e) {
  MSG_LOCATION("Cmp_Expr_Target_Delete");

  if ( !e->is.target || !e->is.value || e->is.imm ) return Success;

  if ( e->resolved < CMP_PRIVILEGED ) {
    if IS_SUCCESSFUL( Var_Release(e->resolved, -e->value.i) ) return Success;
    return Failed;

  } else {
    printf("Qui faccio la cazzata!\n");
    VM_Assemble(ASM_MFREE_O, e->categ, e->value.i);
    if IS_SUCCESSFUL( Var_Release(TYPE_OBJ, -e->value.i) ) return Success;
    return Failed;
  }
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
  Intg signature;
/*  struct {
    unsigned int used : 1;
  } status;*/
  Intg type;
  Intg size;
} DataItem;

static Array *cmp_data_segment = NULL;
static Array *cmp_imm_segment  = NULL;
static char signature_str[4] = "data";
static Intg signature;

/* DESCRIPTION: This function initializes the data segment, the area used
 *  to store strings and the values of other objects.
 */
Task Cmp_Data_Init(void) {
  MSG_LOCATION("Cmp_Data_Init");
  /* Se l'array associata al segmento dati non e' inizializzata,
   * la inizializzo ora!
   */
  if ( cmp_data_segment == NULL ) {
    cmp_data_segment = Array_New(sizeof(char), CMP_TYPICAL_DATA_SIZE);
    if ( cmp_data_segment == NULL ) return Failed;
  }
  signature = *((Intg *) signature_str);
  return Success;
}

/* DESCRIPTION: This function adds a new piece of data to the data segment.
 * NOTE: It returns the address of the data item with respect to the beginning
 *  of the data segment.
 */
Intg Cmp_Data_Add(Intg type, void *data, Intg size) {
  Intg addr;
  DataItem di;
  MSG_LOCATION("Cmp_Data_Add");

  /* Now we insert the data descriptor */
  di.signature = signature;
  di.type = type;
  di.size = size;
  if IS_FAILED( Arr_MPush(cmp_data_segment, & di, sizeof(di)) ) return -1;

  /* And now we insert the piece of data */
  addr = Arr_NumItem(cmp_data_segment);
  if IS_FAILED( Arr_MPush(cmp_data_segment, data, size) ) return -1;

  return addr;
}

/* DESCRIPTION: This function sets the global register gro0 of the VM
 *  to be used as the pointer to the data-segment.
 */
Task Cmp_Data_Prepare(void) {
  void *data_ptr;
  MSG_LOCATION("Cmp_Data_Prepare");

  data_ptr = (void *) Arr_FirstItemPtr(cmp_data_segment, char);
  TASK( VM_Module_Global_Set(TYPE_OBJ, (Intg) 0, & data_ptr) );
  return Success;
}

/* DESCRIPTION: This function destroys the data segment.
 */
void Cmp_Data_Destroy(void) {
  MSG_LOCATION("Cmp_Data_Destroy");
  Arr_Destroy(cmp_data_segment);
}

/* DESCRIPTION: This function initializes the segment of immediates,
 *  the portion of memory used to store strings and values of immediate
 *  objects.
 */
Task Cmp_Imm_Init(void) {
  MSG_LOCATION("Cmp_Imm_Init");
  if ( cmp_imm_segment == NULL ) {
    cmp_imm_segment = Array_New(sizeof(char), CMP_TYPICAL_IMM_SIZE);
    if ( cmp_imm_segment == NULL ) return Failed;
  }
  signature = *((Intg *) signature_str);
  return Success;
}

/* DESCRIPTION: This function adds a new piece of data to the segment
 *  of immediate objects.
 * NOTE: It returns the address of the data item with respect to the beginning
 *  of the segment.
 */
Intg Cmp_Imm_Add(Intg type, void *data, Intg size) {
  Intg addr;
  DataItem di;
  MSG_LOCATION("Cmp_Imm_Add");

  /* Now we insert the data descriptor */
  di.signature = signature;
  di.type = type;
  di.size = size;
  if IS_FAILED( Arr_MPush(cmp_imm_segment, & di, sizeof(di)) ) return -1;

  /* And now we insert the piece of data */
  addr = Arr_NumItem(cmp_imm_segment);
  if IS_FAILED( Arr_MPush(cmp_imm_segment, data, size) ) return -1;
  return addr;
}

/* DESCRIPTION: This function destroys the segment of immediates.
 */
void Cmp_Imm_Destroy(void) {
  MSG_LOCATION("Cmp_Imm_Destroy");
  Arr_Destroy(cmp_imm_segment);
}

/* DESCRIPTION: This function displays the content of the data segment.
 */
Task Cmp_Data_Display(FILE *stream) {
  char *data;
  Intg size, pos, ds;
  DataItem *di;
  MSG_LOCATION("Cmp_Data_Display");

  if ( cmp_data_segment == NULL ) {
    MSG_ERROR("Segmento dati non inizializzato!");
    return Failed;
  }
  data = Arr_FirstItemPtr(cmp_data_segment, char);
  size = Arr_NumItem(cmp_data_segment);

  if ( size < 1 ) {
    fprintf(stream, "*** EMPTY DATA-SEGMENT ***\n");
    return Success;
  }

  fprintf(stream, "*** CONTENT OF THE DATA-SEGMENT ***\n");

  pos = 0;
  while ( pos + sizeof(DataItem) <= size ) {
    di = (DataItem *) data;
    fprintf(stream, "  Address "SIntg", size "SIntg": data of type '%s':\n",
     pos, di->size, Tym_Type_Name(di->type) );
    if ( di->signature != signature ) {
      fprintf(stream, "Error: bad data-block.\n");
      MSG_ERROR("Segmento-dati danneggiato alla posizione %d.", pos);
      return Failed;
    }

    ds = sizeof(DataItem) + di->size;
    pos += ds;
    if ( (di->size < 0) || (pos > size) ) {
      fprintf(stream, "Error: bad data-block.\n");
      MSG_ERROR("Dimensione errata del blocco di dati in posizione %d.", pos);
      return Failed;
    }
    data += ds;
  }

  fprintf(stream, "*** END OF THE DATA-SEGMENT ***\n");
  return Failed;
}

/*****************************************************************************
 * The following functions are needed to handle strings.                     *
 *****************************************************************************/

Task Cmp_String_New(Expression *e, Name *str, int free_str) {
  Intg ts, addr;
  Intg length = str->length;
  MSG_LOCATION("Cmp_String_New");

  /* Creo il tipo di dato corrispondente alla stringa */
  ts = Tym_Build_Array_Of(length, TYPE_CHAR);
  if ( ts < 0 ) return Failed;

  /* Copio la stringa nell'area dati */
  addr = Cmp_Data_Add(ts, str->text, length);
  if ( addr < 0 ) return Failed;

  /* Libero la stringa se devo! */
  if ( free_str ) Name_Free(str);

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
 * The following functions are needed to compile procedures.                 *
 *****************************************************************************/

/* DESCRIPTION: This function is an interface for the function
 *  Tym_Search_Procedure. In fact the purpose of the first is very similar
 *  to the purpose of the second: the function searches the procedure
 *  of type 'procedure' which belong to the opened box, whose prefix
 *  descriptor is 'prefix'. If a suitable procedure is found, its actual
 *  type is assigned to *prototype and its module number is put into
 *  *asm_module. After the call *box will contain the pointer to the
 *  box whose prefix is 'prefix'.
 */
Task Cmp_Procedure_Search
 (Intg procedure, Intg prefix, Box **box, Intg *prototype, Intg *asm_module) {
  Box *b;
  Intg p, dummy;

  if ( prototype == NULL ) prototype = & dummy;

  /* Now we use prefix to identify the box, which is the parent
   * of the procedure
   */
  b = Box_Get( (prefix < 0) ? 0 : prefix );
  if ( b == NULL ) return Failed;
  if ( b->type == TYPE_VOID ) {
    MSG_WARNING("La box di tipo [...] non ammette procedure!");
    return Failed;
  }
  *box = b;

  /* Now we search for the procedure associated with *e */
  p = Tym_Search_Procedure(procedure, b->type, prototype);

  /* If a suitable procedure does not exist, we create it now,
   * and we mark it as "undefined"
   */
  if ( p == TYPE_NONE ) {
    *prototype = TYPE_NONE;
    *asm_module = VM_Module_Next();
    p = Tym_Build_Procedure(procedure, b->type, *asm_module);
    if ( p == TYPE_NONE ) return Failed;
    assert( *asm_module == VM_Module_Undefined(Tym_Type_Name(p)) );
    return Success;

  } else {
    TypeDesc *td;

    td = Tym_Type_Get(p);
    if ( td == NULL ) return Failed;
    assert(td->tot == TOT_PROCEDURE);
    *asm_module = td->asm_mod;
    return Success;
  }
}

/* DESCRIPTION: This function handles the procedures of the box corresponding
 *  to prefix. It generates the assembly code that calls the procedure.
 */
Task Cmp_Procedure(Expression *e, Intg prefix) {
  Box *b;
  Intg asm_module, t;
  Intg prototype;
  MSG_LOCATION("Cmp_Procedure");

  /* First of all we check the attributes of the expression *e */
  if ( e->is.ignore ) goto exit_success;
  if ( !e->is.typed ) {
    MSG_ERROR("L'espressione deve avere tipo!");
    goto exit_failed;
  }
  t = Tym_Type_Resolve_Alias(e->type);
  if ( t == TYPE_VOID ) goto exit_success;

  if IS_FAILED( Cmp_Procedure_Search
   (e->type, prefix, & b, & prototype, & asm_module) ) goto exit_failed;

  /* Now we compile the procedure */
  /* We pass the box which is the parent of the procedure */
  if ( b->value.is.value ) {
    if IS_FAILED( Cmp_Expr_To_Ptr(& b->value, CAT_GREG, (Intg) 1, 0) )
      goto exit_failed;
  }

  /* We pass the argument of the procedure */
  if ( prototype != TYPE_NONE ) {
    /* The argument must be converted first! */
    TASK( Cmp_Expr_Expand(prototype, e) );
  }

  if IS_FAILED( Cmp_Expr_To_Ptr(e, CAT_GREG, (Intg) 2, 0) ) goto exit_failed;
  VM_Assemble(ASM_CALL_I, CAT_IMM, asm_module);
  goto exit_success;

exit_failed:
  (void) Cmp_Expr_Destroy(e);
  return Failed;

exit_success:
  (void) Cmp_Expr_Destroy(e);
  return Success;
}

/*****************************************************************************/
Task Cmp_Def_C_Procedure(Intg procedure, Intg of_type, Task (*C_func)(void)) {
  ModulePtr p;
  Intg asm_module, proc;

  asm_module = VM_Module_Next();
  proc = Tym_Build_Procedure(procedure, of_type, asm_module);
  if ( proc == TYPE_NONE ) return Failed;

  if ( asm_module != VM_Module_Undefined(Tym_Type_Name(proc)) )
    return Failed;

  p.c_func = C_func;
  TASK( VM_Module_Define(asm_module, MODULE_IS_C_FUNC, p) );
  return Success;
}

#include "structure.c"
