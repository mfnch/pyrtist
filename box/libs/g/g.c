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

#if 0
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
#endif

/** Given an array of possible strings (an array of string pointers,
 * terminated by a (char *) NULL pointer), returns the index of the one
 * which matches (via strcasecmp) with the provided one 'string'.
 * In case of no matches the function returns -1.
 */
int g_string_find_in_list(char **list, const char *string) {
  int i = 0;
  const char **s;
  for(s = list; *s != (char *) NULL; s++) {
    if (strcasecmp(*s, string) == 0) return i;
    ++i;
  }
  return -1;
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
    return -1; /* Noe extension */

  ++ext; /* This points to '.', so we can safely increment it! */
  return g_string_find_in_list(extensions, ext);
}

/****************************************************************************
 * Function to manipulate graphic styles (GStyle).                          *
 ****************************************************************************/

void g_style_new(GStyle *gs, GStyle *covered_style) {
  int i;
  for(i = 0; i < G_STYLE_ATTR_NUM; i++)
    gs->attr[i] = (void *) NULL;
  gs->covered_style = covered_style;
}

static int check_attr(GStyleAttr a) {
  if (a >= 0 && a < G_STYLE_ATTR_NUM) return 1;
  g_error("check_attr: unknown GStyleAttr argument.");
  return 0;
}

void *g_style_attr_get(GStyle *gs, GStyle *default_style,
                         GStyleAttr a) {
  if (!check_attr(a)) return (void *) NULL;
  if (gs == (GStyle *) NULL) return (void *) NULL;

  if (gs->attr[a] != (void *) NULL)
    return gs->attr[a];

  else if (gs->covered_style != (GStyle *) NULL)
    return g_style_attr_get(gs->covered_style, default_style, a);

  else
    return g_style_attr_get(default_style, (GStyle *) NULL, a);
}

void g_style_attr_set(GStyle *gs, GStyleAttr a, void *attr_data) {
  if (!check_attr(a)) return;
  gs->attr[a] = attr_data;
}

void g_style_copy_selected(GStyle *dest, GStyle *src,
                           int sel[G_STYLE_ATTR_NUM]) {
  if (sel[G_STYLE_ATTR_DRAW]) {
    dest->draw = src->draw;
    dest->attr[G_STYLE_ATTR_DRAW] =
      (src->attr[G_STYLE_ATTR_DRAW] == NULL) ? NULL : & dest->draw;
  }

  if (sel[G_STYLE_ATTR_DRAW_WHEN]) {
    dest->draw_when = src->draw_when;
    dest->attr[G_STYLE_ATTR_DRAW_WHEN] =
      (src->attr[G_STYLE_ATTR_DRAW_WHEN] == NULL) ? NULL : & dest->draw_when;
  }
}

int g_rdraw(GStyle *gs, GStyle *default_style, DrawWhen now) {
  DrawWhen *when = g_style_get_draw_when(gs, default_style),
           predef_when = DRAW_WHEN_PAUSE;
  DrawStyle *style = g_style_get_draw_style(gs, default_style),
            predef_style = DRAW_EOFILL;
  if (when == (DrawWhen *) NULL) when = & predef_when;
  if (style == (DrawStyle *) NULL) style = & predef_style;
  if (now < *when) return 0;
  grp_rdraw(*style);
  grp_rreset();
  return 1;
}
