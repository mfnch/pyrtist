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

/* For now we always define the abbreviations */
#  ifndef BOX_ABBREV
#    define BOX_ABBREV
#  endif

/** The BoxChar type, the smallest integer in terms of size. */
typedef unsigned char BoxChar;

/** The integer type of Box numbers and the integer type we will try
 * to use whenever possible: 64 bit on 64 bit architectures,
 * 32 bit elsewhere.
 */
typedef long BoxInt;

/** The unsigned integer type that we will try to use whenever possible.
 * Same size of Int.
 */
typedef unsigned long BoxUInt;

/** Here is the definition of the floating point type used by Box.
 */
typedef double BoxReal;

/** Conversion string --> Int */
#  define BoxInt_Of_Str strtol

/** Conversion string --> Real  */
#  define BoxReal_Of_Str strtod

#  define BOXREAL_MAX DBL_MAX
#  define BOXREAL_MIN (-DBL_MAX)

/** Used in the compiler to represent Box types. */
typedef BoxInt BoxType;

/** The 2D point type */
typedef struct {
  BoxReal x, y;
} BoxPoint;

/** We need more than just a pointer when referring to Box objects */
typedef struct {
  void *ptr;   /**< Pointer to the data inside this block */
  void *block; /**< Pointer to the allocated memory block */
} BoxObj;

/** Union of all the intrinsic Box types */
typedef union {
  BoxChar boxchar;
  BoxInt boxint;
  BoxReal boxreal;
  BoxPoint boxpoint;
  BoxObj boxobj;
} BoxValue;

typedef void *BoxPtr;

/** Strings containing the printf formats for the various types */
#  define BoxChar_Fmt "%c"
#  define BoxUInt_Fmt "%lu"
#  define BoxInt_Fmt "%ld"
#  define BoxReal_Fmt "%g"
#  define BoxPoint_Fmt "(%g, %g)"

/** A subtype is simply a structure containing two pointers: one points
 * to the parent, one to the child.
 */
typedef BoxObj BoxSubtype[2];

/** in this macro 'subtype_ptr' should have type 'Subtype *' */
#  define BOXSUBTYPE_PARENT_PTR(subtype_ptr, parent_type) \
  ((parent_type *) ((*(subtype_ptr))[0].ptr))

/** in this macro 'subtype_ptr' should have type 'Subtype *' */
#  define BOXSUBTYPE_CHILD_PTR(subtype_ptr, child_type) \
  (( child_type *) ((*(subtype_ptr))[1].ptr))

typedef struct {
  BoxInt length;
  char *text;
} BoxName;

#  define BOXNAME(str) ((BoxName) {sizeof(str)-1, str})

typedef BoxName BoxData;

/* Le funzioni spesso restituiscono un intero per indicare se tutto e' andato
 * bene o se si e' verificato un errore. Definisco il relativo tipo di dato:
 */
typedef enum {BoxSuccess = 0, BoxFailure = 1} BoxTask;

/* Macro per testare il successo o il fallimento di una funzione.
 */
#  define BOXTASK_IS_SUCCESS(x) (!x)
#  define BOXTASK_IS_FAILURE(x) (x)

/** Macro to call a function returning Task from a function returning Task.
 */
#  define BOXTASK(x) if ( (x) ) return BoxFailure

/* Questa macro permette di usare una indicizzazione "circolare",
 * secondo cui, data una lista di num_items elementi, l'indice 1 si riferisce
 * al primo elemento, 2 al secondo, ..., num_items all'ultimo, num_items+1 al primo,
 * num_items+2 al secondo, ...
 * Inoltre l'indice 0 si riferisce all'ultimo elemento, -1 al pen'ultimo, ...
 */
#define BOX_CIRCULAR_INDEX(num_items, index) \
  ( (index > 0) ? \
    ( (index - 1) % num_items ) \
    : num_items - 1 - ((-index) % num_items) )

/** Use shorthands (without the Box prefix) */
#ifdef BOX_ABBREV
typedef BoxChar Char;
typedef BoxInt Int;
typedef BoxUInt UInt;
typedef BoxReal Real;
typedef BoxPoint Point;
typedef BoxObj Obj;
typedef BoxPtr Ptr;
typedef BoxSubtype Subtype;

typedef BoxName Name;
typedef BoxName Data;

#    define Task BoxTask
#    define Success BoxSuccess
#    define Failed BoxFailure

#    define strtoint BoxInt_Of_Str
#    define strtoreal BoxReal_Of_Str
#    define REAL_MAX BOXREAL_MAX
#    define REAL_MIN BOXREAL_MIN
#    define SUInt BoxUInt_Fmt
#    define SChar BoxChar_Fmt
#    define SInt BoxInt_Fmt
#    define SReal BoxReal_Fmt
#    define SPoint BoxPoint_Fmt

#    define SUBTYPE_PARENT_PTR BOXSUBTYPE_PARENT_PTR
#    define SUBTYPE_CHILD_PTR BOXSUBTYPE_CHILD_PTR
#    define NAME BOXNAME
#    define IS_SUCCESSFUL BOXTASK_IS_SUCCESS
#    define IS_FAILED BOXTASK_IS_FAILURE
#    define TASK BOXTASK

#    define CIRCULAR_INDEX BOX_CIRCULAR_INDEX

/* Error handling in Box is not always sensible, unfortunately.
 * There are situations where failures should not be tolerated. In such cases
 * the best thing to do is just terminate the program, but this is not done
 * and the error is propagated. We should change all the code and decide
 * for each situation if there is a reason why we should propagate the error
 * with task or if we should display a fatal error message and exit.
 * For now we just define the macro ASSERT_TASK to correct these cases
 * where propagating errors turns out to be unnecessary and a fatal error
 * is preferable.
 */

/** When we do not tolerate a failure then we use the ASSERT_TASK macro. */
#    define ASSERT_TASK(x) assert(Success == (x))

#  endif /* BOX_ABBREV */

#endif /* _TYPES_H */
