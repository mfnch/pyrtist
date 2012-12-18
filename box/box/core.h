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
 * @brief Integers associated to the fundamental types. These constant values
 *   are used internally for caching combinations and - in general - for
 *   speeding up the type system (or at least speeding up usage of fundamental
 *   types).
 */
typedef enum {
  BOXTYPEID_NONE  =-1,
  BOXTYPEID_MIN_VAL=0,
  BOXTYPEID_FAST_LOWER = 0,
  BOXTYPEID_CHAR  = 0,
  BOXTYPEID_INT   = 1,
  BOXTYPEID_REAL  = 2,
  BOXTYPEID_POINT = 3,
  BOXTYPEID_PTR   = 4,
  BOXTYPEID_FAST_UPPER = 4,
  BOXTYPEID_OBJ   = 5,
  BOXTYPEID_VOID,
  BOXTYPEID_INIT,
  BOXTYPEID_FINISH,
  BOXTYPEID_COPY,
  BOXTYPEID_BEGIN,
  BOXTYPEID_END,
  BOXTYPEID_PAUSE,
  BOXTYPEID_CPTR,
  BOXTYPEID_TYPE,
  BOXTYPEID_ANY,
  BOXTYPEID_MAX_VAL,
} BoxTypeId;

/**
 * @brief A type in the Box type system. This is currently implemented as a
 *   pointer to an opaque structure.
 */
typedef struct BoxType_struct BoxType;

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
#  define BOXIN

/**
 * This macro expands to nothing. It is used in function prototypes to indicate
 * that the declared function provides a newly created or existing object. This
 * macro has hence a purely aesthetical/declarative purpose.
 */
#  define BOXOUT

/** Type representing C pointers from Box */
typedef void *BoxCPtr;

/**
 * A subtype is simply a structure containing two pointers: one points
 * to the parent, one to the child.
 */
typedef struct {
  BoxPtr child, parent;
} BoxSubtype;

#  define BoxSubtype_Get_Parent_Target(subtype_ptr) \
  ((subtype_ptr)->parent.ptr)

#  define BoxSubtype_Get_Child_Target(subtype_ptr) \
  ((subtype_ptr)->child.ptr)

#  define SUBTYPE_PARENT_PTR(subtype_ptr, parent_type) \
  ((parent_type *) (subtype_ptr)->parent.ptr)

#  define SUBTYPE_CHILD_PTR(subtype_ptr, child_type) \
  (( child_type *) (subtype_ptr)->child.ptr)


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

/**
 * @brief Type used from functions to communicate whether an operation
 *   succeeded or failed.
 */
typedef enum {
  BOXTASK_OK      = 0, /**< Function succeeded */
  BOXTASK_FAILURE = 1, /**< Function failed: caller needs to report error */
  BOXTASK_ERROR   = 2  /**< Function failed: error already reported */
} BoxTask;

/*****************************************************************************/
/* Obsolete stuff below. */

typedef struct {
  BoxInt length;
  char *text;
} BoxName;

#  define BOXNAME(str) ((BoxName) {sizeof(str)-1, str})

typedef BoxName BoxData;

/* Questa macro permette di usare una indicizzazione "circolare", secondo cui,
 * data una lista di num_items elementi, l'indice 1 si riferisce al primo
 * elemento, 2 al secondo, ..., num_items all'ultimo, num_items+1 al primo,
 * num_items+2 al secondo, ...  Inoltre l'indice 0 si riferisce all'ultimo
 * elemento, -1 al pen'ultimo, ...
 */
#define BOX_CIRCULAR_INDEX(num_items, index)            \
  ((index) > 0 ? ((index) - 1) % (num_items)            \
   : (num_items) - 1 - ((-(index)) % (num_items)))

/* Shorthands. */
#    define SUInt BoxUInt_Fmt
#    define SChar BoxChar_Fmt
#    define SInt BoxInt_Fmt
#    define SReal BoxReal_Fmt
#    define SPoint BoxPoint_Fmt
#    define CIRCULAR_INDEX BOX_CIRCULAR_INDEX

/*****************************************************************************/

/**
 * @brief Macro to call a function returning #BoxTask from a function returning
 *   #BoxTask.
 */
#  define BOXTASK(x) \
  do {if (x) return BOXTASK_FAILURE;} while(0)

/**
 * @brief This macro should be used to check - via an assert - that a function
 *   returning #BoxTask succeeded.
 *
 * @note This should be used with functions that are expected to succeed.
 */
#  define ASSERT_TASK(x) \
  do {BoxTask t = (x); assert(t == BOXTASK_OK);} while(0)

typedef struct BoxCoreTypes_struct BoxCoreTypes;

/**
 * @brief Initialize the core types of Box.
 */
BOXEXPORT BoxBool
BoxCoreTypes_Init(BoxCoreTypes *core_types);

/**
 * @brief Finalize the core type of Box.
 */
BOXEXPORT void
BoxCoreTypes_Finish(BoxCoreTypes *core_types);

/**
 * @brief Initialize the type system.
 */
BOXEXPORT BoxBool
Box_Initialize_Type_System(void);

/**
 * @brief Initialize the type system.
 */
BOXEXPORT void
Box_Finalize_Type_System(void);

/**
 * @brief Get the core type corresponding to the given type identifier.
 *
 * @param ct The set of core types from which the type is to be extracted.
 * @param id The type identifier for the type to extract.
 * @return The type corresponding to @p id, extracted from @p ct, or @c NULL
 *   if the operation failed.
 * @note @p ct is initialized if it is not. In other words @p ct can be a
 *   pointer to a zeroed region of memory which can contain
 *   <tt>sizeof(BoxCoreTypes)</tt> bytes.
 */
BOXEXPORT BoxType *
BoxCoreTypes_Get_Type(BoxCoreTypes *ct, BoxTypeId id);

/**
 * @brief Similar to BoxCoreTypes_Get_Type() but uses the global core type
 *   set.
 *
 * @see BoxCoreTypes_Get_Type().
 */
BOXEXPORT BoxType *
Box_Get_Core_Type(BoxTypeId id);

#  define BOX_FATAL_ERROR() Box_Fatal_Error(__FILE__, __LINE__)

#endif /* _BOX_CORE_H */
