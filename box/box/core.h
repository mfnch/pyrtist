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

/** The BoxChar type, the smallest integer in terms of size. */
typedef unsigned char BoxChar;

/** The integer type of Box numbers and the integer type we will try
 * to use whenever possible: this corresponds to the C long int.
 */
typedef long BoxInt;

/** The unsigned integer type that we will try to use whenever possible.
 * Same size of Int.
 */
typedef unsigned long BoxUInt;

/** Here is the definition of the floating point type used by Box.
 */
typedef double BoxReal;

#endif
/* ^^^ temporary stuff. */

#  include <box/ntypes.h>

/**
 * Box core types.
 */
typedef struct BoxCoreTypes_struct {
  BoxType root_type,
          init_type,
          finish_type,
          type_type,
          callable_type,
          char_type,
          int_type,
          real_type,
          point_type,
          pointer_type,
          any_type,
          str_type,
          repr_type,
          stream_type,
          hash_type,
          serialize_type,
          deserialize_type;
} BoxCoreTypes;

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

#endif /* _BOX_CORE_H */
