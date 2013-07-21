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

#ifndef _BOX_SRCPOS_H
#  define _BOX_SRCPOS_H

#  include <box/types.h>
#  include <box/array.h>

/** Type for storing the pointers to file names used by BoxSrcPos objects
 * and friends.
 */
typedef struct {
  BoxArr names;
} BoxSrcName;

/** Type for number of line in a source file */
typedef int BoxSrcPosLine;

/** Type for number of column in a source file */
typedef int BoxSrcPosCol;

/** Object which describes the position in a Box source file (which file,
 * which line and column). It is used to associate debug information to VM
 * code, i.e. to state to which line of source a certain fragment of code
 * corresponds.
 */
typedef struct {
  const char    *file_name;
  BoxSrcPosLine line;
  BoxSrcPosCol  col;
} BoxSrcPos;

/** Object used to define a portion of the source file (to locate errors and
 * add ability to complement error messages with the exact location they
 * refer to)
 */
typedef struct {
  BoxSrcPos begin, end;
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
  BoxArr     file_names;
  BoxArr     assoc_table;
} BoxSrcPosTable;

/** Initialise a file name table. */
void BoxSrcName_Init(BoxSrcName *srcname);

/** Finalise a file name table. */
void BoxSrcName_Finish(BoxSrcName *srcname);

/** Object creator for BoxSrcName */
BoxSrcName *BoxSrcName_New(void);

/** Object destructor for BoxSrcName */
void BoxSrcName_Destroy(BoxSrcName *srcname);

/** Add a new name to the file name table. */
char *BoxSrcName_Add(BoxSrcName *srcname, const char *name);

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

/** Initialise a BoxSrc object as "undefined". */
void BoxSrc_Init(BoxSrc *src);

/** Return a string corresponding to the given BoxSrc object.
 * NOTE: the returned string is allocated with Box_Mem_Alloc and should be
 *  deallocated by the user, calling Box_Mem_Free.
 * Example: "line 1 cols 5-15"
 */
char *BoxSrc_To_Str(BoxSrc *loc);

/** Return (in *result) the smallest BoxSrc object containing both first
 * and second.
 */
void BoxSrc_Merge(BoxSrc *result, BoxSrc *first, BoxSrc *second);

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
