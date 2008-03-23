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
  TS_KIND_METHOD,
  TS_KIND_POINTER,
  TS_KIND_SUBTYPE
} TSKind;

enum {
  TS_TYPE_NONE = -1,
  TS_SIZE_UNKNOWN = -1
};

typedef enum {
  TS_KS_ALIAS=1,
  TS_KS_SPECIES=2,
  TS_KS_SUBTYPE=4
} TSKindSelect;

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
    struct {
      Type parent;
      char *child_name;
    } subtype;
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
  Hashtable *subtypes;
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
#define TS_Size_Get TS_Size

/** A type can be defined as an extension of other types.
 * This is what "MyType = Int" is supposed to do.
 * This function returns the extended type, given the extension type.
 * The function goes up only by one level. If "MyType2" extends "MyType1"
 * which in turn extends "Int", then:
 *
 *   TS_Resolve_Named_Type(MyType2) --> MyType1
 *   TS_Resolve_Named_Type(MyType1) --> Int
 *   TS_Resolve_Named_Type(Int) --> Int
 */
Type TS_Resolve_Named_Type(TS *ts, Type t);

/** Resolve types (useful for comparisons).
 * select is a combination (with operator |) of TSKindSelect
 * values which specify what exactly has to be resolved.
 */
Type TS_Resolve(TS *ts, Type t, Int select);

TSKind TS_Kind(TS *ts, Type t);

#define TS_Is_Member(ts, t) (TS_Kind((ts), (t)) == TS_KIND_MEMBER)
#define TS_Is_Subtype(ts, t) (TS_Kind((ts), (t)) == TS_KIND_SUBTYPE)

Int TS_Align(TS *ts, Int address);

Task TS_Intrinsic_New(TS *ts, Type *i, Int size);

/** Create a new procedure type in p. init tells if the procedure
 * is an initialisation procedure or not.
 */
Task TS_Procedure_New(TS *ts, Type *p, Type parent, Type child, int kind);

/** Get information about the procedure p. This information is stored
 * in the destination specified by the given pointers, but this happens
 * only if the pointer is != NULL.
 * @param ts the type-system data structure
 * @param parent the parent of the procedure
 * @param child the children of the procedure
 * @param kind the kind of the procedure
 * @param sym_num the symbol identification (for registered procedures)
 */
void TS_Procedure_Info(TS *ts, Type *parent, Type *child,
                       int *kind, UInt *sym_num, Type p);

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

/** Create and register a procedure by calling firt TS_Procedure_New
 * and then TS_Procedure_Register. sym_num is the associated symbol
 * identifier.
 */
Int TS_Procedure_Def(Int proc, int kind, Int of_type, Int sym_num);

/** Create a new unregistered subtype: a subtype is unregistered when
 * the parent is not aware of its existance. An unregistered type is defined
 * only giving its name and its parent type. When the subtype is registered
 * the parent type becomes aware of it. In order for the registration to be
 * completed the full type of the subtype must be specified.
 */
Task TS_Subtype_New(TS *ts, Type *new_subtype,
                    Type parent_type, Name *child_name);

/** Register a previously created (and still unregistered) subtype.
 */
Task TS_Subtype_Register(TS *ts, Type subtype, Type subtype_type);

/** Find the registered subtype with name child among the subtypes of type
 * parent. The found subtype is put inside *subtype. If no subtype is found
 * then TS_TYPE_NONE is returned inside *subtype.
 */
void TS_Subtype_Find(TS *ts, Type *subtype, Type parent, Name *child);

Task TS_Name_Set(TS *ts, Type t, const char *name);

char *TS_Name_Get(TS *ts, Type t);

Task TS_Alias_New(TS *ts, Type *a, Type t);

Task TS_Link_New(TS *ts, Type *l, Type t);

#if 0
Task TS_SubType_New(TS *ts, Type *s,)
#endif

Task TS_Structure_Begin(TS *ts, Type *s);

Task TS_Structure_Add(TS *ts, Type s, Type m, const char *m_name);

Task TS_Array_New(TS *ts, Type *a, Type t, Int size);

Task TS_Species_Begin(TS *ts, Type *s);

Task TS_Species_Add(TS *ts, Type s, Type m);

Task TS_Enum_Begin(TS *ts, Type *e);

Task TS_Enum_Add(TS *ts, Type e, Type t);

/** Get the child type of a subtype. */
void TS_Subtype_Get_Child(TS *ts, Type *child, Type subtype);

/** Get the parent type of a subtype. */
void TS_Subtype_Get_Parent(TS *ts, Type *parent, Type subtype);

Task TS_Default_Value(TS *ts, Type *dv_t, Type t, Data *dv);

/** Given an array type (N)X returns X in *memb and the size N
 * of the array in *array_size.
 * An error is generated if array is not an array type.
 */
Task TS_Array_Member(TS *ts, Type *memb, Type array, Int *array_size);

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

/** Counts the member of a structure/species/enum (using TS_Member_Next) */
Int TS_Member_Count(TS *ts, Type s);

/** This function tells if a type t2 is contained into a type t1.
 * The return value is the following:
 * - TS_TYPES_EQUAL: the two types are equal;
 * - TS_TYPES_MATCH: the two types are equal;
 * - TS_TYPES_EXPAND: the two types are compatible, but type t2 should
 *    be expanded to type t1;
 * - TS_TYPES_UNMATCH: the two types are not compatible;
 */
TSCmp TS_Compare(TS *ts, Type t1, Type t2);

#  if 1
/* Enumero i tipi di tipo */
typedef enum {
  TOT_INSTANCE,
  TOT_PTR_TO,
  TOT_ARRAY_OF,
  TOT_SPECIE,
  TOT_PROCEDURE,
  TOT_PROCEDURE2,
  TOT_ALIAS_OF,
  TOT_STRUCTURE
} TypeOfType;

/* Stringhe corrispondenti ai tipi di tipi */
#define TOT_DESCRIPTIONS { \
 "instance of ", "pointer to", "array of ", "set of types", "procedure " }

/* Enumeration of special procedures */
enum {
  PROC_COPY    = -1,
  PROC_DESTROY = -2,
  PROC_SPECIAL_NUM = 3
};

#if 0
/* Struttura usata per descrivere i tipi di dati */
/* This structure has changed too many times. Now it is very ugly and dirty,
 * it should be completely rewritten, but this needs some work, since many
 * functions make assumptions about it.
 */
typedef struct {
  TypeOfType tot;
  Intg size;       /* Spazio occupato in memoria dal tipo */
  char *name;      /* Nome del tipo */

  Intg parent;     /* Specie a cui appartiene il tipo */
  Intg greater;    /* Tipo in cui puo' essere convertito */
  Intg target;     /* Per costruire puntatori a target, array di target, etc */
  Intg procedure;  /* Prima procedura corrispondente al tipo */
  union{
    Intg sym_num;  /* Symbol ID for the procedure */
    Intg arr_size; /* Numero di elementi dell'array */
    Intg st_size;  /* Numero di elementi della struttura */
    Intg sp_size;  /* Numero di elementi della specie */
/*    Symbol *sym;    Simbolo associato al tipo (NULL = non ce n'e'!)*/
  };
} TypeDesc;
#endif

/* Important builtin types */
extern Intg type_Point, type_RealNum, type_IntgNum, type_CharNum;

Intg Tym_Type_Size(Intg t);
TypeOfType Tym_Type_TOT(Int t);
UInt Tym_Proc_Get_Sym_Num(Int t);
Int Tym_Struct_Get_Num_Items(Int t);
const char *Tym_Type_Name(Intg t);
char *Tym_Type_Names(Intg t);
Task Tym_Def_Type(Intg *new_type,
 Intg parent, Name *nm, Intg size, Intg aliased_type);
Intg Tym_Def_Array_Of(Intg num, Intg type);
Intg Tym_Def_Pointer_To(Intg type);
Intg Tym_Def_Alias_Of(Name *nm, Intg type);
int Tym_Compare_Types(Intg type1, Intg type2, int *need_expansion);
Intg Tym_Type_Resolve(Intg type, int not_alias, int not_species);
#define Tym_Type_Resolve_Alias(type) Tym_Type_Resolve(type, 0, 1)
#define Tym_Type_Resolve_Species(type) Tym_Type_Resolve(type, 1, 0)
#define Tym_Type_Resolve_All(type) Tym_Type_Resolve(type, 0, 0)
Intg Tym_Def_Procedure(Intg proc, int second, Intg of_type, Intg sym_num);
Intg Tym_Search_Procedure(Intg proc, int second, Intg of_type,
                          Intg *containing_species);
void Tym_Print_Procedure(FILE *stream, Intg of_type);
Task Tym_Def_Specie(Intg *specie, Intg type);
Task Tym_Def_Structure(Intg *strc, Intg type);
Task Tym_Structure_Get(Intg *type);
Task Tym_Specie_Get(Intg *type);
Int Tym_Specie_Get_Target(Int type);

/*#define Tym_Def_Explicit(new_type, nm) \
  Tym_Def_Type(new_type, TYPE_NONE, nm, (Intg) 0, TYPE_NONE)*/
#define Tym_Def_Explicit_Alias(new_type, nm, type) \
  Tym_Def_Type(new_type, TYPE_NONE, nm, (Intg) -1, type)
#define Tym_Def_Intrinsic(new_type, nm, size) \
  Tym_Def_Type(new_type, TYPE_NONE, nm, size, TYPE_NONE)
#  endif

#endif
