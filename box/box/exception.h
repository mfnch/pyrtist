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
 * @file exception.h
 * @brief Declaration of BoxException and related functionality.
 *
 * This header defines the BoxException, an object used to describe error
 * conditions.
 */

#ifndef _BOX_EXCEPTION_H
#  define _BOX_EXCEPTION_H

#  include <box/types.h>
#  include <box/print.h>

/**
 * Exception object.
 */
typedef struct BoxException_struct BoxException;

/**
 * Create a new exception object.
 */
BOXEXPORT BoxException *
BoxException_Create_Raw(char *msg);

#define BoxException_Create(...) \
  BoxException_Create_Raw(Box_SPrintF(__VA_ARGS__))

/**
 * Destroy an exception object.
 */
BOXEXPORT void
BoxException_Destroy(BoxException *excp);

/**
 * @brief Show the exception to the console and destroy it.
 *
 * @param excp The exception.
 * @return Whether @p excp was not NULL.
 *
 * If the provided exception is not NULL, then print the exception to the
 * screen (stderr) and destroy the exception.
 * @note This function can be used to check for an error condition and abort.
 * See the example below.
 *
 * @code
 * BoxException *exception = My_Function_Returning_Exception();
 * if (BoxException_Check(exception))
 *   abort();
 * @endcode
 */
BOXEXPORT BoxBool
BoxException_Check(BoxException *excp);

BOXEXPORT char *
BoxException_Get_Str(BoxException *excp);

#endif /* _BOX_EXCEPTION_H */
