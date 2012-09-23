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
 * @file cmpptrs.h
 * @brief Helper header included by the sources of the core Box compiler.
 *
 * This header solves one problem: some objects such as BoxVMCode store a
 * pointer to the corresponding compiler. This allows the user not to specify
 * always the couple (BoxCmp, BoxVMCode). On the other side also the BoxCmp
 * structure needs to store a pointer to BoxVMCode. Since we want to declare
 * the two things in different headers we then have a problem: BoxCmp
 * references BoxVMCode, which in turn references BoxCmp. So what object do we
 * need to define first? The solution is to typedef BoxCmp as a pointer to an
 * anonymous structure. This is what BoxCmp needs and this is what we do here.
 */

#ifndef _CMPPTRS_H
#  define _CMPPTRS_H

typedef struct _box_cmp BoxCmp;

#endif /* _CMPPTRS_H */
