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

#if 0
typedef enum {
  BOXOPARGFORM_IMM=0,
  BOXOPARGFORM_LREG,
  BOXOPARGFORM_GREG,
  BOXOPARGFORM_PTR
} BoxOpArgForm;


void BoxOpTranslator_Read(BoxOpTranslator *tr, BoxOp *op) {
  BoxOpWord word1 = tr->bytecode[0];
  if (word1 & 0x1) {
    /* Long format. */
    op->args_type = (word1 >> 1) & 0xf;
    op->size = word1 >> 5;
    op->type = tr->bytecode[1];
  } else {
    /* Short format. */
    op->args_type = (word1 >> 1) & 0xf;
    op->size = (word1 >> 5) & 0x7;
    op->type = (word1 >> 8) & 0xff;
  }
}

#endif
