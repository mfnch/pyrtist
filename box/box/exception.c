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

#include <stdio.h>

#include <box/mem.h>
#include <box/exception.h>


struct BoxException_struct {
  char *msg;
};

/* Create an exception object. */
BoxException *BoxException_Create_Raw(char *msg) {
  BoxException *excp = BoxMem_Safe_Alloc(sizeof(BoxException));
  excp->msg = msg;
  return excp;
}

/* Destroy an exception object. */
void BoxException_Destroy(BoxException *excp) {
  BoxMem_Free(excp);
}

/* Print the given exception to screen. */
BoxBool BoxException_Check(BoxException *excp) {
  if (excp) {
    fprintf(stderr, "Exception: %s.\n", excp->msg);
    BoxException_Destroy(excp);
    return BOXBOOL_TRUE;

  } else
    return BOXBOOL_FALSE;
}

char *
BoxException_Get_Str(BoxException *excp) {
  return (excp) ? Box_Mem_Strdup(excp->msg) : NULL;
}
