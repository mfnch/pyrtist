/***************************************************************************
 *   Copyright (C) 2013 by Matteo Franchin                                 *
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

/**
 * @file parser.h
 * @brief Parser for the Box language.
 *
 * This file declares the functions used for parsing Box source files.
 */

#ifndef _BOX_PARSER_H
#  define _BOX_PARSER_H

#  include <stdlib.h>
#  include <stdint.h>

#  include <box/ast.h>
#  include <box/paths.h>


/**
 * @brief Box parser object.
 */
typedef struct BoxParser_struct BoxParser;


/**
 * @brief Create a new Box lexer object.
 */
BOXEXPORT BoxParser *
BoxParser_Create(BoxPaths *paths);

/**
 * @brief Destroy a Box parser created with BoxParser_Create().
 *
 * @return the list of source file names used in the code labels.
 */
BOXEXPORT BoxAST *
BoxParser_Destroy(BoxParser *bp);

/**
 * @brief Parse the given stream and return an abstract syntax tree.
 *
 * Parse a Box source file from a FILE stream and return the corresponding
 * abstract syntax tree.
 *
 * @param in Input stream.
 * @param in_name File name for the input stream @p in.
 * @param auto_include Source file to be automatically included before starting
 *   to parse the input stream.
 * @param paths Paths to use when resolving includes.
 * @reutrn The corresponding abstract syntax tree or @c NULL in case of errors.
 */
BOXEXPORT BoxAST *
Box_Parse_FILE(FILE *in, const char *in_name,
               const char *auto_include, BoxPaths *paths);

/**
 * @brief Return the next token.
 */
BOXEXPORT BoxBool
BoxParser_Get_Next_Token(BoxParser *bp);

/**
 * @brief Include a new source file.
 */
BOXEXPORT BoxBool
BoxParser_Begin_Include(BoxParser *bp, const char *f);

/**
 * @brief Include a new source file, given its descriptor and file-name.
 */
BOXEXPORT BoxBool
BoxParser_Begin_Include_FILE(BoxParser *bp, FILE *f, const char *fn);

/**
 * @brief Exit from an included source file.
 *
 * @return Whether this was the last included file.
 */
BOXEXPORT BoxBool
BoxParser_End_Include(BoxParser *bp);

/**
 * @brief Check for provided features.
 *
 * Check whether the string feature of length 'length' has been already
 * provided to this function. If the string was never seen before, then
 * return 0 (and does remember the string for the next time the function
 * is called). If the feature was seen before, then return 1.
 * NOTE: This function is called in the 'provide' directive.
 * @return whether the feature was found.
 */
BOXEXPORT BoxBool
BoxParser_Was_Provided(BoxParser *bp, const char *feature, int length);

/**
 * @brief Get the abstract syntax tree associated with the given parser.
 */
BOXEXPORT BoxAST *
BoxParser_Get_AST(BoxParser *bp);

/**
 * @brief Report an error (not related to a particular token).
 *
 * @param bp Parser object.
 * @param fmt Format string.
 * @param ... Arguments corresponding to the format string.
 */
BOXEXPORT void
BoxParser_Log_Err(BoxParser *bp, const char *fmt, ...);

#endif /* _BOX_PARSER_H */
