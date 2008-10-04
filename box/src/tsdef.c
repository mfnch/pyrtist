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

/** @file tsdef.c
 * @brief Functions to create and handle data types (used by typesys.c).
 *
 * This file tries to solve a problem in the definition of the functions
 * for type creation and handling. The problem is the following:
 *   THE CODE TO CREATE A STRUCTURE, A SPECIES, ... IS ALMOST THE SAME,
 *   THE CODE TO CREATE AN ALIAS, AN ARRAY, ... IS ALMOST THE SAME,
 *   THE CODE TO ADD AN ELEMENT TO A STRUCTURE, A SPECIES, ... IS ALMOST
 *   THE SAME... WITH SMALL DIFFERENCES! WE HAVE TO FIND A SOLUTION TO
 *   AVOID RE-WRITING TOO MANY TIMES THE SAME THING: IT WOULD BE
 *   NOT BEAUTIFUL AT ALL TO MANTAIN HUNDREDS OF SIMILAR FUNCTIONS
 *   WHICH DIFFERS ONE FROM THE OTHER ONLY BY TWO OR THREE LINES.
 *   THIS ALSO WOULD LEAD TO ERRORS WHEN MODIFYING THE STRUCTURE TSDesc:
 *   I WOULD CERTAINLY FORGET TO UPDATE ONE OF THESE FUNCTIONS.
 * To solve this problem we use this strategy:
 *  - the common code is written inside this file;
 *  - the function is defined inside 'typesys.c' using the following syntax:
 *  @code
 *    #define NAME_OF_FUNCTION
 *    #  include "tsdef.c"
 *    #undef NAME_OF_FUNCTION
 *  @endcode
 * when this file is included it will define the required function:
 * all is done throughout preprocessor directives.
 * This also helps to make it clear what the differences between
 * different kind of types are!
 * Yes, I know! This preprocessor stuff is too tricky, but to me it seems
 * the better way of doing things!
 * ---
 * Mhhm, not anymore. We should get rid of this replacing macros
 * with functions.
 *  mf, 4 Oct 2008
 */

/* Here we define the head of the function. We also specify what piece
 * of common code will be used to define it.
 */
#if defined(TS_STRUCTURE_BEGIN)
#  define TS_X_BEGIN TS_KIND_STRUCTURE
Task TS_Structure_Begin(TS *ts, Type *s)

#elif defined(TS_SPECIES_BEGIN)
#  define TS_X_BEGIN TS_KIND_SPECIES
Task TS_Species_Begin(TS *ts, Type *s)

#elif defined(TS_ENUM_BEGIN)
#  define TS_X_BEGIN TS_KIND_ENUM
Task TS_Enum_Begin(TS *ts, Type *s)

#elif defined(TS_STRUCTURE_ADD)
#  define TS_X_ADD TS_KIND_STRUCTURE
Task TS_Structure_Add(TS *ts, Type s, Type m, const char *m_name)

#elif defined(TS_SPECIES_ADD)
#  define TS_X_ADD TS_KIND_SPECIES
Task TS_Species_Add(TS *ts, Type s, Type m)

#elif defined(TS_ENUM_ADD)
#  define TS_X_ADD TS_KIND_ENUM
Task TS_Enum_Add(TS *ts, Type s, Type m)

#elif defined(TS_NAME_GET_CASE_STRUCTURE)
#  define TS_NAME_GET_CASE_X TS_KIND_STRUCTURE
#  define GROUP_EMPTY "(,)"
#  define GROUP_ONE "(%~s,)"
#  define GROUP_TWO "%~s, %~s"

#elif defined(TS_NAME_GET_CASE_SPECIES)
#  define TS_NAME_GET_CASE_X TS_KIND_SPECIES
#  define GROUP_EMPTY "(->)"
#  define GROUP_ONE "(%~s->)"
#  define GROUP_TWO "%~s->%~s"

#elif defined(TS_NAME_GET_CASE_ENUM)
#  define TS_NAME_GET_CASE_X TS_KIND_ENUM
#  define GROUP_EMPTY "(|)"
#  define GROUP_ONE "(%~s|)"
#  define GROUP_TWO "%~s|%~s"
#endif

/* here we really define the body of the function */
#if defined(TS_X_BEGIN)
/* Code for TS_Structure_Begin, TS_Species_Begin, etc. */
{
  TSDesc td;
  TS_TSDESC_INIT(& td);
  td.kind = TS_X_BEGIN;
  td.target = TS_TYPE_NONE;
  td.data.last = TS_TYPE_NONE;
  td.size = 0;
  TASK( Type_New(ts, s, & td) );
  return Success;
}

#elif defined(TS_X_ADD)
/* Code for TS_Structure_Add, TS_Species_Add, etc. */
{
  TSDesc td, *m_td, *s_td;
  Type new_m;
  Int m_size;
  m_td = Type_Ptr(ts, m);
  m_size = m_td->size;
  TS_TSDESC_INIT(& td);
  td.kind = TS_KIND_MEMBER;
  td.target = m;
#  ifdef TS_STRUCTURE_ADD
  if (m_name != (char *) NULL) td.name = Mem_Strdup(m_name);
  td.size = TS_Align(ts, TS_Size(ts, s));
#  else
  td.size = m_size;
#  endif
  td.data.member_next = s;
  TASK( Type_New(ts, & new_m, & td) );

  s_td = Type_Ptr(ts, s);
  assert(s_td->kind == TS_X_ADD);
  if (s_td->data.last != TS_TYPE_NONE) {
    m_td = Type_Ptr(ts, s_td->data.last);
    assert(m_td->kind == TS_KIND_MEMBER);
    m_td->data.member_next = new_m;
  }
  s_td->data.last = new_m;
  if (s_td->target == TS_TYPE_NONE) s_td->target = new_m;
#  ifdef TS_STRUCTURE_ADD
  s_td->size += m_size;
  /* We also add the member to the hashtable for fast search */
  if (m_name != (char *) NULL) {
    Name n;
    TASK( Member_Full_Name(ts, & n, s, m_name) );
    HT_Insert_Obj(ts->members, n.text, n.length, & new_m, sizeof(Type));
  }

#  elif defined(TS_ENUM_ADD)
  m_size += TS_Align(ts, sizeof(Int));
  if (m_size > s_td->size) s_td->size = m_size;
#  else
  if (m_size > s_td->size) s_td->size = m_size;
#  endif
  return Success;
}

#elif defined(TS_NAME_GET_CASE_X)
  /* A case in the main switch statement of the TS_Name_Get function */
    if (td->size > 0) {
      char *name = (char *) NULL;
      Type m = td->target;
#ifdef TS_NAME_GET_CASE_STRUCTURE
      Type previous_type = TS_TYPE_NONE;
#endif
      while (1) {
        TSDesc *m_td = Type_Ptr(ts, m);
        char *m_name = TS_Name_Get(ts, m_td->target);
#ifdef TS_NAME_GET_CASE_STRUCTURE
        if (m_td->name != (char *) NULL) {
          if (m_td->target != previous_type)
            m_name = printdup("%~s %s", m_name, m_td->name);
          else {
            Mem_Free(m_name);
            m_name = Mem_Strdup(m_td->name);
          }
        }
        previous_type = m_td->target;
#endif
        m = m_td->data.member_next;
        if (m == t) {
          if (name == (char *) NULL)
            return printdup(GROUP_ONE, m_name);
          else
            return printdup("("GROUP_TWO")", name, m_name);
        } else {
          if (name == (char *) NULL)
            name = m_name; /* no need to free! */
          else
            name = printdup(GROUP_TWO, name, m_name);
        }
      }
    }
    return Mem_Strdup(GROUP_EMPTY);

#endif

#ifdef TS_X_NEW
#  undef TS_X_NEW
#endif

#ifdef TS_X_BEGIN
#  undef TS_X_BEGIN
#endif

#ifdef TS_X_ADD
#  undef TS_X_ADD
#endif

#ifdef TS_NAME_GET_CASE_X
#  undef TS_NAME_GET_CASE_X
#  undef GROUP_EMPTY
#  undef GROUP_ONE
#  undef GROUP_TWO
#endif
