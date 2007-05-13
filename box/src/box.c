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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "types.h"
#include "defaults.h"
#include "messages.h"
#include "array.h"
// #include "str.h"
#include "virtmach.h"
#include "vmproc.h"
#include "vmsym.h"
#include "vmsymstuff.h"
#include "registers.h"
#include "expr.h"
#include "compiler.h"

/******************************************************************************
 *            FUNZIONI DI GESTIONE DELLE BOX (APERTURA E CHIUSURA)            *
 ******************************************************************************/

static BoxStack box_stack, *bs = & box_stack;

Task Box_Init(void) {
  TASK( Arr_New(& bs->box, sizeof(Box), BOX_ARR_SIZE) );
  return Success;
}

void Box_Destroy(void) {
  Arr_Destroy(bs->box);
}

Int Box_Depth(void) {
  return (Arr_NumItem(bs->box) - 1);
}

static Task Box_Def_Prepare(UInt head_sym_num) {
  Int num_var[NUM_TYPES], num_reg[NUM_TYPES];
  RegVar_Get_Nums(num_var, num_reg);
  TASK( VM_Sym_Def_Proc_Head(cmp_vm, head_sym_num, num_var, num_reg) );
  return Success;
}

Task Box_Main_Begin(void) {
  Box *b;
  /* Sets the output for the compiled code */
  UInt main_sheet;
  TASK( VM_Proc_Code_New(cmp_vm, & main_sheet) );
  TASK( VM_Proc_Target_Set(cmp_vm, main_sheet) );
  bs->cur_proc_num = main_sheet;
  TASK( Box_Instance_Begin((Expr *) NULL) );
  b = Arr_LastItemPtr(bs->box, Box);
  TASK( VM_Sym_Proc_Head(cmp_vm, & b->head_sym_num) );
  return Success;
}

void Box_Main_End(void) {
  UInt box_level = Box_Depth();
  Box *b = Arr_FirstItemPtr(bs->box, Box);
  UInt head_sym_num = b->head_sym_num;
  if (box_level > 0) {
    MSG_ERROR("Missing %I closing %s!", box_level,
     box_level > 1 ? "braces" : "brace");
  }
  (void) Box_Instance_End((Expr *) NULL);
  VM_Assemble(cmp_vm, ASM_RET);
  (void) Box_Def_Prepare(head_sym_num);
}

Task Box_Def_Begin(Int proc_type) {
  UInt new_sheet;
  Box b;

  /* Create a new procedure where to write the code
   * and set it as the target of code generation.
   */
  TASK( VM_Proc_Code_New(cmp_vm, & new_sheet) );
  TASK( VM_Proc_Target_Set(cmp_vm, new_sheet) );

  /* Create a new register frame for the new piece of code:
   * allocation of registers should start from scratch!
   */
  Reg_Frame_Push();

  /* Create the header of the procedure: the piece of code
   * where instructions such as "regi 1, 2" appears.
   * These instructions say to the virtual machine how many registers
   * will be used in the procedure. Currently these numbers are not known
   * therfore we use the vmsym module to register a new symbol, which
   * will used later to correct the header, once this information will
   * be known.
   */
  TASK( VM_Sym_Proc_Head(cmp_vm, & b.head_sym_num) );

  /* Now we get the parent and the child of the procedure.
   * We create two expression which will be used to refer
   * to these objects.
   */
  Expr_Parent_And_Child(& b.parent, & b.child, proc_type);

  /* We finally collect all the other data about the box and we push
   * it in the array of opened boxes.
   */
  b.is.second = 0;
  b.syms = NULL;
  b.type = proc_type;
  b.is.definition = 1;
  b.proc_num = new_sheet;
  /*Expr_New_Void(& b.parent);*/
  TASK(Arr_Push(bs->box, & b));
  bs->cur_proc_num = new_sheet;
  return Success;
}

Task Box_Def_End(void) {
  Box *b;
  Int proc_type;
  UInt sym_num, call_num, proc_num;

  b = Arr_LastItemPtr(bs->box, Box);
  assert(b->is.definition);
  VM_Assemble(cmp_vm, ASM_RET);
  TASK(Box_Def_Prepare(b->head_sym_num));
  proc_type = b->type;
  proc_num = b->proc_num;
  TS_Procedure_Sym_Num(cmp->ts, & sym_num, proc_type);
  /* We finally install the code for the procedure */
  TASK( VM_Proc_Install_Code(cmp_vm, & call_num, proc_num,
   "(noname)", Tym_Type_Name(proc_type)) );
  /* And define the symbol */
  TASK( VM_Sym_Def_Call(cmp_vm, sym_num, call_num) );

  {
    Symbol *s;
    for ( s = b->syms; s != (Symbol *) NULL; s = s->brother ) {
      TASK( Cmp_Expr_Destroy(& (s->value), 1) );
      Sym_Symbol_Delete( s );
    }
  }
  TASK( Reg_Frame_Pop() );

  TASK(Arr_Dec(bs->box));
  if (Arr_NumItem(bs->box) > 0) {
    b = Arr_LastItemPtr(bs->box, Box);
    TASK( VM_Proc_Target_Set(cmp_vm, b->proc_num) );
    bs->cur_proc_num = b->proc_num;
  }
  return Success;
}

/* This function opens a new box for the expression *e.
 * If e == NULL the box is considered to be a simple untyped box.
 * If *e is a typed, but un-valued expression, then the creation of a new box
 * of type *e is started.
 * If *e has value, a modification-box for that expression is started.
 */
Task Box_Instance_Begin(Expr *e) {
  Box b;

  Expr_New_Void(& b.child);
  if ( e == NULL ) {
    /* Si tratta di una box void */
    b.is.second = 0;
    b.syms = NULL;         /* Catena dei simboli figli */
    b.type = TYPE_VOID;
    Expr_New_Void(& b.parent);

  } else {
    if ( ! e->is.typed ) {
      MSG_ERROR("Cannot open the box: '%N' has no type!", & e->value.nm);
      Cmp_Expr_Destroy_Tmp(e);
      return Failed;
    }

    b.is.second = 1;
    if ( ! e->is.value ) {
      TASK( Cmp_Expr_Create(e, e->type, /* temporary = */ 1 ) );
      e->is.release = 0;
      b.is.second = 0;
    }

    /* Compilo il descrittore del nuovo esempio di sessione aperto */
    b.type = e->type;
    b.syms = NULL;        /* Catena dei simboli figli */
    b.parent = *e;         /* Valore della sessione */
  }

  /* Creo le labels che puntano all'inizio ed alla fine della box */
  TASK( VM_Label_New_Here(cmp_vm, & b.label_begin) );
  TASK( VM_Label_New_Undef(cmp_vm, & b.label_end) );

  /* Inserisce la nuova sessione */
  b.is.definition = 0; /* This is just an instance, not a definition */
  b.proc_num = bs->cur_proc_num; /* The procedure number where we are now */
  TASK(Arr_Push(bs->box, & b));
  return Success;
}

/** Conclude l'ultimo esempio di sessione aperto, la lista
 * delle variabili esplicite viene analizzata, per ognuna di queste
 * viene eseguita l'azione final_action, dopodiche' viene eliminata.
 */
Task Box_Instance_End(Expr *e) {
  /* Il registro occupato per la sessione ora diventa un registro normale
   * e puo' essere liberato!
   */
  if ( e != NULL ) e->is.release = 1;

  /* Eseguo dei controlli di sicurezza */
  if ( bs->box == NULL ) {
    MSG_ERROR("Nessuna box da terminare");
    return Failed;
  }

  if ( Arr_NumItem(bs->box) < 1 ) {
    MSG_ERROR("Nessuna box da terminare");
    return Failed;
  }

  /* Cancello le variabili esplicite della sessione */
  {
    Box *box = Arr_LastItemPtr(bs->box, Box);
    Symbol *s;

    /* Cancello le labels che puntano all'inizio ed alla fine della box */
    TASK( VM_Label_Destroy(cmp_vm, box->label_begin) );
    TASK( VM_Label_Define_Here(cmp_vm, box->label_end) );
    TASK( VM_Label_Destroy(cmp_vm, box->label_end) );

    for ( s = box->syms; s != (Symbol *) NULL; s = s->brother ) {
      TASK( Cmp_Expr_Destroy(& (s->value), 1) );
      Sym_Symbol_Delete( s );
    }
  }

  TASK(Arr_Dec(bs->box));
  return Success;
}

/*  Cerca fra le scatole aperte di profondita' > depth,
 *  la prima di tipo type. Se depth = 0 parte dall'ultima scatola aperta,
 *  se depth = 1, dalla penultima, etc.
 *  Restituisce la profondita' della scatola cercata o -1 in caso
 *  di ricerca fallita.
 */
Int Box_Search_Opened(Int type, Int depth) {
  Int max_depth, d;
  Box *box;

  assert( (bs->box != NULL) && (depth >= 0) );

  max_depth = Arr_NumItem(bs->box);
  if ( depth >= max_depth ) {
    MSG_ERROR("Indirizzo di box troppo profondo.");
    return -1;
  }

  box = Arr_LastItemPtr(bs->box, Box) - depth;
  for (d = depth; d < max_depth; d++) {
#if 0
    printf("Profondita': "SInt" -- Tipo: '%s'\n",
     d, Tym_Type_Name(box->parent.type));
#endif
    if ( box->type == type ) return d;
    --box;
  }
  return -1;
}

/* This function returns the pointer to the structure Box
 * corresponding to the box with depth 'depth'.
 */
Task Box_Get(Box **box, Int depth) {
  Int max_depth;
  assert(bs->box != NULL);
  if (depth < 0) depth = 0;
  max_depth = Arr_NumItem(bs->box);
  if (depth >= max_depth) {
    MSG_ERROR("Invalid box depth.");
    return Failed;
  }
  *box = Arr_LastItemPtr(bs->box, Box) - depth;
  return Success;
}

Task Box_Parent_Get(Expr *e_parent, Int depth) {
  Box *b;
  TASK( Box_Get(& b, depth) );
  *e_parent = b->parent;
  return Success;
}

Task Box_Child_Get(Expr *e_child, Int depth) {
  Box *b;
  TASK( Box_Get(& b, depth) );
  *e_child = b->child;
  return Success;
}

/********************************[DEFINIZIONE]********************************/

/*  Questa funzione definisce un nuovo simbolo di nome *nm,
 *  attribuendolo alla box di profondita' depth.
 */
Task Sym_Explicit_New(Symbol **sym, Name *nm, Int depth) {
  Int box_lev;
  Symbol *s;
  Box *b;

  /* Controllo che non esista un simbolo omonimo gia' definito */
  assert( (depth >= 0) && (depth <= Box_Depth()) );
  s = Sym_Explicit_Find(nm, depth, EXACT_DEPTH);
  if ( s != NULL ) {
    MSG_ERROR("A symbol with name '%N' has already been defined", nm);
    return Failed;
  }

  /* Introduco il nuovo simbolo nella lista dei simboli correnti */
  s = Sym_Symbol_New(nm);
  if ( s == NULL ) return Failed;

  /* Collego il nuovo simbolo alla lista delle variabili esplicite della
   * box corrente (in modo da eliminarle alla chiusura della box).
   */
  box_lev = Box_Depth() - depth;
  b = Arr_ItemPtr(bs->box, Box, box_lev + 1);
  s->brother = b->syms;
  b->syms = s;

  /* Inizializzo il simbolo */
  s->symattr.is_explicit = 1;
  s->child = NULL;
  s->parent.exp = box_lev;
  s->symtype = VARIABLE;
  *sym = s;
  return Success;
}
