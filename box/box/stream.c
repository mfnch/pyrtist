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

#include <stdint.h>
#include <assert.h>

#include <box/types.h>
#include <box/mem.h>
#include <box/messages.h>
#include <box/vm.h>
#include <box/stream_priv.h>

static void MyStream_Finish(BoxStream *bs) {
  Box_Mem_Free(bs->data);
  bs->data = NULL;
}

static int My_Arch_Is_LE(void) {
  union {
    uint8_t  u8x4[4];
    uint32_t u32x1;

  } check;

  check.u32x1 = 0x01020408;

  if (check.u8x4[0] == 0x08 && check.u8x4[1] == 0x04
      && check.u8x4[2] == 0x02 && check.u8x4[3] == 0x01)
    return 1;

  else if (check.u8x4[0] == 0x01 && check.u8x4[1] == 0x02
           && check.u8x4[2] == 0x04 && check.u8x4[3] == 0x08)
    return 0;

  MSG_FATAL("Cannot determine endianness");
  return 0;
}

void *BoxStream_Init_Generic(BoxStream *bs, size_t data_size) {
  bs->native_order = My_Arch_Is_LE();
  bs->data = Box_Mem_Alloc(data_size);
  bs->mode = BOXSTREAMMODE_DEFAULT;
  bs->fn_finish = MyStream_Finish;
  bs->fn_close = NULL;
  bs->fn_write = NULL;
  bs->fn_read = NULL;
  return bs->data;
}

void BoxStream_Finish(BoxStream *bs) {
  if (bs && bs->fn_finish)
    bs->fn_finish(bs);
}

void BoxStream_Destroy(BoxStream *bs) {
  BoxStream_Finish(bs);
  Box_Mem_Free(bs);
}

/* Close the given stream, releasing the associated resources. */
void BoxStream_Close(BoxStream *bs) {
  if (bs && bs->fn_close && !bs->error)
    bs->fn_close(bs);
}

/* Read a block of memory with the given size and writes it to the stream. */
size_t BoxStream_Write_Mem(BoxStream *dst,
                           const void *src, size_t src_size) {
  if (!dst)
    return 0;

  if (dst->fn_write) {
    if (dst->mode & BOXSTREAMMODE_WO)
      return dst->fn_write(dst, src, src_size);

    else {
      dst->error = BOXSTREAMERR_WRITE_DENIED;
      return 0;
    }

  } else {
    dst->error = BOXSTREAMERR_NOT_AVAILABLE;
    return 0;
  }
}

static size_t My_Write_Values(BoxStream *dst, void *vals,
                              size_t num_vals, size_t val_size) {
  if (!(dst && vals))
    return BOXBOOL_FALSE;

  assert(val_size <= 16);

  if (dst->native_order) {
    size_t num_bytes = num_vals*val_size;
    size_t written_bytes = BoxStream_Write_Mem(dst, vals, num_bytes);
    if (written_bytes == num_bytes)
      return num_vals;
    else
      return written_bytes/val_size;

  } else {
    uint8_t xval[16];
    size_t i;

    for (i = 0; i < num_vals; i++) {
      uint8_t *vp = (uint8_t *) vals++;

      int j;      
      for (j = 0; j < val_size; j++)
        xval[j] = vp[val_size - j - 1];

      if (BoxStream_Write_Mem(dst, & xval[0], val_size) != val_size)
        return i;
    }

    return num_vals;
  }
}

/* Writes the given number of uint16_t values to the given output stream. */
size_t BoxStream_Write_UInt16s(BoxStream *dst, uint16_t *vals,
                               size_t num_vals) {
  return My_Write_Values(dst, vals, num_vals, sizeof(uint16_t));
}

/* Writes the given number of uint32_t values to the given output stream. */
size_t BoxStream_Write_UInt32s(BoxStream *dst, uint32_t *vals,
                               size_t num_vals) {
  return My_Write_Values(dst, vals, num_vals, sizeof(uint32_t));
}

/* Writes the given number of uint64_t values to the given output stream. */
size_t BoxStream_Write_UInt64s(BoxStream *dst, uint64_t *vals,
                               size_t num_vals) {
  return My_Write_Values(dst, vals, num_vals, sizeof(uint64_t));
}

/* Read data from a stream and write it to a block of memory. */
size_t BoxStream_Read_Mem(BoxStream *src, void *dst, size_t dst_size) {
  if (!src)
    return 0;

  if (src->fn_read) {
    if (src->mode & BOXSTREAMMODE_RO)
      return src->fn_read(src, dst, dst_size);

    else {
      src->error = BOXSTREAMERR_READ_DENIED;
      return 0;
    }

  } else {
    src->error = BOXSTREAMERR_NOT_AVAILABLE;
    return 0;
  }
}

static size_t My_Read_Values(BoxStream *src, void *vals,
                             size_t num_vals, size_t val_size) {
  if (!(src && vals))
    return BOXBOOL_FALSE;

  assert(val_size <= 16);

  if (src->native_order) {
    size_t num_bytes = num_vals*val_size;
    size_t read_bytes = BoxStream_Read_Mem(src, vals, num_bytes);
    if (read_bytes == num_bytes)
      return num_vals;
    else
      return read_bytes/val_size;

  } else {
    uint8_t xval[16];
    size_t i;

    for (i = 0; i < num_vals; i++) {
      uint8_t *vp = (uint8_t *) vals++;
      int j;

      if (BoxStream_Read_Mem(src, & xval[0], val_size) != val_size)
        return i;

      for (j = 0; j < val_size; j++)
        vp[val_size - j - 1] = xval[j];
    }

    return num_vals;
  }
}

/* Writes the given number of uint16_t values to the given output stream. */
size_t BoxStream_Read_UInt16s(BoxStream *src, uint16_t *vals,
                               size_t num_vals) {
  return My_Read_Values(src, vals, num_vals, sizeof(uint16_t));
}

/* Writes the given number of uint32_t values to the given output stream. */
size_t BoxStream_Read_UInt32s(BoxStream *src, uint32_t *vals,
                              size_t num_vals) {
  return My_Read_Values(src, vals, num_vals, sizeof(uint32_t));
}

/* Writes the given number of uint64_t values to the given output stream. */
size_t BoxStream_Read_UInt64s(BoxStream *src, uint64_t *vals,
                              size_t num_vals) {
  return My_Read_Values(src, vals, num_vals, sizeof(uint64_t));
}
