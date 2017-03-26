/****************************************************************************
 * Copyright (C) 2013 by Matteo Franchin                                    *
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
 * @file parser_priv.h
 * @brief Implementation of #BoxParser.
 */

#ifndef _BOX_PARSER_PRIV_H
#  define _BOX_PARSER_PRIV_H

#  include <box/parser.h>
#  include <box/paths.h>
#  include <box/array.h>
#  include <box/hashtable.h>
#  include <box/srcpos.h>
#  include <box/srcmap.h>
#  include <box/ast.h>


struct BoxParser_struct {
  BoxPaths      *paths;            /**< Paths used to search for files */
  yyscan_t      scanner;           /**< The Lex scanner */
  BoxAST        *ast;              /**< The Abstract Syntax Tree */
  BoxSrcFullPos full_pos;          /**< Full position in the source file. */
  BoxSrc        src;               /**< Position of the current token */
  size_t        max_include_level, /**< Max number of includeable files */
                comment_level;     /**< Multi-comment level of inclusion */
  BoxArr        include_list;      /**< Stack of saved data for include files */
  BoxHT         provided_features; /**< Hashtb. to remember provided features */
  int           parsing_macro;     /**< Whether we are parsing a macro. */
  BoxArr        macro_content;     /**< Content of currently parsed macro */
};

#endif /* _BOX_PARSER_PRIV_H */
