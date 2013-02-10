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

/**
 * @file bltinarray_priv.h
 * @brief Private definitons for heterogeneous arrays (box Array object).
 */

#ifndef _BOX_BLTINARRAY_PRIV_H
#  define _BOX_BLTINARRAY_PRIV_H

#  include <box/bltinarray.h>

#  include <box/array.h>
#  include <box/obj.h>
#  include <box/vm.h>
#  include <box/vm_priv.h>
#  include <box/str.h>
#  include <box/builtins.h>

struct BoxArray_struct {
  BoxArr arr;
};

struct BoxSet_struct {
  BoxAny index, value;
};

#endif /* _BOX_BLTINARRAY_PRIV_H */
