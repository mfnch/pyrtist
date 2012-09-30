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

#  include <box/types.h>
#  include <box/callable.h>

/**
 * @brief Define a combination and associate a callable to it.
 *
 * Define the combination of the given type between the child @p child and the
 * parent @parent. The given callable is associated to the combination and the
 * combination node is returned.
 * @param parent The parent type.
 * @param type The type of combination.
 * @param child The child type.
 * @param callable The callable to be associated to the combination.
 * @return The combination node, if the operation succeed else MNUL
 */
BOXEXPORT BoxType *
BoxType_Define_Combination(BoxType *parent, BoxCombType type, BoxType *child,
                           BoxCallable *callable);

/**
 * @brief Find a (possibly inherited) combination <tt>child@@parent</tt>
 * of @c parent.
 *
 * The function returns:
 * - @p child if the combination was found and the type @p child is equal to
 *   the combination's child type;
 * - the expansion type, if the combination was found but @p child must be
 *   expanded;
 * - @c NULL if the procedure was not found.
 *
 * @param parent The parent type.
 * @param comb_type The combination type, see #BoxCombType.
 * @param child The child type.
 * @param expand How @p child compares with the found combination's child type.
 * @return The type of the found combination's child type (@c NULL if the
 *   combination was not found).
 * @see BoxCombType
 * @see BoxType_Find_Own_Combination
 */
BOXEXPORT BoxType *
BoxType_Find_Combination(BoxType *parent, BoxCombType comb_type,
                         BoxType *child, BoxTypeCmp *expand);

/**
 * Similar to #BoxType_Find_Combination, but restrict the search to the
 * @p parent type, excluding the inherited combinations.
 * @param parent The parent type.
 * @param comb_type The combination type, see #BoxCombType.
 * @param child The child type.
 * @param expand How @p child compares with the found combination's child type.
 * @return The type of the found combination's child type (@c NULL if the
 *   combination was not found).
 * @see BoxCombType
 * @see BoxType_Find_Combination
 */
BOXEXPORT BoxType *
BoxType_Find_Own_Combination(BoxType *parent, BoxCombType comb_type,
                             BoxType *child, BoxTypeCmp *expand);

/**
 * Similar to BoxType_Find_Combination, but uses the type ID (BoxTypeId)
 * instead of a BoxType * for the child.
 */
BOXEXPORT BoxType *
BoxType_Find_Combination_With_Id(BoxType *parent, BoxCombType type,
                                 BoxTypeId child_id, BoxTypeCmp *expand);

/**
 * Get details about a combination found with BoxType_Find_Combination.
 * @param comb The combination, as returned by #BoxType_Find_Combination.
 * @param type Where to put the child type (if not NULL).
 * @param cb Where to put the callable (if not NULL).
 * @return BOXBOOL_TRUE if comb is a combination node, BOXBOOL_FALSE otherwise.
 */
BOXEXPORT BoxBool
BoxType_Get_Combination_Info(BoxType *comb, BoxType **child, BoxCallable **cb);

/**
 * @brief Generate a call number for calling a combination from bytecode.
 *
 * Generate a call number for calling the combination @p comb from the virtual
 * machine @p vm. If the operation succeed, the function returns
 * @c BOXBOOL_TRUE and the call number is stored in <tt>*call_num</tt>.
 * @param comb The combination, as returned by #BoxType_Find_Combination.
 * @param vm The virtual machine for which a call number is generated.
 * @param call_num Where to store the call number (ignored if @c NULL).
 * @return A boolean which is true iff the operation succeeded.
 * @remarks Note that - if necessary - the current callable for the combination
 *   is replaced with a corresponding VM callable for @p vm.
 */
BOXEXPORT BoxBool
BoxType_Generate_Combination_CallNum(BoxType *comb, BoxVM *vm,
                                     BoxVMCallNum *call_num);

#endif /* _BOX_COMBS_H */
