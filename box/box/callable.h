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
#  include <box/types.h>
#  include <box/obj.h>
#  include <box/vmproc.h>

/**
 * @brief Unique identifier.
 *
 * This is typically a string used to identify procedures.
 */
typedef char BoxUid;

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

/**
 * @brief Abstraction of function in the Box language implementation.
 *
 * A @c BoxCallable object is an object which can be called by passing one
 * parent and one child objects. The parent object is often the result of
 * the call, while the child is typically the input argument.
 * A @a BoxCallable object can be implemented as a C function or as a
 * @c BoxVM procedure.
 *
 * @see BoxVM
 */
typedef struct BoxCallable_struct BoxCallable;

/**
 * @brief Enumeration of the possible implementation for a @c BoxCallable
 * object. 
 *
 * @see BoxCallable
 */
typedef enum {
  BOXCALLABLEKIND_UNDEFINED, /**< Undefined callable. */
  BOXCALLABLEKIND_C_1,       /**< BoxCCall1. */
  BOXCALLABLEKIND_C_2,       /**< BoxCCall2. */
  BOXCALLABLEKIND_C_3,       /**< BoxCCall3. */
  BOXCALLABLEKIND_C_OLD,     /**< BoxCCallOld. */
  BOXCALLABLEKIND_VM         /**< VM call. */
} BoxCallableKind;

/**
 * Equivalent to #BoxSPtr_Link, but does type casting for #BoxCallable.
 * @see BoxSPtr_Link
 */
#define BoxCallable_Link(t) ((BoxCallable *) BoxSPtr_Link(t))

/**
 * Equivalent to #BoxSPtr_Unlink, but does type casting for #BoxCallable.
 * @see BoxSPtr_Unlink
 */
#define BoxCallable_Unlink(t) ((BoxCallable *) BoxSPtr_Unlink(t))

/**
 * Create an undefined callable mapping an object of type @p t_in to an
 * object of type @p t_out. The object can be later defined using
 * BoxCallable_Define_From_CCall1() and friends.
 * @param t_out Type of the output object.
 * @param t_in Type of the input object.
 * @return A new #BoxCallable object.
 */
BOXEXPORT BOXOUT BoxCallable *
BoxCallable_Create_Undefined(BoxType *t_out, BoxType *t_in);

/**
 * @brief Set the unique identifier for the callable.
 *
 * @param cb The callable.
 * @brief uid A unique identifier for @p cb.
 * @return Whether the operation succeeded. @c BOXBOOL_FALSE is returned if
 *   the uid has been already set.
 */
BOXEXPORT BoxBool
BoxCallable_Set_Uid(BoxCallable *cb, BoxUid *uid);

/**
 * @brief Return the unique identifier associated to the given callable.
 *
 * @param cb The callable.
 * @return Return the unique identifier (a #BoxUid object) for @p cb or @c NULL
 *   if @c cb does not have a unique identifier.
 */
BOXEXPORT BoxUid *
BoxCallable_Get_Uid(BoxCallable *cb);

/**
 * Define a callable object from a BoxCCall1 C function.
 * @param call The C function.
 * @return BOXBOOL_TRUE on success, BOXBOOL_FALSE on failure.
 */
BOXEXPORT BOXOUT BoxCallable *
BoxCallable_Define_From_CCall1(BOXIN BoxCallable *cb, BoxCCall1 call);

/**
 * Define a callable object from a BoxCCall2 C function.
 * @param call The C function.
 */
BOXEXPORT BOXOUT BoxCallable *
BoxCallable_Define_From_CCall2(BOXIN BoxCallable *cb, BoxCCall2 call);

/**
 * Define a callable object from old-style C function (BoxCCallOld).
 */
BOXEXPORT BOXOUT BoxCallable *
BoxCallable_Create_From_CCallOld(BoxType *t_out, BoxType *t_in,
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
BoxCallable_Define_From_VM(BOXIN BoxCallable *cb,
                           BOXIN BoxPtr *context,
                           BoxVM *vm, BoxVMCallNum num);

/**
 * @brief Return whether the callable is implemented.
 *
 * @param cb The callable.
 * @return Whether the callable is implemented.
 * @note A callable is implemented when it is 
 */
BOXEXPORT BoxBool
BoxCallable_Is_Implemented(BoxCallable *cb);

/**
 * Return the call number for a VM callable.
 * @param cb The input callable.
 * @param vm The VM to which the call number refers to.
 * @param cn Where to store the call number.
 * @return If @p cb is a callable for a procedure defined in @p vm, then its
 * call number is returned in <tt>*cn</tt> and @c BOXBOOL_TRUE is returned.
 * In all the other cases, @c BOXBOOL_FALSE is returned.
 */
BOXEXPORT BoxBool
BoxCallable_Get_VM_CallNum(BoxCallable *cb, BoxVM *vm, BoxVMCallNum *cn);

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
