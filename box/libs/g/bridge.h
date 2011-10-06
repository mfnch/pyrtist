/****************************************************************************
 * Copyright (C) 2011 by Matteo Franchin                                    *
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

#ifndef _BOX_LIBG_BRIDGE_H

#  define _BOX_LIBG_BRIDGE_H

#  include <box/types.h>

#  include "i_window.h"

/** Here we make some assumptions on the way Window, Matrix and SimplePut are
 * defined in the Box sources. We then also define some procedures
 */

/** Note: this definition is coherent with the following Box definition:
 * Window = ++CPtr
 */
typedef Window *BoxGWindow;

/** Note: this definition is coherent with the following Box definition
 * SimplePut = ++(Window src, Matrix matrix)
 */
typedef struct {
  BoxGWindow src;
  BoxGMatrix matrix;

} BoxGSimplePut;

#endif /* _BOX_LIBG_BRIDGE_H */
