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
  TS_KIND_ENUM,
  TS_KIND_MEMBER,
  TS_KIND_ARRAY,
  TS_KIND_PROC,
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
  Type first_proc;
  union {
    Int array_size;
    Type last;
    Type member_next;
    struct {
      int kind;
      Type parent;
      UInt sym_num;
    } proc;
  } data;
} TSDesc;

/* Used to initialise the structure TSDesc */
#define TS_TSDESC_INIT(tsdesc) \
  (tsdesc)->val = (char *) NULL; \
  (tsdesc)->name = (char *) NULL; \
  (tsdesc)->first_proc = TS_TYPE_NONE

typedef struct {
  Collection *type_descs;
  Hashtable *members;
  Array *name_buffer;
} TS;

typedef enum {
  TS_TYPES_EQUAL=7,
  TS_TYPES_MATCH=3,
  TS_TYPES_EXPAND=1,
  TS_TYPES_UNMATCH=0
} TSCmp;

Task TS_Init(TS *ts);

void TS_Destroy(TS *ts);

Int TS_Size(TS *ts, Type t);

Int TS_Align(TS *ts, Int address);

Task TS_Intrinsic_New(TS *ts, Type *i, Int size);

/** Create a new procedure type in p. init tells if the procedure
 * is an initialisation procedure or not.
 */
Task TS_Procedure_New(TS *ts, Type *p, Type parent, Type child, int kind);

/** Register the procedure p. After this function has been called
 * the procedure p will belong to the list of procedures of its parent.
 * sym_num is the symbol associated with the procedure.
 */
Task TS_Procedure_Register(TS *ts, Type p, UInt sym_num);

/** Obtain the symbol identification number of a registered procedure.
 */
void TS_Procedure_Sym_Num(TS *ts, UInt *sym_num, Type p);

/** Search the given procedure in the list of registered procedures.
 * Return the procedure in *proc, or TS_TYPE_NONE if the procedure
 * has not been found. If the argument of the procedure needs to be expanded
 * *expansion_type is the target type for that expansion. If expansion
 * is not needed then *expansion_type = TS_TYPE_NONE.
 */
Task TS_Procedure_Search(TS *ts, Type *proc, Type *expansion_type,
 Type parent, Type child, int kind);

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

/** Search the member 'm_name' from the members of the structure s.
 * If the member is found then *m will be set with its type number,
 * otherwise *m will be TS_TYPE_NONE
 */
void TS_Member_Find(TS *ts, Type *m, Type s, const char *m_name);

/** Obtain details about a member of a structure (to be used in conjunction
 * with TS_Member_Find)
 */
void TS_Member_Get(TS *ts, Type *t, Int *address, Type m);

/** If m is a structure/species/enum returns its first member.
 * If m is a member, return the next member.
 * It m is the last member, return the parent structure.
 */
Type TS_Member_Next(TS *ts, Type m);

/** This function tells if a type t2 is contained into a type t1.
 * The return value is the following:
 * - TS_TYPES_EQUAL: the two types are equal;
 * - TS_TYPES_MATCH: the two types are equal;
 * - TS_TYPES_EXPAND: the two types are compatible, but type t2 should
 *    be expanded to type t1;
 * - TS_TYPES_UNMATCH: the two types are not compatible;
 */
TSCmp TS_Compare(TS *ts, Type t1, Type t2);

#endif
