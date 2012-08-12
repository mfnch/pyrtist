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

#include <box/types_priv.h>
#include <box/callable.h>
#include <box/obj.h>
#include <box/core.h>
#include <box/coremath.h>

#include <box/core_priv.h>

/**
 * Structure collection all Box core types.
 */
BoxCoreTypes box_core_types;

/* Create primary and intrinsic types. */
static void My_Init_Basic_Types(BoxCoreTypes *core_types, BoxBool *success) {
  struct {
    BoxXXXX **dest;
    const char *name;
    BoxTypeId id;
    size_t size;
    size_t alignment;

  } *row, table[] = {
    {& core_types->type_type, "Type", BOXTYPEID_TYPE,
     sizeof(BoxTypeBundle), __alignof__(BoxTypeBundle)},
    {& core_types->init_type, ".[", BOXTYPEID_INIT,
     (size_t) 0, (size_t) 0},
    {& core_types->finish_type, "].", BOXTYPEID_FINISH,
     (size_t) 0, (size_t) 0},
    {& core_types->CHAR_type, "CHAR", BOXTYPEID_CHAR,
     sizeof(BoxChar), __alignof__(BoxChar)},
    {& core_types->Char_type, "Char", BOXTYPEID_CHAR,
     sizeof(BoxChar), __alignof__(BoxChar)},
    {& core_types->INT_type, "INT", BOXTYPEID_INT,
     sizeof(BoxInt), __alignof__(BoxInt)},
    {& core_types->REAL_type, "REAL", BOXTYPEID_REAL,
     sizeof(BoxReal), __alignof__(BoxReal)},
    {& core_types->POINT_type, "POINT", BOXTYPEID_POINT,
     sizeof(BoxPoint), __alignof__(BoxPoint)},
    {& core_types->PTR_type, "PTR", BOXTYPEID_PTR,
     sizeof(BoxPtr), __alignof__(BoxPtr)},
    {& core_types->root_type, "/", BOXTYPEID_NONE,
     (size_t) 0, (size_t) 0},
    {& core_types->callable_type, "Callable", BOXTYPEID_NONE,
     0, 0},
    {& core_types->REFERENCES_type, "REFERENCES", BOXTYPEID_NONE,
     0, 0},

    {NULL, (const char *) NULL, BOXTYPEID_NONE,
     (size_t) 0, (size_t) 0}
  };

  core_types->type_type = (BoxXXXX *) NULL;

  for (row = & table[0]; row->dest; row++) {
    BoxXXXX *t;

    if (row->id != BOXTYPEID_NONE)
      t = BoxType_Create_Primary(row->id, row->size, row->alignment);
    else
      t = BoxType_Create_Intrinsic(row->size, row->alignment);

    if (t) {
      BoxXXXX *id = BoxType_Create_Ident(t, row->name);
      if (id)
        *row->dest = id;

      else {
        *success = BOXBOOL_FALSE;
        BoxSPtr_Unlink(t);
      }
    
    } else {
      *success = BOXBOOL_FALSE;
      *row->dest = NULL;
    }
  }

  core_types->any_type = BoxType_Create_Any();
  if (!core_types->any_type)
    *success = BOXBOOL_FALSE;

  /* Register combinations for core_types->type_type. */
  if (!Box_Register_Type_Combs(core_types))
    *success = BOXBOOL_FALSE;
}

/* Create species. */
static void My_Init_Species(BoxCoreTypes *ct, BoxBool *success) {
  BoxXXXX *t;

  /* Int = (CHAR => INT) */
  t = BoxType_Create_Species();
  if (t) {
    BoxType_Add_Member_To_Species(t, ct->CHAR_type);
    BoxType_Add_Member_To_Species(t, ct->INT_type);
    ct->Int_type = BoxType_Create_Ident(t, "Int");

  } else {
    ct->Int_type = NULL;
    *success = BOXBOOL_FALSE;
  }

  /* Real = (CHAR => INT => REAL) */
  t = BoxType_Create_Species();
  if (t) {
    BoxType_Add_Member_To_Species(t, ct->CHAR_type);
    BoxType_Add_Member_To_Species(t, ct->INT_type);
    BoxType_Add_Member_To_Species(t, ct->REAL_type);
    ct->Real_type = BoxType_Create_Ident(t, "Real");

  } else {
    ct->Real_type = NULL;
    *success = BOXBOOL_FALSE;
  }

  /* Point = ((Real x, y) => POINT) */
  ct->Point_type = NULL;
  t = BoxType_Create_Species();
  if (t) {
    BoxXXXX *point_struct = BoxType_Create_Structure();
    if (point_struct) {
      BoxType_Add_Member_To_Structure(point_struct, ct->Real_type, "x");
      BoxType_Add_Member_To_Structure(point_struct, ct->Real_type, "y");
      BoxType_Add_Member_To_Species(t, point_struct);
      BoxType_Add_Member_To_Species(t, ct->POINT_type);
      BoxSPtr_Unlink(point_struct);
      ct->Point_type = BoxType_Create_Ident(t, "Point");

    } else
      *success = BOXBOOL_FALSE;

  } else
    *success = BOXBOOL_FALSE;
}

/* Initialize the core types of Box. */
BoxBool BoxCoreTypes_Init(BoxCoreTypes *core_types) {
  BoxBool success = BOXBOOL_TRUE;

  My_Init_Basic_Types(core_types, & success);
  My_Init_Species(core_types, & success);

  /* Register math core functions. */
  if (success)
    success = BoxCoreTypes_Init_Math(& box_core_types);

  return success;
}

/* Finalize the core type of Box. */
void BoxCoreTypes_Finish(BoxCoreTypes *core_types) {
  BoxXXXX *types[] =
   {core_types->root_type,
    core_types->init_type,
    core_types->finish_type,
    core_types->type_type,
    core_types->CHAR_type,
    core_types->INT_type,
    core_types->REAL_type,
    core_types->POINT_type,
    core_types->Char_type,
    core_types->Int_type,
    core_types->Real_type,
    core_types->Point_type,
    core_types->PTR_type,
    core_types->any_type,
    core_types->callable_type,
    core_types->REFERENCES_type,
    NULL};

  BoxXXXX **type;

  for (type = & types[0]; *type; type++)
    BoxSPtr_Unlink(*type);
}

/* Initialize the type system. */
BoxBool Box_Initialize_Type_System(void) {
  return BoxCoreTypes_Init(& box_core_types);
}

/* Finalize the type system. */
void Box_Finalize_Type_System(void) {
  BoxCoreTypes_Finish(& box_core_types);
}
