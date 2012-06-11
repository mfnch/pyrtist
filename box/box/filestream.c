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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <box/types.h>
#include <box/mem.h>
#include <box/stream_private.h>

/** Data specific to the File stream implementation. */
typedef struct {
  FILE            *file;
  int             last_errno;

} MyFileStream;


/* Implementation of BoxStream.fn_close */
BoxStreamErr MyFileStream_Close(BoxStream *stream) {
  if (stream) {
    MyFileStream *fs = stream->data;

    if (fs->file) {
      /* TODO: do it properly: check errno... */
      int ret = fclose(fs->file);
      fs->last_errno = errno;
      if (!ret)
        return BOXSTREAMERR_NONE;
    }
  }

  return BOXSTREAMERR_CLOSE;
}

/* Implementation of BoxStream.fn_write */
size_t MyFileStream_Write(BoxStream *bs, const void *src, size_t src_size) {
  MyFileStream *fs = bs->data;
  size_t ret = fwrite(src, 1, src_size, fs->file);
  fs->last_errno = errno;
  return ret;
}

/* Implementation of BoxStream.fn_read */
size_t MyFileStream_Read(BoxStream *bs, void *src, size_t src_size) {
  MyFileStream *fs = bs->data;
  size_t ret = fread(src, 1, src_size, fs->file);
  fs->last_errno = errno;
  return ret;
}

/* Wrap a FILE object inside a BoxStream object. */
BoxTask BoxStream_Init_From_File(BoxStream *bs, FILE *file) {
  MyFileStream *data =
    BoxStream_Init_Generic(bs, sizeof(MyFileStream));

  if (!data)
    return BOXTASK_FAILURE;

  bs->fn_close = MyFileStream_Close;
  bs->fn_write = MyFileStream_Write;
  
  return BOXTASK_OK;
}

/* Wrap a FILE object inside a BoxStream object. */
BoxStream *BoxStream_Create_From_File(FILE *file) {
  BoxStream *bs = BoxMem_Alloc(sizeof(BoxStream));

  if (!bs)
    return NULL;

  if (BoxStream_Init_From_File(bs, file) == BOXTASK_OK)
    return bs;

  BoxMem_Free(bs);
  return NULL;
}
