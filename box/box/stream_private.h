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
 * @file stream_private.h
 * @brief Private header for the stream module.
 */

#ifndef _BOX_STREAM_PRIVATE_H
#  define _BOX_STREAM_PRIVATE_H

#  include <stdlib.h>

#  include <box/types.h>
#  include <box/stream.h>


/**
 * Finalization function.
 */
typedef void (*BoxStreamFinish)(BoxStream *);

/**
 * Close the stream (e.g. using fclose).
 */
typedef BoxStreamErr (*BoxStreamClose)(BoxStream *);

/**
 * Content of a BoxStream object.
 */
struct BoxStream_struct {
  void            *data;
  BoxStreamErr    error;
  BoxStreamFinish fn_finish;
  BoxStreamClose  fn_close;
};

/**
 * Function used by the specialized BoxStream classes for generic
 * initialization of the BoxStream object.
 */
void *BoxStream_Init_Generic(BoxStream *stream, size_t data_size);

#endif /* _BOX_STREAM_PRIVATE_H */
