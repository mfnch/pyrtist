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
    BoxTypeId    expected;
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
      BoxTypeId type =
        BoxTS_New_Intrinsic_With_Name(ts, bt->size, bt->alignment, bt->name);
      assert(type == bt->expected);
    }
  }
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

static void Type_New(TS *ts, BoxTypeId *new_type, TSDesc *td) {
  TSDesc my_dummy_td,
         *my_td = (td != NULL) ? td : & my_dummy_td;
  UInt nt = BoxOcc_Occupy(& ts->type_descs, my_td);
  *new_type = nt - 1;
}

static TSDesc *Type_Ptr(TS *ts, BoxTypeId t) {
  TSDesc *td = (TSDesc *) BoxOcc_Item_Ptr(& ts->type_descs, t + 1);
  assert(td != NULL);
  return td;
}

void TS_Set_New_Style_Type(BoxTS *ts, BoxTypeId old_type, BoxType *new_type) {
  if (old_type != BOXTYPE_NONE) {
    TSDesc *old_type_td = Type_Ptr(ts, old_type);
    old_type_td->new_type = new_type;
    BoxType_Set_Id(new_type, old_type);
  }
}

BoxType *TS_Get_New_Style_Type(BoxTS *ts, BoxTypeId old_type) {
  if (old_type != BOXTYPE_NONE) {
    TSDesc *old_type_td = Type_Ptr(ts, old_type);
    return old_type_td->new_type;
  }
  return NULL;
}

BoxType *BoxType_From_Id(BoxTS *ts, BoxTypeId id) {
  if ((BoxTypeId) id != BOXTYPE_NONE) {
    TSDesc *old_type_td = Type_Ptr(ts, id);
    return old_type_td->new_type;
  }
  return NULL;
}

/*static BoxTypeId My_New_Type(BoxTS *ts, TSDesc *td)*/

static TSDesc *Resolve(TS *ts, BoxTypeId *rt, BoxTypeId t, int ignore_names) {
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

BoxTypeId TS_Is_Special(BoxTypeId t) {
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

BoxTypeId BoxTS_Obsolete_Resolve_Once(BoxTS *ts, BoxTypeId t,
                                    TSKindSelect select) {
  int resolve, resolve_only_anonimous, is_not_anonimous;
  TSDesc *td;
  BoxTypeId rt;

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

int BoxTS_Resolve_Once(BoxTS *ts, BoxTypeId *t, TSKindSelect ks) {
  BoxTypeId old_t = *t;
  *t = BoxTS_Obsolete_Resolve_Once(ts, old_t, ks);
  return (*t != old_t);
}

BoxTypeId TS_Resolve(BoxTS *ts, BoxTypeId t, TSKindSelect select) {
  BoxTypeId rt = t;
  do {
    t = rt;
    rt = BoxTS_Obsolete_Resolve_Once(ts, t, select);
  } while(rt != t);
  return t;
}

BoxTypeId TS_Get_Core_Type(TS *ts, BoxTypeId t) {
  return TS_Resolve(ts, t, TS_KS_ALIAS | TS_KS_RAISED | TS_KS_SPECIES);
}

BoxTypeId TS_Get_Cont_Type(TS *ts, BoxTypeId t) {
  BoxTypeId r =
    TS_Resolve(ts, t, TS_KS_ALIAS | TS_KS_RAISED | TS_KS_SPECIES);

  if (TS_Is_Empty(ts, r))
    return BOXTYPE_VOID;

  else
    return (r > BOXTYPE_PTR) ? BOXTYPE_OBJ : r;
}

int TS_Is_Fast(TS *ts, BoxTypeId t) {
  BoxTypeId ct = TS_Get_Core_Type(ts, t);
  return (ct >= BOXTYPE_FAST_LOWER && ct <= BOXTYPE_FAST_UPPER);
}

Int TS_Get_Size(TS *ts, BoxTypeId t) {
#if 0
  TSDesc *td = Type_Ptr(ts, t);

  if (!t_new)
    MSG_ERROR("New style type missing for %s", TS_Name_Get(ts, t));
  else if (BoxType_Get_Size(t_new) != td->size)
    MSG_ERROR("Type size mismatch for %s (%d, %d)",
              TS_Name_Get(ts, t), td->size, BoxType_Get_Size(t_new));
#endif

  BoxType *t_new = TS_Get_New_Style_Type(ts, t);
  return BoxType_Get_Size(t_new);
}


TSKind TS_Get_Kind(TS *ts, BoxTypeId t) {
  TSDesc *td = Type_Ptr(ts, t);
  return td->kind;
}

static char *TS_Name_Get_Case(TSKind kind, TS *ts, TSDesc *td, BoxTypeId t,
                              char *empty, char *one, char *two, char *many) {
  char *name = (char *) NULL;
  BoxTypeId m = td->target;
  BoxTypeId previous_type = BOXTYPE_NONE;

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

char *TS_Name_Get(TS *ts, BoxTypeId t) {
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
static void
Member_Full_Name(TS *ts, BoxName *n, BoxTypeId s, const char *m_name) {
  BoxArr_Clear(& ts->name_buffer);
  BoxArr_MPush(& ts->name_buffer, & s, sizeof(BoxTypeId));
  BoxArr_MPush(& ts->name_buffer, m_name, strlen(m_name));
  n->text = (char *) BoxArr_First_Item_Ptr(& ts->name_buffer);
  n->length = BoxArr_Num_Items(& ts->name_buffer);
}

/****************************************************************************
 * Here we define functions which have almost common bodies.                *
 * This is done in a tricky way (look at the documentation inside           *
 * the included file!)                                                      *
 ****************************************************************************/

/*FUNCTIONS: TS_X_New *******************************************************/
static BoxTypeId BoxTS_New_Intrinsic(BoxTS *ts, size_t size, size_t alignment) {
  TSDesc td;
  BoxTypeId new_type;
  assert(size >= 0);
  TS_TSDESC_INIT(& td);
  td.kind = TS_KIND_INTRINSIC;
  td.size = size;
  td.alignment = alignment;
  td.target = BOXTYPE_NONE;
  Type_New(ts, & new_type, & td);
  return new_type;
}

static Task My_Name_Set(TS *ts, BoxTypeId t, const char *name) {
  TSDesc *td = Type_Ptr(ts, t);
  if (td->name != (char *) NULL) {
    MSG_ERROR("My_Name_Set: trying to set the name '%s' for type %I: "
              "this type has already been given the name '%s'!",
              name, t, td->name);
    return BOXTASK_FAILURE;
  }
  td->name = BoxMem_Strdup(name);
  return BOXTASK_OK;
}

BoxTypeId BoxTS_New_Intrinsic_With_Name(BoxTS *ts, size_t size,
                                        size_t alignment, const char *name) {
  BoxTypeId out_old = BoxTS_New_Intrinsic(ts, size, alignment);
  My_Name_Set(ts, out_old, name);

  BoxType *prim_new =
    BoxType_Create_Primary((BoxTypeId) out_old, size, alignment);

  if (prim_new)
    TS_Set_New_Style_Type(ts, out_old, BoxType_Create_Ident(prim_new, name));

  return out_old;
}

static BoxTypeId My_Procedure_New(BoxTS *ts, BoxTypeId child,
                                  BoxCombType comb, BoxTypeId parent) {
  TSDesc td;
  BoxTypeId p;
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
static BoxTypeId My_New(TSKind kind, BoxTS *ts, BoxTypeId src, BoxInt size) {
  TSDesc td;
  TSDesc *src_td = Type_Ptr(ts, src);
  BoxTypeId t_ret;

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

BoxTypeId BoxTS_New_Alias_With_Name(BoxTS *ts, BoxTypeId origin_old,
                                    const char *name) {
  BoxTypeId out_old = My_New(TS_KIND_ALIAS, ts, origin_old, -1);
  My_Name_Set(ts, out_old, name);

  BoxType *origin_new = TS_Get_New_Style_Type(ts, origin_old);
  if (origin_new)
    TS_Set_New_Style_Type(ts, out_old,
                          BoxType_Create_Ident(BoxType_Link(origin_new), name));
  return out_old;
}

BoxTypeId BoxTS_New_Raised(BoxTS *ts, BoxTypeId origin_old) {
  BoxTypeId out_old = My_New(TS_KIND_RAISED, ts, origin_old, -1);
  BoxType *origin_new = TS_Get_New_Style_Type(ts, origin_old);
  if (origin_new)
    TS_Set_New_Style_Type(ts, out_old,
                          BoxType_Create_Raised(BoxType_Link(origin_new)));
  return out_old;
}

/*FUNCTIONS: My_Begin_Composite **********************************************/

/* Code for BoxTS_Begin_Struct, BoxTS_Begin_Species, etc. */
static BoxTypeId My_Begin_Composite(TSKind kind, TS *ts) {
  BoxTypeId t;
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

BoxTypeId BoxTS_Begin_Struct(BoxTS *ts) {
  BoxTypeId out_old = My_Begin_Composite(TS_KIND_STRUCTURE, ts);
  TS_Set_New_Style_Type(ts, out_old, BoxType_Create_Structure());
  return out_old;
}

BoxTypeId BoxTS_Begin_Species(BoxTS *ts) {
  BoxTypeId out_old = My_Begin_Composite(TS_KIND_SPECIES, ts);
  TS_Set_New_Style_Type(ts, out_old, BoxType_Create_Species());
  return out_old;
}

/*FUNCTIONS: My_Add_Member ***************************************************/

/* Code for BoxTS_Add_Struct_Member, BoxTS_Add_Species_Member, etc. */
static void My_Add_Member(TSKind kind, BoxTS *ts, BoxTypeId s, BoxTypeId m,
                          const char *m_name) {

  BoxTypeId new_m;
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
                       & new_m, sizeof(BoxTypeId));
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

void
BoxTS_Add_Struct_Member(BoxTS *ts, BoxTypeId structure, BoxTypeId member_type,
                        const char *member_name) {
  My_Add_Member(TS_KIND_STRUCTURE, ts, structure, member_type, member_name);

  BoxType *structure_new = TS_Get_New_Style_Type(ts, structure);
  if (structure_new) {
    BoxType *member_new = TS_Get_New_Style_Type(ts, member_type);
    if (member_new)
      BoxType_Add_Member_To_Structure(structure_new, member_new, member_name);
  }
}

void
BoxTS_Add_Species_Member(BoxTS *ts, BoxTypeId species, BoxTypeId member) {
  My_Add_Member(TS_KIND_SPECIES, ts, species, member, NULL);

  BoxType *species_new = TS_Get_New_Style_Type(ts, species);
  if (species_new) {
    BoxType *member_new = TS_Get_New_Style_Type(ts, member);
    if (member_new) {
      BoxType_Add_Member_To_Species(species_new, member_new);
      return;
    }
  }

  MSG_ERROR("Could not create species properly!");
}

/****************************************************************************/
/* Procedure registration and search */

/* Register the procedure.
 * The way we handle registration and search is very inefficient.
 * this could and should be improved, but we stick to the simple solution
 * for now!
 */
static void
My_Procedure_Register(BoxTS *ts, BoxTypeId p, BoxVMSymID sym_num) {
  TSDesc *proc_td, *parent_td;
  BoxTypeId parent;
  proc_td = Type_Ptr(ts, p);
  assert(proc_td->kind == TS_KIND_PROC);
  parent = proc_td->data.proc.parent;
  parent_td = Type_Ptr(ts, parent);
  assert(proc_td->first_proc == BOXTYPE_NONE); /* Must not be registered! */
  proc_td->first_proc = parent_td->first_proc;
  parent_td->first_proc = p;
  proc_td->data.proc.sym_num = sym_num;
}


/****************************************************************************/


/* Create a new unregistered subtype: a subtype is unregistered when
 * the parent is not aware of its existance. An unregistered type is defined
 * only giving its name and its parent type. When the subtype is registered
 * the parent type becomes aware of it. In order for the registration to be
 * completed the full type of the subtype must be specified.
 */
BoxTypeId
TS_Subtype_New(TS *ts, BoxTypeId parent_old, const char *child_name) {
  TSDesc td;
  BoxTypeId subtype_old;
  BoxType *parent_new;
  BoxType *subtype_new;

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

int TS_Subtype_Is_Registered(TS *ts, BoxTypeId st) {
  TSDesc *st_td = Type_Ptr(ts, st);
  assert(st_td->kind == TS_KIND_SUBTYPE);
  return (st_td->target != BOXTYPE_NONE);
}

/* Register a previously created (and still unregistered) subtype. */
Task TS_Subtype_Register(TS *ts, BoxTypeId subtype, BoxTypeId subtype_type) {
  TSDesc *s_td = Type_Ptr(ts, subtype);
  BoxTypeId parent, found_subtype;
  BoxName full_name;
  char *child_str;

  BoxType *subtype_new = TS_Get_New_Style_Type(ts, subtype);
  BoxType *subtype_type_new = TS_Get_New_Style_Type(ts, subtype_type);
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
    BoxTypeId found_subtype_type = found_subtype_td->target;
    BoxType *t1 = BoxType_From_Id(ts, found_subtype_type);
    BoxType *t2 = BoxType_From_Id(ts, subtype_type);
    assert(t1 && t2);
    BoxTypeCmp comparison = BoxType_Compare(t1, t2);
    if ((comparison & BOXTYPECMP_EQUAL) != BOXTYPECMP_EQUAL) {
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
                   & subtype, sizeof(BoxTypeId));

  assert(subtype_new && subtype_type_new);
  result_new = BoxType_Register_Subtype(subtype_new, subtype_type_new);
  assert(result_new);

  return BOXTASK_OK;
}

BoxTypeId TS_Subtype_Find(TS *ts, BoxTypeId parent, const char *name) {
  BoxName full_name;
  BoxHTItem *hi;

  do {
    Member_Full_Name(ts, & full_name, parent, name);
    if (BoxHT_Find(& ts->subtypes, full_name.text, full_name.length, & hi))
      return *((BoxTypeId *) hi->object);

  } while (BoxTS_Resolve_Once(ts, & parent, TS_KS_ALIAS | TS_KS_SPECIES));

  return BOXTYPE_NONE;
}

/****************************************************************************/

BoxTypeId BoxTS_New_Any(BoxTS *ts) {
  BoxTypeId dummy_typeid =
    BoxTS_New_Intrinsic(ts, sizeof(BoxAny), __alignof__(BoxAny));
  My_Name_Set(ts, dummy_typeid, "ANY");

  BoxType *t_any = BoxType_Create_Any();

  if (t_any)
    TS_Set_New_Style_Type(ts, dummy_typeid, t_any);
  return dummy_typeid;
}
