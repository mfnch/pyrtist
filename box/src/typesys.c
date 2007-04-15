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

/* $Id$ */

#include <assert.h>
#include <string.h>

#include "defaults.h"
#include "types.h"
#include "typesys.h"
#include "messages.h"
#include "mem.h"
#include "array.h"
#include "collection.h"
#include "hashtable.h"

static void try_it(TS *ts) {
  Type ti, tr, tp, t1, t2, t3, t4, t5, t6, t7, t8;
  (void) TS_Intrinsic_New(ts, & ti, sizeof(Int));
  (void) TS_Name_Set(ts, ti, "Int");
  MSG_ADVICE("Definito il tipo '%~s'", TS_Name_Get(ts, ti));
  (void) TS_Intrinsic_New(ts, & tr, sizeof(Real));
  (void) TS_Name_Set(ts, tr, "Real");
  MSG_ADVICE("Definito il tipo '%~s'", TS_Name_Get(ts, tr));
  (void) TS_Intrinsic_New(ts, & tp, sizeof(Point));
  (void) TS_Name_Set(ts, tp, "Point");
  MSG_ADVICE("Definito il tipo '%~s'", TS_Name_Get(ts, tp));
  (void) TS_Array_New(ts, & t1, ti, 10);
  MSG_ADVICE("Definito il tipo '%~s'", TS_Name_Get(ts, t1));
  (void) TS_Structure_Begin(ts, & t2);
  (void) TS_Structure_Add(ts, t2, tr, "x");
  (void) TS_Structure_Add(ts, t2, ti, "stuff");
  (void) TS_Structure_Add(ts, t2, tp, NULL);
  MSG_ADVICE("Definito il tipo '%~s'", TS_Name_Get(ts, t2));
  (void) TS_Structure_Begin(ts, & t3);
  (void) TS_Structure_Add(ts, t3, t1, "my_array");
  (void) TS_Structure_Add(ts, t3, t2, "my_structure");
  (void) TS_Structure_Add(ts, t3, ti, "my_int");
  MSG_ADVICE("Definito il tipo '%~s'", TS_Name_Get(ts, t3));

  (void) TS_Species_Begin(ts, & t4);
  (void) TS_Species_Add(ts, t4, tp);
  (void) TS_Species_Add(ts, t4, ti);
  (void) TS_Species_Add(ts, t4, tr);
  MSG_ADVICE("Definito il tipo '%~s'", TS_Name_Get(ts, t4));

  (void) TS_Enum_Begin(ts, & t5);
  (void) TS_Enum_Add(ts, t5, tr);
  (void) TS_Enum_Add(ts, t5, ti);
  (void) TS_Enum_Add(ts, t5, tp);
  MSG_ADVICE("Definito il tipo '%~s'", TS_Name_Get(ts, t5));

  TS_Member_Find(ts, & t6, t2, "x");
  MSG_ADVICE("Searching member 'x' of structure %~s: %s!",
   TS_Name_Get(ts, t2), t6 == TS_TYPE_NONE ? "not found" : "found");
  TS_Member_Find(ts, & t6, t3, "my_structure");
  MSG_ADVICE("Searching member 'my_structure' of structure %~s: %s!",
   TS_Name_Get(ts, t3), t6 == TS_TYPE_NONE ? "not found" : "found");
  if (t6 != TS_TYPE_NONE) {
    Type mt;
    Int addr;
    TS_Member_Get(ts, & mt, & addr, t6);
    MSG_ADVICE("Member has type %~s and position: %I",
     TS_Name_Get(ts, mt), addr);
  }

  TS_Member_Find(ts, & t6, t2, "xxx");
  MSG_ADVICE("Searching member 'xxx' of structure %~s: %s!",
   TS_Name_Get(ts, t2), t6 == TS_TYPE_NONE ? "not found" : "found");

  (void) TS_Structure_Begin(ts, & t7);
  (void) TS_Structure_Add(ts, t7, tr, NULL);
  (void) TS_Structure_Add(ts, t7, ti, NULL);
  (void) TS_Structure_Add(ts, t7, tp, NULL);
  MSG_ADVICE("%~s ? %~s = %d", TS_Name_Get(ts, t4), TS_Name_Get(ts, ti),
   TS_Compare(ts, t4, ti));
  MSG_ADVICE("%~s ? %~s = %d", TS_Name_Get(ts, t2), TS_Name_Get(ts, t7),
   TS_Compare(ts, t2, t7));

  {
    Type tproc, texp;
    (void) TS_Procedure_New(ts, & t8, ti, tp, 1);
    (void) TS_Procedure_Register(ts, t8, 123);
    MSG_ADVICE("Registered procedure type %~s", TS_Name_Get(ts, t8));
    (void) TS_Procedure_New(ts, & t8, ti, tr, 1);
    (void) TS_Procedure_Register(ts, t8, 124);
    MSG_ADVICE("Registered procedure type %~s", TS_Name_Get(ts, t8));
    (void) TS_Procedure_New(ts, & t8, ti, t4, 1);
    (void) TS_Procedure_Register(ts, t8, 125);
    MSG_ADVICE("Registered procedure type %~s", TS_Name_Get(ts, t8));

    MSG_ADVICE("Searching procedure %~s$%~s",
     TS_Name_Get(ts, ti), TS_Name_Get(ts, tr));
    (void) TS_Procedure_Search(ts, & tproc, & texp, ti, ti, 1);
    MSG_ADVICE("search result=%s, expansion=%~s",
     tproc != TS_TYPE_NONE ? "found" : "not found",
     texp != TS_TYPE_NONE ? TS_Name_Get(ts, texp) : strdup("not needed"));
  }
}

Task TS_Init(TS *ts) {
  TASK( Clc_New(& ts->type_descs, sizeof(TSDesc), TS_TSDESC_CLC_SIZE) );
  HT(& ts->members, TS_MEMB_HT_SIZE);
  TASK( Arr_New(& ts->name_buffer, sizeof(char), TS_NAME_BUFFER_SIZE) );
  try_it(ts);
  return Success;
}

void TS_Destroy(TS *ts) {
  Clc_Destroy(ts->type_descs);
  HT_Destroy(ts->members);
  Arr_Destroy(ts->name_buffer);
}

static TSDesc *Resolve(TS *ts, Type *rt, Type t) {
  while(1) {
    TSDesc *td = Clc_ItemPtr(ts->type_descs, TSDesc, t);
    switch(td->kind) {
    case TS_KIND_LINK:
    case TS_KIND_ALIAS:
    case TS_KIND_MEMBER:
      t = td->target; break;
    default:
      if (rt != (Type *) NULL) *rt = t;
      return td;
    }
  }
}

static TSDesc *Fully_Resolve(TS *ts, Type *rt, Type t) {
  while(1) {
    TSDesc *td = Clc_ItemPtr(ts->type_descs, TSDesc, t);
    switch(td->kind) {
    case TS_KIND_LINK:
    case TS_KIND_ALIAS:
    case TS_KIND_MEMBER:
    case TS_KIND_SPECIES:
      t = td->target; break;
    default:
      if (rt != (Type *) NULL) *rt = t;
      return td;
    }
  }
}

#if 0
void TS_Ref(TS *ts, Type *new, TSDesc *td) {}

void TS_Unref(TS *ts, Type *t) {}
#endif

Int TS_Size(TS *ts, Type t) {
  TSDesc *td = Clc_ItemPtr(ts->type_descs, TSDesc, t);
  return td->size;
}

Int TS_Align(TS *ts, Int address) {
  return address;
}

Task TS_Name_Set(TS *ts, Type t, const char *name) {
  TSDesc *td = Clc_ItemPtr(ts->type_descs, TSDesc, t);
  if (td->name != (char *) NULL) {
    MSG_ERROR("TS_Name_Set: trying to set the name '%s' for type %I: "
     "this type has been already given the name '%s'!", name, t, td->name);
    return Failed;
  }
  td->name = Mem_Strdup(name);
  return Success;
}

char *TS_Name_Get(TS *ts, Type t) {
  TSDesc *td = Clc_ItemPtr(ts->type_descs, TSDesc, t);
  td = Resolve(ts, & t, t);
  if (td->name != (char *) NULL) return Mem_Strdup(td->name);

  switch(td->kind) {
  case TS_KIND_INTRINSIC:
    return printdup("<size=%I>", td->size);

  case TS_KIND_ARRAY:
    if (td->size > 0) {
      Int as = td->data.array_size;
      return printdup("(%I)%~s", as, TS_Name_Get(ts, td->target));
    } else
      return printdup("()%~s", TS_Name_Get(ts, td->target));

  case TS_KIND_STRUCTURE:
#define TS_NAME_GET_CASE_STRUCTURE
#include "tsdef.c"
#undef TS_NAME_GET_CASE_STRUCTURE

  case TS_KIND_SPECIES:
#define TS_NAME_GET_CASE_SPECIES
#include "tsdef.c"
#undef TS_NAME_GET_CASE_SPECIES

  case TS_KIND_ENUM:
#define TS_NAME_GET_CASE_ENUM
#include "tsdef.c"
#undef TS_NAME_GET_CASE_ENUM

  case TS_KIND_PROC:
    {
      char *proc_kind_strs[4] = {"$", "$", "$$", "$&"};
      char *proc_kind_str = proc_kind_strs[td->data.proc.kind & 3];
      return printdup("%~s%s%~s", TS_Name_Get(ts, td->data.proc.parent),
       proc_kind_str, TS_Name_Get(ts, td->target));
    }

  default:
    return Mem_Strdup("<unknown type>");
  }
}

/* The full name to use in the hashtable of members */
static Task Member_Full_Name(TS *ts, Name *n, Type s, const char *m_name) {
  TASK( Arr_Clear(ts->name_buffer) );
  TASK( Arr_MPush(ts->name_buffer, & s, sizeof(Type)) );
  TASK( Arr_MPush(ts->name_buffer, m_name, strlen(m_name)) );
  n->text = Arr_FirstItemPtr(ts->name_buffer, char);
  n->length = Arr_NumItem(ts->name_buffer);
  return Success;
}

void TS_Member_Find(TS *ts, Type *m, Type s, const char *m_name) {
  Name n;
  HashItem *hi;
  *m = TS_TYPE_NONE;;
  if IS_FAILED(Member_Full_Name(ts, & n, s, m_name)) return;
  if (HT_Find(ts->members, n.text, n.length, & hi))
    *m = *((Type *) hi->object);
}

void TS_Member_Get(TS *ts, Type *t, Int *address, Type m) {
  TSDesc *m_td = Clc_ItemPtr(ts->type_descs, TSDesc, m);
  assert(m_td->kind == TS_KIND_MEMBER);
  if (t != (Type *) NULL) *t = m_td->target;
  if (address != (Int *) NULL) *address = m_td->size;
}

Type TS_Member_Next(TS *ts, Type m) {
  TSDesc *td = Clc_ItemPtr(ts->type_descs, TSDesc, m);
  return td->kind == TS_KIND_MEMBER ? td->data.member_next : td->target;
}

/****************************************************************************
 * Here we define functions which have almost common bodies.                *
 * This is done in a tricky way (look at the documentation inside           *
 * the included file!)                                                      *
 ****************************************************************************/

/*FUNCTIONS: TS_X_New *******************************************************/
Task TS_Intrinsic_New(TS *ts, Type *i, Int size) {
  TSDesc td;
  assert(size >= 0);
  TS_TSDESC_INIT(& td);
  td.kind = TS_KIND_INTRINSIC;
  td.size = size;
  td.target = TS_TYPE_NONE;
  TASK( Clc_Occupy(ts->type_descs, & td, i) );
  return Success;
}

Task TS_Procedure_New(TS *ts, Type *p, Type parent, Type child, int kind) {
  TSDesc td;
  TS_TSDESC_INIT(& td);
  td.kind = TS_KIND_PROC;
  td.size = TS_SIZE_UNKNOWN;
  td.target = child;
  td.data.proc.parent = parent;
  td.data.proc.kind = kind & 3;
  td.data.proc.sym_num = 0;
  TASK( Clc_Occupy(ts->type_descs, & td, p) );
  return Success;
}

#define TS_ALIAS_NEW
#include "tsdef.c"
#undef TS_ALIAS_NEW

#define TS_LINK_NEW
#include "tsdef.c"
#undef TS_LINK_NEW

#define TS_ARRAY_NEW
#include "tsdef.c"
#undef TS_ARRAY_NEW

/*FUNCTIONS: TS_X_Begin *****************************************************/
#define TS_STRUCTURE_BEGIN
#include "tsdef.c"
#undef TS_STRUCTURE_BEGIN

#define TS_SPECIES_BEGIN
#include "tsdef.c"
#undef TS_SPECIES_BEGIN

#define TS_ENUM_BEGIN
#include "tsdef.c"
#undef TS_ENUM_BEGIN

/*FUNCTIONS: TS_X_Add *******************************************************/
#define TS_STRUCTURE_ADD
#include "tsdef.c"
#undef TS_STRUCTURE_ADD

#define TS_SPECIES_ADD
#include "tsdef.c"
#undef TS_SPECIES_ADD

#define TS_ENUM_ADD
#include "tsdef.c"
#undef TS_ENUM_ADD

/****************************************************************************/
/* Procedure registration an search */

/* Register the procedure.
 * The way we handle registration and search is very inefficient.
 * this could and should be improved, but we stick to the simple solution
 * for now!
 */
Task TS_Procedure_Register(TS *ts, Type p, UInt sym_num) {
  TSDesc *proc_td, *parent_td;
  Type parent;
  proc_td = Clc_ItemPtr(ts->type_descs, TSDesc, p);
  assert(proc_td->kind == TS_KIND_PROC);
  parent = proc_td->data.proc.parent;
  parent_td = Clc_ItemPtr(ts->type_descs, TSDesc, parent);
  assert(proc_td->first_proc == TS_TYPE_NONE); /* Must not be registered! */
  proc_td->first_proc = parent_td->first_proc;
  parent_td->first_proc = p;
  proc_td->data.proc.sym_num = sym_num;
  return Success;
}

void TS_Procedure_Sym_Num(TS *ts, UInt *sym_num, Type p) {
  TSDesc *proc_td;
  proc_td = Clc_ItemPtr(ts->type_descs, TSDesc, p);
  assert(proc_td->kind == TS_KIND_PROC);
  assert(proc_td->first_proc != TS_TYPE_NONE); /* Must be registered */
  *sym_num = proc_td->data.proc.sym_num;
}

Task TS_Procedure_Search(TS *ts, Type *proc, Type *expansion_type,
 Type parent, Type child, int kind) {
  TSDesc *p_td, *parent_td;
  Type p, dummy;
  if (proc == (Type *) NULL) proc = & dummy;
  if (expansion_type == (Type *) NULL) expansion_type = & dummy;
  *proc = TS_TYPE_NONE;
  *expansion_type = TS_TYPE_NONE;
  parent_td = Clc_ItemPtr(ts->type_descs, TSDesc, parent);
  for(p = parent_td->first_proc; p != TS_TYPE_NONE; p = p_td->first_proc) {
    TSCmp comparison;
    p_td = Clc_ItemPtr(ts->type_descs, TSDesc, p);
    comparison = TS_Compare(ts, p_td->target, child);
    /*MSG_ADVICE("TS_Procedure_Search: considering %~s", TS_Name_Get(ts, p));*/
    if (comparison != TS_TYPES_UNMATCH) {
      if (comparison == TS_TYPES_EXPAND) *expansion_type = p_td->target;
      *proc = p;
      return Success;
    }
  }
  return Success;
}

/****************************************************************************/

Task TS_Default_Value(TS *ts, Type *dv_t, Type t, Data *dv) {
  MSG_ERROR("Still not implemented!"); return Failed;
}

TSCmp TS_Compare(TS *ts, Type t1, Type t2) {
  TSDesc *td1, *td2;
  TSCmp cmp = TS_TYPES_EQUAL;
  if (t1 == t2) return TS_TYPES_EQUAL;
  while(1) {
    td1 = Resolve(ts, & t1, t1);
    td2 = Fully_Resolve(ts, & t2, t2);
    if (t1 == t2) return cmp;

    switch(td1->kind) {
    case TS_KIND_INTRINSIC:
      return TS_TYPES_UNMATCH;

    case TS_KIND_SPECIES:
      {
        Type m = t1;
        while (1) {
          m = TS_Member_Next(ts, m);
          if (m == t1) return TS_TYPES_UNMATCH;
          if (TS_Compare(ts, m, t2) != TS_TYPES_UNMATCH) {
            if (TS_Member_Next(ts, m) == t1) return cmp;
            return cmp & TS_TYPES_EXPAND;
          }
        }
      }

    case TS_KIND_STRUCTURE:
    case TS_KIND_ENUM:
      /* Should we match member names?
       * For now we tolerate: (x=Real, y=Int) == (z=Real, hgj=Int)
       */
      if (td2->kind != td1->kind)
        return TS_TYPES_UNMATCH;
      else {
        Type m1=t1, m2=t2;
        while (1) {
          m1 = TS_Member_Next(ts, m1);
          m2 = TS_Member_Next(ts, m2);
          if (m1 == t1 || m2 == t2) break;
          cmp &= TS_Compare(ts, m1, m2);
          if (cmp == TS_TYPES_UNMATCH) return TS_TYPES_UNMATCH;
        }
        if (m1 != t1 || m2 != t2) return TS_TYPES_UNMATCH;
        return cmp;
      }

    case TS_KIND_ARRAY:
      if (td2->kind != TS_KIND_ARRAY) return TS_TYPES_UNMATCH;
      if (td1->data.array_size == td2->data.array_size) break;
      if (td1->data.array_size != TS_SIZE_UNKNOWN) return TS_TYPES_UNMATCH;
      cmp &= TS_TYPES_MATCH;
      break;

    case TS_KIND_PROC:
      {
        int k1 = td1->data.proc.kind, k2 = td2->data.proc.kind;
        if ((k1 & k2) == 0) return TS_TYPES_UNMATCH;
      }
      if (TS_Compare(ts, td1->data.proc.parent, td2->data.proc.parent)
          != TS_TYPES_EQUAL)
        return TS_TYPES_UNMATCH;
      return TS_Compare(ts, td1->target, td2->target);

    case TS_KIND_POINTER:
    default:
      MSG_ERROR("TS_Compare: not fully implemented!");
      return TS_TYPES_UNMATCH;
    }

    t1 = td1->target;
    t2 = td2->target;
  }
}
