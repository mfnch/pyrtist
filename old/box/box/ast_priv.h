/****************************************************************************
 * Copyright (C) 2009 by Matteo Franchin                                    *
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
 * @file ast_priv.h
 * @brief Private definitions for ast_priv.h.
 */

#ifndef _BOX_AST_PRIV_H
#  define _BOX_AST_PRIV_H

#  include <stdint.h>

#  include <box/logger.h>
#  include <box/srcmap.h>
#  include <box/ast.h>
#  include <box/hashtable.h>

#  include <box/allocpool_priv.h>
#  include <box/index_priv.h>


/**
 * @brief Implementation of #BoxAST.
 */
struct BoxAST_struct {
  BoxAllocPool pool;       /**< Pool used to allocate the tree's objects. */
  BoxLogger    *logger;    /**< Logger object. */
  BoxASTNode   *root;      /**< Root node of the tree. */
  BoxSrcMap    *src_map;   /**< Position mapping object. */
  BoxIndex     src_names;  /**< Map file name -> number (used in src_map). */
  BoxBool      src_map_ok; /**< Whether there were errors with src_map. */
  BoxBool      is_sane;    /**< Whether the AST is sane (has no errors). */
};

/**
 * @brief Initialize a #BoxAST.
 *
 * @param ast Memory to initialize.
 * @param logger Target for reporting error messages (@c NULL for default).
 */
BOXEXPORT void
BoxAST_Init(BoxAST *ast, BoxLogger *logger);

/**
 * @brief Finalize a #BoxAST object initialized with BoxAST_Init().
 */
BOXEXPORT void
BoxAST_Finish(BoxAST *ast);

#endif /* _BOX_AST_PRIV_H */
