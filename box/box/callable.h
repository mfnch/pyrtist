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
 * @file callable.h
 * @brief Declaration of BoxCallable and related functionality.
 *
 * This header defines the BoxCallable object, an abstraction for a callable
 * entity in Box which may be implemented in either C or Box VM code and may
 * have links to associated data (what - in other languages - is referred to
 * as a closure). The callable object also contains details which allow
 * generating Box VM code which calls the object.
 */

#ifndef _BOX_CALLABLE_H
#  define _BOX_CALLABLE_H

typedef void *BoxException;

typedef BoxException *(*BoxCCall1)(BoxPtr *parent);

typedef BoxException *(*BoxCCall2)(BoxPtr *parent, BoxPtr *child);

typedef BoxException *(*BoxCCall3)(BoxPtr *callable, BoxPtr *parent,
                                   BoxPtr *child);

typedef struct BoxCallable_struct BoxCallable;

typedef enum {
  BOXCALLABLEKIND_C_1, /**< BoxCCall1 */
  BOXCALLABLEKIND_C_2, /**< BoxCCall2 */
  BOXCALLABLEKIND_C_3  /**< BoxCCall3 */
} BoxCallableKind;


/**
 * Create a callable object from a BoxCCall1 C function.
 * @param call The C function.
 * @return BOXBOOL_TRUE on success, BOXBOOL_FALSE on failure.
 */
BOXEXPORT void
BoxCallable_Init_From_CCall1(BoxCallable *cb, BoxCCall1 call);

/**
 * Initialize a callable object from a BoxCCall2 C function.
 * @param call The C function.
 */
BOXEXPORT void
BoxCallable_Init_From_CCall2(BoxCallable *cb, BoxCCall2 call);

/**
 * Create a callable object from a BoxCCall1 C function.
 * @param call The C function.
 * @return The callable object.
 */
BOXEXPORT BoxCallable *
BoxCallable_Create_From_CCall1(BoxCCall1 call);

/**
 * Create a callable object from a BoxCCall2 C function.
 * @param call The C function.
 * @return The callable object.
 */
BOXEXPORT BoxCallable *
BoxCallable_Create_From_CCall2(BoxCCall2 call);

/**
 *
 */
BOXEXPORT BoxException *
BoxCallable_Call1(BoxCallable *cb, BoxPtr parent);

/**
 *
 */
BOXEXPORT BoxException *
BoxCallable_Call2(BoxCallable *cb, BoxPtr *parent, BoxPtr *child);

#endif /* _BOX_CALLABLE_H */
