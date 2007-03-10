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
  Type ti, tr, tp, t1, t2, t3, t4, t5;
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
  (void) TS_Structure_Begin(ts, & t3);
  (void) TS_Structure_Add(ts, t3, t2, "my_structure");
  (void) TS_Structure_Add(ts, t3, t1, "my_array");
  (void) TS_Structure_Add(ts, t3, ti, "my_int");
  MSG_ADVICE("Definito il tipo '%~s'", TS_Name_Get(ts, t3));

  (void) TS_Species_Begin(ts, & t4);
  MSG_ADVICE("Definito il tipo '%~s'", TS_Name_Get(ts, t4));
  (void) TS_Species_Add(ts, t4, tr);
  MSG_ADVICE("Definito il tipo '%~s'", TS_Name_Get(ts, t4));
  (void) TS_Species_Add(ts, t4, ti);
  MSG_ADVICE("Definito il tipo '%~s'", TS_Name_Get(ts, t4));
  (void) TS_Species_Add(ts, t4, tp);
  MSG_ADVICE("Definito il tipo '%~s'", TS_Name_Get(ts, t4));

  (void) TS_Enum_Begin(ts, & t5);
  MSG_ADVICE("Definito il tipo '%~s'", TS_Name_Get(ts, t5));
  (void) TS_Enum_Add(ts, t5, tr);
  MSG_ADVICE("Definito il tipo '%~s'", TS_Name_Get(ts, t5));
  (void) TS_Enum_Add(ts, t5, ti);
  MSG_ADVICE("Definito il tipo '%~s'", TS_Name_Get(ts, t5));
  (void) TS_Enum_Add(ts, t5, tp);
  MSG_ADVICE("Definito il tipo '%~s'", TS_Name_Get(ts, t5));
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

  default:
    return Mem_Strdup("<unknown type>");
  }
}

/****************************************************************************/
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

/* Here we define functions which have almost common bodies.
 * This is done in a tricky way (look at the documentation inside
 * the included file!)
 */
#define TS_ALIAS_NEW
#include "tsdef.c"
#undef TS_ALIAS_NEW

#define TS_LINK_NEW
#include "tsdef.c"
#undef TS_LINK_NEW

#define TS_ARRAY_NEW
#include "tsdef.c"
#undef TS_ARRAY_NEW

/****************************************************************************/
#define TS_STRUCTURE_BEGIN
#include "tsdef.c"
#undef TS_STRUCTURE_BEGIN

#define TS_SPECIES_BEGIN
#include "tsdef.c"
#undef TS_SPECIES_BEGIN

#define TS_ENUM_BEGIN
#include "tsdef.c"
#undef TS_ENUM_BEGIN

/****************************************************************************/
#define TS_STRUCTURE_ADD
#include "tsdef.c"
#undef TS_STRUCTURE_ADD

#define TS_SPECIES_ADD
#include "tsdef.c"
#undef TS_SPECIES_ADD

#define TS_ENUM_ADD
#include "tsdef.c"
#undef TS_ENUM_ADD

Task TS_Default_Value(TS *ts, Type *dv_t, Type t, Data *dv) {
  MSG_ERROR("Stil not implemented!"); return Failed;
}
