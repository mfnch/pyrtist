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
// #include "virtmach.h"
// #include "vmproc.h"
// #include "vmsym.h"
// #include "vmsymstuff.h"
// #include "registers.h"
#include "compiler.h"

/******************************************************************************
 *            FUNZIONI DI GESTIONE DELLE BOX (APERTURA E CHIUSURA)            *
 ******************************************************************************/

/* NOTA: Osserviamo che bisogna distinguere fra:
 *  1) il tipo di simbolo;
 *  2) l'esempio di simbolo.
 * Questo vale per ogni tipo di linguaggio, come il C ad esempio.
 * int, long oppure double sono "tipi di simboli", mentre se definisco
 * int a, b; long c; double d; chiamero' a, b, c, d "esempi di simboli".
 * Quindi avro' procedure per definire un "tipo di sessione" e procedure
 * per definire "un esempio di sessione", ma questo vale non solo per
 * le sessioni, ma anche per le variabili, le funzioni, etc.
 * Distinguo le procedure  per definire gli esempi con la parola Instance
 * (come Sym_Session_New, che definisce un tipo di sessione, e
 * Sym_Session_Instance_Begin, che inizia un esempio di sessione,
 * di tipo gia' definito).
 */
static Array *cmp_box_list;
static Box *cmp_current_box;

Intg cmp_box_level;

/* This function opens a new box for the expression *e.
 * If e == NULL the box is considered to be a simple untyped box.
 * If *e is a typed, but un-valued expression, then the creation of a new box
 * of type *e is started.
 * If *e has value, a modification-box for that expression is started.
 */
Task Cmp_Box_Instance_Begin(Expression *e) {
  Box box;

  MSG_LOCATION("Cmp_Box_Instance_Begin");

  /* Se non esiste la lista delle box aperte la creo ora! */
  if ( cmp_box_list == NULL ) {
    cmp_box_list = Array_New(sizeof(Box), SYM_BOX_LIST_DIM);
    if ( cmp_box_list == NULL ) return Failed;
    cmp_box_level = -1; /* 0 e' la sessione principale */
  }

  ++cmp_box_level; /* Aumento il livello di box */
  if ( e == NULL ) {
    /* Si tratta di una box void */
    box.ID = cmp_box_level;     /* Livello della sessione */
    box.attr.second = 0;
    box.child = NULL;         /* Catena dei simboli figli */
    box.type = TYPE_VOID;
    box.value.type = TYPE_VOID;
    box.value.resolved = TYPE_VOID;
    box.value.is.typed = 1;
    box.value.is.value = 0;

  } else {
    if ( ! e->is.typed ) {
      MSG_ERROR( "Impossibile aprire una box per il simbolo senza tipo '%s'!",
       Name_Str(& e->value.nm) );
      Cmp_Expr_Destroy_Tmp( e );
      return Failed;
    }

    box.attr.second = 1;
    if ( ! e->is.value ) {
      TASK( Cmp_Expr_Create(e, e->type, /* temporary = */ 1 ) );
      e->is.release = 0;
      box.attr.second = 0;
    }

    /* Compilo il descrittore del nuovo esempio di sessione aperto */
    box.ID = cmp_box_level;  /* Livello della sessione */
    box.type = e->type;
    box.child = NULL;        /* Catena dei simboli figli */
    box.value = *e;          /* Valore della sessione */
  }

  /* Creo le labels che puntano all'inizio ed alla fine della box */
  TASK( VM_Label_New_Here(cmp_vm, & box.label_begin) );
  TASK( VM_Label_New_Undef(cmp_vm, & box.label_end) );

  /* Inserisce la nuova sessione */
  TASK(Arr_Push(cmp_box_list, & box));

  /* Imposto il puntatore all'esempio di sessione attualmente aperto */
  cmp_current_box = Arr_LastItemPtr(cmp_box_list, Box);

  return Success;
}

/* DESCRIZIONE: Conclude l'ultimo esempio di sessione aperto, la lista
 *  delle variabili esplicite viene analizzata, per ognuna di queste
 *  viene eseguita l'azione final_action, dopodiche' viene eliminata.
 */
Task Cmp_Box_Instance_End(Expression *e) {
  MSG_LOCATION("Cmp_Box_Instance_End");

  /* Il registro occupato per la sessione ora diventa un registro normale
   * e puo' essere liberato!
   */
  if ( e != NULL ) e->is.release = 1;

  /* Eseguo dei controlli di sicurezza */
  if ( cmp_box_list == NULL ) {
    MSG_ERROR("Nessuna box da terminare");
    return Failed;
  }

  if ( Arr_NumItem(cmp_box_list) < 1 ) {
    MSG_ERROR("Nessuna box da terminare");
    return Failed;
  }

  /* Cancello le variabili esplicite della sessione */
  {
    Box *box = Arr_LastItemPtr(cmp_box_list, Box);
    Symbol *s;

    /* Cancello le labels che puntano all'inizio ed alla fine della box */
    TASK( VM_Label_Destroy(cmp_vm, box->label_begin) );
    TASK( VM_Label_Define_Here(cmp_vm, box->label_end) );
    TASK( VM_Label_Destroy(cmp_vm, box->label_end) );

    for ( s = box->child; s != (Symbol *) NULL; s = s->brother ) {
      TASK( Cmp_Expr_Destroy(& (s->value), 1) );
      Sym_Symbol_Delete( s );
    }
  }

  TASK(Arr_Dec(cmp_box_list));
  --cmp_box_level;
  cmp_current_box = Arr_LastItemPtr(cmp_box_list, Box);
  return Success;
}

/* DESCRIZIONE: Cerca fra le scatole aperte di profondita' > depth,
 *  la prima di tipo type. Se depth = 0 parte dall'ultima scatola aperta,
 *  se depth = 1, dalla penultima, etc.
 *  Restituisce la profondita' della scatola cercata o -1 in caso
 *  di ricerca fallita.
 */
Intg Box_Search_Opened(Intg type, Intg depth) {
  Intg max_depth, d;
  Box *box;

  assert( (cmp_box_list != NULL) && (depth >= 0) );

  max_depth = Arr_NumItem(cmp_box_list);
  if ( depth >= max_depth ) {
    MSG_ERROR("Indirizzo di box troppo profondo.");
    return -1;
  }

  box = Arr_LastItemPtr(cmp_box_list, Box) - depth;
  for (d = depth; d < max_depth; d++) {
#if 1
    printf("Profondita': "SIntg" -- Tipo: '%s'\n",
     d, Tym_Type_Name(box->value.type));
#endif
    if ( box->type == type ) return d;
    --box;
  }
  return -1;
}

/* DESCRIPTION: This function returns the pointer to the structure Box
 *  corresponding to the box with depth 'depth'.
 */
Task Box_Get(Box **box, Intg depth) {
  Intg max_depth;
  assert(cmp_box_list != NULL);
  max_depth = Arr_NumItem(cmp_box_list);
  if ( (depth >= max_depth) || (depth < 0) ) {
    if (depth < 0) {
      MSG_ERROR("Profondita' di box negativa.");
    } else {
      MSG_ERROR("Profondita' di box fuori limite.");
    }
    return Failed;
  }
  *box = Arr_LastItemPtr(cmp_box_list, Box) - depth;
  return Success;
}

/********************************[DEFINIZIONE]********************************/

/* DESCRIZIONE: Questa funzione definisce un nuovo simbolo di nome *nm,
 *  attribuendolo alla box di profondita' depth.
 */
Task Sym_Explicit_New(Symbol **sym, Name *nm, Intg depth) {
  Intg box_lev;
  Symbol *s;
  Box *b;

  MSG_LOCATION("Sym_Explicit_New");

  /* Controllo che non esista un simbolo omonimo gia' definito */
  assert( (depth >= 0) && (depth <= cmp_box_level) );
  s = Sym_Explicit_Find(nm, depth, EXACT_DEPTH);
  if ( s != NULL ) {
    MSG_ERROR("Un simbolo con lo stesso nome esiste gia'");
    return Failed;
  }

  /* Introduco il nuovo simbolo nella lista dei simboli correnti */
  s = Sym_Symbol_New(nm);
  if ( s == NULL ) return Failed;

  /* Collego il nuovo simbolo alla lista delle variabili esplicite della
   * box corrente (in modo da eliminarle alla chiusura della box).
   */
  box_lev = cmp_box_level - depth;
  b = Arr_ItemPtr(cmp_box_list, Box, box_lev + 1);
  s->brother = b->child;
  b->child = s;

  /* Inizializzo il simbolo */
  s->symattr.is_explicit = 1;
  s->child = NULL;
  s->parent.exp = box_lev;
  s->symtype = VARIABLE;
  *sym = s;
  return Success;
}














#if 0
/* NOTA: Osserviamo che bisogna distinguere fra:
 *  1) il tipo di simbolo;
 *  2) l'esempio di simbolo.
 * Questo vale per ogni tipo di linguaggio, come il C ad esempio.
 * int, long oppure double sono "tipi di simboli", mentre se definisco
 * int a, b; long c; double d; chiamero' a, b, c, d "esempi di simboli".
 * Quindi avro' procedure per definire un "tipo di sessione" e procedure
 * per definire "un esempio di sessione", ma questo vale non solo per
 * le sessioni, ma anche per le variabili, le funzioni, etc.
 * Distinguo le procedure  per definire gli esempi con la parola Instance
 * (come Sym_Session_New, che definisce un tipo di sessione, e
 * Sym_Session_Instance_Begin, che inizia un esempio di sessione,
 * di tipo gia' definito).
 */
static Array *cmp_box_list;

Intg cmp_box_level;

Task Box_Init(BoxStack *bs) {
  TASK( Arr_New(& bs->box, sizeof(Box), BOX_ARR_SIZE) );
  return Success;
}

void Box_Destroy(BoxStack *bs) {
  Arr_destroy(bs->box);
}

Task Box_Open_Main(BoxStack *bs) {
}

Task Box_Close_Main(BoxStack *bs) {
}

Task Box_Definition_Begin(BoxStack *bs) {

}

/** This function opens a new box for the expression *e.
 * If e == NULL the box is considered to be a simple untyped box.
 * If *e is a typed, but un-valued expression, then the creation of a new box
 * of type *e is started.
 * If *e has value, a modification-box for that expression is started.
 */
Task Box_Instance_Begin(BoxStack *bs, Expr *e) {
  Box b;

  if (e == (Expr *) NULL) {
    /* Si tratta di una box void */
    b.attr.second = 0;
    b.child = NULL;         /* Catena dei simboli figli */
    b.type = TYPE_VOID;
    Expr_New_Void(& b.value);

  } else {
    if ( ! e->is.typed ) {
      MSG_ERROR("Cannot open the box: '%N' has no type!", & e->value.nm);
      Cmp_Expr_Destroy_Tmp(e);
      return Failed;
    }

    b.attr.second = 1;
    if ( ! e->is.value ) {
      TASK( Cmp_Expr_Create(e, e->type, /* temporary = */ 1) );
      e->is.release = 0;
      b.attr.second = 0;
    }

    /* Compilo il descrittore del nuovo esempio di sessione aperto */
    b.type = e->type;
    b.child = NULL;        /* Catena dei simboli figli */
    b.value = *e;          /* Valore della sessione */
  }

  /* Creo le labels che puntano all'inizio ed alla fine della box */
  TASK( VM_Label_New_Here(cmp_vm, & b.label_begin) );
  TASK( VM_Label_New_Undef(cmp_vm, & b.label_end) );

  /* Inserisce la nuova sessione */
  TASK(Arr_Push(cmp_box_list, & b));

  return Success;
}

/** Conclude l'ultimo esempio di sessione aperto, la lista
 * delle variabili esplicite viene analizzata, per ognuna di queste
 * viene eseguita l'azione final_action, dopodiche' viene eliminata.
 */
Task Box_Instance_End() {
  /* Il registro occupato per la sessione ora diventa un registro normale
   * e puo' essere liberato!
   */
  if ( e != NULL ) e->is.release = 1;

  if ( Arr_NumItem(cmp_box_list) < 1 ) {
    MSG_ERROR("Nessuna box da terminare");
    return Failed;
  }

  /* Cancello le variabili esplicite della sessione */
  {
    Box *box = Arr_LastItemPtr(cmp_box_list, Box);
    Symbol *s;

    /* Cancello le labels che puntano all'inizio ed alla fine della box */
    TASK( VM_Label_Destroy(cmp_vm, box->label_begin) );
    TASK( VM_Label_Define_Here(cmp_vm, box->label_end) );
    TASK( VM_Label_Destroy(cmp_vm, box->label_end) );

    for ( s = box->child; s != (Symbol *) NULL; s = s->brother ) {
      TASK( Cmp_Expr_Destroy(& (s->value), 1) );
      Sym_Symbol_Delete( s );
    }
  }

  TASK(Arr_Dec(cmp_box_list));
  return Success;
}











/* DESCRIZIONE: Cerca fra le scatole aperte di profondita' > depth,
 *  la prima di tipo type. Se depth = 0 parte dall'ultima scatola aperta,
 *  se depth = 1, dalla penultima, etc.
 *  Restituisce la profondita' della scatola cercata o -1 in caso
 *  di ricerca fallita.
 */
Intg Box_Search_Opened(Intg type, Intg depth) {
  Intg max_depth, d;
  Box *box;

  assert( (cmp_box_list != NULL) && (depth >= 0) );

  max_depth = Arr_NumItem(cmp_box_list);
  if ( depth >= max_depth ) {
    MSG_ERROR("Indirizzo di box troppo profondo.");
    return -1;
  }

  box = Arr_LastItemPtr(cmp_box_list, Box) - depth;
  for (d = depth; d < max_depth; d++) {
#if 1
    printf("Profondita': "SIntg" -- Tipo: '%s'\n",
     d, Tym_Type_Name(box->value.type));
#endif
    if ( box->type == type ) return d;
    --box;
  }
  return -1;
}

/* DESCRIPTION: This function returns the pointer to the structure Box
 *  corresponding to the box with depth 'depth'.
 */
Task Box_Get(Box **box, Intg depth) {
  Intg max_depth;
  assert(cmp_box_list != NULL);
  max_depth = Arr_NumItem(cmp_box_list);
  if ( (depth >= max_depth) || (depth < 0) ) {
    if (depth < 0) {
      MSG_ERROR("Profondita' di box negativa.");
    } else {
      MSG_ERROR("Profondita' di box fuori limite.");
    }
    return Failed;
  }
  *box = Arr_LastItemPtr(cmp_box_list, Box) - depth;
  return Success;
}
#endif
