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

#include <box/types.h>
#include <box/mem.h>
#include <box/stream_private.h>

typedef struct {
  FILE            *file;
  BoxStreamFinish saved_fn_finish;

} MyFileStream;


BoxStreamErr MyFileStream_Close(BoxStream *stream) {
  if (stream) {
    MyFileStream *fs = stream->data;

    if (fs->file) {
      /* TODO: do it properly: check errno... */
      if (!fclose(fs->file))
        return BOXSTREAMERR_NONE;
    }
  }

  return BOXSTREAMERR_CLOSE;
}

void MyFileStream_Finish(BoxStream *stream) {
  if (stream) {
    MyFileStream *fs = stream->data;
    (void) MyFileStream_Close(stream);
    stream->fn_finish = fs->saved_fn_finish;
    stream->fn_finish(stream);
  }
}

/* Wrap a FILE object inside a BoxStream object. */
BoxTask BoxStream_Init_From_File(BoxStream *stream, FILE *file) {
  MyFileStream *data =
    BoxStream_Init_Generic(stream, sizeof(MyFileStream));

  if (!data)
    return BOXTASK_FAILURE;

#if 0
  data->saved_fn_finish = stream->fn_finish;
  stream->fn_finish = MyFileStream_Finish;
#endif

  return BOXTASK_OK;
}
