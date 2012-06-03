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

#include <box/types.h>
#include <box/mem.h>
#include <box/vm.h>
#include <box/stream_private.h>

static void MyStream_Finish(BoxStream *bs) {
  BoxMem_Free(bs->data);
  bs->data = NULL;
}

void *BoxStream_Init_Generic(BoxStream *bs, size_t data_size) {
  bs->data = BoxMem_Alloc(data_size);
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
  BoxMem_Free(bs);
}

/* Close the given stream, releasing the associated resources. */
void BoxStream_Close(BoxStream *bs) {
  if (bs && bs->fn_close && !bs->error)
    bs->fn_close(bs);
}

/* Read a block of memory with the given size and writes it to the stream. */
size_t BoxStream_Write_From_Mem(BoxStream *dst,
                                const char *src, size_t src_size) {
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

/* Read data from a stream and write it to a block of memory. */
size_t BoxStream_Read_To_Mem(BoxStream *src, void *dst, size_t dst_size) {
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

#if 0
BoxBool BoxStream_Write_UInt32(BoxStream *src, uint32_t val) {
  if (src) {
    if (src->native_order)
      BoxStream_Write_From_Mem(
    uint8_t buffer[sizeof(uint32_t)];
  }

  return BOXBOOL_FALSE;
}
#endif
