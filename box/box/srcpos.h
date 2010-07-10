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

#  include <stdlib.h>
#  include <box/types.h>
#  include <box/array.h>

/** Object which describes the position in a Box source file (which file,
 * which line and column). It is used to associate debug information to VM
 * code, i.e. to state to which line of source a certain fragment of code
 * corresponds.
 */
typedef struct {
  const char *file_name;
  size_t     line, col;
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

/** Advance the given BoxSrcPos object to the beginning of the next line
 * in the file.
 */
void BoxSrcPos_Next_Line(BoxSrcPos *pos);

/** Return whether the given position object is undefined. */
int BoxSrcPos_Is_Undef(BoxSrcPos *pos);

/** Set the given position object to undefined. */
void BoxSrcPos_Set_Undef(BoxSrcPos *pos);

/** Return a string corresponding to the given BoxSrc object.
 * NOTE: the returned string is allocated with BoxMem_Alloc and should be
 *  deallocated by the user, calling BoxMem_Free.
 * Example: "line 1 cols 5-15"
 */
const char *BoxSrc_To_Str(BoxSrc *loc);

/** Return (in *result) the smallest ASTLoc object containing both first
 * and second.
 */
void BoxSrc_Merge(BoxSrc *result, BoxSrc *first, BoxSrc *second);

#endif
