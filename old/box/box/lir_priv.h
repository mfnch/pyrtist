/****************************************************************************
 * Copyright (C) 2014 by Matteo Franchin                                    *
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
 * @file lir_priv.h
 * @brief Low-level Intermediate Representation for Box (private).
 */

#ifndef _BOX_LIR_PRIV_H
#  define _BOX_LIR_PRIV_H

#  include <box/lir.h>
#  include <box/logger.h>

#  include <box/allocpool_priv.h>

/**
 * @brief Implementation of #BoxLIR.
 */
struct BoxLIR_struct {
  BoxAllocPool   pool;        /**< Pool used to allocate the tree's objects. */
  BoxLogger      *logger;     /**< Logger object. */
  BoxLIRNodeProc *first_proc, /**< First procedure in the chain. */
                 *last_proc,  /**< Last procedure in the chain. */
                 *target;     /**< Target of instruction append. */
  BoxLIRNodeOp   *last_op;    /**< Last instruction in the target's chain. */
  BoxBool        is_sane;     /**< Whether the LIR tree is sane. */
};

/**
 * @brief Initialize a #BoxLIR.
 */
BOXEXPORT void
BoxLIR_Init(BoxLIR *lir);

/**
 * @brief Finalize a #BoxLIR object initialized with BoxLIR_Init().
 */
BOXEXPORT void
BoxLIR_Finish(BoxLIR *lir);

#endif /* _BOX_LIR_PRIV_H */
