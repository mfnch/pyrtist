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
 * @file vmopregs.c
 * @brief Implementation of BoxVMOpRegs functionality.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <box/mem.h>
#include <box/vmopregs_priv.h>


/* Initialize a table of registers. */
BoxBool
BoxVMOpRegs_Init(BoxVMOpRegs *regs, size_t *var_nums, size_t *reg_nums) {
  size_t hi[2*BOX_NUM_REG_TYPES];
  int hi_len;

  BoxUInt16 hi_mask, low_mask, bit_mask;
  int bit_idx;

  /* Initialize the masks which tell us whether the number of registers of a
   * a given type is low (fits inside a unsigned char) or high (a size_t).
   */
  low_mask = 0;
  hi_mask = 0;

  /* Number of entries in the high table. */
  hi_len = 0;

  /* Populate the tables and the masks. */
  for (bit_idx = 0, bit_mask = 1;
       bit_idx < 2*BOX_NUM_REG_TYPES;
       bit_idx++, bit_mask <<= 1) {
    int is_reg = ((bit_idx & 1) != 0);
    int t_idx = bit_idx >> 1;
    size_t num = (is_reg) ? reg_nums[t_idx] : var_nums[t_idx];

    assert(bit_idx != 0);

    if (num) {
      if (num < 0x100) {
        /* num fits inside an `unsigned char'. */
        regs->low_nums[bit_idx] = (unsigned char) num;
        low_mask |= bit_mask;

      } else {
        /* num does not fit inside an `unsigned char': use a size_t.
         * Note that we use the low table to index the high table.
         */
        regs->low_nums[bit_idx] = (unsigned char) hi_len;
        hi[hi_len++] = num;
        hi_mask |= bit_mask;
      }
    }
  }

  /* Populate the object. */
  regs->low_mask = low_mask;
  regs->hi_mask = hi_mask;

  /* Create the high table, but only if necessary. */
  if (hi_len > 0) {
    size_t len = hi_len*sizeof(size_t);
    regs->hi_nums = Box_Mem_Alloc(len);
    if (!regs->hi_nums)
      return BOXBOOL_FALSE;
    (void) memcpy(regs->hi_nums, hi, len);

  } else {
    regs->hi_nums = NULL;
  }

  return BOXBOOL_TRUE;
}

/* Finalize a table of registers. */
void
BoxVMOpRegs_Finish(BoxVMOpRegs *regs) {
  Box_Mem_Free(regs->hi_nums);
}
