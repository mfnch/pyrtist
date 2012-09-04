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

#  include <box/exception.h>
#  include <box/ntypes.h>
#  include <box/vmproc.h>

/**
 * First (and simplest) C implementation of a BoxCallable object: takes just a
 * pointer to the parent.
 * @param parent The pointer to the parent (object over which the callable
 *   operates).
 * @return A BoxException object in case of errors, NULL otherwise.
 */
typedef BoxException *(*BoxCCall1)(BoxPtr *parent);

/**
 * Second C implementation of a BoxCallable object: takes a pointer to the
 * parent and a pointer to the child.
 * @param parent The pointer to the parent.
 * @param child The pointer to the child.
 * @return A BoxException object in case of errors, NULL otherwise.
 */
typedef BoxException *(*BoxCCall2)(BoxPtr *parent, BoxPtr *child);

/**
 * Third (and most complete) C implementation of a BoxCallable object: takes a
 * pointer to the corresponding BoxCallable object (from which the callable
 * context can be retrieved, thus implementing a closure), a pointer to the
 * parent and a pointer to the child.
 * @param callable The BoxCallable object corresponding to this function. This
 *   is typically what is returned by BoxCallable_Create_From_CCall3.
 * @param parent The pointer to the parent.
 * @param child The pointer to the child.
 * @return A BoxException object in case of errors, NULL otherwise.
 */
typedef BoxException *(*BoxCCall3)(BoxPtr *callable, BoxPtr *parent,
                                   BoxPtr *child);

/**
 * Fourth C implementation of a BoxCallable object. This type is provided for
 * compatibility with old code, where the C implementation is a function which
 * takes just a pointer to the VM object.
 * @param vm The calling VM.
 * @return A BoxException object in case of errors, NULL otherwise.
 */
typedef BoxTask (*BoxCCallOld)(BoxVMX *vm);

typedef struct BoxCallable_struct BoxCallable;

typedef enum {
  BOXCALLABLEKIND_UNDEFINED, /**< Undefined callable. */
  BOXCALLABLEKIND_C_1,       /**< BoxCCall1. */
  BOXCALLABLEKIND_C_2,       /**< BoxCCall2. */
  BOXCALLABLEKIND_C_3,       /**< BoxCCall3. */
  BOXCALLABLEKIND_C_OLD,     /**< BoxCCallOld. */
  BOXCALLABLEKIND_VM         /**< VM call. */
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
BOXEXPORT BOXOUT BoxCallable *
BoxCallable_Create_From_CCall1(BoxXXXX *t_out, BoxXXXX *t_in, BoxCCall1 call);

/**
 * Create a callable object from a BoxCCall2 C function.
 * @param call The C function.
 * @return The callable object.
 */
BOXEXPORT BOXOUT BoxCallable *
BoxCallable_Create_From_CCall2(BoxXXXX *t_out, BoxXXXX *t_in, BoxCCall2 call);

/**
 * Create a callable object from a BoxCCall3 C function.
 * @param t_out The type of the returned value.
 * @param t_in The type of the input argument.
 * @param context Data to be associated with the callback. This allows
 *   implementing closures.
 * @param call The C function implementing the callback.
 * @return A new callback object or NULL if the operation failed.
 */
BOXEXPORT BOXOUT BoxCallable *
BoxCallable_Create_From_CCall3(BoxXXXX *t_out, BoxXXXX *t_int,
                               BOXIN BoxPtr *context, BoxCCall3 call);

/**
 * Create a callable object from old-style C function (BoxCCallOld).
 */
BOXEXPORT BOXOUT BoxCallable *
BoxCallable_Create_From_CCallOld(BoxXXXX *t_out, BoxXXXX *t_in,
                                 BoxCCallOld call);

/**
 * Create a callable object from a BoxVM procedure.
 * @param t_out Output type for the callable.
 * @param t_in Input type for the callable.
 * @param context Pointer to additional data required by the callable.
 * @param vm The VM which contains the callable implementation.
 * @param num The call number for the callable implementation.
 * @return A new callable which wraps the procedure `num' of the virtual
 *   machine `vm'.
 */
BOXEXPORT BOXOUT BoxCallable *
BoxCallable_Create_From_VM(BoxXXXX *t_out, BoxXXXX *t_in,
                           BOXIN BoxPtr *context,
                           BoxVM *vm, BoxVMCallNum num);

/**
 *
 */
BOXEXPORT BoxException *
BoxCallable_Call1(BoxCallable *cb, BoxPtr *parent);

/**
 *
 */
BOXEXPORT BoxException *
BoxCallable_Call2(BoxCallable *cb, BoxPtr *parent, BoxPtr *child);

#endif /* _BOX_CALLABLE_H */
