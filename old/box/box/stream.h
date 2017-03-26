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

#  include <stdint.h>

#  include <box/types.h>
#  include <box/vm.h>

/**
 * Stream object. Abstraction which provides a single interface to read/write/
 * seek into a file/string/portion of memory.
 */
typedef struct BoxStream_struct BoxStream;

/**
 * Mode of operation of a BoxStream object.
 */
typedef enum {
  BOXSTREAMMODE_RO=0x1,     /**< Read-only */
  BOXSTREAMMODE_WO=0x2,     /**< Write-only */
  BOXSTREAMMODE_RW=0x3,     /**< Read-Write */
  BOXSTREAMMODE_LE=0x0,     /**< Little-endian */
  BOXSTREAMMODE_BE=0x4,     /**< Big-endian */
  BOXSTREAMMODE_DEFAULT=0x3 /**< Default mode */
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
  BOXSTREAMERR_NONE=0,
  BOXSTREAMERR_CLOSE,
  BOXSTREAMERR_NOT_AVAILABLE,
  BOXSTREAMERR_READ,
  BOXSTREAMERR_READ_DENIED,
  BOXSTREAMERR_WRITE,
  BOXSTREAMERR_WRITE_DENIED,
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
                              intptr_t displacement);

/**
 * Close the given stream, releasing the associated resources.
 * @param bs The stream to close.
 */
BOXEXPORT void BoxStream_Close(BoxStream *bs);

/**
 * Read a block of memory with the given size and writes it to the stream.
 * @param dst the output stream.
 * @param src the pointer to the memory block acting as input.
 * @param src_size the size (num bytes) of the memory block pointed by src.
 * @return the number of bytes effectively written.
 */
BOXEXPORT size_t
BoxStream_Write_Mem(BoxStream *dst, const void *src, size_t src_size);

/**
 * Read data from an input stream src and writes it to an output stream dst.
 * @param dst Output stream. 
 * @param src Input stream.
 * @param src_amount Number of bytes to transfer from input to output stream.
 * @return Number of bytes effectively copied.
 */
BOXEXPORT size_t
BoxStream_Write_Stream(BoxStream *dst, BoxStream *src, size_t src_amount);

/**
 * Writes the given uint8_t value to the given output stream.
 * @param dst Output stream.
 * @param val uint8_t value to write.
 * @return return true if the value was correctly written, false otherwise.
 */
#define BoxStream_Write_UInt8(dst, val) \
  (BoxStream_Write_Mem((dst), & (val), sizeof(uint8_t)))

/**
 * Writes the given number of uint16_t values to the given output stream.
 *
 * If all goes well the function returns "num_vals". In case of failures, the
 * number, "n", of values successfully written is returned. Note that the
 * number of bytes written to the stream may be different from
 * n*sizeof(uint16_t). In other words, the failure may cause a value to be
 * partially written. Dealing with this error condition is left completely
 * to the caller.
 *
 * @param dst Output stream. 
 * @param vals Pointer to the array of unit16_t values.
 * @param num_vals Number of uint16_t to write.
 * @return Number of values effectively written to the output stream.
 */
BOXEXPORT size_t
BoxStream_Write_UInt16s(BoxStream *dst, uint16_t *vals, size_t num_vals);

/**
 * Writes the given uint16_t value to the given stream.
 * @param dst Output stream.
 * @param val uint16_t value to write.
 * @return return true if the value was correctly written, false otherwise.
 */
#define BoxStream_Write_UInt16(dst, val) \
  (BoxStream_Write_UInt32s((dst), & (val), (size_t) 1) == (size_t) 1)

/**
 * Similar to BoxStream_Write_UInt16s, but uses uint32_t rather than uint16_t.
 */
BOXEXPORT size_t
BoxStream_Write_UInt32s(BoxStream *dst, uint32_t *vals, size_t num_vals);

/**
 * Similar to BoxStream_Write_UInt16, but uses uint32_t rather than uint16_t.
 */
#define BoxStream_Write_UInt32(dst, val) \
  (BoxStream_Write_UInt32s((dst), & (val), (size_t) 1) == (size_t) 1)

/**
 * Similar to BoxStream_Write_UInt16s, but uses uint64_t rather than uint16_t.
 */
BOXEXPORT size_t
BoxStream_Write_UInt64s(BoxStream *dst, uint64_t *vals, size_t num_vals);

/**
 * Similar to BoxStream_Write_UInt16, but uses uint64_t rather than uint16_t.
 */
#define BoxStream_Write_UInt64(dst, val) \
  (BoxStream_Write_UInt64s((dst), & (val), (size_t) 1) == (size_t) 1)

/**
 * Read data from a stream and write it to a block of memory.
 * @param src The input stream.
 * @param dst Pointer to the destination block of memory.
 * @param dst_size Size of the destination block.
 * @return number of bytes written to the destination block.
 */
BOXEXPORT size_t
BoxStream_Read_Mem(BoxStream *src, void *dst, size_t dst_size);

/**
 * Read a uint8_t value from the given input stream.
 * @param src Input stream.
 * @param val Pointer to a location of memory where to store the uint8_t value.
 * @return return true if the value was correctly read, false otherwise.
 */
#define BoxStream_Read_UInt8(src, val) \
  BoxStream_Write_Stream((src), & (val), sizeof(uint8_t));

/**
 * Reads the given number of uint16_t values to the given block of memory.
 * @param src Input stream.
 * @param vals Pointer to a block of memory which can contain the read values.
 * @param num_vals Number of values to read.
 * @return return the number of values which were read from the stream.
 */
size_t BoxStream_Read_UInt16s(BoxStream *src, uint16_t *vals,
                              size_t num_vals);

/**
 * Similar to BoxStream_Read_UInt16s, but uses uint32_t rather than uint16_t.
 */
size_t BoxStream_Read_UInt32s(BoxStream *src, uint32_t *vals, size_t num_vals);

/**
 * Similar to BoxStream_Read_UInt16s, but uses uint64_t rather than uint16_t.
 */
size_t BoxStream_Read_UInt64s(BoxStream *src, uint64_t *vals, size_t num_vals);

/**
 * Write a formatted string to an output stream (similarly to ``fprintf``).
 * @param dst the destination stream.
 * @param fmt the format string.
 * @param ... the remaining arguments.
 * @return the number of written characters.
 */
BOXEXPORT size_t BoxStream_Write_Fmt(BoxStream *dst, const char *fmt, ...);

/**
 * Destroy the given stream.
 * @param stream the stream to destroy.
 */
BOXEXPORT void BoxStream_Destroy(BoxStream *stream);

#endif /* _BOX_STREAM_H */
