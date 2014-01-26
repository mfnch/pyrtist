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
 * @file lir.h
 * @brief Low-level Intermediate Representation for Box.
 */

#ifndef _BOX_LIR_H
#  define _BOX_LIR_H

#  include <stdint.h>

#  include <box/core.h>


/**
 * @brief Possible types of nodes in the LIR.
 */
typedef enum {
#  define BOXLIRNODE_DEF(NODE, Node) BOXLIRNODETYPE_##NODE,
#  include <box/lirnodes.h>
#  undef BOXLIRNODE_DEF
} BoxLIRNodeType;

/**
 * @brief Box Compiler's LIR object.
 */
typedef struct BoxLIR_struct BoxLIR;

/**
 * @brief Generic LIR node.
 *
 * LIR nodes have type names starting with BoxLIRNode and can all be safely
 * cast to the generic LIR node here defined.
 *
 * @see BOXLIRNODEHEAD
 */
typedef struct BoxLIRNode_struct BoxLIRNode;

/**
 * @brief First element of a #BoxLIRNode.
 *
 * @see #BOXLIRNODEHEAD
 */
struct BoxLIRNodeHead_struct {
  uint8_t type;
};

/**
 * @brief Beginning of every LIR node.
 *
 * The C standard guarantees that the pointer to a structure is always the
 * pointer to its first member. We therefore demand that every LIR node is
 * declared like this:
 * @code
 * typedef struct BoxLIRNodeXXX {
 *   BOXLIRNODEHEAD
 *   // More stuff here ...
 * }
 * @endcode
 * This guarantees that every LIR node can be safely cast to a generic
 * #BoxLIRNode node.
 */
#define BOXLIRNODEHEAD \
  struct BoxLIRNodeHead_struct head;

/**
 * @brief Implementation of a generic LIR node.
 *
 * @see BoxLIRNode
 */
struct BoxLIRNode_struct {
  BOXLIRNODEHEAD
};

/* Define all nodes types. */
#  include <box/lirnodes.h>

#endif /* _BOX_LIR_H */
