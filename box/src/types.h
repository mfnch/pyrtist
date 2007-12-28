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
#ifndef _TYPES_H
#  define _TYPES_H

/** The integer type of Box numbers and the integer type we will try
 * to use whenever possible: 64 bit on 64 bit architectures,
 * 32 bit elsewhere.
 */
typedef long Int;
typedef long Intg; /**< Alias for Int, which should disappear, one day... */

/** The unsigned integer type that we will try to use whenever possible.
 * Same size of Int.
 */
typedef unsigned long UInt;

/** A subtype is simply a structure containing two pointer: one points
 * to the parent, one to the child.
 */
typedef void *Subtype[2];

/* Qui definisco la "precisione" dei numeri interi e reali.
 * Dopo tali definizioni, definisco pure quelle macro che devono essere
 * cambiate, qualora si cambino le definizioni di Intg e Real.
 */
typedef double Real; /* Numeri in virgola mobile */
#define strtointg strtol /* Conversione stringa->Intg */
#define strtoreal strtod /* Conversione stringa->Real */

/* Definisco il tipo Char, che e' esattamente un char */
typedef unsigned char Char;

/* Tipo di dato "punto" */
typedef struct {
  Real x, y;
} Point;

#define NAME(str) ((Name) {sizeof(str)-1, str})
typedef struct {
  UInt length;
  char *text;
} Name;

typedef Name Data;

/* Stringhe da usare nelle printf per stampare i vari tipi */
#define SUInt "%lu"
#define SChar "%c"
#define SIntg "%ld"
#define SInt "%ld"
#define SReal "%g"
#define SPoint "(%g, %g)"

/* Le funzioni spesso restituiscono un intero per indicare se tutto e' andato
 * bene o se si e' verificato un errore. Definisco il relativo tipo di dato:
 */
typedef enum {Success = 0, Failed = 1} Task;

/* Macro per testare il successo o il fallimento di una funzione.
 */
#define IS_SUCCESSFUL(x) (!x)
#define IS_FAILED(x) (x)

/* Macro per chiamare una funzione che restituisce Task,
 * da una funzione che restituisce Task.
 */
#define TASK(x) if ( x ) return Failed

/* DESCRIZIONE: Questa macro permette di usare una indicizzazione "circolare",
 *  secondo cui, data una lista di num_items elementi, l'indice 1 si riferisce
 *  al primo elemento, 2 al secondo, ..., num_items all'ultimo, num_items+1 al primo,
 *  num_items+2 al secondo, ...
 *  Inoltre l'indice 0 si riferisce all'ultimo elemento, -1 al pen'ultimo, ...
 */
#define CIRCULAR_INDEX(num_items, index) \
  ( (index > 0) ? \
    ( (index - 1) % num_items ) \
    : num_items - 1 - ( (-index) % num_items ) \
  )
#endif
