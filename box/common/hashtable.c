/*
 * Copyright (C) 2006 Matteo Franchin
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
 * This file contains all the functions required to handle hash-tables
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


int main(void) {
  HashTable *ht;
  TASK( Hash_New(ht, 100) );
  TASK( Hash_Insert(ht, Hash_Key("matteo", 6), NULL) );
  Hash_Key
  return 0;
}



UInt Hash_Key(void *key_data, UInt key_size, UInt table_size) {

}

/* This function creates a new hash-table.
 */
HashTable *Hash_New(unsigned int table_size) {

}

Task Hash_Destroy(HashTable *ht) {
}

/*
 */
Task Hash_Insert(HashTable *ht, int branch, HashKey *hk, void *object) {

}

Task Hash_Remove(HashTable *ht) {

}

Task Hash_Run_Through(HashTable *ht) {
}

Task Hash_Diagnostics(HashTable *ht) {
}

/* Search inside the hash-table ht, the object corresponding to the key '*k'.
 * If the object is found, its pointer is stored inside '**object' and 'Success'
 * is returned. If the object is not found the function returns 'Failed'.
 */
Task Hash_Find(HashTable *ht, void **object, Key *k) {
  Symbol *s;

  MSG_LOCATION("Hash_Find");

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
