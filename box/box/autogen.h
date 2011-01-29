/***************************************************************************
 *   Copyright (C) 2006-2011 by Matteo Franchin                            *
 *   fnch@libero.it                                                        *
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

/** @file autogen.h
 * @brief Automatic generation of creators, destructors and similar methods.
 *
 * Imagine you have an object MyObj which has a destructor (= a function
 * to be called before deallocation). Imagine this object is embedded into
 * a structure, such as Struc = (MyObj a, b, c). Then we need to generate
 * a proper destructor for Struc, which calls the destructors for all
 * of its members. This is what this files does.
 */

#ifndef _AUTOGEN_H
#  define _AUTOGEN_H

#  include "types.h"
#  include "cmpptrs.h"

/** Write (in the current active code sheet) the code for the procedure
 * t_child@t_parent. Return 1 if any code has been generated, otherwise
 * return 0.
 */
int Auto_Generate_Code(BoxCmp *c, BoxType t_child, BoxType t_parent);

/** Automatically generate a new procedure for t_child@t_parent, if needed.
 * Return the type for the new procedure if it was generated, otherwise
 * return BOXTYPE_NONE.
 */
BoxType Auto_Generate_Procedure(BoxCmp *c, BoxType t_child, BoxType t_parent);

#endif
