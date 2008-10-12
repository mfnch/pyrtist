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
    Type member_t;
    Expr structure_e;

    ASSERT_TASK( Box_Def_Begin(parent_ct, child, kind) );
    ASSERT_TASK( Box_Parent_Get(& structure_e, 0) );

    member_t = TS_Member_Next(cmp->ts, parent_ct);
    while(TS_Is_Member(cmp->ts, member_t)) {
      if (!TS_Is_Fast(cmp->ts, member_t)) {
        Expr member_e;
        char *member_str = TS_Member_Name_Get(cmp->ts, member_t);
        Name member_n;
        Name_From_Str(& member_n, member_str);
        /* NOTE: This is silly: we should able to iterate over structure members
         * without hashtable lookups! We can already do it, see structure.c
         * we should just make this more robust and change the following lines!
         */
        ASSERT_TASK( Expr_Struc_Member(& member_e, & structure_e, & member_n) );
        if (member_e.is.value && Tym_Type_Size(member_e.resolved) > 0) {
          (void) Box_Procedure_Call_Void(& member_e, child, kind,
                                         BOX_MSG_SILENT);
        }
        ASSERT_TASK( Cmp_Expr_Destroy_Tmp(& member_e) );
      }

      member_t = TS_Member_Next(cmp->ts, member_t);
    }

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
        Int call_num = VM_Alloc_Method_Get(cmp->vm, parent, child);
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
