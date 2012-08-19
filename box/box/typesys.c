/****************************************************************************
 * Copyright (C) 2008-2011 by Matteo Franchin                               *
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

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "defaults.h"
#include "types.h"
#include "messages.h"
#include "mem.h"
#include "strutils.h"
#include "array.h"
#include "occupation.h"
#include "hashtable.h"
#include "vm_private.h"
#include "typesys.h"

#include <box/core.h>
#include <box/obj.h>
#include <box/callable.h>
#include <box/combs.h>


static void Destroy_TSDesc(void *td) {
  BoxSPtr_Unlink(((TSDesc *) td)->new_type);
  BoxMem_Free(((TSDesc *) td)->name);
  ((TSDesc *) td)->name = 0;
}

void TS_Init_Builtin_Types(TS *ts) {
  static struct {
    const char *name;
    BoxType    expected;
    size_t     size,
               alignment;

  } *bt, builtin_types[] = {
    { "CHAR",    BOXTYPE_CHAR, sizeof(BoxChar),  __alignof__(BoxChar)},
    {  "INT",     BOXTYPE_INT, sizeof(BoxInt),   __alignof__(BoxInt)},
    { "REAL",    BOXTYPE_REAL, sizeof(BoxReal),  __alignof__(BoxReal)},
    {"POINT",   BOXTYPE_POINT, sizeof(BoxPoint), __alignof__(BoxPoint)},
    {  "PTR",     BOXTYPE_PTR, sizeof(BoxPtr),   __alignof__(BoxPtr)},
    {  "OBJ",     BOXTYPE_OBJ, sizeof(BoxPtr),   __alignof__(BoxPtr)},
    { "VOID",    BOXTYPE_VOID, 0, 0},
    { "(.[)",  BOXTYPE_CREATE, 0, 0},
    { "(].)", BOXTYPE_DESTROY, 0, 0},
    {  "(=)",    BOXTYPE_COPY, 0, 0},
    {  "([)",   BOXTYPE_BEGIN, 0, 0},
    {  "(])",     BOXTYPE_END, 0, 0},
    {  "(;)",   BOXTYPE_PAUSE, 0, 0},
    { "CPTR",    BOXTYPE_CPTR, sizeof(BoxCPtr), __alignof__(BoxCPtr)},
    {NULL, 0, 0, 0}
  };

  if (BoxOcc_Max_Index( & ts->type_descs) != 0) {
    MSG_FATAL("Cannot setup intrinsic types: type system already used!");

  } else {
    for(bt = & builtin_types[0]; bt->name != NULL; bt++) {
      BoxType type =
        BoxTS_New_Intrinsic_With_Name(ts, bt->size, bt->alignment, bt->name);
      assert(type == bt->expected);
    }
  }
}

static BoxException *My_Raise_Not_Implemented(BoxPtr *parent) {
  return BoxException_Create();
}

static BoxCallable *My_Create_Not_Implemented_Callable(void) {
  static BoxCallable *cb = NULL;

  if (!cb) {
    BoxXXXX *void_type = BoxType_Create_Ident(BoxType_Create_Intrinsic(0, 0),
                                              "Void");
    cb = BoxCallable_Create_From_CCall1(void_type, void_type,
                                        My_Raise_Not_Implemented);
    BoxSPtr_Unlink(void_type);
  }

  return BoxSPtr_Link(cb);
}

void TS_Init(TS *ts) {
  /* Initialize the new type system. */
  Box_Initialize_Type_System();

  BoxOcc_Init(& ts->type_descs, sizeof(TSDesc), TS_TSDESC_CLC_SIZE);
  BoxOcc_Set_Finalizer(& ts->type_descs, Destroy_TSDesc);
  BoxHT_Init_Default(& ts->members,  TS_MEMB_HT_SIZE);
  BoxHT_Init_Default(& ts->subtypes, TS_SUBT_HT_SIZE);
  BoxArr_Init(& ts->name_buffer, sizeof(char), TS_NAME_BUFFER_SIZE);
}

extern int num_type_nodes;
extern size_t total_size_of_types;

void TS_Finish(TS *ts) {
  BoxOcc_Finish(& ts->type_descs);
  BoxHT_Finish(& ts->members);
  BoxHT_Finish(& ts->subtypes);
  BoxArr_Finish(& ts->name_buffer);

  Box_Finalize_Type_System();
#if 0
  printf("Number of type nodes allocated: %d\n", num_type_nodes);
  printf("Total size of alloaction for types: %d\n", (int) total_size_of_types);
#endif
}

static void Type_New(TS *ts, Type *new_type, TSDesc *td) {
  TSDesc my_dummy_td,
         *my_td = (td != NULL) ? td : & my_dummy_td;
  UInt nt = BoxOcc_Occupy(& ts->type_descs, my_td);
  *new_type = nt - 1;
}

static TSDesc *Type_Ptr(TS *ts, Type t) {
  TSDesc *td = (TSDesc *) BoxOcc_Item_Ptr(& ts->type_descs, t + 1);
  assert(td != NULL);
  return td;
}

void TS_Set_New_Style_Type(BoxTS *ts, BoxType old_type, BoxXXXX *new_type) {
  if (old_type != BOXTYPE_NONE) {
    TSDesc *old_type_td = Type_Ptr(ts, old_type);
    old_type_td->new_type = new_type;
  }
}

BoxXXXX *TS_Get_New_Style_Type(BoxTS *ts, BoxType old_type) {
  if (old_type != BOXTYPE_NONE) {
    TSDesc *old_type_td = Type_Ptr(ts, old_type);
    return old_type_td->new_type;
  }
  return NULL;
}

BoxXXXX *BoxType_From_Id(BoxTS *ts, BoxTypeId id) {
  if (id != BOXTYPE_NONE) {
    TSDesc *old_type_td = Type_Ptr(ts, id);
    return old_type_td->new_type;
  }
  return NULL;
}

/*static BoxType My_New_Type(BoxTS *ts, TSDesc *td)*/

static TSDesc *Resolve(TS *ts, BoxType *rt, BoxType t, int ignore_names) {
  while(1) {
    TSDesc *td = Type_Ptr(ts, t);
    switch(td->kind) {
    case TS_KIND_ALIAS:
    case TS_KIND_MEMBER:
      t = td->target;
      if (ignore_names || td->name == NULL)
        break;

    default:
      if (rt != NULL)
        *rt = t;
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

BoxType TS_Is_Special(BoxType t) {
  switch(t) {
  case BOXTYPE_CREATE:
  case BOXTYPE_DESTROY:
  case BOXTYPE_BEGIN:
  case BOXTYPE_END:
  case BOXTYPE_PAUSE:
    return t;
  default:
    return BOXTYPE_NONE;
  }
}

BoxType BoxTS_Obsolete_Resolve_Once(BoxTS *ts, BoxType t,
                                    TSKindSelect select) {
  int resolve, resolve_only_anonimous, is_not_anonimous;
  TSDesc *td;
  Type rt;

  if (t == BOXTYPE_NONE)
    return BOXTYPE_NONE;

  td = Type_Ptr(ts, t);

  rt = td->target;
  switch(td->kind) {
  case TS_KIND_MEMBER:
    return rt;
  case TS_KIND_ALIAS:
    resolve = ((select & TS_KS_ALIAS) != 0); break;
  case TS_KIND_RAISED:
    resolve = ((select & TS_KS_RAISED) != 0); break;
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
  is_not_anonimous = (td->name != NULL);
  if (resolve_only_anonimous && is_not_anonimous)
    return t;
  return resolve ? rt : t;
}

int BoxTS_Resolve_Once(BoxTS *ts, BoxType *t, TSKindSelect ks) {
  BoxType old_t = *t;
  *t = BoxTS_Obsolete_Resolve_Once(ts, old_t, ks);
  return (*t != old_t);
}

BoxType TS_Resolve(BoxTS *ts, BoxType t, TSKindSelect select) {
  BoxType rt = t;
  do {
    t = rt;
    rt = BoxTS_Obsolete_Resolve_Once(ts, t, select);
  } while(rt != t);
  return t;
}

Type TS_Get_Core_Type(TS *ts, Type t) {
  return TS_Resolve(ts, t, TS_KS_ALIAS | TS_KS_RAISED | TS_KS_SPECIES);
}

BoxType TS_Get_Cont_Type(TS *ts, BoxType t) {
  BoxType r =
    TS_Resolve(ts, t, TS_KS_ALIAS | TS_KS_RAISED | TS_KS_SPECIES);

  if (TS_Is_Empty(ts, r))
    return BOXTYPE_VOID;

  else
    return (r > BOXTYPE_PTR) ? BOXTYPE_OBJ : r;
}

int TS_Is_Fast(TS *ts, Type t) {
  Type ct = TS_Get_Core_Type(ts, t);
  return (ct >= BOXTYPE_FAST_LOWER && ct <= BOXTYPE_FAST_UPPER);
}

Int TS_Get_Size(TS *ts, Type t) {
#if 0
  TSDesc *td = Type_Ptr(ts, t);

  if (!t_new)
    MSG_ERROR("New style type missing for %s", TS_Name_Get(ts, t));
  else if (BoxType_Get_Size(t_new) != td->size)
    MSG_ERROR("Type size mismatch for %s (%d, %d)",
              TS_Name_Get(ts, t), td->size, BoxType_Get_Size(t_new));
#endif

  BoxXXXX *t_new = TS_Get_New_Style_Type(ts, t);
  return BoxType_Get_Size(t_new);
}

TSKind TS_Get_Kind(TS *ts, Type t) {
  TSDesc *td = Type_Ptr(ts, t);
  return td->kind;
}

static Task TS_Name_Set(TS *ts, Type t, const char *name) {
  TSDesc *td = Type_Ptr(ts, t);
  if (td->name != (char *) NULL) {
    MSG_ERROR("TS_Name_Set: trying to set the name '%s' for type %I: "
              "this type has already been given the name '%s'!",
              name, t, td->name);
    return BOXTASK_FAILURE;
  }
  td->name = BoxMem_Strdup(name);
  return BOXTASK_OK;
}

static char *TS_Name_Get_Case(TSKind kind, TS *ts, TSDesc *td, Type t,
                              char *empty, char *one, char *two, char *many) {
  char *name = (char *) NULL;
  Type m = td->target;
  Type previous_type = BOXTYPE_NONE;

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

  case TS_KIND_RAISED:
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
    return printdup("%~s@%~s", TS_Name_Get(ts, td->target),
                    TS_Name_Get(ts, td->data.proc.parent));

  case TS_KIND_SUBTYPE:
    return printdup("%~s.%s",
                    TS_Name_Get(ts, td->data.subtype.parent),
                    td->data.subtype.child_name);
  default:
    return BoxMem_Strdup("<unknown type>");
  }
}

/* The full name to use in the hashtable of members */
static void Member_Full_Name(TS *ts, BoxName *n, Type s, const char *m_name) {
  BoxArr_Clear(& ts->name_buffer);
  BoxArr_MPush(& ts->name_buffer, & s, sizeof(Type));
  BoxArr_MPush(& ts->name_buffer, m_name, strlen(m_name));
  n->text = (char *) BoxArr_First_Item_Ptr(& ts->name_buffer);
  n->length = BoxArr_Num_Items(& ts->name_buffer);
}

Type My_Find_Struct_Member(BoxTS *ts, BoxType s, const char *m_name) {
  BoxName n;
  BoxHTItem *hi;
  s = TS_Resolve(ts, s, TS_KS_ALIAS | TS_KS_SPECIES | TS_KS_RAISED);
  Member_Full_Name(ts, & n, s, m_name);
  if (BoxHT_Find(& ts->members, n.text, n.length, & hi))
    return *((Type *) hi->object);
  else
    return BOXTYPE_NONE;
}

BoxType BoxTS_Find_Struct_Member(BoxTS *ts, BoxType s, const char *m_name) {
  BoxType result_old = My_Find_Struct_Member(ts, s, m_name);
  BoxXXXX *s_new = TS_Get_New_Style_Type(ts, s);
  assert(s_new);
  BoxXXXX *result_new =
    BoxType_Find_Structure_Member(BoxType_Get_Stem(s_new), m_name);

  if ((!result_new) != (result_old == BOXTYPE_NONE)) {
    MSG_ERROR("Structure find mismatch for '%s' in '%s'",
              m_name, TS_Name_Get(ts, s));

  } else if (result_new) {
    BoxXXXX *rr_new;
    BoxBool x =
      BoxType_Get_Structure_Member(result_new, NULL, NULL, NULL, & rr_new);
    assert(x);
    BoxType rr_old = BoxTS_Obsolete_Resolve_Once(ts, result_old, 0);
    assert(TS_Get_New_Style_Type(ts, rr_old) == rr_new);
  }

  return result_old;
}


BoxType BoxTS_Get_Struct_Member(BoxTS *ts, BoxType m, size_t *address) {
  TSDesc *m_td = Type_Ptr(ts, m);
  assert(m_td->kind == TS_KIND_MEMBER);
  if (address != NULL)
    *address = m_td->size;
  return m_td->target;
}

const char *BoxTS_Get_Struct_Member_Name(BoxTS *ts, BoxType member) {
  TSDesc *m_td = Type_Ptr(ts, member);
  assert(m_td->kind == TS_KIND_MEMBER);
  return m_td->name;
}

BoxType BoxTS_Get_Next_Struct_Member(BoxTS *ts, BoxType m) {
  TSDesc *td = Type_Ptr(ts, m);
  return td->kind == TS_KIND_MEMBER ? td->data.member_next : td->target;
}

/****************************************************************************
 * Here we define functions which have almost common bodies.                *
 * This is done in a tricky way (look at the documentation inside           *
 * the included file!)                                                      *
 ****************************************************************************/

/*FUNCTIONS: TS_X_New *******************************************************/
static BoxType BoxTS_New_Intrinsic(BoxTS *ts, size_t size, size_t alignment) {
  TSDesc td;
  BoxType new_type;
  assert(size >= 0);
  TS_TSDESC_INIT(& td);
  td.kind = TS_KIND_INTRINSIC;
  td.size = size;
  td.alignment = alignment;
  td.target = BOXTYPE_NONE;
  Type_New(ts, & new_type, & td);
  return new_type;
}

BoxType BoxTS_New_Intrinsic_With_Name(BoxTS *ts, size_t size,
                                      size_t alignment, const char *name) {
  BoxType out_old = BoxTS_New_Intrinsic(ts, size, alignment);
  TS_Name_Set(ts, out_old, name);

  BoxXXXX *prim_new =
    BoxType_Create_Primary((BoxTypeId) out_old, size, alignment);

  if (prim_new)
    TS_Set_New_Style_Type(ts, out_old, BoxType_Create_Ident(prim_new, name));

  return out_old;
}

static BoxType BoxTS_Procedure_New(BoxTS *ts, BoxType child,
                                   BoxComb comb, BoxType parent) {
  TSDesc td;
  BoxType p;
  TS_TSDESC_INIT(& td);
  td.kind = TS_KIND_PROC;
  td.size = BOX_SIZE_UNKNOWN;
  td.target = child;
  td.data.proc.parent = parent;
  td.data.proc.combine = comb;
  td.data.proc.sym_num = 0;
  Type_New(ts, & p, & td);
  return p;
}

/*FUNCTIONS: My_New **********************************************************/

/* Common code for BoxTs_New_Alias, BoxTS_New_Raised and BoxTS_New_Array. */
static BoxType My_New(TSKind kind, BoxTS *ts, BoxType src, BoxInt size) {
  TSDesc td;
  TSDesc *src_td = Type_Ptr(ts, src);
  BoxType t_ret;

  TS_TSDESC_INIT(& td);
  td.kind = kind;
  td.target = src;
  if (kind == TS_KIND_ARRAY) {
    td.size = size < 0 ? BOX_SIZE_UNKNOWN : size*src_td->size;
    td.alignment = src_td->alignment;
    td.data.array_size = size;
  } else {
    td.size = src_td->size;
    td.alignment = src_td->alignment;
  }
  Type_New(ts, & t_ret, & td);
  return t_ret;
}

BoxType BoxTS_New_Alias_With_Name(BoxTS *ts, BoxType origin_old,
                                  const char *name) {
  BoxType out_old = My_New(TS_KIND_ALIAS, ts, origin_old, -1);
  TS_Name_Set(ts, out_old, name);

  BoxXXXX *origin_new = TS_Get_New_Style_Type(ts, origin_old);
  if (origin_new)
    TS_Set_New_Style_Type(ts, out_old,
                          BoxType_Create_Ident(BoxType_Link(origin_new), name));
  return out_old;
}

BoxType BoxTS_New_Raised(BoxTS *ts, BoxType origin_old) {
  BoxType out_old = My_New(TS_KIND_RAISED, ts, origin_old, -1);
  BoxXXXX *origin_new = TS_Get_New_Style_Type(ts, origin_old);
  if (origin_new)
    TS_Set_New_Style_Type(ts, out_old,
                          BoxType_Create_Raised(BoxType_Link(origin_new)));
  return out_old;
}

BoxType BoxTS_Get_Raised(BoxTS *ts, BoxType t) {
  BoxType resolved_t = TS_Resolve(ts, t, TS_KS_ALIAS | TS_KS_SPECIES);
  TSDesc *resolved_td = Type_Ptr(ts, resolved_t);
  if (resolved_td->kind == TS_KIND_RAISED)
    return resolved_td->target;

  else
    return BOXTYPE_NONE;
}

/*FUNCTIONS: My_Begin_Composite **********************************************/

/* Code for BoxTS_Begin_Struct, BoxTS_Begin_Species, etc. */
static BoxType My_Begin_Composite(TSKind kind, TS *ts) {
  BoxType t;
  TSDesc td;
  TS_TSDESC_INIT(& td);
  td.kind = kind;
  td.target = BOXTYPE_NONE;
  td.data.last = BOXTYPE_NONE;
  td.size = 0;
  td.alignment = 0;
  Type_New(ts, & t, & td);
  return t;
}

BoxType BoxTS_Begin_Struct(BoxTS *ts) {
  BoxType out_old = My_Begin_Composite(TS_KIND_STRUCTURE, ts);
  TS_Set_New_Style_Type(ts, out_old, BoxType_Create_Structure());
  return out_old;
}

BoxType BoxTS_Begin_Species(BoxTS *ts) {
  BoxType out_old = My_Begin_Composite(TS_KIND_SPECIES, ts);
  TS_Set_New_Style_Type(ts, out_old, BoxType_Create_Species());
  return out_old;
}

/*FUNCTIONS: My_Add_Member ***************************************************/

/* Code for BoxTS_Add_Struct_Member, BoxTS_Add_Species_Member, etc. */
static void My_Add_Member(TSKind kind, BoxTS *ts, BoxType s, BoxType m,
                          const char *m_name) {

  BoxType new_m;
  TSDesc *m_td, *s_td, *new_m_td;

  /* First we add the new descriptor, then we populate it
   * (otherwise we may incur in realloc problems with the descriptor pointers
   * having to be updated).
   */
  Type_New(ts, & new_m, NULL);
  new_m_td = Type_Ptr(ts, new_m);
  s_td = Type_Ptr(ts, s);
  m_td = Type_Ptr(ts, m);

  /* Check that what we are doing makes sense (avoiding adding struct members
   * to enums, etc.).
   */
  assert(s_td->kind == kind);

  /* Create a member descriptor which is linked to the member type */
  TS_TSDESC_INIT(new_m_td);
  new_m_td->kind = TS_KIND_MEMBER;
  new_m_td->target = m;
  new_m_td->data.member_next = s;
  new_m_td->alignment = m_td->alignment;

  /* Alignment is always the maximum of the alignments of the members
   * (and this holds for structures, species and enums).
   */
  if (m_td->alignment > s_td->alignment)
    s_td->alignment = m_td->alignment;

  if (kind == TS_KIND_STRUCTURE) {
    /* Need to do this small computation to retrieve the structure size without
     * padding (as s_td->size is the actual structure size, with padding).
     */
    size_t s_size = 0;
    if (s_td->data.last != BOXTYPE_NONE) {
      TSDesc *prev_m_td = Type_Ptr(ts, s_td->data.last);
      assert(prev_m_td->kind == TS_KIND_MEMBER);
      s_size += prev_m_td->size + TS_Get_Size(ts, prev_m_td->target);
    }

    /* The one below is actually the 'offset' of the member in the structure,
     * not the 'size' (typesys.c needs to be rewritten another time, as it is
     * not very well designed, admittedly).
     */
    new_m_td->size = BoxMem_Align_Offset(s_size, m_td->alignment);

    if (m_name != NULL)
      new_m_td->name = BoxMem_Strdup(m_name);

  } else
    new_m_td->size = m_td->size;

  /* Link the last member descriptor of the structure/enum/species to the one
   * we just created.
   */
  if (s_td->data.last != BOXTYPE_NONE) {
    TSDesc *prev_m_td = Type_Ptr(ts, s_td->data.last);
    assert(prev_m_td->kind == TS_KIND_MEMBER);
    prev_m_td->data.member_next = new_m;
  }

  /* Update the structure/... to take account of the inserted member. */
  s_td->data.last = new_m;
  if (s_td->target == BOXTYPE_NONE)
    s_td->target = new_m;

  /* Compute the new structure/... size. */
  switch(kind) {
  case TS_KIND_STRUCTURE:
    s_td->size = BoxMem_Get_Multiple_Size(new_m_td->size + m_td->size,
                                          s_td->alignment);
    /* We also add the member to the hashtable for fast search */
    if (m_name != NULL) {
      BoxName n;
      Member_Full_Name(ts, & n, s, m_name);
      BoxHT_Insert_Obj(& ts->members, n.text, n.length,
                       & new_m, sizeof(Type));
    }
    break;

  case TS_KIND_ENUM:
    {
      /* We need to do soemthing about alignment, here (but first, we should
       * implement enums ;-)
       */
      size_t m_size = m_td->size + sizeof(Int);
      if (m_size > s_td->size)
        s_td->size = m_size;
      break;
    }

  case TS_KIND_SPECIES:
    s_td->size = m_td->size;
    break;

  default:
    if (m_td->size > s_td->size)
      s_td->size = m_td->size;
    break;
  }
}

void BoxTS_Add_Struct_Member(BoxTS *ts, BoxType structure, BoxType member_type,
                             const char *member_name) {
  My_Add_Member(TS_KIND_STRUCTURE, ts, structure, member_type, member_name);

  BoxXXXX *structure_new = TS_Get_New_Style_Type(ts, structure);
  if (structure_new) {
    BoxXXXX *member_new = TS_Get_New_Style_Type(ts, member_type);
    if (member_new)
      BoxType_Add_Member_To_Structure(structure_new, member_new, member_name);
  }
}

void BoxTS_Add_Species_Member(BoxTS *ts, BoxType species, BoxType member) {
  My_Add_Member(TS_KIND_SPECIES, ts, species, member, NULL);

  BoxXXXX *species_new = TS_Get_New_Style_Type(ts, species);
  if (species_new) {
    BoxXXXX *member_new = TS_Get_New_Style_Type(ts, member);
    if (member_new) {
      BoxType_Add_Member_To_Species(species_new, member_new);
      return;
    }
  }

  MSG_ERROR("Could not create species properly!");
}

BoxType BoxTS_Get_Species_Target(BoxTS *ts, BoxType species) {
  TSDesc *s_td = Type_Ptr(ts, species);
  return BoxTS_Obsolete_Resolve_Once(ts, s_td->data.last, 0);
}

/****************************************************************************/
/* Procedure registration and search */

/* Register the procedure.
 * The way we handle registration and search is very inefficient.
 * this could and should be improved, but we stick to the simple solution
 * for now!
 */
static void BoxTS_Procedure_Register(BoxTS *ts, BoxType p,
                                     BoxVMSymID sym_num) {
  TSDesc *proc_td, *parent_td;
  BoxType parent;
  proc_td = Type_Ptr(ts, p);
  assert(proc_td->kind == TS_KIND_PROC);
  parent = proc_td->data.proc.parent;
  parent_td = Type_Ptr(ts, parent);
  assert(proc_td->first_proc == BOXTYPE_NONE); /* Must not be registered! */
  proc_td->first_proc = parent_td->first_proc;
  parent_td->first_proc = p;
  proc_td->data.proc.sym_num = sym_num;
}

int BoxTS_Procedure_Is_Registered(BoxTS *ts, BoxComb comb, BoxType p) {
  TSDesc *proc_td, *parent_td;
  BoxType parent, child;

  proc_td = Type_Ptr(ts, p);
  assert(proc_td->kind == TS_KIND_PROC);

  /* Get parent */
  parent = proc_td->data.proc.parent;
  parent_td = Type_Ptr(ts, parent);

  /* Iter over procedure of parent and check whether the proc is there */
  for (child = parent_td->first_proc; child != BOXTYPE_NONE; ) {
    TSDesc *child_td = Type_Ptr(ts, child);
    if (child_td->data.proc.combine == comb && child == p)
      return 1;

    child = child_td->first_proc;
  }

  /* One may, at this point, do another iteration and try to use TS_Compare
   * just not to rely to match of types IDs. It is not clear at this point
   * if this will be ever required, so we postpone the implementation.
   */
  return 0;
}

void BoxTS_Procedure_Unregister(BoxTS *ts, BoxComb comb, BoxType p) {
  TSDesc *proc_td, *parent_td;
  BoxType parent, child,
          *prev_child_ptr;

  /* The following check has to stay there until we know that we don't
   * need to check for procedures with TS_Compare
   */
  if (!BoxTS_Procedure_Is_Registered(ts, comb, p)) {
    MSG_FATAL("BoxTS_Procedure_Unregister: procedure is not registered!");
    assert(0);
  }

  proc_td = Type_Ptr(ts, p);
  assert(proc_td->kind == TS_KIND_PROC);

  /* Get parent */
  parent = proc_td->data.proc.parent;
  parent_td = Type_Ptr(ts, parent);

  /* Iter over procedure of parent to find the proc in the chain */
  prev_child_ptr = & parent_td->first_proc;
  child = parent_td->first_proc;

  while (child != BOXTYPE_NONE) {
    TSDesc *child_td = Type_Ptr(ts, child);
    if (child_td->data.proc.combine == comb && child == p) {
      *prev_child_ptr = child_td->first_proc;
      return;
    }

    /* Go to next */
    prev_child_ptr = & child_td->first_proc;
    child = child_td->first_proc;
  }

  assert(0);
}

BoxVMSymID TS_Procedure_Get_Sym(TS *ts, Type p) {
  TSDesc *proc_td;
  proc_td = Type_Ptr(ts, p);
  assert(proc_td->kind == TS_KIND_PROC);
  return proc_td->data.proc.sym_num;
}

void BoxTS_Procedure_Info(BoxTS *ts, BoxType *parent, BoxType *child,
                          BoxComb *comb, BoxVMSymID *sym_num, BoxType p) {
  TSDesc *proc_td;
  proc_td = Type_Ptr(ts, p);
  assert(proc_td->kind == TS_KIND_PROC);
  if (parent != NULL)
    *parent = proc_td->data.proc.parent;
  if (child != NULL)
    *child = proc_td->target;
  if (comb != NULL)
    *comb = proc_td->data.proc.combine;
  if (sym_num != NULL)
    *sym_num = proc_td->data.proc.sym_num;
}

static BoxType My_Procedure_Search(BoxTS *ts, BoxType *expansion_type,
                                   BoxType child, BoxComb comb,
                                   BoxType parent) {
  TSDesc *p_td, *parent_td;
  Type p, dummy;
  if (expansion_type == NULL)
    expansion_type = & dummy;
  *expansion_type = BOXTYPE_NONE;
  parent_td = Type_Ptr(ts, parent);
  /*MSG_ADVICE("BoxTS_Procedure_Search: searching procedure");*/
  for (p = parent_td->first_proc; p != BOXTYPE_NONE; p = p_td->first_proc) {
    TSCmp comparison;
    p_td = Type_Ptr(ts, p);
    /*MSG_ADVICE("BoxTS_Procedure_Search: considering %~s",
                 TS_Name_Get(ts, p));*/
    /*MSG_ADVICE("BoxTS_Procedure_Search: '%s', comparing '%s' == '%s'",
               Tym_Type_Names(parent),
               Tym_Type_Names(p_td->target), Tym_Type_Names(child));*/
    if (p_td->data.proc.combine == comb) {
      comparison = TS_Compare(ts, p_td->target, child);
      if (comparison != TS_TYPES_UNMATCH) {
        if (comparison == TS_TYPES_EXPAND)
          *expansion_type = p_td->target;
        return p;
      }
    }
  }

  return BOXTYPE_NONE;
}

BoxType BoxTS_Procedure_Search(TS *ts, BoxType *expansion_type,
                               BoxType child, BoxComb comb, BoxType parent,
                               TSSearchMode mode) {
  BoxType previous_parent;
  int search_inherited = (mode & TSSEARCHMODE_INHERITED) != 0;
  BoxType proc;

  do {
    proc = My_Procedure_Search(ts, expansion_type, child, comb, parent);
    if (proc != BOXTYPE_NONE)
      break;

    /* Resolve the parent type and retry */
    previous_parent = parent;
    parent =
      BoxTS_Obsolete_Resolve_Once(ts, parent,
                                  TS_KS_ALIAS | TS_KS_RAISED | TS_KS_SPECIES);
  } while (parent != previous_parent && search_inherited);

  return proc;
}

/****************************************************************************/


/* Create a new unregistered subtype: a subtype is unregistered when
 * the parent is not aware of its existance. An unregistered type is defined
 * only giving its name and its parent type. When the subtype is registered
 * the parent type becomes aware of it. In order for the registration to be
 * completed the full type of the subtype must be specified.
 */
BoxType TS_Subtype_New(TS *ts, Type parent_old, const char *child_name) {
  TSDesc td;
  BoxType subtype_old;
  BoxXXXX *parent_new;
  BoxXXXX *subtype_new;

  TS_TSDESC_INIT(& td);
  td.kind = TS_KIND_SUBTYPE;
  td.size = BOX_SIZE_UNKNOWN;
  td.target = BOXTYPE_NONE;
  td.data.subtype.parent = parent_old;
  td.data.subtype.child_name = strdup(child_name);
  Type_New(ts, & subtype_old, & td);

  parent_new = TS_Get_New_Style_Type(ts, parent_old);
  assert(parent_new);
  subtype_new = BoxType_Create_Subtype(parent_new, child_name, NULL);
  assert(subtype_new);
  TS_Set_New_Style_Type(ts, subtype_old, BoxType_Link(subtype_new));
  return subtype_old;
}

int TS_Subtype_Is_Registered(TS *ts, Type st) {
  TSDesc *st_td = Type_Ptr(ts, st);
  assert(st_td->kind == TS_KIND_SUBTYPE);
  return (st_td->target != BOXTYPE_NONE);
}

BoxType TS_Subtype_Get_Parent(TS *ts, Type st_old) {
  TSDesc *st_td = Type_Ptr(ts, st_old);
  BoxXXXX *st_new = TS_Get_New_Style_Type(ts, st_old);
  BoxXXXX *parent_new;

  assert(st_new);
  assert(st_td->kind == TS_KIND_SUBTYPE);

  BoxType_Get_Subtype_Info(st_new, NULL, & parent_new, NULL);
  assert(parent_new == TS_Get_New_Style_Type(ts, st_td->data.subtype.parent));

  return st_td->data.subtype.parent;
}

BoxType TS_Subtype_Get_Child(TS *ts, Type st_old) {
  TSDesc *st_td = Type_Ptr(ts, st_old);
  BoxXXXX *st_new = TS_Get_New_Style_Type(ts, st_old);
  BoxXXXX *child_new;

  assert(st_new);
  assert(st_td->kind == TS_KIND_SUBTYPE);

  BoxType_Get_Subtype_Info(st_new, NULL, NULL, & child_new);
  assert(child_new == TS_Get_New_Style_Type(ts, st_td->target));
  return st_td->target;
}

/* Register a previously created (and still unregistered) subtype. */
Task TS_Subtype_Register(TS *ts, Type subtype, Type subtype_type) {
  TSDesc *s_td = Type_Ptr(ts, subtype);
  Type parent, found_subtype;
  BoxName full_name;
  char *child_str;

  BoxXXXX *subtype_new = TS_Get_New_Style_Type(ts, subtype);
  BoxXXXX *subtype_type_new = TS_Get_New_Style_Type(ts, subtype_type);
  BoxBool result_new;

  if (s_td->target != BOXTYPE_NONE) {
    MSG_ERROR("Cannot redefine subtype '%~s'", TS_Name_Get(ts, subtype));
    return BOXTASK_FAILURE;
  }

  child_str = s_td->data.subtype.child_name;
  parent = s_td->data.subtype.parent;
  found_subtype = TS_Subtype_Find(ts, parent, child_str);
  if (found_subtype != BOXTYPE_NONE) {
    TSDesc *found_subtype_td = Type_Ptr(ts, found_subtype);
    Type found_subtype_type = found_subtype_td->target;
    TSCmp comparison = TS_Compare(ts, found_subtype_type, subtype_type);
    if ((comparison & TS_TYPES_MATCH) == 0) {
      MSG_ERROR("Cannot redefine subtype '%~s'", TS_Name_Get(ts, subtype));
      return BOXTASK_FAILURE;
    }
    return BOXTASK_OK;
  }

  /* CHECK: MAYBE WE SHOULD RESOLVE THE TYPE HERE */
  s_td->target = subtype_type;
  s_td->size = sizeof(Subtype);
  Member_Full_Name(ts, & full_name, parent, child_str);
  BoxHT_Insert_Obj(& ts->subtypes,
                   full_name.text, full_name.length,
                   & subtype, sizeof(Type));

  assert(subtype_new && subtype_type_new);
  result_new = BoxType_Register_Subtype(subtype_new, subtype_type_new);
  assert(result_new);

  return BOXTASK_OK;
}

BoxType TS_Subtype_Find(TS *ts, BoxType parent, const char *name) {
  BoxName full_name;
  BoxHTItem *hi;

  do {
    Member_Full_Name(ts, & full_name, parent, name);
    if (BoxHT_Find(& ts->subtypes, full_name.text, full_name.length, & hi))
      return *((BoxType *) hi->object);

  } while (BoxTS_Resolve_Once(ts, & parent, TS_KS_ALIAS | TS_KS_SPECIES));

  return BOXTYPE_NONE;
}

/****************************************************************************/

Task TS_Default_Value(TS *ts, Type *dv_t, Type t, BoxData *dv) {
  MSG_ERROR("Still not implemented!"); return BOXTASK_FAILURE;
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

    case TS_KIND_RAISED:
      return TS_TYPES_UNMATCH;

    case TS_KIND_SPECIES:
      {
        Type m = t1;
        while (1) {
          m = BoxTS_Get_Next_Struct_Member(ts, m);
          if (m == t1) return TS_TYPES_UNMATCH;
          if (TS_Compare(ts, m, t2) != TS_TYPES_UNMATCH) {
            if (BoxTS_Get_Next_Struct_Member(ts, m) == t1) return cmp;
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
          m1 = BoxTS_Get_Next_Struct_Member(ts, m1);
          m2 = BoxTS_Get_Next_Struct_Member(ts, m2);
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
      if (td1->data.array_size != BOX_SIZE_UNKNOWN)
        return TS_TYPES_UNMATCH;
      cmp &= TS_TYPES_MATCH;
      break;

    case TS_KIND_PROC:
      if (td1->data.proc.combine != td2->data.proc.combine)
        return TS_TYPES_UNMATCH;
      
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

void BoxTSStrucIt_Init(BoxTS *ts, BoxTSStrucIt *it, BoxType s) {
  BoxType s_core = TS_Get_Core_Type(ts, s);
  TSDesc *s_core_td = Type_Ptr(ts, s_core);
  BoxType member = s_core_td->target;
  TSDesc *member_td = Type_Ptr(ts, member);

  if (member_td->kind == TS_KIND_MEMBER) {
    it->ts = ts;
    it->position = 0;
    it->td = member_td;
    it->member = BoxTS_Obsolete_Resolve_Once(ts, member, TS_KS_NONE);
    it->has_more = 1;

  } else {
    it->has_more = 0;
  }
}

void BoxTSStrucIt_Advance(BoxTSStrucIt *it) {
  if (it->td->kind == TS_KIND_MEMBER && it->has_more) {
    BoxType next_member = it->td->data.member_next;
    TSDesc *next_member_td = Type_Ptr(it->ts, next_member);
    it->position = next_member_td->size;
    it->member = BoxTS_Obsolete_Resolve_Once(it->ts, next_member, TS_KS_NONE);
    it->td = next_member_td;
    it->has_more = (next_member_td->kind == TS_KIND_MEMBER);
  }
}

BoxType BoxTS_Procedure_Define(BoxTS *ts, BoxType child_old, BoxComb comb,
                               BoxType parent_old, BoxVMSymID sym_id) {
  BoxXXXX *child_new = TS_Get_New_Style_Type(ts, child_old);
  BoxXXXX *parent_new = TS_Get_New_Style_Type(ts, parent_old);
  BoxCallable *callable;
  BoxCombType comb_type_new;

  switch (comb) {
  case BOXCOMB_CHILDOF: comb_type_new = BOXCOMBTYPE_AT; break;
  case BOXCOMB_COPYTO: comb_type_new = BOXCOMBTYPE_COPY; break;
  case BOXCOMB_MOVETO:
  default:
    assert(0);
  }

  BoxType p = BoxTS_Procedure_New(ts, child_old, comb, parent_old);
  BoxTS_Procedure_Register(ts, p, sym_id);

  assert(child_new && parent_new);
  callable = My_Create_Not_Implemented_Callable();
  BoxBool result = BoxType_Define_Combination(parent_new, comb_type_new,
                                              child_new, callable);
  assert(result);

  return p;
}
