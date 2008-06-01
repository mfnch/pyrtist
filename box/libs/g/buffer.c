/****************************************************************************
 * Copyright (C) 2008 by Matteo Franchin                                    *
 *                                                                          *
 * This file is part of Box.                                                *
 *                                                                          *
 *   Box is free software: you can redistribute it and/or modify it         *
 *   under the terms of the GNU Lesser General Public License as published  *
 *   by the Free Software Foundation, either version 3 of the License, or   *
 *   (at your option) any later version.                                    *
 *                                                                          *
 *   Box is distributed in the hope that it will be useful,                 *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Lesser General Public License for more details.                    *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with Box.  If not, see <http://www.gnu.org/licenses/>.   *
 ****************************************************************************/

/* buffer.c - aprile 2002
 *
 * Questo file contiene il codice necessario per gestire un buffer.
 * Si possono inserire e togliere elementi senza preoccuparsi
 * dei ridimensionamenti del buffer.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* De-commentare per includere i messaggi di debug */
/*#define DEBUG*/

#include "debug.h"
#include "error.h"
#include "buffer.h"

/* Questa macro provvede ad allargare il buffer
 * in modo che riesca a contenere n elementi
 */
#define BUFF_EXPAND(buffer, n) \
  if ( n > buffer->dim ) { \
    if ( buffer->dim == 0 ) { \
      for( buffer->dim = buffer->mindim; \
       buffer->dim < n; buffer->dim *= 2); \
      buffer->size = (buffer->dim) * buffer->elsize; \
      buffer->ptr = malloc( buffer->size ); \
    } else { \
      for(; buffer->dim < n; buffer->dim *= 2); \
      buffer->size = (buffer->dim)*buffer->elsize; \
      buffer->ptr = realloc( buffer->ptr, buffer->size ); \
    } \
    if ( buffer->ptr == NULL ) { \
      FATALMSG("buffer.c", "Memoria esaurita"); \
      return 0; \
    }\
    }

/* Questa macro provvede a contrarre il buffer
 * qualora sia vuoto per piu' di 3/4
 * in modo tale che sia almeno mezzo pieno.
 * n e' il numero di elementi del buffer.
 */
#define BUFF_SHRINK(buffer, n) \
  if (buffer->dim > buffer->mindim) { \
    for(; 4*n < buffer->dim; buffer->dim /= 2); \
    if (buffer->dim < buffer->mindim) \
      buffer->dim = buffer->mindim; \
    buffer->size = (buffer->dim)*buffer->elsize; \
    buffer->ptr = realloc( buffer->ptr, buffer->size ); \
    if ( buffer->ptr == NULL ) { \
      ERRORMSG("buffer.c", "Problemi con realloc"); \
      return 0; \
    } \
    }

/* NOME: buff_create
 * DESCRIZIONE: Crea un buffer con:
 *  elsize = numero byte per elemento
 *  mindim = dimensione minima del buffer (in elementi)
 *  Restituisce 1 se tutto va bene, 0 in caso di errore.
 * NOTA: buffer e' un puntatore ad una struttura di tipo
 *  buff (gia' precedentemente allocata).
 */
int buff_create(buff *buffer, short elsize, long mindim) {
  if (elsize<1 || mindim<1) {
    ERRORMSG("buff_create", "Parametri errati");
    buffer->id = 0;
    return 0;
  }

  buffer->id = buffID;
  buffer->ptr = NULL;
  buffer->dim = 0;
  buffer->size = 0;
  buffer->numel = 0;
  buffer->mindim = mindim;
  buffer->elsize = elsize;

  return 1;
}

int buff_dup(buff *dest, buff *src) {
  if (src->id != buffID) {
    ERRORMSG("buff_recycle", "Buffer danneggiato");
    return 0;
  }

  *dest = *src;
  if (dest->size > 0 && dest->ptr != (void *) NULL) {
    dest->ptr = (void *) malloc(dest->size);
    (void) memcpy(dest->ptr, src->ptr, dest->size);
  }
  return 1;
}

/* NOME: buff_create
 * DESCRIZIONE: Converte il buffer specificato, in un nuovo buffer
 *  con elementi di dimensione diversa (elsize).
 *  mindim = dimensione minima del buffer (in elementi)
 *  Restituisce 1 se tutto va bene, 0 in caso di errore.
 * NOTA: Il buffer creato sara' vuoto.
 */
int buff_recycle(buff *buffer, short elsize, long mindim) {
  if ( (elsize<1) || (mindim<1) ) {
    ERRORMSG("buff_recycle", "Parametri errati");
    return 0;
  }

  if ( buffer->id != buffID ) {
    ERRORMSG("buff_recycle", "Buffer danneggiato");
    return 0;
  }

  /* Calcolo quanti elementi di dimensione elsize ci stanno nella zona
   * allocata per il vecchio buffer
   */
  buffer->dim = buffer->size / elsize;
  if ( buffer->dim < 1 ) {
    /* Non c'e' memoria sufficiente:
     * distruggo il vecchio buffer e ne creo uno nuovo!
     */
    free( buffer->ptr );
    buffer->id = 0;

    return buff_create(buffer, elsize, mindim);
  }

  buffer->numel = 0;
  buffer->mindim = mindim;
  buffer->elsize = elsize;
  return 1;
}

/* NOME: buff_push
 * DESCRIZIONE: Inserisce un elemento in coda nel buffer,
 *  preoccupandosi di reallocare la memoria se necessario.
 *  Restituisce 1 solo in caso di successo.
 */
int buff_push(buff *buffer, void *elem) {
  void *tptr;
  long tpos;

  if (buffer->id == buffID) {
    tpos = buffer->numel++ * buffer->elsize;

    /* Controlla che il buffer non debba essere allargato */
    BUFF_EXPAND(buffer, buffer->numel);

    tptr = buffer->ptr + tpos;
    memcpy(tptr, elem, buffer->elsize);

  } else {
    ERRORMSG("buff_push", "Buffer non inizializzato");
    return 0;
  }

  return 1;
}

/* NOME: buff_mpush
 * DESCRIZIONE: Inserisce pi elementi in coda nel buffer,
 *  preoccupandosi di reallocare la memoria se necessario.
 *  Restituisce 1 solo in caso di successo.
 */
int buff_mpush(buff *buffer, void *elem, long numel) {
  void *tptr;
  long tpos;

  if (buffer->id == buffID) {
    if (numel < 1) {
      ERRORMSG("buff_mpush", "Parametri errati");
      return 0;
    }

    tpos = buffer->numel * buffer->elsize;
    buffer->numel += numel;

    /* Controlla che il buffer non debba essere allargato */
    BUFF_EXPAND(buffer, buffer->numel);

    tptr = buffer->ptr + tpos;
    memcpy(tptr, elem, numel*buffer->elsize);

  } else {
    ERRORMSG("buff_mpush", "Buffer non inizializzato");
    return 0;
  }

  return 1;
}

/* NOME: buff_bigenough
 * DESCRIZIONE: Se necessario allarga il buffer in modo che contenga
 *  numel elementi. Restituisce 1 in caso di successo.
 */
int buff_bigenough(buff *buffer, long numel) {
  PRNMSG("buff_bigenough: Controllo se e' necessario allargare il buffer\n...");
  if (buffer->id == buffID) {
    if (numel < 0) {
      ERRORMSG("buff_bigenough", "Parametri errati");
      EXIT_ERR("numel < 0!\n");
    }

    PRNMSG("Provo...\n");
    /* Controlla che il buffer non debba essere allargato */
    BUFF_EXPAND(buffer, numel);
    PRNMSG("Fatto!\n");

  } else {
    ERRORMSG("buff_bigenough", "Buffer non inizializzato");
    EXIT_ERR("buffer distrutto o non inizializzato!\n");
  }

  EXIT_OK("Ok!\n");
}

/* NOME: buff_smallenough
 * DESCRIZIONE: Se necessario riduce il buffer in modo che non sia
 *  troppo vuoto. Restituisce 1 in caso di successo.
 */
int buff_smallenough(buff *buffer, long numel) {
  if (buffer->id == buffID) {
    if (numel < 0) {
      ERRORMSG("buff_smallenough", "Parametri errati");
      return 0;
    }

    /* Controlla che il buffer non debba essere allargato */
    BUFF_SHRINK(buffer, numel);

  } else {
    ERRORMSG("buff_smallenough", "Buffer non inizializzato");
    return 0;
  }

  return 1;
}

/* NOME: buff_clear
 * DESCRIZIONE: Svuota il buffer, riallocandolo se la sua dimensione
 *  supera la dimensione "di riposo".
 *  Restituisce 1 in caso di successo.
 */
int buff_clear(buff *buffer) {
  if (buff_smallenough(buffer, 0))
    buffer->numel = 0;
  else
    return 0;

  return 1;
}

/* NOME: buff_free
 * DESCRIZIONE: De-alloca il buffer, riportandolo alla condizione in cui
 *  buff_create lo aveva posto.
 */
void buff_free(buff *buffer) {
  if (buffer->id == buffID) {
    free(buffer->ptr);
    buffer->ptr = NULL;
    buffer->dim = 0;
    buffer->size = 0;
    buffer->numel = 0;
  }
}
