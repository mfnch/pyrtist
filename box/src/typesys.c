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
  Type t, t1;
  (void) TS_Intrinsic_New(ts, & t, sizeof(Int));
  (void) TS_Name_Set(ts, t, "Int");
  MSG_ADVICE("Definito il tipo '%~s'", TS_Name_Get(ts, t));
  (void) TS_Array_New(ts, & t1, t, 10);
  MSG_ADVICE("Definito il tipo '%~s'", TS_Name_Get(ts, t1));
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
    MSG_ERROR("TS_Name_Set: trying to set the name '%s' for type %d: "
     "this type has been already given the name '%s'!", name, t, td->name);
    return Failed;
  }
  td->name = strdup(name);
  return Success;
}

const char *TS_Name_Get_Orig(TS *ts, Type t) {
  TSDesc *td = Clc_ItemPtr(ts->type_descs, TSDesc, t);
  if (td->name != (char *) NULL) return td->name;

  switch(td->kind) {
  case TS_KIND_INTRINSIC:
    return print("<size=%I>", td->size);
  case TS_KIND_ARRAY:
    if (td->size > 0) {
      Int array_size = td->data.array_size;
      return print("(%d)%s", array_size, TS_Name_Get_Orig(ts, td->target));
    } else
      return print("()%s", TS_Name_Get_Orig(ts, td->target));
  default:
    return "<unknown type>";
  }
}

char *TS_Name_Get(TS *ts, Type t) {
  return strdup(TS_Name_Get_Orig(ts, t));
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
  MSG_ERROR("Stil not implemented!"); return Failed;
}

Task TS_Structure_Add(TS *ts, Type s, Type m, char *m_name) {
  MSG_ERROR("Stil not implemented!"); return Failed;
}

Task TS_Array_New(TS *ts, Type *a, Type t, Int size) {
  TSDesc td;
  TSDesc *target_td = Clc_ItemPtr(ts->type_descs, TSDesc, t);
  td.kind = TS_KIND_ARRAY;
  td.target = t;
  td.size = size < 0 ? -1 : size*target_td->size;
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
