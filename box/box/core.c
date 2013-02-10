/****************************************************************************
 * Copyright (C) 2012 by Matteo Franchin                                    *
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

#include <box/callable.h>
#include <box/obj.h>
#include <box/core.h>
#include <box/coremath.h>
#include <box/str.h>
#include <box/bltinarray.h>

#include <box/types_priv.h>
#include <box/bltinarray_priv.h>
#include <box/core_priv.h>

/**
 * Structure collection all Box core types.
 */
static BoxCoreTypes box_core_types;


static void
My_Set_Type(BoxCoreTypes *ct, BoxTypeId t_id, BoxType *t, BoxBool *success) {
  assert(t_id >= BOXTYPEID_MIN_VAL && t_id < BOXTYPEID_MAX_VAL);
  ct->types[t_id] = t;
  if (!t) {
    *success = BOXBOOL_FALSE;
    BoxSPtr_Unlink(t);
  }
}

static void
My_Create_Type(BoxCoreTypes *core_types, const char *name,
               BoxTypeId id, size_t sz, size_t algn, BoxBool *success) {
  BoxType *t;

  if (id != BOXTYPEID_NONE)
    t = BoxType_Create_Primary(id, sz, algn);
  else
    t = BoxType_Create_Intrinsic(sz, algn);

  My_Set_Type(core_types, id, BoxType_Create_Ident(t, name), success);
}

/* Create primary and intrinsic types. */
static void
My_Init_Basic_Types(BoxCoreTypes *core_types, BoxBool *success) {
  int idx;

  struct {
    const char *name;
    BoxTypeId id;
    size_t size;
    size_t alignment;
  } *row, table[] = {
    {"Type", BOXTYPEID_TYPE, sizeof(BoxTypeBundle), __alignof__(BoxTypeBundle)},
    {"(.[)", BOXTYPEID_INIT, (size_t) 0, (size_t) 0},
    {"(].)", BOXTYPEID_FINISH, (size_t) 0, (size_t) 0},
    {"Char", BOXTYPEID_CHAR, sizeof(BoxChar), __alignof__(BoxChar)},
    {"INT", BOXTYPEID_INT, sizeof(BoxInt), __alignof__(BoxInt)},
    {"REAL", BOXTYPEID_REAL, sizeof(BoxReal), __alignof__(BoxReal)},
    {"POINT", BOXTYPEID_POINT, sizeof(BoxPoint), __alignof__(BoxPoint)},
    {"PTR", BOXTYPEID_PTR, sizeof(BoxPtr), __alignof__(BoxPtr)},
    {"Obj", BOXTYPEID_OBJ, sizeof(BoxPtr), __alignof__(BoxPtr)},
    {"Void", BOXTYPEID_VOID, (size_t) 0,  (size_t) 0},
    {"(=)", BOXTYPEID_COPY, (size_t) 0,  (size_t) 0},
    {"([)", BOXTYPEID_BEGIN, (size_t) 0, (size_t) 0},
    {"(])", BOXTYPEID_END, (size_t) 0, (size_t) 0},
    {"(;)", BOXTYPEID_PAUSE, (size_t) 0, (size_t) 0},
    {"CPtr", BOXTYPEID_CPTR, sizeof(BoxCPtr), __alignof__(BoxCPtr)},
    {"Str", BOXTYPEID_STR, sizeof(BoxStr), __alignof__(BoxStr)},
    {"ARRAY", BOXTYPEID_ARRAY, sizeof(BoxArray), __alignof__(BoxArray)},
    {(const char *) NULL, BOXTYPEID_NONE, (size_t) 0, (size_t) 0}

#if 0
    {& core_types->root_type, "/", BOXTYPEID_NONE,
     (size_t) 0, (size_t) 0},
    {& core_types->REFERENCES_type, "REFERENCES", BOXTYPEID_NONE,
     0, 0},
#endif
  };

  /* Set all the entries in the table to NULL. */
  for (idx = BOXTYPEID_MIN_VAL; idx < BOXTYPEID_MAX_VAL; idx++)
    core_types->types[idx] = NULL;

  /* Populate the table. */
  for (row = & table[0]; row->name; row++)
    My_Create_Type(core_types, row->name,
                   row->id, row->size, row->alignment, success);

  My_Set_Type(core_types, BOXTYPEID_ANY, BoxType_Create_Any(), success);

  /* Register combinations for core_types->type_type. */
  if (!Box_Register_Type_Combs(core_types))
    *success = BOXBOOL_FALSE;
}

/* Create species. */
static void My_Init_Composite_Types(BoxCoreTypes *ct, BoxBool *success) {
  BoxType *t;
  BoxType *real_couple;

  /* Int = (CHAR => INT) */
  t = BoxType_Create_Species();
  if (t) {
    BoxType_Add_Member_To_Species(t, ct->types[BOXTYPEID_CHAR]);
    BoxType_Add_Member_To_Species(t, ct->types[BOXTYPEID_INT]);
    t = BoxType_Create_Ident(t, "Int");
  }
  My_Set_Type(ct, BOXTYPEID_SINT, t, success);

  /* Real = (CHAR => INT => REAL) */
  t = BoxType_Create_Species();
  if (t) {
    BoxType_Add_Member_To_Species(t, ct->types[BOXTYPEID_CHAR]);
    BoxType_Add_Member_To_Species(t, ct->types[BOXTYPEID_INT]);
    BoxType_Add_Member_To_Species(t, ct->types[BOXTYPEID_REAL]);
    t = BoxType_Create_Ident(t, "Real");
  }
  My_Set_Type(ct, BOXTYPEID_SREAL, t, success);

  /* Point = ((Real x, y) => POINT) */
  t = BoxType_Create_Species();
  real_couple = BoxType_Create_Structure();
  if (t) {
    BoxType *real_type = ct->types[BOXTYPEID_SREAL];
    if (real_type && real_couple) {
      BoxType_Add_Member_To_Structure(real_couple, real_type, "x");
      BoxType_Add_Member_To_Structure(real_couple, real_type, "y");
      BoxType_Add_Member_To_Species(t, real_couple);
      BoxType_Add_Member_To_Species(t, ct->types[BOXTYPEID_POINT]);
      t = BoxType_Create_Ident(t, "Point");
    } else {
      (void) BoxType_Unlink(t);
      t = NULL;
    }
  }
  My_Set_Type(ct, BOXTYPEID_REAL_COUPLE, real_couple, success);
  My_Set_Type(ct, BOXTYPEID_SPOINT, t, success);

  /* Print = Void */
  t = BoxType_Create_Ident(BoxType_Link(ct->types[BOXTYPEID_VOID]), "Print");
  My_Set_Type(ct, BOXTYPEID_PRINT, t, success);

  /* Repr = Str */
  t = BoxType_Create_Ident(BoxType_Link(ct->types[BOXTYPEID_STR]), "Repr");
  My_Set_Type(ct, BOXTYPEID_REPR, t, success);

  /* Num = Int */
  t = BoxType_Create_Ident(BoxType_Link(ct->types[BOXTYPEID_SINT]), "Num");
  My_Set_Type(ct, BOXTYPEID_NUM, t, success);

  /* Compare = Int */
  t = BoxType_Create_Ident(BoxType_Link(ct->types[BOXTYPEID_INT]), "Compare");
  My_Set_Type(ct, BOXTYPEID_COMPARE, t, success);

  /* If = Int */
  t = BoxType_Create_Raised(BoxType_Link(ct->types[BOXTYPEID_INT]));
  t = BoxType_Create_Ident(t, "If");
  My_Set_Type(ct, BOXTYPEID_IF, t, success);

  /* Else = Void */
  t = BoxType_Create_Raised(BoxType_Link(ct->types[BOXTYPEID_VOID]));
  t = BoxType_Create_Ident(t, "Else");
  My_Set_Type(ct, BOXTYPEID_ELSE, t, success);

  /* Elif = Int */
  t = BoxType_Create_Raised(BoxType_Link(ct->types[BOXTYPEID_INT]));
  t = BoxType_Create_Ident(t, "Elif");
  My_Set_Type(ct, BOXTYPEID_ELIF, t, success);

  /* For = Int */
  t = BoxType_Create_Raised(BoxType_Link(ct->types[BOXTYPEID_INT]));
  t = BoxType_Create_Ident(t, "For");
  My_Set_Type(ct, BOXTYPEID_FOR, t, success);
}

/* Initialize the core types of Box. */
BoxBool BoxCoreTypes_Init(BoxCoreTypes *core_types) {
  BoxBool success = BOXBOOL_TRUE;

  core_types->initialized = BOXBOOL_TRUE;
  My_Init_Basic_Types(core_types, & success);
  My_Init_Composite_Types(core_types, & success);
  BoxArray_Register_Combs();

#if 0
  /* Register math core functions. */
  if (success)
    success = BoxCoreTypes_Init_Math(core_types);
#endif

  return success;
}

/* Finalize the core type of Box. */
void BoxCoreTypes_Finish(BoxCoreTypes *core_types) {
  int idx;

  /* Set all the entries in the table to NULL. */
  for (idx = BOXTYPEID_MIN_VAL; idx < BOXTYPEID_MAX_VAL; idx++)
    BoxSPtr_Unlink(core_types->types[idx]);

  core_types->initialized = BOXBOOL_FALSE;
}

/* Initialize the type system. */
BoxBool Box_Initialize_Type_System(void) {
  return BoxCoreTypes_Init(& box_core_types);
}

/* Finalize the type system. */
void Box_Finalize_Type_System(void) {
  BoxCoreTypes_Finish(& box_core_types);
}

/* Get the core type corresponding to the given type identifier. */
BoxType *
BoxCoreTypes_Get_Type(BoxCoreTypes *ct, BoxTypeId id) {
  unsigned int idx = (unsigned int) id; 

  if (idx >= BOXTYPEID_MIN_VAL && idx < BOXTYPEID_MAX_VAL) {
    if (ct->initialized)
      return ct->types[idx];
    else if (BoxCoreTypes_Init(ct))
      return ct->types[idx];
  }

  return NULL;
}

/* Similar to BoxCoreTypes_Get_Type() but uses the global core type set. */
BoxType *
Box_Get_Core_Type(BoxTypeId id) {
  return BoxCoreTypes_Get_Type(& box_core_types, id);
}
