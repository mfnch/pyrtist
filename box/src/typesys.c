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
#include "array.h"
#include "collection.h"
#include "hashtable.h"

static void try_it(TS *ts) {
  Type ti, tr, tp, t1, t2;
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
  MSG_ADVICE("Definito il tipo '%~s'", TS_Name_Get(ts, t2));
  (void) TS_Structure_Add(ts, t2, tr, "x");
  MSG_ADVICE("Definito il tipo '%~s'", TS_Name_Get(ts, t2));
  (void) TS_Structure_Add(ts, t2, ti, "stuff");
  MSG_ADVICE("Definito il tipo '%~s'", TS_Name_Get(ts, t2));
  (void) TS_Structure_Add(ts, t2, tp, NULL);
  MSG_ADVICE("Definito il tipo '%~s'", TS_Name_Get(ts, t2));
}

Task TS_Init(TS *ts) {
  TASK( Clc_New(& ts->type_descs, sizeof(TSDesc), TS_TSDESC_CLC_SIZE) );
  HT(& ts->members, TS_MEMB_HT_SIZE);
  try_it(ts);
  return Success;
}

void TS_Destroy(TS *ts) {
  Clc_Destroy(ts->type_descs);
  HT_Destroy(ts->members);
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

Task TS_Intrinsic_New(TS *ts, Type *i, Int size) {
  TSDesc td;
  assert(size >= 0);
  td.kind = TS_KIND_INTRINSIC;
  td.size = size;
  td.target = TS_TYPE_NONE;
  td.name = (char *) NULL;
  td.val = NULL;
  TASK( Clc_Occupy(ts->type_descs, & td, i) );
  return Success;
}

Task TS_Name_Set(TS *ts, Type t, const char *name) {
  TSDesc *td = Clc_ItemPtr(ts->type_descs, TSDesc, t);
  if (td->name != (char *) NULL) {
    MSG_ERROR("TS_Name_Set: trying to set the name '%s' for type %I: "
     "this type has been already given the name '%s'!", name, t, td->name);
    return Failed;
  }
  td->name = strdup(name);
  return Success;
}

char *TS_Name_Get(TS *ts, Type t) {
  TSDesc *td = Clc_ItemPtr(ts->type_descs, TSDesc, t);
  if (td->name != (char *) NULL) return strdup(td->name);

  switch(td->kind) {
  case TS_KIND_INTRINSIC:
    return printdup("<size=%I>", td->size);
  case TS_KIND_STRUCTURE:
    if (td->size > 0) {
      char *name = (char *) NULL;
      Type m = td->target;
      while (1) {
        TSDesc *m_td = Clc_ItemPtr(ts->type_descs, TSDesc, m);
        char *m_name = TS_Name_Get(ts, m_td->target);
        if (m_td->name != (char *) NULL)
          m_name = printdup("%s=%~s", m_td->name, m_name);
        m = m_td->data.member_next;
        if (m == t) {
          if (name == (char *) NULL)
            return printdup("(%~s,)", m_name);
          else
            return printdup("(%~s, %~s)", name, m_name);
        } else {
          if (name == (char *) NULL)
            name = m_name; /* no need to free! */
          else
            name = printdup("%~s, %~s", name, m_name);
        }
      }
    }
    return strdup("(,)");
  case TS_KIND_ARRAY:
    if (td->size > 0) {
      Int as = td->data.array_size;
      return printdup("(%I)%~s", as, TS_Name_Get(ts, td->target));
    } else
      return printdup("()%~s", TS_Name_Get(ts, td->target));
  default:
    return strdup("<unknown type>");
  }
}

Task TS_Alias_New(TS *ts, Type *a, Type t) {
  TSDesc td;
  TSDesc *target_td = Clc_ItemPtr(ts->type_descs, TSDesc, t);
  td.kind = TS_KIND_ALIAS;
  td.target = t;
  td.size = target_td->size;
  td.name = (char *) NULL;
  td.val = NULL;
  TASK( Clc_Occupy(ts->type_descs, & td, a) );
  return Success;
}

Task TS_Link_New(TS *ts, Type *l, Type t) {
  TSDesc td;
  TSDesc *target_td = Clc_ItemPtr(ts->type_descs, TSDesc, t);
  td.kind = TS_KIND_LINK;
  td.target = t;
  td.size = target_td->size;
  td.name = (char *) NULL;
  td.val = NULL;
  TASK( Clc_Occupy(ts->type_descs, & td, l) );
  return Success;
}

Task TS_Structure_Begin(TS *ts, Type *s) {
  TSDesc td;
  td.kind = TS_KIND_STRUCTURE;
  td.target = TS_TYPE_NONE;
  td.data.structure_last = TS_TYPE_NONE;
  td.size = 0;
  td.name = (char *) NULL;
  td.val = NULL;
  TASK( Clc_Occupy(ts->type_descs, & td, s) );
  return Success;
}

Task TS_Structure_Add(TS *ts, Type s, Type m, const char *m_name) {
  TSDesc td, *m_td, *s_td;
  Type new_m;
  Int m_size;
  m_td = Clc_ItemPtr(ts->type_descs, TSDesc, m);
  m_size = m_td->size;
  td.kind = TS_KIND_MEMBER;
  td.target = m;
  td.size = TS_Align(ts, TS_Size(ts, s));
  td.name = (char *) NULL;
  if (m_name != (char *) NULL) td.name = strdup(m_name);
  td.data.member_next = s;
  td.val = NULL;
  TASK( Clc_Occupy(ts->type_descs, & td, & new_m) );

  s_td = Clc_ItemPtr(ts->type_descs, TSDesc, s);
  m_td = Clc_ItemPtr(ts->type_descs, TSDesc, s_td->data.structure_last);
  s_td->data.structure_last = new_m;
  if (s_td->target == TS_TYPE_NONE) s_td->target = new_m;
  s_td->size += m_size;
  m_td->data.member_next = new_m;
  return Success;
}

Task TS_Array_New(TS *ts, Type *a, Type t, Int size) {
  TSDesc td;
  TSDesc *target_td = Clc_ItemPtr(ts->type_descs, TSDesc, t);
  td.kind = TS_KIND_ARRAY;
  td.target = t;
  td.size = size < 0 ? TS_SIZE_UNKNOWN : size*target_td->size;
  td.name = (char *) NULL;
  td.val = NULL;
  td.data.array_size = size;
  TASK( Clc_Occupy(ts->type_descs, & td, a) );
  return Success;
}

Task TS_Species_Begin(TS *ts, Type *s) {
  MSG_ERROR("Stil not implemented!"); return Failed;
}

Task TS_Species_Add(TS *ts, Type s, Type m) {
  MSG_ERROR("Stil not implemented!"); return Failed;
}

Task TS_Enum_Begin(TS *ts, Type *e) {
  MSG_ERROR("Stil not implemented!"); return Failed;
}

Task TS_Enum_Add(TS *ts, Type e, Type t) {
  MSG_ERROR("Stil not implemented!"); return Failed;
}

Task TS_Default_Value(TS *ts, Type *dv_t, Type t, Data *dv) {
  MSG_ERROR("Stil not implemented!"); return Failed;
}
