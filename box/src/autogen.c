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
#include "typesys.h"
#include "compiler.h"
#include "vmalloc.h"
#include "vmsymstuff.h"
#include "autogen.h"

/** Function used to auto-generate the destructor method for the given
 * type.
 */
Task Auto_Destructor_Create(Type t) {
  Int method_num = VM_Alloc_Method_Get(cmp->vm, t, VM_OBJ_METHOD_DESTROY);
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

static void Create_Iterator_Structure(Type t) {
  Type ct = TS_Core_Type(cmp->ts, t);
  Type member;
  assert(TS_Is_Structure(cmp->ts, ct));
  member = TS_Member_Next(cmp->ts, ct);
  while(TS_Is_Member(cmp->ts, member)) {
    /* Resolve the member into a proper type */
#ifdef DEBUG
    Type rm = TS_Resolve(cmp->ts, member, TS_KS_NONE);
    printf("Structure member has type %s\n", TS_Name_Get(cmp->ts, rm));
#endif
    member = TS_Member_Next(cmp->ts, member);
  }
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
      Int VM_Alloc_Method_Get(cmp->vm, parent, VMAlcMethod m);

    }*/

    Type parent_ct = TS_Core_Type(cmp->ts, parent);
    switch(TS_Kind(cmp->ts, parent_ct)) {
    case TS_KIND_STRUCTURE:
      Create_Iterator_Structure(parent);
      break;
    case TS_KIND_SUBTYPE:
      return;
    default:
      return;
    }
  }
}
