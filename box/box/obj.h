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

#include <box/ntypes.h>


/**
 * A pointer to a target object decorated with the type of the target.
 * This is using for boxing/unboxing objects.
 */
typedef struct {
  BoxXXXX *type;
  BoxPtr  ptr;
} BoxAny;

#  define BoxAny_Init(obj) \
  do {(obj)->type = NULL; BoxPtr_Init(& (obj)->ptr);} while(0)

/**
 * Finalize an Any object.
 */
BOXEXPORT void
BoxAny_Finish(BoxAny *any);

/**
 * Object header. Every object allocation includes some extra space to contain
 * This structure, which contains the type of the object and the number of
 * references that other objects make to it.
 */
typedef struct BoxObjHeader_struct {
  size_t  num_refs;
  BoxXXXX *type;
} BoxObjHeader;

/**
 * Single Pointer to a Box object.
 */
typedef void *BoxSPtr;

/**
 * Allocate space for an object of the given type.
 * This is an internal function which is not meant to be used externally.
 */
BoxSPtr BoxSPtr_Alloc(BoxXXXX *t);

/**
 * Raw allocation function.
 * This is an internal function which is not meant to be used externally.
 */
BoxSPtr BoxSPtr_Raw_Alloc(BoxXXXX *t, size_t obj_size);

/**
 * Add a reference to an object and return it.
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
 * Function to be used in conjunction with BoxSPtr_Unlink_Begin.
 * @see BoxSPtr_Unlink_Begin
 */
BOXEXPORT void
BoxSPtr_Unlink_End(BoxSPtr src);

/**
 * Remove a reference to an object, destroying it, if unreferenced.
 * @param src Object to unreference.
 * @return src if the object was unreferenced but not destroyed, NULL if the
 *   object was unreferenced and destroyed.
 */
BOXEXPORT BoxSPtr
BoxSPtr_Unlink(BoxSPtr src);

/**
 * Remove a reference to an object, destroying it, if unreferenced.
 * @param src Object to unreference.
 * @return BOXBOOL_TRUE if the object was unreferenced and destroyed, BOXBOOL_FALSE if the
 *   object was unreferenced and destroyed.
 */
BOXEXPORT BoxBool
BoxPtr_Unlink(BoxPtr *src);

#endif /* _BOX_OBJ_H */
