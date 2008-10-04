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
#if defined(TS_NAME_GET_CASE_STRUCTURE)
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
#if defined(TS_NAME_GET_CASE_X)
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

#ifdef TS_NAME_GET_CASE_X
#  undef TS_NAME_GET_CASE_X
#  undef GROUP_EMPTY
#  undef GROUP_ONE
#  undef GROUP_TWO
#endif
