/* symbols.c - Autore: Franchin Matteo - maggio 2004
 *
 * This file contains all the functions required to handle symbols...
 */

/* #define DEBUG */

/*#include <string.h>*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

#include "types.h"
#include "messages.h"
#include "defaults.h"
#include "str.h"
#include "array.h"
#include "virtmach.h"
#include "compiler.h"


/* Variabili interne */
static Symbol *hashtable[SYM_HASHSIZE];

/* Funzioni statiche definite in questo file: */
static UInt Imp_Check_Name_Conflicts(Symbol *s);

/******************************************************************************
 *   FUNZIONI ELEMENTARI DI GESTIONE DEI SIMBOLI (CREAZIONE ED ELEMINAZIONE)  *
 ******************************************************************************
 * These functions perform the basic operations with the hash-table:
 * create, destroy and find its items.
 */

/* DESCRIZIONE: Funzione che associa ad ogni nome un intero in modo
 *  "abbastanza casuale". Serve per trovare velocemente le variabili
 *  accedendo alla tavola hashtable, invece di scorrere tutta la lista
 *  delle variabili definite.
 */
UInt Sym__Hash(char *name, UInt leng)
{
  register UInt i, hashval;

  MSG_LOCATION("Sym__Hash");

  hashval = 0;
  for(i = 0; i<leng; i++)
    hashval = *(name++) + SYM_PARAMETER*hashval;

  return (hashval % SYM_HASHSIZE);
}

/* DESCRIZIONE: Cerca fra i simboli gia' definiti, quelli di nome name
 *  (di lunghezza leng) e chiama la funzione action per ogni corrispondenza
 *  trovata. Ad action viene passato il descrittore di simbolo s.
 *  action deve restituire:
 *   0 affinche' la ricerca continui;
 *   1 affinche' la ricerca finisca e Sym_Symbol_Find termini restituendo s;
 *   2 o altro affinche' la ricerca finisca e Sym_Symbol_Find termini
 *     restituendo NULL (in caso di errore).
 */
Symbol *Sym_Symbol_Find(Name *nm, SymbolAction action)
{
  Symbol *s;

  MSG_LOCATION("Sym_Symbol_Find");

  for ( s = hashtable[Sym__Hash(nm->text, nm->length)];
    s != (Symbol *) NULL; s = s->next ) {

    if IS_SUCCESSFUL( Str_Eq2(nm->text, nm->length, s->name, s->leng) ) {
      switch ( action(s) ) {
      case 0:
        break;
      case 1:
        return s; break;
      default:
        return (Symbol *) NULL;
      }
    }
  }
  return (Symbol *) NULL;
}

/* DESCRIZIONE: Stampa su stream i livelli di occupazione della hash-table.
 * NOTA: Serve per il debug.
 */
#ifdef DEBUG
void Sym_Symbol_Diagnostic(FILE *stream)
{
  UInt i;
  Symbol *s;

  fprintf(stream, "Stato di riempimento della hash-table:\n");
  for(i=0; i<SYM_HASHSIZE; i++) {
    UInt j = 0;

    for ( s = hashtable[i]; s != (Symbol *) NULL; s = s->next )
      ++j;

    if ( (j % 5) == 4 )
      fprintf(stream, SUInt ":" SUInt "\t", i, j);
    else
      fprintf(stream, SUInt ":" SUInt "\n", i, j);
  }
}
#endif

/* DESCRIZIONE: Inserisce un nuovo simbolo nella tavola dei simboli correnti,
 *  senza preoccuparsi della presenza di simboli dello stesso tipo.
 *  Restituisce il puntatore al descrittore del simbolo o NULL in caso di errore.
 */
Symbol *Sym_Symbol_New(Name *nm)
{
  register Symbol *s, *t;
  UInt hashval;

  MSG_LOCATION("Sym_Symbol_New");

  s = (Symbol *) malloc(sizeof(Symbol));
  if ( s == NULL ) {
    MSG_ERROR("Memoria esaurita");
    return (Symbol *) NULL;
  }

  s->name = Str_Dup(nm->text, nm->length);
  if ( s->name == NULL ) {
      MSG_ERROR("Memoria esaurita");
      free(s); return (Symbol *) NULL;
  }

  hashval = Sym__Hash(s->name, s->leng = nm->length);

  t = hashtable[hashval];
  s->previous = NULL;
  s->next = t;
  if (t != NULL) t->previous = s;
  hashtable[hashval] = s;

/*#ifdef DEBUG
  Sym_Symbol_Diagnostic(stderr);
  #endif*/

  return s;
}

/* DESCRIZIONE: Elimina il simbolo s dalla lista dei simboli correnti.
 * NOTA: Se s = NULL, esce senza far niente.
 */
void Sym_Symbol_Delete(Symbol *s)
{
  MSG_LOCATION("Sym_Symbol_Delete");

  if ( s == NULL ) return;

  if ( s->previous == NULL ) {
    if ( s->next == NULL ) {
      hashtable[Sym__Hash(s->name, s->leng)] = NULL;
      free(s->name); free(s); return;
    } else {
      hashtable[Sym__Hash(s->name, s->leng)] = s->next;
      (s->next)->previous = NULL;
      free(s->name); free(s); return;
    }

  } else {
    if ( s->next == NULL ) {
      (s->previous)->next = NULL;
      free(s->name); free(s); return;
    } else {
      (s->previous)->next = s->next;
      (s->next)->previous = s->previous;
      free(s->name); free(s); return;
    }
  }
}

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
 * If *e is an typed, but un-valued expression the creation of a new box
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
    box.child = NULL;	        /* Catena dei simboli figli */
    box.type = TYPE_VOID;
    box.value.type = TYPE_VOID;
    box.value.resolved = TYPE_VOID;
    box.value.is.typed = 1;
    box.value.is.value = 0;
    box.ID = cmp_box_level;     /* Livello della sessione */

  } else {
    if ( ! e->is.typed ) {
      MSG_ERROR( "Impossibile aprire una box per il simbolo senza tipo '%s'!",
       Name_To_Str(& e->value.nm) );
      Cmp_Expr_Destroy( e );
      return Failed;
    }

    if ( ! e->is.value ) {
      TASK( Cmp_Expr_Create(e, e->type, /* temporary = */ 1 ) );
      e->is.release = 0;
    }

    /* Compilo il descrittore del nuovo esempio di sessione aperto */
    box.type = e->type;
    box.child = NULL;        /* Catena dei simboli figli */
    box.value = *e;          /* Valore della sessione */
    box.ID = cmp_box_level;  /* Livello della sessione */
  }

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

    for ( s = box->child; s != (Symbol *) NULL; s = s->brother ) {
      TASK( Cmp_Expr_Destroy( & (s->value) ) );
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

/* Tipo che funge da genitore per i tipi impliciti */
static Intg sym_cur_parent = TYPE_NONE;

/* DESCRIZIONE: Funzione del tipo SymbolAction, usata con Sym_Symbol_Find,
 *  all'interno di Sym_Implicit_New, per controllare che non esistano
 *  conflitti fra i nomi.
 */
static UInt Imp_Check_Name_Conflicts(Symbol *s) {
  MSG_LOCATION("Imp_Check_Name_Conflicts");

  if ( ! s->symattr.is_explicit ) {
    /* Il simbolo e' implicito */
    if ( s->parent.imp == sym_cur_parent ) return 1;
  }

  return 0; /* Faccio continuare la ricerca */
}

/* DESCRIPTION: This function search a member of parent with name nm.
 * NOTE: Returns NULL if it doesn't find any member with name nm.
 */
Task Sym_Implicit_Find(Symbol **s, Intg parent, Name *nm) {
  sym_cur_parent = parent;
  *s = Sym_Symbol_Find(nm, Imp_Check_Name_Conflicts);
  return (*s == NULL) ? Failed : Success;
}

/* Defines a new implicit symbol, mwmber of the type 'parent'.
 */
Task Sym_Implicit_New(Symbol **new_sym, Intg parent, Name *nm) {
  Symbol *s, *ps;

  MSG_LOCATION("Sym_Implicit_New");

  ps = Tym_Symbol_Of_Type(parent);
  if ( ps == NULL ) return Failed;

  /* Controllo che non esista un simbolo omonimo gia' definito */
  sym_cur_parent = parent;
  s = Sym_Symbol_Find(nm, Imp_Check_Name_Conflicts);
  if ( s != NULL ) {
    MSG_ERROR("Un simbolo con lo stesso nome esiste gia'");
    return Failed;
  }

  /* Introduco il nuovo simbolo nella lista dei simboli correnti */
  s = Sym_Symbol_New(nm);
  if ( s == NULL ) return Failed;

  /* Collego il nuovo simbolo alla lista delle variabili implicite
   * del genitore parent (in modo da poterle eliminare quando
   * si vuole eliminare parent).
   */
  s->brother = ps->child;
  ps->child = s;

  /* Inizializzo il simbolo */
  s->symattr.is_explicit = 0;
  s->parent.imp = parent;
  s->symtype = VARIABLE;
  assert(new_sym != NULL);
  *new_sym = s;
  return Success;
}

/******************************************************************************
 *           FUNZIONI DI GESTIONE SIMBOLI (IMPLICITI ED ESPLICITI)            *
 ******************************************************************************/
/* Le funzioni che seguono servono per la creazione, rimozione e ricerca
 * di simboli espliciti ed impliciti.
 */

/* VARIABILI UTILIZZATE DA Sym_Find_... */
static Intg sym_find_box;
static Symbol *sym_found;

/**********************************[RICERCA]**********************************/

/* FUNZIONE INTERNA USATA DA Sym_Explicit_Find */
static UInt Sym__Explicit_Find(Symbol *s) {
  MSG_LOCATION("Sym__Explicit_Find");
  return (s->symattr.is_explicit) && (s->parent.exp == sym_find_box);
}

/* FUNZIONE INTERNA USATA DA Sym_Explicit_Find */
static UInt Sym__Explicit_Find_All(Symbol *s) {
  MSG_LOCATION("Sym__Explicit_Find");
  if ( s->symattr.is_explicit ) {
    if ( sym_found == NULL ) {
      if ( s->parent.exp <= sym_find_box) sym_found = s;
      return 0;
    } else {
      if ( (s->parent.exp <= sym_find_box)
       && (sym_found->parent.exp < s->parent.exp) )
        sym_found = s;
      return 0;
    }
  }
  return 0;
}

/* DESCRIZIONE: Questa funzione cerca un simbolo esplicito di nome *nm,
 *  appartenente alla scatola di livello box.
 *  Se box = EXACT_DEPTH cerca nella scatola di profondita' pari a depth,
 *  se box = NO_EXACT_DEPTH cerca a partire dalla scatola di profondita'
 *  depth e via via nelle scatole di profondita' superiore.
 */
Symbol *Sym_Explicit_Find(Name *nm, Intg depth, int mode) {
  MSG_LOCATION("Sym_Explicit_Find");
  if ( mode == EXACT_DEPTH ) {
    sym_find_box = cmp_box_level - depth;
    assert( (sym_find_box >= 0) && (depth >= 0) );
    return Sym_Symbol_Find(nm, Sym__Explicit_Find);

  } else {
    sym_found = NULL;
    sym_find_box = cmp_box_level - depth;
    (void) Sym_Symbol_Find(nm, Sym__Explicit_Find_All);
    return sym_found;
  }
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
