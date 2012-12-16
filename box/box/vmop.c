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

#include <assert.h>

#include "vmop_priv.h"


/**
 * @brief Compute the size of a data type in #BoxVMWord.
 */
#define MY_SIZE_IN_WORDS(sz) \
  (((sz) + sizeof(BoxVMWord) - 1)/sizeof(BoxVMWord))

/* Get the length of a VM instruction in words. */
unsigned int
BoxOp_Get_Length(BoxOp *op) {
  BoxTypeId type_id = op->desc->t_id;
  int sz = (op->has_data) ? MY_SIZE_IN_WORDS(size_of_type[type_id]) : 0;

  if (op->num_args >= 2) {
    /* 2 arguments. */

    /* For now we do not have instructions with 2 arguments and attached
     * data...
     */
    assert(!op->has_data);

    if (op->format == BOXOPFMT_UNDECIDED) {
      const BoxInt u8_sign = ~((BoxInt) 0x7f);
      BoxInt a0_sign = op->args[0] & u8_sign;
      BoxBool a0_is_u8 = (a0_sign == 0 || a0_sign == u8_sign);
      BoxInt a1_sign = op->args[1] & u8_sign;
      BoxBool a1_is_u8 = (a1_sign == 0 || a1_sign == u8_sign);
      op->format = (a0_is_u8 && a1_is_u8) ? BOXOPFMT_SHORT : BOXOPFMT_LONG;
    }

    op->length = (op->format == BOXOPFMT_SHORT) ? 1 + sz : 4 + sz;
    return op->length;

  } else if (op->num_args == 1) {
    /* 1 argument. */
    assert(type_id == BOXTYPEID_INT
           || (op->args_forms & 0x3) != BOXOPARGFORM_IMM);

    if (op->format == BOXOPFMT_UNDECIDED) {
      const BoxInt u16_sign = ~((BoxInt) 0x7fff);
      BoxInt a0_sign = op->args[0] & u16_sign;
      BoxBool a0_is_u16 = (a0_sign == 0 || a0_sign == u16_sign);
      op->format = (a0_is_u16) ? BOXOPFMT_SHORT : BOXOPFMT_LONG;
    }

    op->length = (op->format == BOXOPFMT_SHORT) ? 1 + sz : 3 + sz;
    return op->length;

  } else {
    /* 0 arguments. */
    op->format = BOXOPFMT_SHORT;
    op->length = 1 + sz;
    return op->length;
  }
}

/* VM instruction writer. */
BoxBool
BoxOp_Write(BoxOp *op, BoxVMWord *bytecode) {
  void *data;

  if (op->format == BOXOPFMT_SHORT) {
    /* SHORT INSTRUCTION: we assemble the istruction header in the following way:
     * (note: 1 is represented with bit 0 = 1 and all other bits = 0)
     *  bit 0: true if the instruction is long
     *  bit 1-4: type of arguments
     *  bit 5-7: length of instruction
     *  bit 8-15: type of instruction
     *  (bit 16-23: left empty for argument 1)
     *  (bit 24-31: left empty for argument 2)
     */ 
    data = & bytecode[1];
    if (op->num_args >= 2)
      bytecode[0] = ((op->args[1] & 0xff) << 24
                     | (op->args[0] & 0xff) << 16
                     | (op->id & 0xff) << 8
                     | (op->length & 0x7) << 5
                     | (op->args_forms & 0xf) << 1);
    else if (op->num_args == 1)
      bytecode[0] = ((op->args[0] & 0xff) << 16
                     | (op->id & 0xff) << 8
                     | (op->length & 0x7) << 5
                     | (op->args_forms & 0xf) << 1);
    else
      bytecode[0] = ((op->id & 0xff) << 8
                     | (op->length & 0x7) << 5
                     | (op->args_forms & 0xf) << 1);
  } else {
    /* LONG INSTRUCTION: we assemble the istruction header in the following
     * way:
     *  FIRST FOUR BYTES:
     *    bit 0: true if the instruction is long
     *    bit 1-4: type of arguments
     *    bit 5-31: length of instruction
     *  SECOND FOUR BYTES:
     *    bit 0-31: type of instruction
     *  (THIRD FOUR BYTES: argument 1)
     *  (FOURTH FOUR BYTES: argument 2)
     */
    bytecode[0] = ((op->length & 0x07ff) << 5
                   | (op->args_forms & 0xf) << 1
                   | 0x1);
    bytecode[1] = op->id;

    if (op->num_args >= 2) {
      bytecode[2] = op->args[0];
      bytecode[3] = op->args[1];
      data = & bytecode[4];
    } else if (op->num_args == 1) {
      bytecode[2] = op->args[0];
      data = & bytecode[3];
    } else
      data = & bytecode[2];
  }

  if (op->has_data) {
    BoxTypeId type_id = op->desc->t_id;
    unsigned int sz = size_of_type[type_id], sz_words = MY_SIZE_IN_WORDS(sz);
    ((BoxVMWord *) data)[sz_words - 1] = 0;
    memcpy(data, op->data, sz);
  }

  return BOXBOOL_TRUE;
}
