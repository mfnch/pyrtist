/* array.c - Autore: Franchin Matteo - 3 maggio 2004
 *
 * Questo file contiene il codice necessario per gestire un array.
 * Si possono inserire e togliere elementi senza preoccuparsi
 * dei ridimensionamenti dell'array (che avviene automaticamente).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* De-commentare per includere i messaggi di debug */
/*#define DEBUG*/
/*#define DEBUG_RESIZE*/

#include "types.h"
#include "debug.h"
#include "messages.h"
#include "array.h"

#ifdef DEBUG_RESIZE
#define DEBUG_MSG(msg) printf(msg)
#else
#define DEBUG_MSG(msg)
#endif

/* Questa macro provvede ad allargare l'array
 * in modo che riesca a contenere n elementi
 */
#define ARRAY_EXPAND(a, n)                               \
  if ( n > a->dim ) {                                    \
    if ( a->dim == 0 ) {                                 \
      for( a->dim = a->mindim; a->dim < n; a->dim *= 2); \
      a->size = (a->dim) * a->elsize;                    \
      DEBUG_MSG("ARRAY_EXPAND: array allocated!\n");     \
      a->ptr = malloc( a->size );                        \
    } else {                                             \
      for(; a->dim < n; a->dim *= 2);                    \
      a->size = (a->dim)*a->elsize;                      \
      a->ptr = realloc( a->ptr, a->size );               \
      DEBUG_MSG("ARRAY_EXPAND: array resized!\n");       \
    }                                                    \
    if ( a->ptr == NULL ) {                              \
      MSG_FATAL("Memoria esaurita");                     \
      return Failed;                                     \
    }                                                    \
  }

/* Questa macro provvede a contrarre l'array
 * qualora sia vuota per piu' di 3/4
 * in modo tale che sia almeno mezza piena.
 * n e' il numero di elementi dell'array.
 */
#define ARRAY_SHRINK(a, n)                               \
  if (a->dim > a->mindim) {                              \
    for(; 4*n < a->dim; a->dim /= 2);                    \
    if (a->dim < a->mindim)                              \
      a->dim = a->mindim;                                \
    a->size = (a->dim)*a->elsize;                        \
    a->ptr = realloc( a->ptr, a->size );                 \
    DEBUG_MSG("ARRAY_SHRINK: array resized!\n");         \
    if ( a->ptr == NULL ) {                              \
      MSG_ERROR("Problemi con realloc");                 \
      return Failed;                                     \
    }                                                    \
  }

/* DESCRIZIONE: Crea una nuova array con:
 *  elsize = numero byte per elemento
 *  mindim = dimensione minima dell'array (in elementi)
 *  Restituisce il puntatore all'array (tipo Array *) se tutto va bene,
 *  NULL in caso di errore.
 */
Array *Array_New(UInt elsize, UInt mindim) {
  Array *a;

  MSG_LOCATION("Array_New");

  if ( elsize < 1 ) {
    MSG_ERROR("Parametri errati");
    return NULL;
  }

  a = (Array *) malloc( sizeof(Array) );
  if ( a == NULL ) {
    MSG_ERROR("Memoria esaurita");
    return NULL;
  }

  a->ID = ARR_ID;
  a->ptr = NULL;
  a->dim = 0;
  a->size = 0;
  a->numel = 0;
  a->mindim = mindim;
  a->elsize = elsize;

  return a;
}

/* DESCRIZIONE: Converte l'array specificata, in una nuova array
 *  con elementi di dimensione diversa (elsize).
 *  mindim = dimensione minima dell'array (in elementi)
 *  Restituisce il puntatore alla nuova array se tutto va bene,
 *  NULL in caso di errore.
 * NOTA: L'array creata sara' vuota.
 */
Array *Arr_Recycle(Array *a, UInt elsize, UInt mindim) {
  MSG_LOCATION("Arr_Recycle");

  if ( elsize < 1 ) {
  MSG_ERROR("Parametri errati");
  return NULL;
  }

  if ( a->ID != ARR_ID ) {
    MSG_ERROR("Array danneggiato");
    return NULL;
  }

  /* Calcolo quanti elementi di dimensione elsize ci stanno nella zona
    * allocata per la vecchia array
    */
  a->dim = a->size / elsize;
  if ( a->dim < 1 ) {
    /* Non c'e' memoria sufficiente:
      * distruggo la vecchia array e ne creo una nuova!
      */
    free( a->ptr );
    a->ID = 0;
    return Array_New(elsize, mindim);
  }

  a->numel = 0;
  a->mindim = mindim;
  a->elsize = elsize;
  return a;
}

/* DESCRIZIONE: Inserisce un elemento in coda nel array,
 *  preoccupandosi di reallocare la memoria se necessario.
 *  Restituisce 1 solo in caso di successo.
 */
Task Arr_Push(Array *a, void *elem)
{
  MSG_LOCATION("Arr_Push");
  if (a->ID == ARR_ID) {
    void *tptr;
    UInt tpos = a->numel++ * a->elsize;

    /* Controlla che l'array non debba essere allargata */
    ARRAY_EXPAND(a, a->numel);

    tptr = a->ptr + tpos;
    memcpy(tptr, elem, a->elsize);
    return Success;

  } else {
    MSG_ERROR("Array non inizializzata");
    return Failed;
  }

  return Failed;
}

/* DESCRIZIONE: Inserisce piu' elementi in coda nell'array,
 *  preoccupandosi di reallocare la memoria se necessario.
 *  Restituisce 1 solo in caso di successo.
 */
Task Arr_MPush(Array *a, void *elem, UInt numel)
{
	MSG_LOCATION("Arr_MPush");

	if (numel < 1) {
		MSG_ERROR("Parametri errati");
		return Failed;
	}

	if (a->ID == ARR_ID) {
		void *tptr;
		UInt tpos = a->numel * a->elsize;

		a->numel += numel;

		/* Controlla che l'array non debba essere allargata */
		ARRAY_EXPAND(a, a->numel);

		tptr = a->ptr + tpos;
		memcpy(tptr, elem, numel * a->elsize);
		return Success;

	} else {
		MSG_ERROR("Array non inizializzata");
		return Failed;
	}

	return Failed;
}

/* DESCRIZIONE: Se necessario allarga l'array in modo che contenga
 *  numel elementi. Restituisce 1 in caso di successo.
 */
Task Arr_BigEnough(Array *a, UInt numel) {
  MSG_LOCATION("Arr_BigEnough");

  if (a->ID == ARR_ID) {
    if (numel < 0) {
      MSG_ERROR("Parametri errati");
      return Failed;
    }

    /* Controlla che l'array non debba essere allargata  */
    ARRAY_EXPAND(a, numel);
    return Success;

  } else {
    MSG_ERROR("Array non inizializzata");
    return Failed;
  }

  return Failed;
}

/* DESCRIZIONE: Se necessario riduce l'array in modo che non sia
 *  troppo vuota. Restituisce 1 in caso di successo.
 */
Task Arr_SmallEnough(Array *a, UInt numel) {
  MSG_LOCATION("Arr_SmallEnough");

  if (a->ID == ARR_ID) {
    if (numel < 0) {
      MSG_ERROR("Parametri errati");
      return Failed;
    }

    /* Controlla che l'array non debba essere allargato */
    ARRAY_SHRINK(a, numel);
    return Success;

  } else {
    MSG_ERROR("Array non inizializzata");
    return Failed;
  }

  return Failed;
}

/* DESCRIPTION: This function inserts how_many items (items is the pointer
 *  to these items) into the array a at position where.
 */
Task Arr_Insert(Array *a, Intg where, Intg how_many, void *items) {
  MSG_LOCATION("Arr_Insert");

  if ( how_many < 1 ) return Success;
  if ( a->ID == ARR_ID ) {
    register Intg numel = a->numel, new_dim, elsize = a->elsize, to_move;
    register void *src;

    if (where < 1) {
      MSG_ERROR("Punto di inserimento negativo!"); return Failed;

    } else if (where > numel) {
      new_dim = where + how_many - 1;
      to_move = 0;

    } else {
      new_dim = numel + how_many;
      to_move = numel - where + 1;
    }

    /* Now we expand the array */
    ARRAY_EXPAND(a, (a->numel = new_dim));

    /* Now we create the space to contain the items to be inserted */
    src = a->ptr + ((where-1) * elsize);
    numel = how_many * elsize;
    if ( to_move > 0 ) (void) memmove(src + numel, src, to_move*elsize);

    /* Now we insert the items */
    (void) memcpy(src, items, numel);
    return Success;

  } else {
      MSG_ERROR("Array non inizializzata");
      return Failed;
  }
}

/* DESCRIPTION: This function appends to the queque of the array a,
 *  how_many empty elements (initialized with 0).
 */
Task Arr_Empty(Array *a, Intg how_many) {
  MSG_LOCATION("Arr_Empty");
    /* Very similar to Arr_Push */
  if (a->ID == ARR_ID) {
    UInt tpos;
    if ( how_many < 1 ) return Success;
    tpos = a->numel * a->elsize;
    ARRAY_EXPAND(a, (a->numel += how_many));
    (void) memset(a->ptr + tpos, 0, how_many * a->elsize);
    return Success;

  } else {
    MSG_ERROR("Array non inizializzata");
    return Failed;
  }
}

/* DESCRIZIONE: Svuota l'array, riallocandola se la sua dimensione
 *  supera la dimensione "di riposo".
 *  Restituisce 1 in caso di successo.
 */
Task Arr_Clear(Array *a) {
  if IS_SUCCESSFUL(Arr_SmallEnough(a, 0)) {
    a->numel = 0;
    return Success;
  }
  return Failed;
}

/* DESCRIZIONE: Distrugge l'array.
 */
void Arr_Destroy(Array *a) {
  if ( a != NULL) {
    if (a->ID == ARR_ID) {
      free(a->ptr);
      free(a);
    }
  }
  return;
}

/* DESCRIZIONE: Distrugge l'array, mantenendo pero' l'area di memoria
 *  ad essa associata. Restituisce proprio il puntatore a tale area.
 * NOTA: il puntatore a sara' inutilizzabile dopo l'esecuzione di questa
 *  funzione.
 */
void *Arr_Data_Only(Array *a) {
  void *data_ptr;

  if ( a != NULL) {
    if (a->ID == ARR_ID) {
      data_ptr = a->ptr;
      free(a);
      return data_ptr;
    }
  }
  return NULL;
}
