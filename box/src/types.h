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

/* $Id$ */

/** @file types.h
 * @brief Basic definition of frequently used types.
 *
 * Some basic definitions which are used throughout all the Box compiler
 * and libraries.
 */

#ifndef _TYPES_H
#  define _TYPES_H

#  include <float.h>

/** The integer type of Box numbers and the integer type we will try
 * to use whenever possible: 64 bit on 64 bit architectures,
 * 32 bit elsewhere.
 */
typedef long Int;

/** The unsigned integer type that we will try to use whenever possible.
 * Same size of Int.
 */
typedef unsigned long UInt;

/* Qui definisco la "precisione" dei numeri interi e reali.
 * Dopo tali definizioni, definisco pure quelle macro che devono essere
 * cambiate, qualora si cambino le definizioni di Int e Real.
 */
typedef double Real; /* Numeri in virgola mobile */
/* Conversione stringa->Int */
#define strtoint strtol
/* Conversione stringa->Real */
#define strtoreal strtod

#define REAL_MAX DBL_MAX
#define REAL_MIN (-DBL_MAX)

/* Definisco il tipo Char, che e' esattamente un char */
typedef unsigned char Char;

/* Tipo di dato "punto" */
typedef struct {
  Real x, y;
} Point;

/** We need more than just a pointer when referring to Box objects */
typedef struct {
  void *ptr;   /**< Pointer to the data inside this block */
  void *block; /**< Pointer to the allocated memory block */
} Obj;

typedef void *Ptr;

/** A subtype is simply a structure containing two pointers: one points
 * to the parent, one to the child.
 */
typedef Obj Subtype[2];

/* in this macro 'subtype_ptr' should have type 'Subtype *' */
#define SUBTYPE_PARENT_PTR(subtype_ptr, parent_type) \
  ((parent_type *) ((*(subtype_ptr))[0].ptr))

/* in this macro 'subtype_ptr' should have type 'Subtype *' */
#define SUBTYPE_CHILD_PTR(subtype_ptr, child_type) \
  (( child_type *) ((*(subtype_ptr))[1].ptr))

#define NAME(str) ((Name) {sizeof(str)-1, str})
typedef struct {
  UInt length;
  char *text;
} Name;

typedef Name Data;

/* Stringhe da usare nelle printf per stampare i vari tipi */
#define SUInt "%lu"
#define SChar "%c"
#define SInt "%ld"
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

/** Macro to call a function returning Task from a function returning Task.
 */
#define TASK(x) if ( (x) ) return Failed

/* Error handling in Box is not always sensible, unfortunately.
 * There are situations where failures should not be tolerated. In such cases
 * the best thing to do is just terminate the program, but this is not done
 * and the error is propagated. We should change all the code and decide
 * for each situation if there is a reason why we should propagate the error
 * with task or if we should display a fatal error message and exit.
 * For now we just define the macro ASSERT_TASK to correct these cases
 * where propagating errors turn out to be unnecessary and a fatal error
 * is preferable.
 */

/** When we do not tolerate a failure then we use the ASSERT_TASK macro. */
#define ASSERT_TASK(x) assert(Success == (x))

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
