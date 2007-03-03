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
#include "hashtable.h"

Task TS_Init(TS *ts) {
  /* Replace Array with Collection <-----------------------------------------------*/
  TASK( Arr_New(& ts->type_descs, sizeof(TSDesc), TS_TSDESC_ARR_SIZE) );
  HT(& ts->members, TS_MEMB_HT_SIZE);
  return Success;
}

void TS_Destroy(TS *ts) {
  Arr_Destroy(ts->type_descs);
  HT_Destroy(ts->members);
}

#if 0
void TS_Def(TS *ts, Type *new, TSDesc *td) {}

void TS_Undef(TS *ts, Type *t) {}
#endif

Task TS_Intrinsic_New(TS *ts, Type *i, Int size) {
  TSDesc td;
  assert(size >= 0);
  td.kind = TS_KIND_INTRINSIC;
  td.size = size;
  td.target = TS_TYPE_NONE;
  td.name = (char *) NULL;
  td.val = NULL;
  TASK( Arr_Push(ts->type_descs, & td) );
  *i = Arr_NumItems(ts->type_descs);
  return Success;
}

Task TS_Name_Set(TS *ts, Type t, const char *name) {
  TSDesc *td = Arr_ItemPtr(ts->type_descs, TSDesc, t);
  if (td->name != (char *) NULL) {
    MSG_ERROR("TS_Name_Set: trying to set the name '%s' for type %d: "
     "this type has been already given the name '%s'!", name, t, td->name);
    return Failed;
  }
  td->name = strdup(name);
  return Success;
}

Task TS_Alias_New(TS *ts, Type *a, Type t) {
  TSDesc td;
  TSDesc *target_td = Arr_ItemPtr(ts->type_descs, TSDesc, t);
  td.kind = TS_KIND_ALIAS;
  td.target = t;
  td.size = target_td->size;
  td.name = (char *) NULL;
  td.val = NULL;
  TASK( Arr_Push(ts->type_descs, & td) );
  *a = Arr_NumItems(ts->type_descs);
  return Success;
}

Task TS_Link_New(TS *ts, Type *l, Type t) {
  TSDesc td;
  TSDesc *target_td = Arr_ItemPtr(ts->type_descs, TSDesc, t);
  td.kind = TS_KIND_LINK;
  td.target = t;
  td.size = target_td->size;
  td.name = (char *) NULL;
  td.val = NULL;
  TASK( Arr_Push(ts->type_descs, & td) );
  *l = Arr_NumItems(ts->type_descs);
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
  TSDesc *target_td = Arr_ItemPtr(ts->type_descs, TSDesc, t);
  td.kind = TS_KIND_ARRAY;
  td.target = t;
  td.size = size < 0 ? -1 : size*target_td->size;
  td.name = (char *) NULL;
  td.val = NULL;
  TASK( Arr_Push(ts->type_descs, & td) );
  *a = Arr_NumItems(ts->type_descs);
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
