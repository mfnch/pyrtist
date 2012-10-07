/****************************************************************************
 * Copyright (C) 2012 by Matteo Franchin                                    *
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

/**
 * @file core.h
 * @brief Definition of core types and objects.
 *
 * This module defines Box core types and objects.
 */

#ifndef _BOX_CORE_H
#  define _BOX_CORE_H

#ifndef NTYPES
/* ^^^ temporary stuff. */

#  include <float.h>
#  include <stdlib.h>

/* Detect whether we are being compiled on MS Windows platforms */
#  ifndef __WINDOWS__
#    if defined(WIN32) || defined(_WIN32)
#      define __WINDOWS__
#    endif
#  endif

#  ifdef __WINDOWS__
/**
 * Macro to use as a prefix when declaring functions in a shared library that
 * are meant to be used from outside callers.
 */
#    define BOXEXPORT extern __declspec(dllexport)
#  else
#    define BOXEXPORT extern
#  endif

/**
 * The BoxChar type, the smallest integer in terms of size.
 */
typedef unsigned char BoxChar;

/**
 * The integer type of Box numbers and the integer type we will try
 * to use whenever possible: this corresponds to the C long int.
 */
typedef long BoxInt;

/**
 * The unsigned integer type that we will try to use whenever possible.
 * Same size of Int.
 */
typedef unsigned long BoxUInt;

/**
 * Fundamental boolean type used in the Box compiler API.
 */
typedef int BoxBool;

enum {
  BOXBOOL_FALSE=0,
  BOXBOOL_TRUE=1
};

/**
 * Here is the definition of the floating point type used by Box.
 */
typedef double BoxReal;

/**
 * Conversion string --> Int
 */
#  define BoxInt_Of_Str strtol

/**
 * Conversion string --> Real
 */
#  define BoxReal_Of_Str strtod

#  define BOXREAL_MAX DBL_MAX
#  define BOXREAL_MIN (-DBL_MAX)

/**
 * The 2D point type.
 */
typedef struct BoxPoint_struct {
  BoxReal x, y;
} BoxPoint;

/**
 * We need more than just a pointer when referring to Box objects.
 */
typedef struct BoxPtr_struct {
  void *ptr,   /**< Pointer to the data inside this block */
       *block; /**< Pointer to the allocated memory block */
} BoxPtr;

/** Reset an extended BoxPtr pointer to NULL (point to nothing) */
#  define BoxPtr_Init(p) \
  do {(p)->block = (p)->ptr = NULL;} while(0)

#  define BoxPtr_Nullify BoxPtr_Init

/** Detach a pointer so that it does not reference its source */
#  define BoxPtr_Detach(p) \
  do {(p)->block = NULL;} while(0)

/** Whether a BoxPtr pointer is NULL */
#  define BoxPtr_Is_Null(p) ((p)->ptr == NULL)

/** Whether a BoxPtr pointer is detached (not bound to a memory block) */
#  define BoxPtr_Is_Detached(p) ((p)->block == NULL)

/** Get the target pointer associated to the given BoxPtr extended pointer. */
#  define BoxPtr_Get_Target(p) ((p)->ptr)

/** Get the block of the given BoxPtr extended pointer. */
#  define BoxPtr_Get_Block(p) ((p)->block)

/**
 * This macro expands to nothing. It is used in function prototypes to indicate
 * that the declared function steals a reference to the passed object. This
 * macro has hence a purely aesthetical/declarative purpose.
 */
#define BOXIN

/**
 * This macro expands to nothing. It is used in function prototypes to indicate
 * that the declared function provides a newly created or existing object. This
 * macro has hence a purely aesthetical/declarative purpose.
 */
#define BOXOUT

/** Type representing C pointers from Box */
typedef void *BoxCPtr;

/**
 * A subtype is simply a structure containing two pointers: one points
 * to the parent, one to the child.
 */
typedef struct {
  BoxPtr child, parent;
} BoxSubtype;

/**
 * Union of all the intrinsic Box types.
 */
typedef union {
  BoxChar  box_char;
  BoxInt   box_int;
  BoxReal  box_real;
  BoxPoint box_point;
  BoxPtr   box_obj;
} BoxValue;

/* Strings containing the printf formats for the various types */
#  define BoxChar_Fmt "%c"
#  define BoxUInt_Fmt "%lu"
#  define BoxInt_Fmt "%ld"
#  define BoxReal_Fmt "%g"
#  define BoxPoint_Fmt "(%g, %g)"

#endif
/* ^^^ temporary stuff. */

typedef struct BoxCoreTypes_struct BoxCoreTypes;

/**
 * Object containing all the core types of Box.
 */
extern BoxCoreTypes box_core_types;

/**
 * Initialize the core types of Box.
 */
BOXEXPORT BoxBool
BoxCoreTypes_Init(BoxCoreTypes *core_types);

/**
 * Finalize the core type of Box.
 */
BOXEXPORT void
BoxCoreTypes_Finish(BoxCoreTypes *core_types);

/**
 * Initialize the type system.
 */
BOXEXPORT BoxBool
Box_Initialize_Type_System(void);

/**
 * Initialize the type system.
 */
BOXEXPORT void
Box_Finalize_Type_System(void);

#    define BOX_FATAL_ERROR() Box_Fatal_Error(__FILE__, __LINE__)

#endif /* _BOX_CORE_H */
