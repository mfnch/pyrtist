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

static TS *last_ts; /* Just for transition: will be removed! */

static void try_it(TS *ts) {
  Type ti, tr, tp, t1, t2, t3, t4, t5, t6, t7, t8;
  (void) TS_Intrinsic_New(ts, & ti, sizeof(Int));
  (void) TS_Name_Set(ts, ti, "Int");
  MSG_ADVICE("Definito il tipo '%~s'", TS_Name_Get(ts, ti));
  MSG_ADVICE("Il corrispondente numero e' %d", (int) ti);
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
  last_ts = ts; /* Just for transition: will be removed! */
  /*try_it(ts);*/
  return Success;
}

void TS_Destroy(TS *ts) {
  Clc_Destroy(ts->type_descs);
  HT_Destroy(ts->members);
  Arr_Destroy(ts->name_buffer);
}

static Task Type_New(TS *ts, Type *new_type, TSDesc *td) {
  Int nt;
  TASK( Clc_Occupy(ts->type_descs, td, & nt) );
  *new_type = nt-1;
  return Success;
}

static TSDesc *Type_Ptr(TS *ts, Type t) {
  return Clc_ItemPtr(ts->type_descs, TSDesc, t+1);
}

static TSDesc *Resolve(TS *ts, Type *rt, Type t) {
  while(1) {
    TSDesc *td = Type_Ptr(ts, t);
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
    TSDesc *td = Type_Ptr(ts, t);
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

Type TS_Resolve(TS *ts, Type t, int resolve_alias, int resolve_species) {
  int resolve;
  while(1) {
    TSDesc *td = Type_Ptr(ts, t);
    switch(td->kind) {
    case TS_KIND_LINK: resolve = 1; break;
    case TS_KIND_MEMBER: resolve = 1; break;
    case TS_KIND_ALIAS: resolve = resolve_alias; break;
    case TS_KIND_SPECIES: resolve = resolve_species; break;
    default: resolve = 0; break;
    }
    if (!resolve) return t;
    t = td->target;
  }
}

#if 0
void TS_Ref(TS *ts, Type *new, TSDesc *td) {}

void TS_Unref(TS *ts, Type *t) {}
#endif

Int TS_Size(TS *ts, Type t) {
  TSDesc *td = Type_Ptr(ts, t);
  return td->size;
}

TSKind TS_What_Is(TS *ts, Type t) {
  TSDesc *td = Type_Ptr(ts, t);
  return td->kind;
}

Int TS_Align(TS *ts, Int address) {
  return address;
}

Task TS_Name_Set(TS *ts, Type t, const char *name) {
  TSDesc *td = Type_Ptr(ts, t);
  if (td->name != (char *) NULL) {
    MSG_ERROR("TS_Name_Set: trying to set the name '%s' for type %I: "
     "this type has been already given the name '%s'!", name, t, td->name);
    return Failed;
  }
  td->name = Mem_Strdup(name);
  return Success;
}

char *TS_Name_Get(TS *ts, Type t) {
  TSDesc *td = Type_Ptr(ts, t);
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
  TSDesc *m_td = Type_Ptr(ts, m);
  assert(m_td->kind == TS_KIND_MEMBER);
  if (t != (Type *) NULL) *t = m_td->target;
  if (address != (Int *) NULL) *address = m_td->size;
}

Type TS_Member_Next(TS *ts, Type m) {
  TSDesc *td = Type_Ptr(ts, m);
  return td->kind == TS_KIND_MEMBER ? td->data.member_next : td->target;
}

Int TS_Member_Count(TS *ts, Type s) {
  Int count = 0;
  TSDesc *td = Type_Ptr(ts, s);
  switch(td->kind) {
  case TS_KIND_SPECIES:
  case TS_KIND_STRUCTURE:
  case TS_KIND_ENUM:
    while(1) {
      Int next = TS_Member_Next(ts, s);
      if (! TS_Is_Member(ts, next)) break;
      ++count;
    };
  default:
    MSG_FATAL("Trying to count members of a the non-membered type %~s",
     TS_Name_Get(ts, s));
  }
  return count;
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
  TASK( Type_New(ts, i, & td) );
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
  TASK( Type_New(ts, p, & td) );
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
  proc_td = Type_Ptr(ts, p);
  assert(proc_td->kind == TS_KIND_PROC);
  parent = proc_td->data.proc.parent;
  parent_td = Type_Ptr(ts, parent);
  assert(proc_td->first_proc == TS_TYPE_NONE); /* Must not be registered! */
  proc_td->first_proc = parent_td->first_proc;
  parent_td->first_proc = p;
  proc_td->data.proc.sym_num = sym_num;
  return Success;
}

void TS_Procedure_Sym_Num(TS *ts, UInt *sym_num, Type p) {
  TSDesc *proc_td;
  proc_td = Type_Ptr(ts, p);
  assert(proc_td->kind == TS_KIND_PROC);
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
  parent_td = Type_Ptr(ts, parent);
  for(p = parent_td->first_proc; p != TS_TYPE_NONE; p = p_td->first_proc) {
    TSCmp comparison;
    p_td = Type_Ptr(ts, p);
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

















/****************************************************************************/
/* Code for transition from typeman.c to typesys.c
 * This code reimplement typeman.c as a wrapper around typesys.c
 */

void Tym_Procedure_Sym_Num(UInt *sym_num, Type p) {
  TS_Procedure_Sym_Num(last_ts, sym_num, p);
}

#ifdef EMULATE_TYPEMAN

#include <stdlib.h>

#include "compiler.h"
#include "str.h"

Int Tym_Type_Size(Int t) {return (Int) TS_Size(last_ts, (Type) t);}

TypeOfType Tym_Type_TOT(Int t) {
  switch(TS_What_Is(last_ts, (Type) t)) {
  case TS_KIND_INTRINSIC: return TOT_INSTANCE;
  case TS_KIND_LINK: return TOT_ALIAS_OF;
  case TS_KIND_ALIAS: return TOT_ALIAS_OF;
  case TS_KIND_SPECIES: return TOT_SPECIE;
  case TS_KIND_STRUCTURE: return TOT_STRUCTURE;
  case TS_KIND_MEMBER: return TOT_ALIAS_OF;
  case TS_KIND_ARRAY: return TOT_ARRAY_OF;
  case TS_KIND_PROC:
    return TOT_PROCEDURE;
    return TOT_PROCEDURE2;
  case TS_KIND_POINTER: return TOT_PTR_TO;
  case TS_KIND_ENUM:
    assert(0);
    return 0;
  }
}

const char *Tym_Type_Name(Int t) {
  static char *last_name = (char *) NULL;
  if (last_name != (char *) NULL) {
    free(last_name);
    last_name = (char *) NULL;
  }
  last_name = TS_Name_Get(last_ts, (Type) t);
  return last_name;
}

char *Tym_Type_Names(Int t) {
  static int tym_num_name = -1;
  static char *tym_name[TYM_NUM_TYPE_NAMES];

  register char *str;
  if ( tym_num_name < 0 ) {
    int i;
    for(i = 0; i < TYM_NUM_TYPE_NAMES; i++) tym_name[i] = (char *) NULL;
    tym_num_name = 0;
  }
  free(tym_name[tym_num_name]);
  str = strdup(Tym_Type_Name(t));
  tym_name[tym_num_name] = str;
  tym_num_name = (tym_num_name + 1) % TYM_NUM_TYPE_NAMES;
  return str;
}

Task Tym_Def_Type(Int *new_type,
 Int parent, Name *nm, Int size, Int aliased_type) {
  Symbol *s;
  Type type;
  char *name;

  /* First of all I create the symbol with name *nm */
  assert(parent == TYPE_NONE);
  TASK( Sym_Explicit_New(& s, nm, 0) );
  s->symattr.is_explicit = 1;

  /* Now I create a new type for the box */
  if ( size < 0 ) {
    TASK( TS_Alias_New(last_ts, & type, aliased_type) );

  } else {
    TASK(TS_Intrinsic_New(last_ts, & type, size));
  }
  name = Name_To_Str(nm);
  (void) TS_Name_Set(last_ts, type, name);
  free(name);

  /* I set all the remaining values of the structure s */
  s->symtype = VARIABLE;
  *new_type = type;
  Expr_New_Type(& s->value, type);
  return Success;
}

Int Tym_Def_Array_Of(Int num, Int type) {
  Type array;
  assert(TS_Array_New(last_ts, & array, type, num) == Success);
  return array;
}

Int Tym_Def_Alias_Of(Name *nm, Int type) {
  Type alias;
  assert(TS_Alias_New(last_ts, & alias, type) == Success);
  return alias;
}

int Tym_Compare_Types(Intg type1, Intg type2, int *need_expansion) {
  switch(TS_Compare(last_ts, type1, type2)) {
  case TS_TYPES_MATCH:
  case TS_TYPES_EXPAND:
    *need_expansion = 1;
    return 1;
  case TS_TYPES_EQUAL:
    *need_expansion = 0;
    return 1;
  default:
  case TS_TYPES_UNMATCH:
    *need_expansion = 0;
    return 0;
  }
}

Int Tym_Type_Resolve(Int type, int not_alias, int not_species) {
  return TS_Resolve(last_ts, type, ! not_alias, ! not_species);
}

Int Tym_Def_Procedure(Int proc, int second, Int of_type, Int sym_num) {
  Type procedure;
  int kind = second ? 2 : 1;
  assert(TS_Procedure_New(last_ts, & procedure, of_type, proc, kind)==Success);
  assert(TS_Procedure_Register(last_ts, procedure, sym_num)==Success);
  return procedure;
}

Int Tym_Search_Procedure(Int proc, int second, Int of_type,
                         Int *exp) {

  Type found;
  int kind = second ? 2 : 1;
  assert(TS_Procedure_Search(last_ts, & found, exp, of_type, proc, kind)
         ==Success);
  return found;
}

Task Tym_Def_Specie(Int *specie, Int type) {
  if (*specie == TYPE_NONE) {
    TASK( TS_Species_Begin(last_ts, specie) );
  }
  return TS_Species_Add(last_ts, *specie, type);
}

Task Tym_Def_Structure(Int *strc, Intg type) {
  if (*strc == TYPE_NONE) {
    TASK( TS_Structure_Begin(last_ts, strc) );
  }
  return TS_Structure_Add(last_ts, *strc, type, NULL);
}

Task Tym_Specie_Get(Int *type) {
  *type = TS_Member_Next(last_ts, *type);
  return Success;
}

Task Tym_Structure_Get(Int *type) {
  *type = TS_Member_Next(last_ts, *type);
  return Success;
}

Int Tym_Specie_Get_Target(Int type) {
  TSDesc *s_td = Type_Ptr(last_ts, type);
  return s_td->data.last;
}

UInt Tym_Proc_Get_Sym_Num(Int t) {
  UInt sym_num;
  TS_Procedure_Sym_Num(last_ts, & sym_num, t);
  return sym_num;
}

Int Tym_Struct_Get_Num_Items(Int t) {
  return TS_Member_Count(last_ts, t);
}

#else

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "str.h"
#include "virtmach.h"
#include "registers.h"
#include "compiler.h"
#include "box.h"

/* #define DEBUG_COMPARE_TYPES */

/******************************************************************************
 * Le procedure che seguono servono a gestire i tipi definiti dall'utente.    *
 ******************************************************************************/
static TypeDesc *tym_recent_typedesc;
static Int tym_recent_type;
static char *tym_special_name[PROC_SPECIAL_NUM] = {
  "",
  "{copy}", "{destroy}"
};

static Array *tym_type_list = NULL;
static int tym_num_name = -1;
static char *tym_name[TYM_NUM_TYPE_NAMES];

/* DESCRIPTION: This function returns the integer index that will
 *  be associated to the new type, that will be created by the next call
 *  to the function Tym_Type_New.
 */
static Intg Tym_Type_Next(void) {
  if ( tym_type_list == NULL ) return 0;
  return Arr_NumItem(tym_type_list);
}

/* DESCRIZIONE: Inserisce un nuovo tipo nella lista dei tipi e restituisce
 *  il puntatore dell'elemento inserito all'interno della lista (o NULL
 *  in caso di errore).
 * NOTE: It updates tym_recent_type and tym_recent_typedesc.
 */
static TypeDesc *Tym_Type_New(Name *nm) {
  TypeDesc td;

  /* Se l'array dei tipi non e' stato inizializzato, lo faccio ora */
  if ( tym_type_list == NULL ) {
    tym_type_list = Array_New(sizeof(TypeDesc), CMP_TYPICAL_NUM_TYPES);
    if ( tym_type_list == NULL ) return NULL;
  }

  td.name = Str_Dup(nm->text, nm->length);
  td.parent = TYPE_NONE;
  td.greater = TYPE_NONE;
  td.procedure = TYPE_NONE;

  /* Inserisce il nuovo tipo nella lista */
  if IS_FAILED(Arr_Push(tym_type_list, & td)) return NULL;

  tym_recent_type = Arr_NumItem(tym_type_list) - 1;
  return (tym_recent_typedesc = Arr_LastItemPtr(tym_type_list, TypeDesc));
}

/* DESCRIPTION: Returns the descriptor of the type t.
 * NOTE: It updates tym_recent_type and tym_recent_typedesc.
 */
static TypeDesc *Tym_Type_Get(Intg t) {
  MSG_LOCATION("Tym_Type_Get");

  if ( (t < 0) || (t >= Arr_NumItem(tym_type_list)) ) {
    MSG_ERROR("Tipo sconosciuto!");
    return NULL;
  }

  tym_recent_type = t;
  return (tym_recent_typedesc = Arr_ItemPtr(tym_type_list, TypeDesc, t+1));
}

/* DESCRIPTION: Returns the size of an object of type t
 *  (or -1 in case of errors).
 */
Intg Tym_Type_Size(Intg t) {
  TypeDesc *td;
  MSG_LOCATION("Tym_Type_Size");

  if ( (t < 0) || (t >= Arr_NumItem(tym_type_list)) ) return -1;
  td = Arr_ItemPtr(tym_type_list, TypeDesc, t+1);
  return td->size;
}

TypeOfType Tym_Type_TOT(Int t) {
  TypeDesc *td;
  assert(t >= 0 && t < Arr_NumItem(tym_type_list));
  td = Arr_ItemPtr(tym_type_list, TypeDesc, t+1);
  return td->tot;
}

UInt Tym_Proc_Get_Sym_Num(Int t) {
  TypeDesc *td;
  assert(t >= 0 && t < Arr_NumItem(tym_type_list));
  td = Arr_ItemPtr(tym_type_list, TypeDesc, t+1);
  assert(td->tot == TOT_PROCEDURE || td->tot == TOT_PROCEDURE2);
  return td->sym_num;
}

Int Tym_Struct_Get_Num_Items(Int t) {
  TypeDesc *td;
  assert(t >= 0 && t < Arr_NumItem(tym_type_list));
  td = Arr_ItemPtr(tym_type_list, TypeDesc, t+1);
  assert(td->tot == TOT_STRUCTURE);
  return td->st_size;
}

/* DESCRIPTION: Restituisce il nome del tipo t.
 */
Task Tym__Type_Name(Intg t, Array *name) {
  TypeDesc *td;
  MSG_LOCATION("Tym__Type_Name");

  if (t < 0) {
    register Intg tt = -t;
    if ( tt < PROC_SPECIAL_NUM ) {
      char *nm = tym_special_name[tt];
      return Arr_MPush(name, nm, strlen(nm));
    }

    MSG_ERROR("Unknown type!");
    return Arr_MPush(name, "Unknown", 7);
  }

  if (t >= Arr_NumItem(tym_type_list)) {
    MSG_ERROR("Unknown type!");
    return Arr_MPush(name, "Unknown", 7);
  }

  td = Arr_ItemPtr(tym_type_list, TypeDesc, t+1);

  switch (td->tot) {
   case TOT_INSTANCE:
    return Arr_MPush(name, td->name, strlen(td->name));

   case TOT_PTR_TO:
    if IS_FAILED( Arr_MPush(name, "pointer to ", 11) ) return Failed;
    return Tym__Type_Name(td->target, name);

   case TOT_ARRAY_OF: {
      char str[128];
      register Intg s = td->arr_size;
      if ( s > 0 ) sprintf(str, "array("SIntg") of ", s);
              else sprintf(str, "array() of ");
      if IS_FAILED( Arr_MPush(name, str, strlen(str)) ) return Failed;
    }
    return Tym__Type_Name(td->target, name);

   case TOT_PROCEDURE:
    TASK( Arr_MPush(name, "procedure ", 10) );
    TASK( Tym__Type_Name(td->target, name)  );
    TASK( Arr_MPush(name, " of ", 4)        );
    TASK( Tym__Type_Name(td->parent, name)  );
    return Success;

   case TOT_PROCEDURE2:
    TASK( Arr_MPush(name, "procedure ", 10) );
    TASK( Tym__Type_Name(td->target, name)  );
    TASK( Arr_MPush(name, " of ", 4)        );
    TASK( Tym__Type_Name(td->parent, name)  );
    TASK( Arr_MPush(name, "[]", 2)        );
    return Success;

   case TOT_SPECIE: {
      register Intg t = td->target;
      register int not_first = 0;
      TASK( Arr_MPush(name, "species(", 8) );
      do {
        td = Tym_Type_Get(t);
        if ( td == NULL ) {
          MSG_ERROR("Tym__Type_Name: Array dei tipi danneggiata(1)!");
          return Failed;
        }
        if ( td->tot != TOT_ALIAS_OF ) {
          MSG_ERROR("Tym__Type_Name: Array dei tipi danneggiata(2)!");
          return Failed;
        }
        if ( not_first ) {TASK( Arr_MPush(name, "->", 2) );}
        TASK( Tym__Type_Name(td->target, name) );
        t = td->greater;
        not_first = 1;
      } while( td->greater != TYPE_NONE );
      TASK( Arr_Push(name, ")") );
      return Success;
    }

   case TOT_ALIAS_OF:
    if ( td->name == NULL )
      return Tym__Type_Name(td->target, name);
    else
      return Arr_MPush(name, td->name, strlen(td->name));

   case TOT_STRUCTURE: {
      register Intg t = td->target;
      register int not_first = 0;
      TASK( Arr_MPush(name, "structure(", 10) );
      do {
        td = Tym_Type_Get(t);
        if ( td == NULL ) {
          MSG_ERROR("Tym__Type_Name: Array dei tipi danneggiata(3)!");
          return Failed;
        }
        if ( td->tot != TOT_ALIAS_OF ) {
          MSG_ERROR("Tym__Type_Name: Array dei tipi danneggiata(4)!");
          return Failed;
        }
        if ( not_first ) {TASK( Arr_MPush(name, ", ", 2) );}
        TASK( Tym__Type_Name(td->target, name) );
        t = td->greater;
        not_first = 1;
      } while( td->greater != TYPE_NONE );
      TASK( Arr_Push(name, ")") );
      return Success;
    }

   default:
    MSG_ERROR("Array dei tipi danneggiata!");
    return Arr_MPush(name, "Unknown", 7);
  }
}

/* DESCRIPTION: Returns the name of the type t.
 * NOTE: This functions builds the name into an array-object (type: Array)
 *  and then returns the pointer to that array.
 *  There is no need to free the string returned: the same array is used
 *  for all the calls to Tym_Type_Name.
 *  As a consequence to that behaviour YOU CANNOT USE Tym_Type_Name MORE
 *  THAN ONCE INSIDE ONE SINGLE printf STATEMENT.
 *  For this purpose use Tym_Type_Names.
 */
const char *Tym_Type_Name(Intg t) {
  static Array *name = NULL;
  const char ending_char = '\0';

  /* Set up or clear the array of chars where the name of type
   * will be composed.
   */
  if ( name == NULL ) {
    name = Array_New(sizeof(char), TYM_TYPICAL_TYPE_NAME_SIZE);
    if ( name == NULL ) return NULL;

  } else {
     if IS_FAILED( Arr_Clear(name) ) return NULL;
  }

  /* The name will be composed by the recursive function Tym__Type_Name */
  if IS_FAILED( Tym__Type_Name(t, name) ) return NULL;
  if IS_FAILED( Arr_Push(name, (void *) & ending_char) ) return NULL;

  return Arr_FirstItemPtr(name, char);
}

/* DESCRIPTION: Returns the name of the type t.
 *  This function is similar to Tym_Type_Names, but can used more than
 *  once inside a printf statement. For example:
 *   printf("Type 1: '%s' -- Type 2: '%s' -- Type 3: '%s'\n",
 *    Tym_Type_Names(type1), Tym_Type_Names(type2), Tym_Type_Names(type3));
 */
char *Tym_Type_Names(Intg t) {
  register char *str;
  if ( tym_num_name < 0 ) {
    register int i;
    for(i = 0; i < TYM_NUM_TYPE_NAMES; i++) tym_name[i] = NULL;
    tym_num_name = 0;
  }
  free(tym_name[tym_num_name]);
  str = strdup(Tym_Type_Name(t));
  tym_name[tym_num_name] = str;
  tym_num_name = (tym_num_name + 1) % TYM_NUM_TYPE_NAMES;
  return str;
}

Intg Cmp_Align(Intg addr) {
  return addr;
}

/* This function defines a new type and a corresponding new symbol.
 * It behaves in two distinct ways: if 'parent != TYPE_NONE', it defines
 * a type 'new_type' which will be member of 'parent',
 * if 'parent == TYPE_NONE', the new type will be an independent
 * (explicit) type. There is another important thing to say:
 * if 'size < 0' the new type will be simply an alias of the type given
 * inside 'aliased_type'

 The new type is returned inside *new_type.

 * It creates a new type associated to the new box and returns it.
 * NOTE: In case of error returns TYPE_NONE.
 *  If size < 0 an alias of aliased_type will be created.
 * Tym_Def_Type_With_Name
 */
Task Tym_Def_Type(Intg *new_type,
 Intg parent, Name *nm, Intg size, Intg aliased_type) {
  Symbol *s;
  TypeDesc *td;
  Intg type;
  MSG_LOCATION("Tym_Def_Type");

  /* First of all I create the symbol with name *nm */
  assert( parent == TYPE_NONE );
  TASK( Sym_Explicit_New(& s, nm, 0) );
  s->symattr.is_explicit = 1;

  /* Now I create a new type for the box */
  if ( size < 0 ) {
    type = Tym_Def_Alias_Of(nm, aliased_type);
    if ( type == TYPE_NONE ) return Failed;
    if ( (td = Tym_Type_Get(type)) == NULL ) return Failed;

  } else {
    td = Tym_Type_New(nm);
    type = tym_recent_type; /*Tym_Type_Newer();*/
    if ( (td == NULL) || (type < 0) ) return Failed;
    td->tot = TOT_INSTANCE;
    td->size = size;
    td->target = TYPE_NONE;
  }

  /* I set all the remaining values of the structure s */
  s->symtype = VARIABLE;
  *new_type = type;
  Expr_New_Type(& s->value, type);
  return Success;
}

/******************************************************************************
 * Le procedure che seguono servono a creare tipi composti di dati            *
 * (array di ..., puntatori a..., etc).                                       *
 ******************************************************************************/

/* DESCRIPTION: This function creates a new type of data using the type "type".
 *  The new data type is "array of num object of kind type".
 *  If num < 1 the function assumes that the size of the array is unspecified.
 * NOTE: The new type will be returned or TYPE_NONE in case of errors.
 * NOTE 2: It updates tym_recent_type and tym_recent_typedesc.
 */
Intg Tym_Def_Array_Of(Intg num, Intg type) {
  Intg size;
  TypeDesc *td;
  MSG_LOCATION("Tym_Def_Array_Of");

  if ( (td = Tym_Type_Get(type)) == NULL ) return TYPE_NONE;
  /* Faccio i controlli sul tipo type */
  switch (td->tot) {
   case TOT_INSTANCE: case TOT_PTR_TO: case TOT_ARRAY_OF: break;
   default:
    MSG_ERROR("Impossibile creare una array di '%s'.",
     Tym_Type_Name(type));
    return TYPE_NONE;
  }
  size = td->size;
  if ( size < 1 ) {
    MSG_ERROR("Cannot create a undefined size array.");
    return TYPE_NONE;
  }

  /* Creo il nuovo tipo */
  if ( (td = Tym_Type_New( Name_Empty() )) == NULL ) return TYPE_NONE;
  td->arr_size = num;
  td->size = ( (size > 0) && (num > 0) ) ? size * num : 0;
  td->tot = TOT_ARRAY_OF;
  td->target = type;
  return tym_recent_type;
}

#if 0
/* DESCRIPTION: This function creates a new type of data using the type "type".
 *  The new data type is "pointer to object of kind type".
 * NOTE: The new type will be returned or TYPE_NONE in case of errors.
 * NOTE 2: It updates tym_recent_type and tym_recent_typedesc.
 */
Intg Tym_Def_Pointer_To(Intg type) {
  TypeDesc *td;
  MSG_LOCATION("Tym_Def_Pointer_To");

  if ( (td = Tym_Type_Get(type)) == NULL ) return TYPE_NONE;
  /* Faccio i controlli sul tipo type */
  switch (td->tot) {
   case TOT_INSTANCE: case TOT_PTR_TO: case TOT_ARRAY_OF: break;
   default:
    MSG_ERROR("Impossibile creare un puntatore a '%s'.",
     Tym_Type_Name(type));
    return TYPE_NONE;
  }
  if ( td->size < 1 ) {
    MSG_ERROR("Cannot create a pointer to a zero-size object ('%s').",
     Tym_Type_Name(type));
    return TYPE_NONE;
  }

  /* Creo il nuovo tipo */
  if ( (td = Tym_Type_New( Name_Empty() )) == NULL ) return TYPE_NONE;
  td->size = SIZEOF_PTR;
  td->tot = TOT_PTR_TO;
  td->target = type;
  return tym_recent_type;
}
#endif

/* DESCRIPTION: This function creates a new type of data using the type "type".
 *  The new data type is "alias of type".
 * NOTE: The new type will be returned or TYPE_NONE in case of errors.
 * NOTE 2: It updates tym_recent_type and tym_recent_typedesc.
 */
Intg Tym_Def_Alias_Of(Name *nm, Intg type) {
  TypeDesc *td;
  Intg size;
  MSG_LOCATION("Tym_Def_Alias_Of");

  if ( nm == NULL ) nm = Name_Empty();

  /* DOVREI RISOLVERE GLI ALIAS??? */
  if ( (td = Tym_Type_Get(type)) == NULL ) return TYPE_NONE;
  /* Faccio i controlli sul tipo type */
  switch (td->tot) {
   case TOT_INSTANCE: case TOT_PTR_TO: case TOT_ARRAY_OF:
   case TOT_SPECIE: case TOT_ALIAS_OF: case TOT_STRUCTURE:
     break;
   default:
    MSG_ERROR("Cannot create an alias of '%s'.",
     Tym_Type_Name(type));
    return TYPE_NONE;
  }
  size = td->size;

  /* Creo il nuovo tipo */
  if ( (td = Tym_Type_New( nm )) == NULL ) return TYPE_NONE;
  td->size = size;
  td->tot = TOT_ALIAS_OF;
  td->target = type;
  return tym_recent_type;
}

/* DESCRIPTION: This function returns 1 if the type type1 is equal to
 *  (or 'contains') the type type2 and returns 0 otherwise.
 *  In OTHER WORDS this function give an answer to the question:
 *  "an object of type 'type2' belong to the cathegory of objects
 *  of type 'type1'?".
 * EXAMPLES: array[] of Int contains array[5] of Int
 *            specie (Real > Int > Char) contains Int
 * NOTE: If type2 needs to be 'expanded' into type1 and need_expansion != NULL,
 *  then *need_expansion will be setted to 1. In other cases *need_expansion
 *  will be leaved untouched.
 */
int Tym_Compare_Types(Intg type1, Intg type2, int *need_expansion) {
  int first, dummy;
  TypeDesc *td1, *td2;

#ifdef DEBUG_COMPARE_TYPES
  printf( "Type_Compare_Types(%s, %s)\n",
   Tym_Type_Names(type1), Tym_Type_Names(type2) );
#endif

  need_expansion = (need_expansion == NULL) ? & dummy : need_expansion;

  first = 1;
  for(;;) {
    if ( type1 == type2 ) return 1;
    type1 = Tym_Type_Resolve_Alias(type1);
    type2 = Tym_Type_Resolve_All(type2);
    if ( type1 == type2 ) return 1;
    td1 = Tym_Type_Get(type1);
    td2 = Tym_Type_Get(type2);
    if ( (td1 == NULL) || (td2 == NULL) ) {
      MSG_ERROR("Tipi errati in Tym_Compare_Types!");
      return 0;
    }

    switch( td1->tot ) {
     case TOT_INSTANCE: /* type1 != type2 !!! */
      return 0;

     case TOT_PTR_TO:
      if (td2->tot == TOT_PTR_TO) break;
      return 0;

     case TOT_ARRAY_OF:
      if (td2->tot != TOT_ARRAY_OF) return 0;
      {
        register Intg td1s = td1->arr_size, td2s = td2->arr_size;
        if (td1s == td2s) break;
        if ((td1s < 1) && first) break;
        return 0;
      }

     case TOT_SPECIE: {
        register Intg t = td1->target;
        do {
          td1 = Tym_Type_Get(t);
          if ( td1 == NULL ) {
            MSG_ERROR("Array dei tipi danneggiata!");
            return 0;
          }
          if ( Tym_Compare_Types(t, type2, need_expansion) ) {
            *need_expansion |= (td1->greater != TYPE_NONE);
            return 1;
          }
          t = td1->greater;
        } while( td1->greater != TYPE_NONE );
      }
      return 0;

     case TOT_PROCEDURE:
      if (td2->tot != TOT_PROCEDURE) return 0;
      return (
           Tym_Compare_Types(td1->parent, td2->parent, need_expansion)
        && Tym_Compare_Types(td1->target, td2->target, need_expansion) );

     case TOT_PROCEDURE2:
      if (td2->tot != TOT_PROCEDURE2) return 0;
      return (
           Tym_Compare_Types(td1->parent, td2->parent, need_expansion)
        && Tym_Compare_Types(td1->target, td2->target, need_expansion) );

     case TOT_STRUCTURE: {
        register Intg t1 = td1->target, t2 = td2->target;
        if (td2->tot != TOT_STRUCTURE) return 0;
        do {
#ifdef DEBUG_COMPARE_TYPES
          printf("Structure are equals? testing '%s' <--> '%s': ",
           Tym_Type_Names(t1), Tym_Type_Names(t2));
#endif
          td1 = Tym_Type_Get(t1);
          td2 = Tym_Type_Get(t2);
          if ( (td1 == NULL) || (td2 == NULL) ) {
            MSG_ERROR("Array dei tipi danneggiata(2)!");
            return 0;
          }
          if ( ! Tym_Compare_Types(t1, t2, need_expansion) ) {return 0;}
#ifdef DEBUG_COMPARE_TYPES
          if (! *need_expansion) printf("no ");
          printf("expansion needed!\n");
#endif
          t1 = td1->greater;
          t2 = td2->greater;
        } while( (t1 != TYPE_NONE) && (t2 != TYPE_NONE) );
        if ( t1 == t2 ) return 1; /* t1 == t2 == TYPE_NONE */
        return 0;
      }

     default:
      MSG_ERROR("Tym_Compare_Types still not fully implemented!");
      return 0;
    }

    first = 0;
    type1 = td1->target;
    type2 = td2->target;
  }

  return 0;
}

/* DESCRIPTION: This function resolves the alias of types.
 *  It returns the final target of the alias.
 * NOTE: If there is something wrong, this functions simply ignore it
 *  and returns type.
 */
Intg Tym_Type_Resolve(Intg type, int not_alias, int not_species) {
  TypeDesc *td;
  MSG_LOCATION("Tym_Type_Resolve");

  while( type >= 0 ) {
    td = Tym_Type_Get(type);
    if ( td == NULL ) return type;
    switch( td->tot ) {
     case TOT_ALIAS_OF:
      if ( not_alias ) return type;
      type = td->target;
      continue;

     case TOT_SPECIE:
      if ( not_species ) return type;
      type = td->greater;
      continue;
     default:
      return type;
    }
  }
  return type;
}

/* This function defines the procedure proc@of_type and associates to it
 * the symbol 'sym_num'. If second == 0, then the procedure is a first one
 * (those called when an object is beeing created: Object[ procedure ]),
 * if second == 1, then the procedure is a second procedure (those called
 * when an object is beeing used: Object[][ procedure ]).
 */
Intg Tym_Def_Procedure(Intg proc, int second, Intg of_type, Intg sym_num) {
  TypeDesc *td, *p;
  Intg new_procedure;
  MSG_LOCATION("Tym_Def_Procedure");

  if ( (p = Tym_Type_Get(of_type)) == NULL ) return TYPE_NONE;
  switch (p->tot) {
   case TOT_INSTANCE: case TOT_PTR_TO: case TOT_ARRAY_OF: case TOT_ALIAS_OF:
     break;
   default:
    MSG_ERROR("Impossibile creare una procedura di tipo '%s'.",
     Tym_Type_Name(of_type));
    return TYPE_NONE;
  }

  /* Creo il nuovo tipo */
  if ( (td = Tym_Type_New( Name_Empty() )) == NULL ) return TYPE_NONE;
  td->tot = second ? TOT_PROCEDURE2 : TOT_PROCEDURE;
  td->size = 0;
  td->parent = of_type;
  td->target = proc;
  td->sym_num = sym_num;
  td->procedure = p->procedure;
  new_procedure = tym_recent_type;
  /* The previous call to Tym_Type_New may had required to realloc
   * the array of types, so we must re-get the address of p.
   */
  if ( (p = Tym_Type_Get(of_type)) == NULL ) return TYPE_NONE;
  p->procedure = new_procedure;
  return new_procedure;
}

Task Tym_Procedure_Info(Int proc_type, Int *child, Int *parent,
 int *second, UInt *sym_num) {
  TypeDesc *td;
  int t;

  td = Tym_Type_Get(Tym_Type_Resolve_Alias(proc_type));
  if (td == (TypeDesc *) NULL) return Failed;
  t = (td->tot == TOT_PROCEDURE) | (td->tot == TOT_PROCEDURE2) << 1;
  if (t != 0) {
    if (child != (Int *) NULL) *child = td->target;
    if (parent != (Int *) NULL) *parent = td->parent;
    if (sym_num != (UInt *) NULL) *sym_num = td->sym_num;
    if (second != (int *) NULL) *second = (t == 2);
    return Success;

  } else {
    MSG_ERROR("Tym_Procedure_Info: the type you gave is not a procedure!");
    return Failed;
  }
}

/* This function searches for the existence of the procedure proc@of_type.
 * NOTE: This function doesn't produce any error messages, when a procedure
 *  has not been found (it simply returns TYPE_NONE!).
 * NOTE: Box can handle procedures like the following: (Scalar, Scalar)@Print
 *  When the compiler finds something like: Print[ (1, 1.2) ] it should:
 *   1) convert 1 --> 1.0 (convert Int --> Real)
 *   2) build the structure (1.0, 1.2)
 *   3) call (Scalar, Scalar)@Print and pass it the new created structure.
 *  These operations are not always necessary. This is the case
 *  of the following example: Print[ (1.0, 1.2) ]
 *  In this case the compiler can pass the structure to the procedure
 *  without conversions.
 *  Tym_Search_Procedure can recognize if conversions should be performed:
 *  it sets *containing_species = species, if this happens. species is
 *  the species which 'contains' the type proc ( '(Scalar, Scalar)'
 *  in the previous example). In other cases *containing_species = TYPE_NONE.
 */
Intg Tym_Search_Procedure(Intg proc, int second, Intg of_type,
                          Intg *containing_species) {
  TypeDesc *td, *td0;
  Intg procedure, dummy, proc_tot = second ? TOT_PROCEDURE2 : TOT_PROCEDURE;
  int need_expansion = 0;
  MSG_LOCATION("Tym_Search_Procedure");

  if ( (td0 = td = Tym_Type_Get(of_type)) == NULL ) return TYPE_NONE;
  if ( containing_species == NULL ) containing_species = & dummy;

  if ( proc < 0 ) {
    *containing_species = TYPE_NONE;
    while ( (procedure = td->procedure) != TYPE_NONE ) {
      td = Tym_Type_Get(procedure);

      if (td == NULL) {
        MSG_ERROR("Array dei tipi danneggiata!");
        return TYPE_NONE;

      } else {
        int tot_ok = (td->tot == TOT_PROCEDURE || td->tot == TOT_PROCEDURE2);
        int parent_ok = (td->parent == of_type);

        if ( !(tot_ok && parent_ok) ) {
          MSG_ERROR("Type corruption: procedure item has %s type, %s parent.",
              tot_ok ? "right" : "wrong", parent_ok ? "right" : "wrong");
          return TYPE_NONE;
        }
      }

      if ( td->target == proc && td->tot == proc_tot ) return procedure;
    }

  } else {
    while ( (procedure = td->procedure) != TYPE_NONE ) {
      td = Tym_Type_Get(procedure);

      if (td == NULL) {
        MSG_ERROR("Array dei tipi danneggiata!");
        return TYPE_NONE;

      } else {
        int tot_ok = (td->tot == TOT_PROCEDURE || td->tot == TOT_PROCEDURE2);
        int parent_ok = (td->parent == of_type);

        if ( !(tot_ok && parent_ok) ) {
          MSG_ERROR("Type corruption: procedure item has %s type, %s parent.",
              tot_ok ? "right" : "wrong", parent_ok ? "right" : "wrong");
          return TYPE_NONE;
        }
      }

      if ( td->target >= 0 && td->tot == proc_tot ) {
        if (Tym_Compare_Types(td->target, proc, & need_expansion) ) {
          *containing_species = (need_expansion) ? td->target : TYPE_NONE;
          return procedure;
        }
      }
    }
  }

/*  if ( td0->tot == TOT_ALIAS_OF )
    return Tym_Search_Procedure(proc, td0->target);*/

  return TYPE_NONE;
}

/* DESCRIPTION: This function defines a new specie of types.
 *  You can define a new specie as in the following example:
 *   EXAMPLE: (Real < Intg < Char)
 *            new_specie = TYPE_NONE;
 *            Tym_Def_Specie(*new_specie, TYPE_REAL);
 *            Tym_Def_Specie(*new_specie, TYPE_INTG);
 *            Tym_Def_Specie(*new_specie, TYPE_CHAR);
 * NOTE: Species are used to do automatic conversions. For instance:
 *  consider the function cosine: Cos[x]. This is a real valued function
 *  of the real variable x. What happens if you write Cos[1]?
 *  The answer is rather simple: 1 is an integer, so the compiler will search
 *  the procedure Intg@Cos. This is not what one would expect.
 *  A better behaviour is: conversion of 1 into 1.0 (real value)
 *  and execution of the procedure Real@Cos.
 *  To accomplish what has just been described, one should define a species
 *  of types in the following way: Scalar = (Real < Intg < Char).
 *  then one should define Scalar@Cos. Now when the compiler finds Cos[x]
 *  where x is a Real, an Intg or a Char, it converts x into a Real
 *  and then executes Real@Cos.
 *  A Scalar is essentially a Real with only one difference: you can assign
 *  to a Scalar a Real value, an Intg value or a Char value. The compiler
 *  will chose the right conversion for you!
 */
Task Tym_Def_Specie(Intg *specie, Intg type) {
  if ( *specie == TYPE_NONE ) {
    register TypeDesc *td;
    Intg great_type, great_size, specie_type;

    /* Creo un alias di type */
    great_type = Tym_Def_Alias_Of(Name_Empty(), type);
    if ( great_type == TYPE_NONE ) return Failed;
    td = tym_recent_typedesc;
    specie_type = Tym_Type_Next();
    great_size = td->size;
    td->parent = specie_type;
    td->greater = TYPE_NONE;

    /* Creo la specie */
    td = Tym_Type_New( Name_Empty() );
    if ( td == NULL ) return Failed;
    td->tot = TOT_SPECIE;
    td->size = great_size;
    td->sp_size = 1;  /* La specie per ora contiene un solo elemento! */
    td->target = great_type;  /* lower type   */
    td->greater = great_type; /* greater type */

    *specie = specie_type;
    return Success;

  } else {
    register TypeDesc *td;
    Intg lower_type, greater_type;

    /* Trovo il descrittore della specie */
    td = Tym_Type_Get(*specie);
    if ( td == NULL ) return Failed;
    assert( td->tot == TOT_SPECIE );
    ++(td->sp_size);
    greater_type = td->target;
    td->target = Tym_Type_Next();

    /* Creo il prossimo elemento della specie */
    lower_type = Tym_Def_Alias_Of(Name_Empty(), type);
    if ( lower_type == TYPE_NONE ) return Failed;
    td = tym_recent_typedesc;
    td->parent = *specie;
    td->greater = greater_type;
    return Success;
  }
}

/* DESCRIPTION: This function defines a new structure of types.
 *  You can define a new structure in a way very similar to the
 *  one used to define species.
 */
Task Tym_Def_Structure(Intg *strc, Intg type) {
  if ( *strc == TYPE_NONE ) {
    register TypeDesc *td;
    Intg first_type, strc_size, strc_type;

    /* Creo un alias di type */
    first_type = Tym_Def_Alias_Of(Name_Empty(), type);
    if ( first_type == TYPE_NONE ) return Failed;
    td = tym_recent_typedesc;
    strc_type = Tym_Type_Next();
    strc_size = td->size;
    td->parent = strc_type;
    td->greater = TYPE_NONE;

    /* Creo la struttura */
    td = Tym_Type_New( Name_Empty() );
    if ( td == NULL ) return Failed;
    td->tot = TOT_STRUCTURE;
    td->size = strc_size;
    td->st_size = 1;  /* La struttura per ora contiene un solo elemento! */
    td->target = first_type;  /* first element of the structure */
    td->greater = first_type; /*  last element of the structure */

    *strc = strc_type;
    return Success;

  } else {
    register TypeDesc *strc_td, *new_td, *last_td;
    register Intg new;

    /* Creo il prossimo elemento della struttura */
    new = Tym_Def_Alias_Of(Name_Empty(), type);
    if ( new == TYPE_NONE ) return Failed;
    new_td = tym_recent_typedesc;
    new_td->parent = *strc;
    new_td->greater = TYPE_NONE;

    /* Trovo il descrittore della struttura */
    strc_td = Tym_Type_Get(*strc);
    if ( strc_td == NULL ) return Failed;
    assert( strc_td->tot == TOT_STRUCTURE );

    /* Concateno questo nuovo elemento agli altri */
    last_td = Tym_Type_Get(strc_td->greater);
    if ( last_td == NULL ) return Failed;
    last_td->greater = new;
    strc_td->greater = new;
    ++(strc_td->st_size);
    strc_td->size += new_td->size;
    return Success;
  }
}

/* DESCRIPTION: We explain this function with an example:
 *  Suppose you want to obtain the list of types which made up
 *  the structure s = ('c', 0.5, 23). First of all you call:
 *   Intg s_copy = s, *t = & s_copy;
 *   TASK( Tym_Structure_Get(t));
 *  This call will put the type of the first member of s into *t:
 *  so, if s is really a structure, the function will assign *t = TYPE_CHAR
 *  and will return 'Success'. If s is not a structure it will return 'Failed'
 *  Now, if you want the second member, you should call again:
 *   TASK( Tym_Structure_Get(t));
 *  In this case, after this call: *t = TYPE_REAL. When there are no more
 *  members,the function will put: *t = TYPE_NONE.
 * NOTE: you shouldn't change *type between one call and another.
 *  This code - for example - MUST BE AVOIDED:
 *   Intg s_copy = s, *t = & s_copy;
 *   TASK( Tym_Structure_Get(t));
 *   *t = Tym_Type_Resolve_All(*t);  <-- E R R O R !
 *   TASK( Tym_Structure_Get(t));
 * NOTE 2: It updates: tym_recent_type = *type, tym_recent_typedesc = ...
 * NOTE 3: FULL EXAMPLE:
 *   Intg member_type = structure_type;
 *   TASK(Tym_Structure_Get(& member_type));
 *   while (member_type != TYPE_NONE) {
 *     ... // <-- here we can use member_type
 *     TASK(Tym_Structure_Get(& member_type));
 *   };
 */
Task Tym_Structure_Get(Intg *type) {
  TypeDesc *td;

  /* Trovo il descrittore della struttura */
  assert( *type != TYPE_NONE );
  td = Tym_Type_Get(*type);
  if ( td == NULL ) return Failed;

  if ( td->tot == TOT_STRUCTURE ) {
    *type = td->target;
    return Success;

  } else {
    assert( td->tot == TOT_ALIAS_OF );
    *type = td->greater;
    return Success;
  }
}


/* Similar to Tym_Structure_Get, but for species. */
Task Tym_Specie_Get(Intg *type) {
  TypeDesc *td;

  /* Trovo il descrittore della struttura */
  assert( *type != TYPE_NONE );
  td = Tym_Type_Get(*type);
  if ( td == NULL ) return Failed;

  if ( td->tot == TOT_SPECIE ) {
    *type = td->target;
    return Success;

  } else {
    assert( td->tot == TOT_ALIAS_OF );
    *type = td->greater;
    return Success;
  }
}

/* Similar to Tym_Structure_Get, but for species. */
Int Tym_Specie_Get_Target(Int type) {
  TypeDesc *td;
  assert(type != TYPE_NONE);
  td = Tym_Type_Get(type);
  if (td == NULL) return Failed;
  assert(td->tot == TOT_SPECIE);
  return td->greater;
}

#endif
