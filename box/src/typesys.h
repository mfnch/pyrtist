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

/** @file typesys.h
 * @brief The type system for the Box compiler.
 *
 * Some words.
 */

#ifndef _TYPESYS_H
#  define _TYPESYS_H

#  include "types.h"
#  include "collection.h"
#  include "hashtable.h"

typedef Int Type;

typedef enum {
  TS_KIND_INTRINSIC=1,
  TS_KIND_LINK,
  TS_KIND_ALIAS,
  TS_KIND_SPECIES,
  TS_KIND_STRUCTURE,
  TS_KIND_MEMBER,
  TS_KIND_ENUM,
  TS_KIND_ARRAY,
  TS_KIND_POINTER
} TSKind;

enum {
  TS_TYPE_NONE = -1,
  TS_SIZE_UNKNOWN = -1
};

typedef struct {
  TSKind kind;
  Int size;
  char *name;
  void *val;
  Type target;
  union {
    Int array_size;
    Type structure_last;
    Type species_last;
    Type last;
    Type member_next;
  } data;
} TSDesc;

typedef struct {
  Collection *type_descs;
  Hashtable *members;
} TS;

Task TS_Init(TS *ts);

void TS_Destroy(TS *ts);

Int TS_Size(TS *ts, Type t);

Int TS_Align(TS *ts, Int address);

Task TS_Intrinsic_New(TS *ts, Type *i, Int size);

Task TS_Name_Set(TS *ts, Type t, const char *name);

char *TS_Name_Get(TS *ts, Type t);

Task TS_Alias_New(TS *ts, Type *a, Type t);

Task TS_Link_New(TS *ts, Type *l, Type t);

Task TS_Structure_Begin(TS *ts, Type *s);

Task TS_Structure_Add(TS *ts, Type s, Type m, const char *m_name);

Task TS_Array_New(TS *ts, Type *a, Type t, Int size);

Task TS_Species_Begin(TS *ts, Type *s);

Task TS_Species_Add(TS *ts, Type s, Type m);

Task TS_Enum_Begin(TS *ts, Type *e);

Task TS_Enum_Add(TS *ts, Type e, Type t);

Task TS_Default_Value(TS *ts, Type *dv_t, Type t, Data *dv);

#endif
