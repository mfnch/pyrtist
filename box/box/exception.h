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

/**
 * Exception object.
 */
typedef struct BoxException_struct BoxException;

/**
 * Create a new exception object.
 */
BOXEXPORT BoxException *
BoxException_Create(void);

/**
 * Destroy an exception object.
 */
BOXEXPORT void
BoxException_Destroy(BoxException *excp);

#endif /* _BOX_EXCEPTION_H */
