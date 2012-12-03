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

/* SHORT INSTRUCTION: we assemble the istruction header in the following way:
 * (note: 1 is represented with bit 0 = 1 and all other bits = 0)
 *  bit 0: true if the instruction is long
 *  bit 1-4: type of arguments
 *  bit 5-7: length of instruction
 *  bit 8-15: type of instruction
 *  (bit 16-23: left empty for argument 1)
 *  (bit 24-31: left empty for argument 2)
 */
#define BOXVM_WRITE_SHORTOP_HEADER(i_pos, i_eye, i_type, is_long, i_len, \
                                   arg_type) \
  do {i_eye = (((i_type) & 0xff) << 3 | ((i_len)  & 0x7)) << 4 \
              | ((arg_type) & 0xf);                            \
      i_eye = i_eye << 1 | ((is_long) & 0x1);} while(0)

#define BOXVM_WRITE_SHORTOP_1ARG(i_pos, i_eye, arg) \
  do {*((i_pos)++) = ((arg) & 0xff) << 16 | (i_eye);} while(0)

#define BOXVM_WRITE_SHORTOP_2ARGS(i_pos, i_eye, arg1, arg2) \
  do {*(i_pos++) = i_eye = \
      (((arg2) & 0xff)<<8 | ((arg1) & 0xff))<<16 | i_eye;} while(0)

#define BOXVM_READ_SHORTOP_1ARG(i_pos, i_eye, arg) \
  do {arg = (signed char) ((i_eye >>= 8) & 0xff);} while(0)

#define BOXVM_READ_SHORTOP_2ARGS(i_pos, i_eye, arg1, arg2) \
  do {arg1 = (signed char) ((i_eye >>= 8) & 0xff); \
      arg2 = (signed char) ((i_eye >>= 8) & 0xff);} while(0)

/* LONG INSTRUCTION: we assemble the istruction header in the following way:
 *  FIRST FOUR BYTES:
 *    bit 0: true if the instruction is long
 *    bit 1-4: type of arguments
 *    bit 5-31: length of instruction
 *  SECOND FOUR BYTES:
 *    bit 0-31: type of instruction
 *  (THIRD FOUR BYTES: argument 1)
 *  (FOURTH FOUR BYTES: argument 2)
 */
#define BOXVM_WRITE_LONGOP_HEADER(i_pos, i_eye, i_type, is_long, i_len, \
                                  arg_type)                             \
  do {i_eye = ((i_len) & 0x07ff)<<4 | ((arg_type) & 0xf);               \
      i_eye = i_eye<<1 | ((is_long) & 0x1);                             \
      *(i_pos++) = i_eye; *(i_pos++) = i_type;} while(0)


#define BOXVM_READ_LONGOP_1ARG(i_pos, i_eye, arg) \
  do {arg = i_eye = *((i_pos)++);} while(0)

#define BOXVM_READ_LONGOP_2ARGS(i_pos, i_eye, arg1, arg2) \
  do {arg1 = *(i_pos++); arg2 = i_eye = *(i_pos++);} while(0)


#define BOXVM_READ_OP_HEADER(op_ptr, op_word, op_type, op_size,         \
                             op_arg_type, op_is_long)                   \
  do {                                                                  \
    (op_word) = *((op_ptr)++);                                          \
    (op_is_long) = ((op_word) & 0x1); (op_word) >>= 1;                  \
    if ((op_is_long)) {                                                 \
      (op_arg_type) = (op_word) & 0xf;                                  \
      (op_size) = ((op_word) >>= 4);                                    \
      (op_type) = *((op_ptr)++);                                        \
    } else {                                                            \
      (op_arg_type) = (op_word) & 0xf;                                  \
      (op_size) = ((op_word) >>= 4) & 0x7;                              \
      (op_type) = ((op_word) >>= 3) & 0xff;                             \
    }                                                                   \
  } while(0)

/**
 * @brief Portable cast from uint8_t to int8_t.
 *
 * The C standard (C99, ISO/IEC 9899:1999) says that conversion from unsigned
 * to signed integer has an implementation-defined result (when the unsigned
 * value cannot be represented in the signed type). This function implements a
 * cast from uint16_t to int16_t with implementation-independent result.
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
 * This function translates a serialized instruction into a #BoxOpDesc
 * structure containing all the information about the instruction.
 */
static inline BoxBool
BoxOpTranslator_Read(BoxOpExecutor *executor,
                     BoxVMWord *bytecode, BoxOpDesc *op) {
  BoxVMWord word1 = bytecode[0];
  const BoxVMInstrDesc *exec_table = executor->vmx->vm->exec_table;

  if (word1 & 0x1) {
    /* Long format. */
    op->args_type = (word1 >> 1) & 0xf;
    op->size = word1 >> 5;
    op->type = bytecode[1];
    if (op->type < BOX_NUM_OPS) {
      const BoxVMInstrDesc *idesc = & exec_table[op->type];
      executor->vmx->idesc = idesc;
      op->has_data = idesc->has_data;
      op->num_args = idesc->num_args;
      if (idesc->num_args == 2) {
        op->args[0] = (BoxInt) BoxInt32_From_UInt32(bytecode[2]);
        op->args[1] = (BoxInt) BoxInt32_From_UInt32(bytecode[3]);
        op->tail = & bytecode[4];
      } else if (idesc->num_args == 1) {
        op->args[0] = (BoxInt) BoxInt32_From_UInt32(bytecode[2]);
        op->tail = & bytecode[3];
      } else
        op->tail = & bytecode[2];
      return BOXBOOL_TRUE;
    }

  } else {
    /* Short format. */
    op->args_type = (word1 >> 1) & 0xf;
    op->size = (word1 >> 5) & 0x7;
    op->type = (word1 >> 8) & 0xff;
    if (op->type < BOX_NUM_OPS) {
      const BoxVMInstrDesc *idesc = & exec_table[op->type];
      executor->vmx->idesc = idesc;
      op->tail = & bytecode[1];
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

#endif /* _BOX_VMOP_PRIV_H */
