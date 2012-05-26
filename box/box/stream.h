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
 * @file stream.h
 * @brief Generic binary stream.
 */

#ifndef _BOX_STREAM_H
#  define _BOX_STREAM_H

#  include <box/types.h>
#  include <box/vm.h>

/**
 * Mode of operation of a BoxStream object.
 */
typedef enum {
  BOXSTREAMMODE_RO, /**< Read-only */
  BOXSTREAMMODE_WO, /**< Write-only */
  BOXSTREAMMODE_RW  /**< Read-Write */

} BoxStreamMode;

/**
 * Wrap a FILE object inside a BoxStream object.
 * @param file the source FILE object.
 * @return a new BoxStream object.
 */
BOXEXPORT BoxStream *
  BoxStream_Create_From_File(FILE *file);

/**
 * Wrap a portion of memory inside a BoxStream object.
 * @param src the pointer to the portion of memory.
 * @param src_size the size of the portion of memory.
 * @return a new BoxStream object.
 */
BOXEXPORT BoxStream *
  BoxStream_Create_From_Memory(void *src, size_t src_size);

/**
 * Set the operation mode for the given stream.
 * @param bs the stream.
 * @param m the operation mode.
 * @return whether the operation was successful.
 */
BOXEXPORT BoxTask BoxStream_Set_Mode(BoxStream *bs, BoxStreamMode m);

/**
 * Get the operation mode for the given stream.
 * @param bs the stream.
 * @return the operation mode.
 */
BOXEXPORT BoxStreamMode BoxStream_Get_Mode(BoxStream *bs);

/**
 *
 */
typedef enum {

} BoxStreamErr;

/**
 *
 */
typedef enum {
  BOXSTREAMORIGIN_START,   /**< */
  BOXSTREAMORIGIN_CURRENT, /**< */
  BOXSTREAMORIGIN_END      /**< */

} BoxStreamOrigin;

/**
 *
 * @param bs The stream.
 * @return
 */
BOXEXPORT BoxStreamErr BoxStream_Has_Error(BoxStream *bs);

/**
 *
 * @param bs The stream.
 * @return
 */
BOXEXPORT BoxBool BoxStream_Is_Resizable(BoxStream *bs);

/**
 *
 * @param bs The stream.
 * @return
 */
BOXEXPORT size_t BoxStream_Get_Capacity(BoxStream *bs);

/**
 *
 * @param bs The stream.
 * @return
 */
BOXEXPORT size_t BoxStream_Get_Size(BoxStream *bs);

/**
 *
 * @param bs The stream.
 * @return
 */
BOXEXPORT size_t BoxStream_Get_Position(BoxStream *bs);

/**
 *
 * @param bs The stream.
 * @return
 */
BOXEXPORT void BoxStream_Seek(BoxStream *bs, BoxStreamOrigin orig,
                              ssize_t displacement);

/**
 *
 * @param 
 * @return
 */
BOXEXPORT size_t
BoxStream_Write_From_Mem(BoxStream *dst, const char *src, size_t src_size);

/**
 * Read data from an input stream src and writes it to an output stream dst.
 * @param dst Output stream. 
 * @param src Input stream.
 * @param src_amount Number of bytes to transfer from input to output stream.
 * @return Number of bytes effectively copied.
 */
BOXEXPORT size_t
BoxStream_Write_From_Stream(BoxStream *dst,
                            BoxStream *src, size_t src_amount);

/**
 * Read data from a stream and write it to a block of memory.
 * @param src The input stream.
 * @param dst Pointer to the destination block of memory.
 * @param dst_size Size of the destination block.
 * @return number of bytes written to the destination block.
 */
BOXEXPORT size_t
BoxStream_Read_To_Mem(BoxStream *src, void *dst, size_t *dst_size);

/**
 * Destroy the given stream.
 * @param stream the stream to destroy.
 */
BOXEXPORT void BoxStream_Destroy(BoxStream *stream);

#endif /* _BOX_STREAM_H */
