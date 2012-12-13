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

#ifndef _BOX_VMOP_PRIV_H
#  define _BOX_VMOP_PRIV_H

/**
 * @brief Maximum number of arguments for a VM instruction.
 */
#  define BOX_OP_MAX_NUM_ARGS (2)

/**
 * @brief Instruction format.
 */
typedef enum {
  BOXOPFMT_SHORT,  /**< Short form of instruction. */
  BOXOPFMT_LONG,   /**< Long form of instruction. */
  BOXOPFMT_UNKNOWN /**< Unknown form of instruction. */
} BoxOpFmt;

/**
 * @brief Implementation of #BoxOp.
 */
struct BoxOp_struct {
  BoxOpId   id;         /**< Instruction identifiers (an integer). */
  const BoxOpDesc
            *desc;      /**< Instruction description (used internally). */
  BoxInt    next;       /**< Advance offset. */
  BoxOpFmt  format;     /**< Instruction format. */
  int       args_forms; /**< Type of arguments of instruction. */
  int       num_args;   /**< Number of arguments (excluding data). */
  BoxInt    args[2];    /**< Raw argument values. */
  BoxBool   has_data;   /**< Whether the instruction has associated data. */
  BoxVMWord *data;      /**< Pointer to the instruction data. */
};

/**
 * @brief Portable cast from uint8_t to int8_t.
 *
 * The C standard (C99, ISO/IEC 9899:1999) says that conversion from unsigned
 * to signed integer has an implementation-defined result (when the unsigned
 * value cannot be represented in the signed type). This function implements a
 * cast from uint8_t to int8_t with implementation-independent result.
 * Compilers should be able to easily translate this to efficient code.
 */
static inline int8_t
BoxInt8_From_UInt8(uint8_t x) {
  return (x & (uint8_t) 0x80) ? -((int8_t) -x) : (int8_t) x;
}

/**
 * @brief Portable cast from uint16_t to int16_t.
 *
 * Similar to BoxInt8_From_UInt8(), but works on 16 bit integers.
 */
static inline int16_t
BoxInt16_From_UInt16(uint16_t x) {
  return (x & (uint16_t) 0x8000) ? -((int16_t) -x) : (int16_t) x;
}

/**
 * @brief Portable cast from uint32_t to int32_t.
 *
 * Similar to BoxInt8_From_UInt8(), but works on 16 bit integers.
 */
static inline int32_t
BoxInt32_From_UInt32(uint32_t x) {
  return (x & (uint32_t) 0x80000000) ? -((int32_t) -x) : (int32_t) x;
}

/**
 * @brief VM instruction reader.
 *
 * This function translates a serialized instruction into a #BoxOp
 * structure containing all the information about the instruction.
 */
static inline BoxBool
BoxOp_Read(BoxOp *op, BoxVMX *vmx, BoxVMWord *bytecode) {
  BoxVMWord word1 = bytecode[0];
  const BoxOpDesc *exec_table = vmx->vm->exec_table;

  if (word1 & 0x1) {
    /* Long format. */
    op->args_forms = (word1 >> 1) & 0xf;
    op->next = word1 >> 5;
    op->id = bytecode[1];
    if (op->id < BOX_NUM_OPS) {
      const BoxOpDesc *idesc = & exec_table[op->id];
      vmx->idesc = idesc;
      op->has_data = idesc->has_data;
      op->num_args = idesc->num_args;
      if (idesc->num_args == 2) {
        op->args[0] = (BoxInt) BoxInt32_From_UInt32(bytecode[2]);
        op->args[1] = (BoxInt) BoxInt32_From_UInt32(bytecode[3]);
        op->data = & bytecode[4];
      } else if (idesc->num_args == 1) {
        op->args[0] = (BoxInt) BoxInt32_From_UInt32(bytecode[2]);
        op->data = & bytecode[3];
      } else
        op->data = & bytecode[2];
      return BOXBOOL_TRUE;
    }

  } else {
    /* Short format. */
    op->args_forms = (word1 >> 1) & 0xf;
    op->next = (word1 >> 5) & 0x7;
    op->id = (word1 >> 8) & 0xff;
    if (op->id < BOX_NUM_OPS) {
      const BoxOpDesc *idesc = & exec_table[op->id];
      vmx->idesc = idesc;
      op->data = & bytecode[1];
      op->has_data = idesc->has_data;
      op->num_args = idesc->num_args;
      if (idesc->num_args == 2) {
        op->args[0] = (BoxInt) BoxInt8_From_UInt8(word1 >> 16);
        op->args[1] = (BoxInt) BoxInt8_From_UInt8(word1 >> 24);
      } else if (idesc->num_args == 1)
        op->args[0] = (BoxInt) BoxInt8_From_UInt8(word1 >> 16);
      return BOXBOOL_TRUE;
    }
  }

  return BOXBOOL_FALSE;
}

#if 0
/**
 * @brief VM instruction writer.
 *
 * This function translates a #BoxOp structure containing all the information
 * about one instruction into a serialized instruction in @p bytecode.
 */
static inline BoxBool
BoxOp_Write(BoxOp *op, BoxVMX *vmx, BoxVMWord *bytecode) {
  if (op->num_args == 2) {
    if (op->format == BOXOPFMT_UNKNOWN) {
      const BoxInt u8_sign = ~((BoxInt) 0x7f);
      BoxInt a0_sign = op->args[0] & u8_sign;
      BoxBool a0_is_u8 = (a0_sign == 0 || a0_sign == u8_sign);
      BoxInt a1_sign = op->args[1] & u8_sign;
      BoxBool a1_is_u8 = (a1_sign == 0 || a1_sign == u8_sign);
      op->format = (a0_is_u8 && a1_is_u8) ? BOXOPFMT_SHORT : BOXOPFMT_LONG;
    }

  } else if (op->num_args == 1) {
    if (op->format == BOXOPFMT_UNKNOWN) {
      const BoxInt u16_sign = ~((BoxInt) 0x7fff);
      BoxInt a0_sign = op->args[0] & u16_sign;
      BoxBool a0_is_u16 = (a0_sign == 0 || a0_sign == u16_sign);
      op->format = (a0_is_u16) ? BOXOPFMT_SHORT : BOXOPFMT_LONG;
    }
  }

  return BOXBOOL_FALSE;
}
#endif

#endif /* _BOX_VMOP_PRIV_H */
