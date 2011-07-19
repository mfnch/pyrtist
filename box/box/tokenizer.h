/***************************************************************************
 *   Copyright (C) 2006 by Matteo Franchin                                 *
 *   fnch@libero.it                                                        *
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

/** @file tokenizer.h
 * @brief The tokenizer for the Box language.
 *
 * This file declares the function used for the lexical analysis of Box
 * sources, which is done using flex.
 */

#ifndef _TOKENIZER_H
#  define _TOKENIZER_H

#  include "srcpos.h"
#  include "ast.h"
#  include <box/paths.h>

/** Box lexer */
typedef struct _struct_BoxLex BoxLex;

/** Create a new Box lexer object. */
BoxLex *BoxLex_Create(BoxPaths *paths);

/** Destroy a Box lexer create with BoxLex_Create.
 * @return the list of source file names used in the code labels.
 */
BoxSrcName *BoxLex_Destroy(BoxLex *bl);

/** Return the next token. */
int BoxLex_Next_Token(BoxLex *bl);

/** Include a new source file. */
BoxTask BoxLex_Begin_Include(BoxLex *bl, const char *f);

/** Include a new source file, given its descriptor and filename. */
BoxTask BoxLex_Begin_Include_FILE(BoxLex *bl, FILE *f, const char *fn);

/** Exit from an included source file.
 * @return 1 if this was the last included file, 0 otherwise.
 */
int BoxLex_End_Include(BoxLex *bl);

/** Checks whether the string feature of length 'length' has been already
 * provided to this function. If the string was never seen before, then
 * return 0 (and does remember the string for the next time the function
 * is called). If the feature was seen before, then return 1.
 * NOTE: This function is called in the '#provide' directive.
 * @return 1 if the feature was found, 0 otherwise.
 */
int BoxLex_Was_Provided(BoxLex *bl, const char *feature, int length);

#endif /* _TOKENIZER_H */
