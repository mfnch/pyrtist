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
 * @file vmopregs_priv.h
 * @brief Declarations of the BoxVMOpRegs object and related functionality.
 */

#ifndef _BOX_VMOPREGS_PRIV_H
#  define _BOX_VMOPREGS_H

#  include <box/core.h>
#  include <box/integers.h>

#  define BOX_NUM_REG_TYPES (5)

/**
 * @brief Table describing how many registers should be allocated for the
 *   different types.
 */
typedef struct {
  size_t *hi_nums;
  BoxUInt16 hi_mask, low_mask;
  unsigned char low_nums[2*BOX_NUM_REG_TYPES];
} BoxVMOpRegs;


/**
 * Initialize a table of registers.
 */
BOXEXPORT BoxBool
BoxVMOpRegs_Init(BoxVMOpRegs *regs, size_t *var_nums, size_t *reg_nums);

/**
 * Finalize a table of registers.
 */
BOXEXPORT void
BoxVMOpRegs_Finish(BoxVMOpRegs *regs);

#endif /* _BOX_VMOPREGS_PRIV_H */
