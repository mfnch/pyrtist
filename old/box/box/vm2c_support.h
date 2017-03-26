/****************************************************************************
 * Copyright (C) 2013 by Matteo Franchin                                    *
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

#ifndef _VM2C_SUPPORT_H
#  define _VM2C_SUPPORT_H

#  include <box/types.h>
#  include <box/obj.h>
#  include <box/exception.h>

#  define box2c_lea_i(reg) \
  do {(ro0).block = 0; (ro0).ptr = & (reg);} while(0)

#  define box2c_shift_o(arg1, arg2)                     \
  do { BoxPtr *lhs = & (arg1), *rhs = & (arg2);         \
    if (lhs != rhs) {                                   \
      if (!BoxPtr_Is_Detached(lhs))                     \
        (void) BoxPtr_Unlink(lhs);                      \
      *lhs = *rhs;                                      \
      BoxPtr_Detach(rhs);                               \
    } } while(0)

#  define box2c_add_i(arg1, arg2) do {(arg1) += arg2;} while(0)

#endif /* _VM2C_SUPPORT_H */
