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
 * @file ast.h
 * @brief Abstract syntax tree related functionality.
 *
 * A nice description...
 */

#ifndef _BOX_AST_PRIV_H
#  define _BOX_AST_PRIV_H

#  include <box/ast.h>

#  include <box/allocpool_priv.h>

/**
 * @brief Implementation of #BoxAST.
 */
struct BoxAST_struct {
  BoxAllocPool pool;  /**< Pool used to allocate the tree's objects. */
  BoxASTNode   *root; /**< Root node of the tree. */
};

/**
 * @brief Initialize a #BoxAST.
 */
BOXEXPORT void
BoxAST_Init(BoxAST *ast);

/**
 * @brief Finalize a #BoxAST object initialized with BoxAST_Init().
 */
BOXEXPORT void
BoxAST_Finish(BoxAST *ast);

#endif /* _BOX_AST_PRIV_H */
