/****************************************************************************
 * Copyright (C) 2012 by Matteo Franchin                                    *
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
 * @file combs.h
 * @brief Module to add, find and manipulate type combinations.
 *
 * This header gives access to the part of the Box type system which allows
 * creating and finding combinations. A type combination is a relation
 * between two types.
 */

#ifndef _BOX_COMBS_H
#  define _BOX_COMBS_H

#  include <box/ntypes.h>
#  include <box/callable.h>

/**
 * Define a combination 'child'@'parent' and associate an action to it.
 */
BOXEXPORT BoxBool
BoxType_Define_Combination(BoxXXXX *parent, BoxCombType type, BoxXXXX *child,
                           BoxCallable *callable);

/**
 * Find the procedure 'left'@'right' and return:
 * - 'left' if the procedure was found and the type 'left' is equal to the
 *   procedure left type;
 * - the expansion type, if the procedure was found but 'left' must be
 *   expanded;
 * - BOXTYPE_NONE if the procedure was not found.
 */
BOXEXPORT BoxXXXX *
BoxType_Find_Combination(BoxXXXX *parent, BoxCombType type, BoxXXXX *child,
                         BoxTypeCmp *expand);
/**
 * Similar to BoxType_Find_Combination, but uses the type ID (BoxTypeId)
 * instead of a BoxXXXX * for the child.
 */
BOXEXPORT BoxXXXX *
BoxType_Find_Combination_With_Id(BoxXXXX *parent, BoxCombType type,
                                 BoxTypeId child_id, BoxTypeCmp *expand);

/**
 * Get details about a combination found with BoxType_Find_Combination.
 * @param comb The combination, as returned by BoxType_Find_Combination.
 * @param type Where to put the child type.
 * @param action Where to put the combination action.
 * @return BOXBOOL_TRUE if comb is a combination node, BOXBOOL_FALSE otherwise.
 */
BOXEXPORT BoxBool
BoxType_Get_Combination_Info(BoxXXXX *comb, BoxXXXX **child, BoxCallable **cb);

#endif /* _BOX_COMBS_H */
