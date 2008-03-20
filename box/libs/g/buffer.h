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

/* Questo file contiene le definizioni necessarie per poter
 * gestire le chiamate a buffer.c
 */

#ifndef _BUFFER_H

#  define _BUFFER_H

/* Struttura di controllo del buffer */
typedef struct {
  long id;      /* Costante identificativa di struttura inizializzata */
  void *ptr;    /* Puntatore alla zona di memoria che contiene i dati */
  long dim;     /* Numero massimo di elementi contenibili nel buffer */
  long size;    /* Dimensione in bytes della zona di memoria allocata */
  long mindim;  /* Valore minimo di dim */
  short elsize; /* Dimensione in bytes di ogni elemento */
  long numel;   /* Numero di elementi attualmente inseriti */
  long chain;   /* Quantita' inutilizzata (utile per eventuali catene di elementi) */
} buff;

/* Procedure definite in buffer.c */
int buff_create(buff *buffer, short elsize, long mindim);
int buff_dup(buff *dest, buff *src);
int buff_recycle(buff *buffer, short elsize, long mindim);
int buff_push(buff *buffer, void *elem);
int buff_mpush(buff *buffer, void *elem, long numel);
int buff_bigenough(buff *buffer, long numel);
int buff_smallenough(buff *buffer, long numel);
int buff_clear(buff *buffer);
void buff_free(buff *buffer);

#define buffID    0x66626468

#define buff_just_allocated(buffer) buffer->id = 0
#define buff_created(buffer)  ((buffer)->id == buffID ? 1 : 0)
#define buff_chain(buffer)    ((buffer)->chain)
#define buff_numitem(buffer)  ((buffer)->numel)
#define buff_ptr(buffer)    ((buffer)->ptr)
#define buff_item(buffer, type, n)  *((type *) ((buffer)->ptr + ((n)-1)*((long) (buffer)->elsize)))
#define buff_firstitem(buffer, type)  *((type *) ((buffer)->ptr))
#define buff_lastitem(buffer, type)  *((type *) ((buffer)->ptr + ((buffer)->numel-1)*((long) (buffer)->elsize)))
#define buff_itemptr(buffer, type, n)  ((type *) ((buffer)->ptr + ((n)-1)*((long) (buffer)->elsize)))
#define buff_firstitemptr(buffer, type)  ((type *) ((buffer)->ptr))
#define buff_lastitemptr(buffer, type)  ((type *) ((buffer)->ptr + ((buffer)->numel-1)*((long) (buffer)->elsize)))
#define buff_dec(buffer)  buff_smallenough((buffer), --(buffer)->numel)

#define buff_assert(lh) assert((buffer)->ID == buffID)

#endif
