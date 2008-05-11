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

#include <stdlib.h>
#include <stdio.h>
#include <strings.h>

#include "types.h"
#include "virtmach.h"
#include "g.h"

void g_error(const char *msg) {
  printf("Error: %s\n", msg);
}

void g_warning(const char *msg) {
  printf("Warning: %s\n", msg);
}

Task g_optcolor_set(OptColor *oc, Color *c) {
  if (c->r < -0.5) {
    if (oc->alternative == (OptColor *) NULL) {
      g_error("Cannot unset the color.");
      return Failed;
    }
    oc->selected = oc->alternative;
    return Success;

  } else {
    oc->selected = oc;
    oc->local = *c;
    return Success;
  }
}

Task g_optcolor_set_rgb(OptColor *oc, Real r, Real g, Real b) {
  Color c;
  c.r = r; c.g = g; c.b = b;
  return g_optcolor_set(oc, & c);
}

Color *g_optcolor_get(OptColor *oc) {
  if (oc == (OptColor *) NULL || oc->selected == oc)
    return & oc->local;
  else
    return g_optcolor_get(oc->selected);
}

void g_optcolor_alternative_set(OptColor *oc, OptColor *alternative) {
  oc->alternative = alternative;
}

/** Given an array of possible extensions (which is just an array
 * made up by the pointers to the corresponding strings, terminated
 * by a NULL pointer), returns the index of the extension of 'file_name'.
 * If the extension of 'file_name' is not found among the provided array,
 * the function returns -1.
 */
int file_extension(char **extensions, const char *file_name) {
  const char *c = file_name, *ext = (char *) NULL;
  /* First we determine the extension in the file_name */
  for(; *c != '\0'; c++)
    if (*c == '.') ext = c;

  if (ext == (char *) NULL)
    return -1;

  else {
    int i = 0;
    char **e;
    ++ext; /* This points to '.', so we can safely increment it! */
    for(e = extensions; *e != (char *) NULL; e++) {
      if (strcasecmp(ext, *e) == 0) return i;
      ++i;
    }
    return -1;
  }
}
