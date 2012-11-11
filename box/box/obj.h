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
 * @file obj.h
 * @brief Definition of object allocation and managig functions.
 *
 * This module defines basic functions to allocate Box objects and manage
 * them.
 */

#ifndef _BOX_OBJ_H
#  define _BOX_OBJ_H

#include <box/types.h>
#include <box/exception.h>

/**
 * @brief Initialize a subtype.
 *
 * @param st Pointer to a region of memory ready to contain a #BoxSubtype
 *   object.
 */
BOXEXPORT void
BoxSubtype_Init(BoxSubtype *st);

/**
 * @brief Finalize a subtype.
 *
 * @param Pointer to the subtype.
 */
BOXEXPORT void
BoxSubtype_Finish(BoxSubtype *st);

/**
 * A pointer to a target object decorated with the type of the target.
 * This is using for boxing/unboxing objects.
 */
typedef struct {
  BoxType *type;
  BoxPtr  ptr;
} BoxAny;

#  define BoxAny_Init(obj) \
  do {(obj)->type = NULL; BoxPtr_Init(& (obj)->ptr);} while(0)

/**
 * @brief Get the type of the object stored inside the #BoxAny object.
 */
#  define BoxAny_Get_Type(obj) \
  ((obj)->type)

/**
 * @brief Get the pointer to the object stored inside the #BoxAny object.
 */
#  define BoxAny_Get_Obj(obj) \
  (& (obj)->ptr)

/**
 * Finalize an Any object.
 */
BOXEXPORT void
BoxAny_Finish(BoxAny *any);

/**
 * @brief Change the boxed object stored inside the given any object.
 *
 * @param any The ANY object which should be changed to reference @p obj.
 * @param obj The object which should be put inside @p any.
 * @param t The type of @p obj.
 * @return Whether the boxing operation was successful.
 */
BOXEXPORT BoxBool
BoxAny_Box(BoxPtr *any, BoxPtr *obj, BoxType *t);

/**
 * @brief Retrieve the boxed object stored inside the given any object.
 *
 * @param obj An unitialised pointer which will be set with a fresh reference
 *   to the object contained inside @p any.
 * @param any The @c ANY object containing the boxed object.
 * @param t The type of the boxed object.
 * @return Whether the unboxing operation was successful. Unboxing @p any can
 *   be done only if the contained object has a type which matches @p t.
 */
BOXEXPORT BoxBool
BoxAny_Unbox(BOXOUT BoxPtr *obj, BoxPtr *any, BoxType *t);

/**
 * Object header. Every object allocation includes some extra space to contain
 * This structure, which contains the type of the object and the number of
 * references that other objects make to it.
 */
typedef struct BoxObjHeader_struct {
  size_t  num_refs;
  BoxType *type;
} BoxObjHeader;

/**
 * Single Pointer to a Box object.
 */
typedef void *BoxSPtr;

/**
 * @brief Allocate space for an object of the given type.
 *
 * This is an internal function which is not meant to be used externally.
 */
BOXEXPORT BoxSPtr
BoxSPtr_Alloc(BoxType *t);

/**
 * @brief Raw allocation function.
 *
 * This is an internal function which is not meant to be used externally.
 */
BOXEXPORT BoxSPtr
BoxSPtr_Raw_Alloc(BoxType *t, size_t obj_size);

/**
 * @brief Add a reference to an object and return it.
 *
 * @param src The object to reference.
 * @return Return its argument, src.
 */
BOXEXPORT BoxSPtr
BoxSPtr_Link(BoxSPtr src);

/**
 * This function, which should be used in conjunction with BoxSPtr_Unlink_End,
 * allows unlinking an object and performing some extra operations before
 * object destruction. It should be used in the following way:
 * @code
 * if (BoxSPtr_Unlink_Begin(src)) {
 *   // do something
 *   BoxSPtr_Unlink_End(src);
 * }
 * @endcode
 * The function BoxSPtr_Unlink_Begin checks the reference counts of src. If
 * this is greater than one, then the reference count is decreased and
 * BOXBOOL_FALSE is returned. If the reference count is one, then the function
 * does not change the reference count, but rather returns BOXBOOL_TRUE.
 * The function BoxSPtr_Unlink_End does finally increase the reference count
 * and destroy the object.
 * @param src Object to unlink.
 * @return BOXBOOL_TRUE if the object has just one reference, otherwise return
 *   BOXBOOL_FALSE.
 * @see BoxSPtr_Unlink
 */
BOXEXPORT BoxBool
BoxSPtr_Unlink_Begin(BoxSPtr src);

/**
 * @brief Function to be used in conjunction with BoxSPtr_Unlink_Begin.
 *
 * @see BoxSPtr_Unlink_Begin
 */
BOXEXPORT void
BoxSPtr_Unlink_End(BoxSPtr src);

/**
 * @brief Remove a reference to an object, destroying it, if unreferenced.
 *
 * @param src Object to unreference.
 * @return src if the object was unreferenced but not destroyed, NULL if the
 *   object was unreferenced and destroyed.
 */
BOXEXPORT BoxSPtr
BoxSPtr_Unlink(BoxSPtr src);

/**
 * @brief Get the type of the allocated object.
 *
 * @param obj An object allocated with BoxSPtr_Alloc() or BoxSPtr_Raw_Alloc().
 * @return The type of @p obj.
 */
BOXEXPORT BoxType *
BoxSPtr_Get_Type(BoxSPtr obj);

/**
 * @brief Reference the given object.
 *
 * @param src Object to reference.
 * @return @p src is returned.
 */
BOXEXPORT BoxPtr *
BoxPtr_Link(BoxPtr *src);

/**
 * @brief Remove a reference to an object, destroying it, if unreferenced.
 *
 * @param src Object to unreference.
 * @return @c BOXBOOL_TRUE if the object was unreferenced but not destroyed,
 *   @c BOXBOOL_FALSE if the object was unreferenced and also destroyed.
 */
BOXEXPORT BoxBool
BoxPtr_Unlink(BoxPtr *src);

/**
 * @brief Create a new object of the given type and return a pointer to it.
 *
 * @param ptr Location where to put the pointer to the object.
 * @param t The type descriptor.
 * @return Whether the operation was successful.
 */
BOXEXPORT BoxBool
BoxPtr_Create_Obj(BOXOUT BoxPtr *ptr, BoxType *t);

/**
 * @brief Copy an object of the given type.
 *
 * @param dst Pointer to the destination.
 * @param src Pointer to the source.
 * @param t The type descriptor.
 * @return Whether the operation was successful.
 */
BOXEXPORT BoxBool
BoxPtr_Copy_Obj(BoxPtr *dst, BoxPtr *src, BoxType *t);

/**
 * @brief Try to convert a BoxPtr object to a simple single pointer.
 *
 * This function returns the a pointer @c sptr, if a pointer @c src exists such
 * that @p src can be obtained from <tt>BoxPtr_Init_From_SPtr(src, sptr)</tt>.
 *
 * @param src The input double pointer.
 * @return The single pointer corresponding to @p src, or @c NULL if such
 *   @c src cannot be expressed by using a single pointer.
 */
BOXEXPORT void *
BoxPtr_Get_SPtr(const BoxPtr *src);

/**
 * @brief Combine two objects from their types and pointers.
 *
 * @see Box_Combine_Any
 */
BOXEXPORT BoxBool
Box_Combine(BoxType *t_parent, BoxPtr *parent,
            BoxType *t_child, BoxPtr *child, BoxException **except);

/**
 * @brief Combine two objects boxed as #BoxAny objects.
 *
 * @param parent A #BoxAny object containing the parent.
 * @param child A #BoxAny object containing the child.
 * @param except Pointer where to store a #BoxException, in case the
 *   combination is found and returns with failure.
 * @return @c BOXBOOL_TRUE if the combination was found, else @c BOXBOOL_FALSE
 *
 * @note <tt>*except</tt> is set only when the function returns @c BOXBOOL_TRUE.
 * @see Box_Combine
 */
BOXEXPORT BoxBool
Box_Combine_Any(BoxAny *parent, BoxAny *child, BoxException **except);

#endif /* _BOX_OBJ_H */
