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

#ifndef _BOX_LIBG_CONSTRAINTS_H
#  define _BOX_LIBG_CONSTRAINTS_H

#  include <box/types.h>
#  include <box/str.h>

#  include "obj.h"
#  include "i_window.h"

/** C declaration of the Constraints Box object (see constraints.box). */
typedef struct {
  BoxStr        freedom;     /**< String representing the degrees of
                             freedom in the transformation. */
  BoxLibGObj    constraints; /**< Near constraints encoded in a Obj object. */
  BoxLibGWindow window;      /**< Window object. */

} BoxLibGConstraints;

/** C counterpart of the Box 'Transform' */
typedef struct {
  BoxPoint translation,
           rotation_center, 
           scale_factors;
  BoxReal rotation_angle;

} BoxLibGTransform;

#endif /* _BOX_LIBG_CONSTRAINTS_H */
