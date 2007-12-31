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
#include "str.h"
#include "array.h"
#include "collection.h"
#include "hashtable.h"

static TS *last_ts; /* Just for transition: will be removed! */

Task TS_Init(TS *ts) {
  TASK( Clc_New(& ts->type_descs, sizeof(TSDesc), TS_TSDESC_CLC_SIZE) );
  HT(& ts->members,  TS_MEMB_HT_SIZE);
  HT(& ts->subtypes, TS_SUBT_HT_SIZE);
  TASK( Arr_New(& ts->name_buffer, sizeof(char), TS_NAME_BUFFER_SIZE) );
  last_ts = ts; /* Just for transition: will be removed! */
  return Success;
}

void TS_Destroy(TS *ts) {
  Clc_Destroy(ts->type_descs);
  HT_Destroy(ts->members);
  HT_Destroy(ts->subtypes);
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

static TSDesc *Resolve(TS *ts, Type *rt, Type t, int ignore_names) {
  while(1) {
    TSDesc *td = Type_Ptr(ts, t);
    switch(td->kind) {
    case TS_KIND_LINK:
    case TS_KIND_ALIAS:
    case TS_KIND_MEMBER:
      t = td->target;
      if (ignore_names) break;
      if (td->name == (char *) NULL) break;
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
      t = td->target; break;
    case TS_KIND_SPECIES:
      t = td->data.last; break;
    default:
      if (rt != (Type *) NULL) *rt = t;
      return td;
    }
  }
}

Type TS_Resolve(TS *ts, Type t, int resolve_alias, int resolve_species) {
  if (t == TS_TYPE_NONE)
    return TS_TYPE_NONE;
  else {
    int resolve;
    while(1) {
      TSDesc *td = Type_Ptr(ts, t);
      Type rt = td->target;
      switch(td->kind) {
      case TS_KIND_LINK:
      case TS_KIND_MEMBER:
        resolve = 1; break;
      case TS_KIND_ALIAS:
        resolve = resolve_alias; break;
      case TS_KIND_SPECIES:
        rt = td->data.last; resolve = resolve_species; break;
      default:
        resolve = 0;
      }
      if (!resolve) return t;
      t = rt;
    }
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
     "this type has already been given the name '%s'!", name, t, td->name);
    return Failed;
  }
  td->name = Mem_Strdup(name);
  return Success;
}

char *TS_Name_Get(TS *ts, Type t) {
  TSDesc *td = Type_Ptr(ts, t);
  td = Resolve(ts, & t, t, 0);
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
      char *proc_kind_strs[4] = {"err", "@", "@@", "@&"};
      char *proc_kind_str = proc_kind_strs[td->data.proc.kind & 3];
      return printdup("%~s%s%~s", TS_Name_Get(ts, td->target),
                      proc_kind_str, TS_Name_Get(ts, td->data.proc.parent));
    }

  case TS_KIND_SUBTYPE:
    return printdup("%~s.%s",
                    TS_Name_Get(ts, td->data.subtype.parent),
                    td->data.subtype.child_name);
  default:
    return Mem_Strdup("<unknown type>");
  }
}

Task TS_Array_Member(TS *ts, Type *memb, Type array, Int *array_size) {
  Type a = TS_Resolve(ts, array, 1, 1);
  TSDesc *a_td = Type_Ptr(ts, a);
  if (a_td->kind != TS_KIND_ARRAY) {
    MSG_ERROR("Cannot extract element of the non-array type '%~s'",
     TS_Name_Get(ts, array));
    return Failed;
  }
  *memb = a_td->target;
  if (array_size != (Int *) NULL) *array_size = a_td->data.array_size;
  return Success;
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
  *m = TS_TYPE_NONE;
  s = TS_Resolve(ts, s, 1, 1);
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
  Int count=0, next=s;
  TSDesc *td = Type_Ptr(ts, s);
  switch(td->kind) {
  case TS_KIND_SPECIES:
  case TS_KIND_STRUCTURE:
  case TS_KIND_ENUM:
    while(1) {
      next = TS_Member_Next(ts, next);
      if (! TS_Is_Member(ts, next)) break;
      ++count;
    };
    break;
  default:
    MSG_FATAL("Trying to count members of the non-membered type %~s",
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
/* Procedure registration and search */

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

void TS_Procedure_Info(TS *ts, Type *parent, Type *child,
                       int *kind, UInt *sym_num, Type p) {
  TSDesc *proc_td;
  proc_td = Type_Ptr(ts, p);
  assert(proc_td->kind == TS_KIND_PROC);
  if (parent != (Type *) NULL)
    *parent = proc_td->data.proc.parent;
  if (child != (Type *) NULL)
    *child = proc_td->target;
  if (kind != (int *) NULL)
    *kind = proc_td->data.proc.kind;
  if (sym_num != (UInt *) NULL)
    *sym_num = proc_td->data.proc.sym_num;
}

Task TS_Procedure_Search(TS *ts, Type *proc, Type *expansion_type,
 Type parent, Type child, int kind) {
  TSDesc *p_td, *parent_td;
  Type p, dummy;
  assert(kind == 1 || kind == 2);
  if (proc == (Type *) NULL) proc = & dummy;
  if (expansion_type == (Type *) NULL) expansion_type = & dummy;
  *proc = TS_TYPE_NONE;
  *expansion_type = TS_TYPE_NONE;
  parent_td = Type_Ptr(ts, parent);
  /*MSG_ADVICE("TS_Procedure_Search: searching procedure");*/
  for(p = parent_td->first_proc; p != TS_TYPE_NONE; p = p_td->first_proc) {
    TSCmp comparison;
    int p_kind;
    p_td = Type_Ptr(ts, p);
    /*MSG_ADVICE("TS_Procedure_Search: considering %~s", TS_Name_Get(ts, p));*/
    p_kind = p_td->data.proc.kind;
    assert(p_kind != 0 && (p_kind | 3) == 3);
    if ((p_kind & kind) != 0) {
      /*MSG_ADVICE("TS_Procedure_Search: '%s', comparing '%s' == '%s'",
                 Tym_Type_Names(parent),
                 Tym_Type_Names(p_td->target), Tym_Type_Names(child));*/
      comparison = TS_Compare(ts, p_td->target, child);
      if (comparison != TS_TYPES_UNMATCH) {
        if (comparison == TS_TYPES_EXPAND) *expansion_type = p_td->target;
        *proc = p;
        return Success;
      }
    }
  }
  return Success;
}

Int TS_Procedure_Def(Int proc, int kind, Int of_type, Int sym_num) {
  Type procedure;
  /*MSG_ADVICE("TS_Procedure_Def: new procedure '%s' of '%s'",
             Tym_Type_Names(proc), Tym_Type_Names(of_type));*/
  assert(kind != 0 && (kind | 3) == 3); /* kind can be 1, 2 or 3 */
  Task t = TS_Procedure_New(last_ts, & procedure, of_type, proc, kind);
  assert(t == Success);
  t = TS_Procedure_Register(last_ts, procedure, sym_num);
  assert(t == Success);
  return procedure;
}

/****************************************************************************/


/* Create a new unregistered subtype: a subtype is unregistered when
 * the parent is not aware of its existance. An unregistered type is defined
 * only giving its name and its parent type. When the subtype is registered
 * the parent type becomes aware of it. In order for the registration to be
 * completed the full type of the subtype must be specified.
 */
Task TS_Subtype_New(TS *ts, Type *new_subtype,
                    Type parent_type, Name *child_name) {
  TSDesc td;
  TS_TSDESC_INIT(& td);
  td.kind = TS_KIND_SUBTYPE;
  td.size = TS_SIZE_UNKNOWN;
  td.target = TS_TYPE_NONE;
  td.data.subtype.parent = parent_type;
  td.data.subtype.child_name = Name_To_Str(child_name);
  return Type_New(ts, new_subtype, & td);
}

/* Register a previously created (and still unregistered) subtype.
 */
Task TS_Subtype_Register(TS *ts, Type subtype, Type subtype_type) {
  TSDesc *s_td = Type_Ptr(ts, subtype);
  Type parent, found_subtype;
  Name child_name, full_name;
  char *child_str;

  if (s_td->target != TS_TYPE_NONE) {
    MSG_ERROR("Cannot redefine subtype '%~s'", TS_Name_Get(ts, subtype));
    return Failed;
  }

  child_str = s_td->data.subtype.child_name;
  Name_From_Str(& child_name, child_str);
  parent = s_td->data.subtype.parent;
  TS_Subtype_Find(ts, & found_subtype, parent, & child_name);
  if (found_subtype != TS_TYPE_NONE) {
    TSDesc *found_subtype_td = Type_Ptr(ts, found_subtype);
    Type found_subtype_type = found_subtype_td->target;
    TSCmp comparison = TS_Compare(ts, found_subtype_type, subtype_type);
    if ((comparison & TS_TYPES_MATCH) == 0) {
      MSG_ERROR("Cannot redefine subtype '%~s'", TS_Name_Get(ts, subtype));
      return Failed;
    }
    return Success;
  }

  /* CHECK: MAYBE WE SHOULD RESOVE THE TYPE HERE */
  s_td->target = subtype_type;
  s_td->size = sizeof(Subtype);
  TASK( Member_Full_Name(ts, & full_name, parent, child_str) );
  HT_Insert_Obj(ts->subtypes,
                full_name.text, full_name.length,
                & subtype, sizeof(Type));
  return Success;
}

void TS_Subtype_Find(TS *ts, Type *subtype, Type parent, Name *child) {
  Name full_name;
  HashItem *hi;
  char *child_str = Name_To_Str(child);
  /*s = TS_Resolve(ts, s, 1, 1);*/
  Task t = Member_Full_Name(ts, & full_name, parent, child_str);
  Mem_Free(child_str);
  *subtype = TS_TYPE_NONE;
  if IS_FAILED(t) return;
  if (HT_Find(ts->subtypes, full_name.text, full_name.length, & hi))
    *subtype = *((Type *) hi->object);
}

#if 0
ParentType.ChildName = ChildType;

Window.Save = Void
()Char@Window.Save[Print["Saving window into ", $$::;]]

#endif



/****************************************************************************/

Task TS_Default_Value(TS *ts, Type *dv_t, Type t, Data *dv) {
  MSG_ERROR("Still not implemented!"); return Failed;
}

TSCmp TS_Compare(TS *ts, Type t1, Type t2) {
  TSDesc *td1, *td2;
  TSCmp cmp = TS_TYPES_EQUAL;
  if (t1 == t2) return TS_TYPES_EQUAL;
  while(1) {
    td1 = Resolve(ts, & t1, t1, 1);
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
 * This code re-implements typeman.c as a wrapper around typesys.c
 */

void Tym_Procedure_Sym_Num(UInt *sym_num, Type p) {
  TS_Procedure_Sym_Num(last_ts, sym_num, p);
}

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
  case TS_KIND_SUBTYPE:
  default:
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
  Mem_Free(name);

  /* I set all the remaining values of the structure s */
  s->symtype = VARIABLE;
  *new_type = type;
  Expr_New_Type(& s->value, type);
  return Success;
}

Int Tym_Def_Array_Of(Int num, Int type) {
  Type array;
  Task t = TS_Array_New(last_ts, & array, type, num);
  assert(t == Success);
  return array;
}

Int Tym_Def_Alias_Of(Name *nm, Int type) {
  Type alias;
  Task t = TS_Alias_New(last_ts, & alias, type);
  assert(t == Success);
  TS_Name_Set(last_ts, alias, Name_To_Str(nm));
  return alias;
}

int Tym_Compare_Types(Intg type1, Intg type2, int *need_expansion) {
  int dummy;
  if (need_expansion == (int *) NULL) need_expansion = & dummy;
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
  Task t = TS_Procedure_New(last_ts, & procedure, of_type, proc, kind);
  assert(t == Success);
  t = TS_Procedure_Register(last_ts, procedure, sym_num);
  assert(t == Success);
  return procedure;
}

Int Tym_Search_Procedure(Int proc, int second, Int of_type,
                         Int *exp) {

  Type found;
  int kind = second ? 2 : 1;
  Task t = TS_Procedure_Search(last_ts, & found, exp, of_type, proc, kind);
  assert(t == Success);
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
  if (! TS_Is_Member(last_ts, *type)) *type = TYPE_NONE;
  return Success;
}

Task Tym_Structure_Get(Int *type) {
  *type = TS_Member_Next(last_ts, *type);
  if (! TS_Is_Member(last_ts, *type)) *type = TYPE_NONE;
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
