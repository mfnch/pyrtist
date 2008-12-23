/****************************************************************************
 * Copyright (C) 2008 by Matteo Franchin                                    *
 *                                                                          *
 * This file is part of Box.                                                *
 *                                                                          *
 *   Box is free software: you can redistribute it and/or modify it         *
 *   under the terms of the GNU Lesser General Public License as published  *
 *   by the Free Software Foundation, either version 3 of the License, or   *
 *   (at your option) any later version.                                    *
 *                                                                          *
 *   Box is distributed in the hope that it will be useful,                 *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Lesser General Public License for more details.                    *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with Box.  If not, see <http://www.gnu.org/licenses/>.   *
 ****************************************************************************/

/* $Id$ */

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "defaults.h"
#include "types.h"
#include "messages.h"
#include "mem.h"
#include "str.h"
#include "array.h"
#include "collection.h"
#include "hashtable.h"
#include "virtmach.h"
#include "typesys.h"


static TS *last_ts; /* Just for transition: will be removed! */

/* This will disappear in the future */
static char *last_name = (char *) NULL;

static Task Destroy_TSDesc(void *td) {
  BoxMem_Free(((TSDesc *) td)->name);
  ((TSDesc *) td)->name = 0;
  return Success;
}

Task TS_Init(TS *ts) {
  TASK( Clc_New(& ts->type_descs, sizeof(TSDesc), TS_TSDESC_CLC_SIZE) );
  Clc_Destructor(ts->type_descs, Destroy_TSDesc);
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
  BoxMem_Free(last_name);
  last_name = (char *) NULL;
}

static Task Type_New(TS *ts, Type *new_type, TSDesc *td) {
  UInt nt;
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

int TS_Is_Anonymous(TS *ts, Type t) {
  TSDesc *td;

  if (t == TS_TYPE_NONE) return 1;

  td = Type_Ptr(ts, t);
  return (td->name == (char *) NULL);
}

int TS_Is_Special(Type t) {
  switch(t) {
  case TYPE_OPEN:
  case TYPE_CLOSE:
  case TYPE_PAUSE:
  case TYPE_DESTROY:
    return 1;
  default:
    return 0;
  }
}

Type TS_Resolve_Once(TS *ts, Type t, TSKindSelect select) {
  int resolve, resolve_only_anonimous, is_not_anonimous;
  TSDesc *td;
  Type rt;

  if (t == TS_TYPE_NONE) return TS_TYPE_NONE;

  td = Type_Ptr(ts, t);

  rt = td->target;
  switch(td->kind) {
  case TS_KIND_MEMBER:
    resolve = 1; break;
  case TS_KIND_ALIAS:
    resolve = ((select & TS_KS_ALIAS) != 0); break;
  case TS_KIND_DETACHED:
    resolve = ((select & TS_KS_DETACHED) != 0); break;
  case TS_KIND_SPECIES:
    rt = td->data.last;
    resolve = ((select & TS_KS_SPECIES) != 0);
    break;
  case TS_KIND_SUBTYPE:
    resolve = ((select & TS_KS_SUBTYPE) != 0);
    /* NOTE: the child of a subtype is never a subtype itself!
     *  Consequently the resolution will take place only once!
     */
    break;
  default:
    resolve = 0;
  }

  resolve_only_anonimous = (select & TS_KS_ANONYMOUS) != 0;
  is_not_anonimous = (td->name != (char *) NULL);
  if (resolve_only_anonimous && is_not_anonimous) return t;
  return resolve ? rt : t;
}

Type TS_Resolve(TS *ts, Type t, TSKindSelect select) {
  Type rt = t;
  do {
    t = rt;
    rt = TS_Resolve_Once(ts, t, select);
  } while(rt != t);
  return t;
}

Type TS_Core_Type(TS *ts, Type t) {
  return TS_Resolve(ts, t, TS_KS_ALIAS | TS_KS_DETACHED | TS_KS_SPECIES);
}

int TS_Is_Fast(TS *ts, Type t) {
  Type ct = TS_Core_Type(ts, t);
  return (ct >= TYPE_FAST_FIRST && ct <= TYPE_FAST_LAST);
}

int TS_Structure_Is_Fast(TS *ts, Type structure) {
  int fast_structure = 1;
  Type ct = TS_Core_Type(ts, structure), member_t;

  assert(TS_Is_Structure(ts, ct));
  member_t = TS_Member_Next(ts, ct);
  while(TS_Is_Member(ts, member_t)) {
    /* Resolve the member into a proper type */
    fast_structure &= TS_Is_Fast(ts, member_t);
    member_t = TS_Member_Next(ts, member_t);
  }

  return fast_structure;
}

Int TS_Size(TS *ts, Type t) {
  TSDesc *td = Type_Ptr(ts, t);
  return td->size;
}

TSKind TS_Kind(TS *ts, Type t) {
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
              "this type has already been given the name '%s'!",
              name, t, td->name);
    return Failed;
  }
  td->name = BoxMem_Strdup(name);
  return Success;
}

static char *TS_Name_Get_Case(TSKind kind, TS *ts, TSDesc *td, Type t,
                              char *empty, char *one, char *two, char *many) {
  char *name = (char *) NULL;
  Type m = td->target;
  Type previous_type = TS_TYPE_NONE;

  if (td->size < 1)
    return BoxMem_Strdup(empty);

  while (1) {
    TSDesc *m_td = Type_Ptr(ts, m);
    char *m_name = TS_Name_Get(ts, m_td->target);
    if (kind == TS_KIND_STRUCTURE) {
      if (m_td->name != (char *) NULL) {
        if (m_td->target != previous_type)
          m_name = printdup("%~s %s", m_name, m_td->name);
        else {
          BoxMem_Free(m_name);
          m_name = BoxMem_Strdup(m_td->name);
        }
      }
      previous_type = m_td->target;
    }

    m = m_td->data.member_next;
    if (m == t) {
      if (name == (char *) NULL)
        return printdup(one, m_name);
      else
        return printdup(two, name, m_name);
    } else {
      if (name == (char *) NULL)
        name = m_name; /* no need to free! */
      else
        name = printdup(many, name, m_name);
    }
  }
}

char *TS_Name_Get(TS *ts, Type t) {
  TSDesc *td = Type_Ptr(ts, t);
  td = Resolve(ts, & t, t, 0);
  if (td->name != (char *) NULL) return BoxMem_Strdup(td->name);

  switch(td->kind) {
  case TS_KIND_INTRINSIC:
    return printdup("<size=%I>", td->size);

  case TS_KIND_DETACHED:
    return printdup("++%~s", TS_Name_Get(ts, td->target));

  case TS_KIND_ARRAY:
    if (td->size > 0) {
      Int as = td->data.array_size;
      return printdup("(%I)%~s", as, TS_Name_Get(ts, td->target));
    } else
      return printdup("()%~s", TS_Name_Get(ts, td->target));

  case TS_KIND_STRUCTURE:
    return TS_Name_Get_Case(TS_KIND_STRUCTURE, ts, td, t,
                            "(,)", "(%~s,)", "(%~s, %~s)", "%~s, %~s");

  case TS_KIND_SPECIES:
    return TS_Name_Get_Case(TS_KIND_SPECIES, ts, td, t,
                            "(->)", "(%~s->)", "(%~s->%~s)", "%~s->%~s");

  case TS_KIND_ENUM:
    return TS_Name_Get_Case(TS_KIND_ENUM, ts, td, t,
                            "(|)", "(%~s|)", "(%~s|%~s)", "%~s|%~s");

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
    return BoxMem_Strdup("<unknown type>");
  }
}

Task TS_Array_Member(TS *ts, Type *memb, Type array, Int *array_size) {
  Type a = TS_Resolve(ts, array,   TS_KS_ALIAS | TS_KS_SPECIES
                                 | TS_KS_DETACHED);
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
  s = TS_Resolve(ts, s, TS_KS_ALIAS | TS_KS_SPECIES | TS_KS_DETACHED);
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

char *TS_Member_Name_Get(TS *ts, Type member) {
  TSDesc *m_td = Type_Ptr(ts, member);
  assert(m_td->kind == TS_KIND_MEMBER);
  return m_td->name;
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

/*FUNCTIONS: TS_X_New *******************************************************/

/* Common code for Ts_Alias_New, TS_Detached_New and TS_Array_New. */
static Task TS_X_New(TSKind kind, TS *ts, Type *dst, Type src, Int size) {
  TSDesc td;
  TSDesc *src_td = Type_Ptr(ts, src);
  TS_TSDESC_INIT(& td);
  td.kind = kind;
  td.target = src;
  if (kind == TS_KIND_ARRAY) {
    td.size = size < 0 ? TS_SIZE_UNKNOWN : size*src_td->size;
    td.data.array_size = size;
  } else {
    td.size = src_td->size;
  }
  TASK( Type_New(ts, dst, & td) );
  return Success;
}

Task TS_Alias_New(TS *ts, Type *alias, Type origin) {
  return TS_X_New(TS_KIND_ALIAS, ts, alias, origin, -1);
}

Task TS_Detached_New(TS *ts, Type *detached, Type origin) {
  return TS_X_New(TS_KIND_DETACHED, ts, detached, origin, -1);
}

Task TS_Array_New(TS *ts, Type *array, Type item, Int num_items) {
  return TS_X_New(TS_KIND_ARRAY, ts, array, item, num_items);
}

/*FUNCTIONS: TS_X_Begin *****************************************************/

/* Code for TS_Structure_Begin, TS_Species_Begin, etc. */
static Task TS_X_Begin(TSKind kind, TS *ts, Type *s) {
  TSDesc td;
  TS_TSDESC_INIT(& td);
  td.kind = kind;
  td.target = TS_TYPE_NONE;
  td.data.last = TS_TYPE_NONE;
  td.size = 0;
  return Type_New(ts, s, & td);
}

Task TS_Structure_Begin(TS *ts, Type *structure) {
  return TS_X_Begin(TS_KIND_STRUCTURE, ts, structure);
}

Task TS_Species_Begin(TS *ts, Type *species) {
  return TS_X_Begin(TS_KIND_SPECIES, ts, species);
}

Task TS_Enum_Begin(TS *ts, Type *enumeration) {
  return TS_X_Begin(TS_KIND_ENUM, ts, enumeration);
}

/*FUNCTIONS: TS_X_Add *******************************************************/

/* Code for TS_Structure_Add, TS_Species_Add, etc. */
static Task TS_X_Add(TSKind kind, TS *ts, Type s, Type m,
                     const char *m_name) {
  TSDesc td, *m_td, *s_td;
  Type new_m;
  Int m_size;
  m_td = Type_Ptr(ts, m);
  m_size = m_td->size;
  TS_TSDESC_INIT(& td);
  td.kind = TS_KIND_MEMBER;
  td.target = m;
  if (kind == TS_KIND_STRUCTURE) {
    if (m_name != (char *) NULL) td.name = BoxMem_Strdup(m_name);
    td.size = TS_Align(ts, TS_Size(ts, s));
  } else {
    td.size = m_size;
  }
  td.data.member_next = s;
  TASK( Type_New(ts, & new_m, & td) );

  s_td = Type_Ptr(ts, s);
  assert(s_td->kind == kind);
  if (s_td->data.last != TS_TYPE_NONE) {
    m_td = Type_Ptr(ts, s_td->data.last);
    assert(m_td->kind == TS_KIND_MEMBER);
    m_td->data.member_next = new_m;
  }
  s_td->data.last = new_m;
  if (s_td->target == TS_TYPE_NONE) s_td->target = new_m;

  switch(kind) {
  case TS_KIND_STRUCTURE:
    s_td->size += m_size;
    /* We also add the member to the hashtable for fast search */
    if (m_name != (char *) NULL) {
      Name n;
      TASK( Member_Full_Name(ts, & n, s, m_name) );
      HT_Insert_Obj(ts->members, n.text, n.length, & new_m, sizeof(Type));
    }
    break;

  case TS_KIND_ENUM:
    m_size += TS_Align(ts, sizeof(Int));
    if (m_size > s_td->size) s_td->size = m_size;
    break;

  default:
    if (m_size > s_td->size) s_td->size = m_size;
    break;
  }
  return Success;
}

Task TS_Structure_Add(TS *ts, Type structure, Type member_type,
                      const char *member_name) {
  return TS_X_Add(TS_KIND_STRUCTURE, ts, structure, member_type, member_name);
}

Task TS_Species_Add(TS *ts, Type species, Type member) {
  return TS_X_Add(TS_KIND_SPECIES, ts, species, member, NULL);
}

Task TS_Enum_Add(TS *ts, Type enumeration, Type member) {
  return TS_X_Add(TS_KIND_ENUM, ts, enumeration, member, NULL);
}

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

void TS_Procedure_Sym_Num_Get(TS *ts, UInt *sym_num, Type p) {
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

void TS_Procedure_Search(TS *ts, Type *proc, Type *expansion_type,
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
        return;
      }
    }
  }
}

void TS_Procedure_Inherited_Search(TS *ts, Type *proc, Type *expansion_type,
                                   Type parent, Type child, int kind) {
  Type dummy, previous_parent;
  assert(kind == 1 || kind == 2);
  if (proc == (Type *) NULL) proc = & dummy;
  do {
    TS_Procedure_Search(ts, proc, expansion_type, parent, child, kind);
    if (*proc != TS_TYPE_NONE) break;
    /* Resolve the parent type and retry */
    previous_parent = parent;
    parent = TS_Resolve_Once(ts, parent,
                             TS_KS_ALIAS | TS_KS_DETACHED | TS_KS_SPECIES |
                             TS_KS_SUBTYPE);
  } while (parent != previous_parent);
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
  BoxMem_Free(child_str);
  *subtype = TS_TYPE_NONE;
  if IS_FAILED(t) return;
  if (HT_Find(ts->subtypes, full_name.text, full_name.length, & hi))
    *subtype = *((Type *) hi->object);
}

void TS_Subtype_Get_Child(TS *ts, Type *child, Type subtype) {
  TSDesc *s_td = Type_Ptr(ts, subtype);

  assert(s_td->kind == TS_KIND_SUBTYPE);
  assert(s_td->target != TS_TYPE_NONE);
  assert(s_td->size != TS_SIZE_UNKNOWN);
  *child = s_td->target;
}

void TS_Subtype_Get_Parent(TS *ts, Type *parent, Type subtype) {
  TSDesc *s_td = Type_Ptr(ts, subtype);

  assert(s_td->kind == TS_KIND_SUBTYPE);
  assert(s_td->target != TS_TYPE_NONE);
  assert(s_td->size != TS_SIZE_UNKNOWN);
  *parent = s_td->data.subtype.parent;
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

    case TS_KIND_DETACHED:
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
      /* Member names are not matched, i.e. we tolerate:
       *   (Real x, y) == (Real z, hgj)
       * If one wants incompatible types, he should use type detachment
       * and define: Point = ++(Real x, y)
       * This way Point will be different to all the other types, including
       * (Real a, b) and (Real, Real).
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

#include <stdlib.h>

#include "compiler.h"
#include "str.h"

Int Tym_Type_Size(Int t) {return (Int) TS_Size(last_ts, (Type) t);}

const char *Tym_Type_Name(Int t) {
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
  BoxMem_Free(name);

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

int Tym_Compare_Types(Int type1, Int type2, int *need_expansion) {
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
  Int select;
  select  = not_alias ? 0 : TS_KS_ALIAS;
  select |= not_species ? 0 : TS_KS_SPECIES;
  return TS_Resolve(last_ts, type, select);
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

Task Tym_Def_Specie(Int *specie, Int type) {
  if (*specie == TYPE_NONE) {
    TASK( TS_Species_Begin(last_ts, specie) );
  }
  return TS_Species_Add(last_ts, *specie, type);
}

Task Tym_Def_Structure(Int *strc, Int type) {
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

Int Tym_Struct_Get_Num_Items(Int t) {
  return TS_Member_Count(last_ts, t);
}
