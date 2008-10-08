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

#if 0
/** Function used to auto-generate the destructor method for the given
 * type.
 */
Task Auto_Destructor_Create(Type t) {
  Int method_num = VM_Alloc_Method_Get(cmp->vm, t, TYPE_DESTROY);
  if (method_num < 0) {
    Type found;
    TS_Procedure_Search(cmp->ts, & found, (Type *) NULL, t, TYPE_DESTROY, 1);
    /*MSG_ADVICE("Searching destructor for %~s: %s", TS_Name_Get(cmp->ts, t),
               (found != TS_TYPE_NONE) ? "found" : "not found");*/
    if (found != TS_TYPE_NONE) {
      UInt sym_num;
      TS_Procedure_Sym_Num_Get(cmp->ts, & sym_num, found);
      return VM_Sym_Alloc_Method_Register(cmp->vm, sym_num, t, TYPE_DESTROY);
    }
  }
  return Success;
}
#endif

#define DEBUG 0

#define ASSERT_TASK(x) assert(Success == (x))

static Task Struc_Destroy(VMProgram *vmp) {
  Obj *structure = BOX_VM_THIS_OBJ(vmp);

  /*printf("Destroying a structure!\n");*/
  return Success;
}

static void Create_Structure_Method(Type parent, Type child, int kind) {
  Type parent_ct = TS_Core_Type(cmp->ts, parent);
  Int call_num, destr_call_num;

  if (TS_Structure_Is_Fast(cmp->ts, parent_ct)) return;

  /* Create an iterator for the core type, if it is not present yet */
  call_num = VM_Alloc_Method_Get(cmp->vm, parent_ct, child);
  if (call_num < 0) {
    Type member_t;
    Expr structure_e, callback_e;

    ASSERT_TASK( Box_Procedure_Begin(parent_ct, child, child) );
    ASSERT_TASK( Box_Parent_Get(& structure_e, 0) );

    member_t = TS_Member_Next(cmp->ts, parent_ct);
    while(TS_Is_Member(cmp->ts, member_t)) {
      if (TS_Is_Fast(cmp->ts, member_t)) {
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
          ASSERT_TASK( Box_Hack2(& member_e, child, kind, BOX_MSG_SILENT) );
          /*ASSERT_TASK( Cmp_Expr_Container_Change(& member_e, CONTAINER_ARG(1)) );*/
          /*(void) Box_Procedure_Call_Void(TYPE_CLOSE, BOX_DEPTH_UPPER, BOX_MSG_SILENT);*/
        }
        ASSERT_TASK( Cmp_Expr_Destroy_Tmp(& member_e) );
      }

      member_t = TS_Member_Next(cmp->ts, member_t);
    }

    ASSERT_TASK( Box_Procedure_End(& call_num) );

    /* Now that a call number has been associated to the procedure, we register
     * it as a special allocation method.
     */
    ASSERT_TASK( VM_Alloc_Method_Set(cmp->vm, parent_ct, child, call_num) );
  }

  /* We finally install the code (a C function) for destroying the structure:
   * all the structures share the same destructor, which uses simply
   * the structure iterator (which is different for each structure) to call
   * the destructors of all its members.
   */
  ASSERT_TASK( VM_Proc_Install_CCode(cmp_vm, & destr_call_num, Struc_Destroy,
                                     "#struc_destroy", "#struc_destroy") );
  /* ... and register it as a destructor for the structure. */
  ASSERT_TASK( VM_Alloc_Method_Set(cmp->vm, parent, TYPE_DESTROY,
                                   destr_call_num) );
}

#if 0
One problem:

  // Define MyType with a creator and a destructor
  num_of_MyType_objs = 0
  MyType = ++(Int id,)
  ([)@MyType[.id = num_of_MyType_objs++]
  (\)@MyType[Print["Destroying MyType object n. ", $$.n;]]

  // Now we embed the object into another object
  Container = (MyType n1, n2, n3)
  c = Container[]

This should work perfectly! But what if we define the desctructor
at the end? Here we show what we mean:

  // Define MyType with a creator and a destructor
  num_of_MyType_objs = 0
  MyType = ++(Int id,)
  ([)@MyType[.id = num_of_MyType_objs++]

  // Now we embed the object into another object
  Container = (MyType n1, n2, n3)
  c = Container[]
  (\)@MyType[Print["Destroying MyType object n. ", $$.n;]]

Should we accept that the MyType objects embedded into 'c'
will not have the destructors called as appropriate
or should we avoid this by signalling an error/warning?
#endif


void Auto_Acknowledge_Call(Type parent, Type child, int kind) {
  /* We check if the child is a special type: ([), (]), (;), (\).
   * These are the case for which we need to propagate the methods from
   * contained objects to their container.
   */
  if (TS_Is_Special(child)) {
    Type found;
    Int call_num = VM_Alloc_Method_Get(cmp->vm, parent, child);
    if (call_num >= 0) return;

    TS_Procedure_Search(cmp->ts, & found, (Type *) NULL, parent, child, 1);
    if (found != TS_TYPE_NONE) {
      UInt sym_num;
      TS_Procedure_Sym_Num_Get(cmp->ts, & sym_num, found);
      ASSERT_TASK( VM_Sym_Alloc_Method_Register(cmp->vm, sym_num,
                                                parent, child) );
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
