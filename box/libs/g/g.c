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
  char **s;
  for(s = list; *s != (char *) NULL; s++) {
    if (strcasecmp(*s, string) == 0) return i;
    ++i;
  }
  return -1;
}

void g_string_list_print(FILE *out, char **list, char *separator) {
  char **s, *sep = "";
  if (separator == (char *) NULL) separator = ", ";
  for(s = list; *s != (char *) NULL; s++) {
    fprintf(out, "%s%s", sep, *s);
    sep = separator;
  }
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

void g_style_clear(GStyle *gs) {
  int i;
  if (gs->attr[G_STYLE_ATTR_BORD_DASHES] != NULL)
    free(gs->bord_dashes.dashes);
  for(i = 0; i < G_STYLE_ATTR_NUM; i++)
    gs->attr[i] = (void *) NULL;
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

Int g_style_get_bord_num_dashes(GStyle *gs, GStyle *default_style) {
  void *d = g_style_attr_get(gs, default_style, G_STYLE_ATTR_BORD_DASHES);
  if (d == NULL) return 0;
  return ((GDashes *) d)->num;
}

Real *g_style_get_bord_dashes(GStyle *gs, GStyle *default_style) {
  void *d = g_style_attr_get(gs, default_style, G_STYLE_ATTR_BORD_DASHES);
  if (d == NULL) return 0;
  return ((GDashes *) d)->dashes;
}

Real g_style_get_bord_dash_offset(GStyle *gs, GStyle *default_style) {
  void *d = g_style_attr_get(gs, default_style, G_STYLE_ATTR_BORD_DASHES);
  if (d == NULL) return 0.0;
  return ((GDashes *) d)->offset;
}

void g_style_set_bord_dashes(GStyle *gs, Int num_dashes, Real *dashes,
                             Real offset) {
  g_style_unset_bord_dashes(gs);
  if (num_dashes >= 0) {
    Int dashes_size = sizeof(Real)*num_dashes;
    Real *dashes_cpy = (Real *) malloc(dashes_size);
    if (dashes_cpy == (Real *) NULL) {
      g_error("g_style_set_bord_dashes: malloc failed!");
      return;
    }
    (void) memcpy(dashes_cpy, dashes, dashes_size);
    gs->bord_dashes.dashes = dashes_cpy;
    gs->bord_dashes.num = num_dashes;
    gs->bord_dashes.offset = offset;
    gs->attr[G_STYLE_ATTR_BORD_DASHES] = & gs->bord_dashes;
  }
}

void g_style_unset_bord_dashes(GStyle *gs) {
  if (gs->attr[G_STYLE_ATTR_BORD_DASHES] != NULL)
    free(gs->bord_dashes.dashes);
  gs->attr[G_STYLE_ATTR_BORD_DASHES] = NULL;
}

void g_style_copy_selected(GStyle *dest, GStyle *src,
                           int sel[G_STYLE_ATTR_NUM]) {
  if (sel[G_STYLE_ATTR_FILL_STYLE]) {
    dest->fill_style = src->fill_style;
    dest->attr[G_STYLE_ATTR_FILL_STYLE] =
      (src->attr[G_STYLE_ATTR_FILL_STYLE] == NULL) ?
      NULL : & dest->fill_style;
  }

  if (sel[G_STYLE_ATTR_DRAW_WHEN]) {
    dest->draw_when = src->draw_when;
    dest->attr[G_STYLE_ATTR_DRAW_WHEN] =
      (src->attr[G_STYLE_ATTR_DRAW_WHEN] == NULL) ? NULL : & dest->draw_when;
  }

  if (sel[G_STYLE_ATTR_BORD_COLOR]) {
    dest->bord_color = src->bord_color;
    dest->attr[G_STYLE_ATTR_BORD_COLOR] =
      (src->attr[G_STYLE_ATTR_BORD_COLOR] == NULL) ?
      NULL : & dest->bord_color;
  }

  if (sel[G_STYLE_ATTR_BORD_WIDTH]) {
    dest->bord_width = src->bord_width;
    dest->attr[G_STYLE_ATTR_BORD_WIDTH] =
      (src->attr[G_STYLE_ATTR_BORD_WIDTH] == NULL) ?
      NULL : & dest->bord_width;
  }

  if (sel[G_STYLE_ATTR_BORD_JOIN_STYLE]) {
    dest->bord_join_style = src->bord_join_style;
    dest->attr[G_STYLE_ATTR_BORD_JOIN_STYLE] =
      (src->attr[G_STYLE_ATTR_BORD_JOIN_STYLE] == NULL) ?
      NULL : & dest->bord_join_style;
  }

  if (sel[G_STYLE_ATTR_BORD_MITER_LIMIT]) {
    dest->bord_miter_limit = src->bord_miter_limit;
    dest->attr[G_STYLE_ATTR_BORD_MITER_LIMIT] =
      (src->attr[G_STYLE_ATTR_BORD_MITER_LIMIT] == NULL) ?
      NULL : & dest->bord_miter_limit;
  }

  if (sel[G_STYLE_ATTR_BORD_CAP]) {
    dest->bord_cap = src->bord_cap;
    dest->attr[G_STYLE_ATTR_BORD_CAP] =
      (src->attr[G_STYLE_ATTR_BORD_CAP] == NULL) ?
      NULL : & dest->bord_cap;
  }

  if (sel[G_STYLE_ATTR_BORD_DASHES]) {
    g_style_unset_bord_dashes(dest);
    if (src->attr[G_STYLE_ATTR_BORD_DASHES] != NULL) {
      Int num_dashes = g_style_get_bord_num_dashes(src, (GStyle *) NULL);
      Real *dashes = g_style_get_bord_dashes(src, (GStyle *) NULL);
      Real dash_offset = g_style_get_bord_dash_offset(src, (GStyle *) NULL);
      g_style_set_bord_dashes(dest, num_dashes, dashes, dash_offset);
    }
  }
}

int g_rdraw(GStyle *gs, GStyle *default_style, DrawWhen now) {
  DrawWhen *when = g_style_get_draw_when(gs, default_style),
           predef_when = DRAW_WHEN_PAUSE;
  FillStyle *fill_style = g_style_get_fill_style(gs, default_style),
            predef_fill_style = FILLSTYLE_EO;
  Color *bord_color = g_style_get_bord_color(gs, default_style),
        predef_bord_color = (Color) {0.0, 0.0, 0.0, 1.0};
  Real *bord_width = g_style_get_bord_width(gs, default_style),
       predef_bord_width = 0.0;
  JoinStyle *bord_join_style = g_style_get_bord_join_style(gs, default_style),
       predef_bord_join_style = JOIN_STYLE_ROUND;
  Real *bord_miter_limit = g_style_get_bord_miter_limit(gs, default_style),
       predef_bord_miter_limit = 10.0;
  CapStyle *bord_cap = g_style_get_bord_cap(gs, default_style),
           predef_bord_cap = CAP_STYLE_BUTT;
  Int bord_num_dashes = g_style_get_bord_num_dashes(gs, default_style);
  Real *bord_dashes = g_style_get_bord_dashes(gs, default_style);
  Real bord_dash_offset = g_style_get_bord_dash_offset(gs, default_style);
  DrawStyle style;

  if (when == (DrawWhen *) NULL) when = & predef_when;
  if (fill_style == (FillStyle *) NULL) fill_style = & predef_fill_style;
  if (bord_color == (Color *) NULL) bord_color = & predef_bord_color;
  if (bord_join_style == (JoinStyle *) NULL)
    bord_join_style = & predef_bord_join_style;
  if (bord_width == (Real *) NULL) bord_width = & predef_bord_width;
  if (bord_miter_limit == (Real *) NULL)
    bord_miter_limit = & predef_bord_miter_limit;
  if (bord_cap == (CapStyle *) NULL) bord_cap = & predef_bord_cap;

  if (now < *when) return 0;
  style.fill_style = *fill_style;
  style.bord_width = *bord_width;
  style.bord_join_style = *bord_join_style;
  style.bord_miter_limit = *bord_miter_limit;
  style.bord_color = *bord_color;
  style.bord_cap = *bord_cap;
  style.bord_num_dashes = bord_num_dashes;
  style.bord_dashes = bord_dashes;
  style.bord_dash_offset = bord_dash_offset;
  grp_rdraw(& style);
  grp_rreset();
  return 1;
}
