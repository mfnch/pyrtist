/***************************************************************************
 *   Copyright (C) 2006 by Matteo Franchin                                 *
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

#include "types.h"
#include "typesys.h"
#include "compiler.h"
#include "vmalloc.h"
#include "vmsymstuff.h"
#include "autogen.h"

/** Function used to auto-generate the destructor method for the given
 * type.
 */
Task Auto_Destructor_Create(Type t) {
  Int method_num = VM_Alloc_Method_Get(cmp->vm, t, VM_ALC_DESTRUCTOR);
  if (method_num < 0) {
    Type found;
    TS_Procedure_Search(cmp->ts, & found, (Type *) NULL, t, TYPE_DESTROY, 1);
    if (found != TS_TYPE_NONE) {
      UInt sym_num;
      TS_Procedure_Sym_Num(cmp->ts, & sym_num, found);
      return VM_Sym_Alloc_Method_Register(cmp->vm, sym_num, (Int) t);
    }
  }
  return Success;
}

void Auto_Acknowledge_Call(Type parent, Type child, int kind) {
}
