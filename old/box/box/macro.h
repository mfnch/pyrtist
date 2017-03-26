/***************************************************************************
 *   Copyright (C) 2012 by Matteo Franchin                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _BOX_MACRO_H
#  define _BOX_MACRO_H

#  include <box/types.h>

/** Enumeration of possible errors resulting from parsing a macro with
 * BoxMacro_Parse.
 */
typedef enum {
  BOXMACROERR_NAME=1,
  BOXMACROERR_COLON,
  BOXMACROERR_NUM_ARGS,
  BOXMACROERR_STRING,
  BOXMACROERR_UNKNOWN

} BoxMacroErr;

#define BOXMACRO_MAX_ARGS 3

struct BoxMacro_struct {
  char   *text;     /**< A NUL terminated string */
  char   *name;     /**< A pointer to the macro name in 'text' */
  char   *args[BOXMACRO_MAX_ARGS];
                    /**< The arguments of the macro in 'text' */
  char   *delim;    /**< First space after argument */
  size_t num_args;  /**< Number of arguments in 'args' */
};

/** Type containing the result of a BoxMacro_Parse call. */
typedef struct BoxMacro_struct BoxMacro;

/** Prepare a 'BoxMacro' object to parse the string 'text'.
 * 'text' is corrupted while parsing, so - if you care about the content of
 * 'text' - you should copy it before calling 'BoxMacro_Parse'.
 */
void BoxMacro_Init(BoxMacro *bm, char *text);

/** Finalise a BoxMacro object. */
#define BoxMacro_Finish(bm)

/** Parse the macro associated to the BoxMacro object. */
BoxMacroErr BoxMacro_Parse(BoxMacro *bm);

/** Get the name of the macro, after a call to BoxMacro_Parse. */
#define BoxMacro_Get_Name(bm) ((bm)->name)

/** Get the number of arguments, after a call to BoxMacro_Parse */
#define BoxMacro_Get_Num_Args(bm) ((bm)->num_args)

/** Get the number of arguments, after a call to BoxMacro_Parse */
#define BoxMacro_Get_Arg(bm, idx) ((bm)->args[(idx)])

#endif /* _BOX_MACRO_H */
