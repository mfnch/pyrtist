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
 * @file vm2c.h
 * @brief Convert Box VM code to C.
 */

#ifndef _BOX_VM2C_H
#  define _BOX_VM2C_H

#  include <stdio.h>

#  include <box/types.h>
#  include <box/vm.h>

/**
 * @brief Convert the VM object to C source code.
 */
BOXEXPORT BoxBool
BoxVM_Export_To_C_Code(BoxVM *vm, FILE *out);

#endif /* _BOX_VM2C_H */
