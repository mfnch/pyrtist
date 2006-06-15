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

/* array.h - 3 maggio 2004 */

#ifndef _ARRAY_H
#define _ARRAY_H

/* Struttura di controllo dell'array */
typedef struct {
  long ID;      /* Costante identificativa di struttura inizializzata */
  void *ptr;    /* Puntatore alla zona di memoria che contiene i dati */
  long dim;     /* Numero massimo di elementi contenibili */
  long size;    /* Dimensione in bytes della zona di memoria allocata */
  long mindim;  /* Valore minimo di dim */
  short elsize; /* Dimensione in bytes di ogni elemento */
  long numel;   /* Numero di elementi attualmente inseriti */
  Task (*destroy)(void *); /* Quantita riservata all'estensione Chest */
  long chain;   /* Quantita riservata all'estensione Chest */
  long max_idx; /* Quantita riservata all'estensione Chest */
} Array;

/* Procedure definite in array.c */
Array *Array_New(UInt elsize, UInt mindim);
Task Arr_New(Array **new_array, UInt elsize, UInt mindim);
Array *Arr_Recycle(Array *a, UInt elsize, UInt mindim);
Task Arr_Push(Array *a, void *elem);
Task Arr_MPush(Array *a, void *elem, UInt numel);
Task Arr_Insert(Array *a, Intg where, Intg how_many, void *items);
Task Arr_Empty(Array *a, Intg how_many);
Task Arr_BigEnough(Array *a, UInt numel);
Task Arr_SmallEnough(Array *a, UInt numel);
Task Arr_Clear(Array *a);
void Arr_Destroy(Array *a);
Task Arr_Data_Only(Array *a, void **data_ptr);
Task Arr_Iter(Array *a, Task (*action)(void *));
Task Arr_Overwrite(Array *a, Intg dest, void *src, UInt n);

/* Valore che contrassegna le array correttamente inizializzate */
#define ARR_ID 0x66626468

/* Macro utili */
#define Arr_Chain(a)	((a)->chain)
#define Arr_NumItem(a)	((a)->numel)
#define Arr_Ptr(a)		((a)->ptr)
#define Arr_Item(a, type, n)	*((type *) ((a)->ptr + ((n)-1)*((UInt) (a)->elsize)))
#define Arr_FirstItem(a, type)	*((type *) ((a)->ptr))
#define Arr_LastItem(a, type)	*((type *) ((a)->ptr + ((a)->numel-1)*((UInt) (a)->elsize)))
#define Arr_ItemPtr(a, type, n) ((type *) ((a)->ptr + ((n)-1)*((UInt) (a)->elsize)))
#define Arr_FirstItemPtr(a, type)	((type *) ((a)->ptr))
#define Arr_LastItemPtr(a, type)	((type *) ((a)->ptr + ((a)->numel-1)*((UInt) (a)->elsize)))
#define Arr_Dec(a) Arr_SmallEnough((a), --(a)->numel)
#define Arr_Inc(a) Arr_BigEnough((a), ++(a)->numel)
#define Arr_MDec(a, n) Arr_SmallEnough((a), (a)->numel -= n)
#define Arr_MInc(a, n) Arr_BigEnough((a), (a)->numel += n)
#endif
