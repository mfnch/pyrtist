/****************************************************************************
 * Copyright (C) 2010 by Matteo Franchin                                    *
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
 * @file srcpos.h
 * @brief Definitions of source position types.
 */

#ifndef _BOX_SRCPOS_H
#  define _BOX_SRCPOS_H

#  include <stdint.h>

#  include <box/types.h>
#  include <box/array.h>


/**
 * @brief Integer type used to store line numbers in source files.
 */
typedef uint32_t BoxSrcLine;

/**
 * @brief Integer type used to store column numbers in source files.
 */
typedef uint32_t BoxSrcCol;

/**
 * @brief Array of file names.
 *
 * Type for storing the pointers to file names used by #BoxSrcPos objects
 * and friends.
 */
typedef struct {
  BoxArr names;
} BoxSrcName;


typedef struct {
  const char *file_name;
  BoxSrcLine line;
  BoxSrcCol  col;
} BoxSrcPos;

/**
 * @brief Linear position of a character in a source file (>= 1).
 *
 * This is simply the number of characters read by the tokenizer so far.
 * This is a positive integer number and is therefore easier to handle than
 * a full position (#BoxSrcFullPos).
 */
typedef unsigned int BoxSrcLinPos;

typedef unsigned int BoxSrcIdx;

/**
 * @brief Identify a portion of source code.
 *
 * Object used to define a portion of the source file (to locate errors and add
 * ability to improve error messages by specifying the exact portion of source
 * code they refer to).
 */
typedef struct {
  BoxSrcLinPos begin, end;
} BoxSrc;

/** "Linear" position in the source */
typedef long BoxOutPos;

/** Association between the position in the source and the position in the
 * output (just an integer) */
typedef struct {
  BoxSrcPos src_pos;
  BoxOutPos out_pos;
} BoxSrcAssoc;

/** Object containing information about where */
typedef struct {
  BoxArr    file_names;
  BoxArr    assoc_table;
} BoxSrcPosTable;

/** Initialise a file name table. */
void BoxSrcName_Init(BoxSrcName *srcname);

/** Finalise a file name table. */
void BoxSrcName_Finish(BoxSrcName *srcname);

/**
 * @brief Creator for #BoxSrcName.
 */
BOXEXPORT BoxSrcName *
BoxSrcName_Create(void);

/**
 * @brief Destructor for #BoxSrcName.
 */
BOXEXPORT void
BoxSrcName_Destroy(BoxSrcName *srcname);

/**
 * @brief Add a new name to the file name table.
 *
 * @param sn List of names to which @p name is added.
 * @param name Name to add.
 * @return A copy of the original string. Allocation responsibility for it lies
 *   with the #BoxSrcName object, which must therefore exists while the pointer
 *   returned by this function is used.
 */
BOXEXPORT const char *
BoxSrcName_Add(BoxSrcName *sn, const char *name);

/** Initialise the source position table pointed by 'pt'. */
void BoxSrcPosTable_Init(BoxSrcPosTable *pt);

/** Finalise the source position table pointed by 'pt'. */
void BoxSrcPosTable_Finish(BoxSrcPosTable *pt);

/** Reset the position table clearing all its content. */
void BoxSrcPosTable_Clear(BoxSrcPosTable *pt);

/** Reduce the memory required by the table to the minimum necessary. */
void BoxSrcPosTable_Compactify(BoxSrcPosTable *pt);

/** Associate the output position 'op' to the source position 'sp'. */
void BoxSrcPosTable_Associate(BoxSrcPosTable *pt,
                              BoxOutPos op, BoxSrcPos *sp);

BoxSrcPos *BoxSrcPosTable_Get_Src_Of(BoxSrcPosTable *pt, BoxOutPos o_pos);

/** Initialise a source positon object, BoxSrcPos, to point at the beginning
 * of the given file.
 */
void BoxSrcPos_Init(BoxSrcPos *pos, const char *file_name);

/** Return a string corresponding to the given BoxSrcPos object.
 * NOTE: the returned string is allocated with Box_Mem_Alloc and should be
 *  deallocated by the user, calling Box_Mem_Free.
 */
char *BoxSrcPos_To_Str(BoxSrcPos *pos);

/** Advance the given BoxSrcPos object to the beginning of the next line
 * in the file.
 */
void BoxSrcPos_Next_Line(BoxSrcPos *pos);

/** Return whether the given position object is undefined. */
int BoxSrcPos_Is_Undef(BoxSrcPos *pos);

/** Set the given position object to undefined. */
void BoxSrcPos_Set_Undef(BoxSrcPos *pos);

/**
 * @brief Initialise a #BoxSrc object as "undefined".
 */
BOXEXPORT void
BoxSrc_Init(BoxSrc *src);

/**
 * @brief Return a string corresponding to the given #BoxSrc object.
 *
 * @param The input object.
 * @return Something like "line 1 cols 5-15"
 * @note the returned string is allocated with Box_Mem_Alloc() and should be
 *  deallocated by the user, calling Box_Mem_Free().
 */
BOXEXPORT char *
BoxSrc_To_Str(BoxSrc *loc);

/**
 * @brief Return the smallest #BoxSrc object containing the given two.
 *
 * @param result This is where the result is returned.
 * @param first The first object.
 * @param second The second object.
 */
BOXEXPORT void
BoxSrc_Merge(BoxSrc *result, BoxSrc *first, BoxSrc *second);

/** Set the line number in the BoxSrcPos structure. */
#define BoxSrcPos_Set_Line(bl, line_num) \
  do {(bl)->line = (line_num);} while(0)

/** Set the column number in the BoxSrcPos structure. */
#define BoxSrcPos_Set_Col(bl, col_num) \
  do {(bl)->col = (col_num);} while(0)

/** Set the file name in the BoxSrcPos structure. */
#define BoxSrcPos_Set_File_Name(bl, name_of_file) \
  do {(bl)->file_name = (name_of_file);} while(0)

#endif
