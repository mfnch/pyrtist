/***************************************************************************
 *   Copyright (C) 2006 by Matteo Franchin (fnch@libero.it)                *
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

#include <assert.h>

#include "types.h"
/*#include "messages.h"*/
#include "str.h"
#include "typesys.h"
#include "compiler.h"
#include "virtmach.h"
#include "vmalloc.h"
#include "vmproc.h"
#include "vmsymstuff.h"
#include "autogen.h"

static void Create_Structure_Method(Type parent, Type child, int kind) {
  Type parent_ct = TS_Core_Type(cmp->ts, parent);
  Type found;

  TS_Procedure_Search(cmp->ts, & found, (Type *) NULL,
                      parent_ct, child, kind);
  if (found == TS_TYPE_NONE) {
    Expr structure_e, member_e, iter_e;
    int members_left;

    ASSERT_TASK( Box_Def_Begin(parent_ct, child, kind) );
    ASSERT_TASK( Box_Parent_Get(& structure_e, 0) );

    iter_e = structure_e;
    Expr_Struc_Iter(& member_e, & iter_e, & members_left);
    while (members_left > 0) {
      Type member_t = member_e.type;
      if (!TS_Is_Fast(cmp->ts, member_t)) {
        if (member_e.is.value && Tym_Type_Size(member_e.resolved) > 0) {
          (void) Box_Procedure_Call_Void(& member_e, child, kind,
                                         BOX_MSG_SILENT);
        }
      }

      Expr_Struc_Iter(& member_e, & iter_e, & members_left);
    };

    ASSERT_TASK( Box_Def_End() );
  }

}

void Auto_Acknowledge_Call(Type parent, Type child, int kind) {
  /* We check if the child is a special type: ([), (]), (;), (\).
   * These are the case for which we need to propagate the methods from
   * contained objects to their container.
   */
  if (TS_Is_Special(child)) {
    Type found;
    TS_Procedure_Search(cmp->ts, & found, (Type *) NULL,
                        parent, child, kind);
    if (found != TS_TYPE_NONE) {
      if (child == TYPE_DESTROY) {
        UInt sym_num;
        Int call_num = BoxVM_Alloc_Method_Get(cmp->vm, parent, child);
        if (call_num >= 0) return;
        TS_Procedure_Sym_Num_Get(cmp->ts, & sym_num, found);
        ASSERT_TASK( VM_Sym_Alloc_Method_Register(cmp->vm, sym_num,
                                                  parent, child) );
      }
      return;

    } else {
      Type parent_ct = TS_Core_Type(cmp->ts, parent);
      switch(TS_Kind(cmp->ts, parent_ct)) {
      case TS_KIND_STRUCTURE:
        Create_Structure_Method(parent, child, kind);
        break;
      case TS_KIND_SUBTYPE:
        return;
      default:
        return;
      }
    }
  }
}
