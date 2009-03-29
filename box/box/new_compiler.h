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

/** @file compiler.h
 * @brief The compiler of Box.
 *
 * A nice description...
 */

#ifndef _NEW_COMPILER_H
#  define _NEW_COMPILER_H

#include "types.h"
#include "virtmach.h"
#include "ast.h"

typedef struct {
  BoxArr stack;
  BoxVM vm;
} BoxCmp;

void BoxCmp_Init(BoxCmp *c);

void BoxCmp_Finish(BoxCmp *c);

BoxCmp *BoxCmp_New(void);

void BoxCmp_Destroy(BoxCmp *c);

void BoxCmp_Compile(BoxCmp *c, ASTNode *program);

#endif /* _COMPILER_H */
