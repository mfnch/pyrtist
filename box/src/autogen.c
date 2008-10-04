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
#include "typesys.h"
#include "compiler.h"
#include "vmalloc.h"
#include "vmsymstuff.h"
#include "autogen.h"

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
      return VM_Sym_Alloc_Method_Register(cmp->vm, sym_num, (Int) t);
    }
  }
  return Success;
}

#define DEBUG 0

#define ASSERT_TASK(x) assert(Success == (x))

static void Create_Iterator_Structure(Type t, Type proc) {
  Type ct = TS_Core_Type(cmp->ts, t);
  Type member_t;
  Expr parent_e, member_e;
  /* A structure is fast if it contains only fast types. fast structures do not
   * need to have an iterator. They do not require to propagate the basic
   * methods.
   */
  int fast_structure = 0;

  assert(TS_Is_Structure(cmp->ts, ct));
  member_t = TS_Member_Next(cmp->ts, ct);
  while(TS_Is_Member(cmp->ts, member_t)) {
    /* Resolve the member into a proper type */
#if DEBUG == 1
    Type rm = TS_Resolve(cmp->ts, member_t, TS_KS_NONE);
    printf("Structure member has type %s\n", TS_Name_Get(cmp->ts, rm));
#endif
    member_t = TS_Member_Next(cmp->ts, member_t);
  }

  if (fast_structure) return;

  /* We need to create the iterator */
#if 0
  ASSERT_TASK( Box_Procedure_Begin(TYPE_VOID, TYPE_VOID, TYPE_VOID) );
  ASSERT_TASK( Box_Parent_Get(& parent_e, 0) );

  member_t = TS_Member_Next(cmp->ts, ct);
  while(TS_Is_Member(cmp->ts, member_t)) {
    char *member_n = TS_Name_Get()
    ASSERT_TASK( Expr_Struc_Member(& member_e, member_t, ) );
    member_t = TS_Member_Next(cmp->ts, member_t);
  }

//   Task Expr_Struc_Member(Expr *m, Expr *s, Name *m_name)

  ASSERT_TASK( Box_Procedure_End(NULL) );
#endif
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
    Type proc, arg_proto;
    /* Now we search if the method already exist */
    TS_Procedure_Inherited_Search(cmp->ts, & proc, & arg_proto,
                                  parent, child, kind);

    /* If the method exist, then make sure that the VM agrees on that! */
    /*if (proc != TYPE_NONE) {
      Int VM_Alloc_Method_Get(cmp->vm, parent, child);

    }*/

    if (proc == TS_TYPE_NONE) {
      Type parent_ct = TS_Core_Type(cmp->ts, parent);
      switch(TS_Kind(cmp->ts, parent_ct)) {
      case TS_KIND_STRUCTURE:
        Create_Iterator_Structure(parent, child);
        break;
      case TS_KIND_SUBTYPE:
        return;
      default:
        return;
      }
    }
  }
}
