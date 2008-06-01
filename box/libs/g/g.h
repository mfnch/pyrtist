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

#ifndef _G_H
#  define _G_H

#  include <stdlib.h>
#  include <stdio.h>

#  include "types.h"
#  include "graphic.h"

void g_error(const char *msg);
void g_warning(const char *msg);

/** Given an array of possible strings (an array of string pointers,
 * terminated by a (char *) NULL pointer), returns the index of the one
 * which matches (via strcasecmp) with the provided one 'string'.
 * In case of no matches the function returns -1.
 */
int g_string_find_in_list(char **list, const char *string);


/** Print a list of strings (as accepted by 'g_string_find_in_list')
 * to the stream 'out', using 'separator' as a separator.
 */
void g_string_list_print(FILE *out, char **list, char *separator);

/** Given an array of possible extensions (which is just an array
 * made up by the pointers to the corresponding string, terminated
 * by a NULL pointer), returns the index of the extension of 'file_name'.
 * If the extension of 'file_name' is not found among the provided array,
 * the function returns -1.
 */
int file_extension(char **extensions, const char *file_name);

/** Attributes in GStyle */
typedef enum {
  G_STYLE_ATTR_FILL_STYLE=0,
  G_STYLE_ATTR_DRAW_WHEN,
  G_STYLE_ATTR_BORD_COLOR,
  G_STYLE_ATTR_BORD_WIDTH,
  G_STYLE_ATTR_BORD_JOIN_STYLE,
  G_STYLE_ATTR_BORD_MITER_LIMIT,
  G_STYLE_ATTR_BORD_DASHES,
  G_STYLE_ATTR_NUM
} GStyleAttr;

/** Decides when the call to rdraw should happen.
 * NOTE: order is important, since we use <, ==, > operators with
 * DrawWhen values.
 */
typedef enum {
  DRAW_WHEN_PAUSE=0,        /**< Draw should happen on ';' and on ']' */
  DRAW_WHEN_END=1,          /**< Draw should happen on ']' */
  DRAW_WHEN_EXPLICIT_DRAW=2 /**< Draw is decided by the user with .Draw[] */
} DrawWhen;

/** The graphic style contains information about the fill style,
 * the border color and style, etc.
 */
typedef struct _g_style {
  struct _g_style *covered_style;
  void *attr[G_STYLE_ATTR_NUM];
  FillStyle fill_style;
  DrawWhen draw_when;
  Color bord_color;
  Real bord_width, bord_miter_limit;
  JoinStyle bord_join_style;
  Real *bord_dashes;
} GStyle;

/** Create a new transparent style (transparent means that non of its
 * attributes is set and hence it has no effect at all, i.e. it is
 * "transparent"). 'covered_style' is the style, which will be used
 * for the unset attributes. After this call, the style 'gs' will be
 * equivalent to the style 'covered_style'. However calls to
 * 'g_style_attr_set' can change this situation.
 */
void g_style_new(GStyle *gs, GStyle *covered_style);

/** Unset all the attributes of the style 'gs'.
 * This can also be considered as a destructor of 'GStyle' objects.
 */
void g_style_clear(GStyle *gs);

/** Get an attribute from the style 'gs' (or the styles it covers).
 * If the specified attribute 'a' is not set, then uses 'default_style'.
 * If the attribute 'a' is unset also for 'default_style', then NULL
 * is returned.
 */
void *g_style_attr_get(GStyle *gs, GStyle *default_style,
                       GStyleAttr a);

/** Set the attribute 'a' to 'attr_data'. */
void g_style_attr_set(GStyle *gs, GStyleAttr a, void *attr_data);

#define G_STYLE_NONE ((GStyle *) NULL)

/* fill_style */
#define g_style_set_fill_style(gs, ds) \
  do {(gs)->fill_style = (ds); \
      g_style_attr_set((gs), G_STYLE_ATTR_FILL_STYLE, \
      & (gs)->fill_style);} while (0)

#define g_style_unset_fill_style(gs) \
  g_style_attr_set((gs), G_STYLE_ATTR_FILL_STYLE, NULL)

#define g_style_get_fill_style(gs, default_style) \
  ((FillStyle *) g_style_attr_get((gs), default_style, \
                                  G_STYLE_ATTR_FILL_STYLE))

/* draw_when */
#define g_style_set_draw_when(gs, dw) \
  do {(gs)->draw_when = (dw); \
      g_style_attr_set((gs), G_STYLE_ATTR_DRAW_WHEN, \
                       & (gs)->draw_when);} while (0)

#define g_style_unset_draw_when(gs) \
  g_style_attr_set((gs), G_STYLE_ATTR_DRAW_WHEN, NULL)

#define g_style_get_draw_when(gs, default_style) \
  ((DrawWhen *) g_style_attr_get((gs), default_style, G_STYLE_ATTR_DRAW_WHEN))

/* bord_color */

/** void g_style_set_bord_color(GStyle *gs, Color *c) */
#define g_style_set_bord_color(gs, c) \
  do {(gs)->bord_color = *(c); \
      g_style_attr_set((gs), G_STYLE_ATTR_BORD_COLOR, \
                       & (gs)->bord_color);} while (0)

/** void g_style_unset_bord_color(GStyle *gs) */
#define g_style_unset_bord_color(gs) \
  g_style_attr_set((gs), G_STYLE_ATTR_BORD_COLOR, NULL)

/** Color *g_style_get_bord_color(GStyle *gs) */
#define g_style_get_bord_color(gs, default_style) \
  ((Color *) g_style_attr_get((gs), default_style, G_STYLE_ATTR_BORD_COLOR))

/* bord_width */

/** void g_style_set_bord_width(GStyle *gs, Real *width) */
#define g_style_set_bord_width(gs, width) \
  do {(gs)->bord_width = *(width); \
      g_style_attr_set((gs), G_STYLE_ATTR_BORD_WIDTH, \
                       & (gs)->bord_width);} while (0)

/** void g_style_unset_bord_width(GStyle *gs) */
#define g_style_unset_bord_width(gs) \
  g_style_attr_set((gs), G_STYLE_ATTR_BORD_WIDTH, NULL)

/** Real *g_style_get_bord_width(GStyle *gs) */
#define g_style_get_bord_width(gs, default_style) \
  ((Real *) g_style_attr_get((gs), default_style, G_STYLE_ATTR_BORD_WIDTH))

/* bord_join_style */

/** void g_style_set_bord_join_style(GStyle *gs, JoinStyle *js) */
#define g_style_set_bord_join_style(gs, js) \
  do {(gs)->bord_join_style = *(js); \
      g_style_attr_set((gs), G_STYLE_ATTR_BORD_JOIN_STYLE, \
                       & (gs)->bord_join_style);} while (0)

/** void g_style_unset_bord_join_style(GStyle *gs) */
#define g_style_unset_bord_join_style(gs) \
  g_style_attr_set((gs), G_STYLE_ATTR_BORD_JOIN_STYLE, NULL)

/** JoinStyle *g_style_get_bord_join_style(GStyle *gs) */
#define g_style_get_bord_join_style(gs, default_style) \
  ((JoinStyle *) g_style_attr_get((gs), default_style, \
                                  G_STYLE_ATTR_BORD_JOIN_STYLE))

/* bord_miter_limit */

/** void g_style_set_bord_miter_limit(GStyle *gs, Real *ml) */
#define g_style_set_bord_miter_limit(gs, ml) \
  do {(gs)->bord_miter_limit = *(ml); \
      g_style_attr_set((gs), G_STYLE_ATTR_BORD_MITER_LIMIT, \
                       & (gs)->bord_miter_limit);} while (0)

/** void g_style_unset_bord_miter_limit(GStyle *gs) */
#define g_style_unset_bord_miter_limit(gs) \
  g_style_attr_set((gs), G_STYLE_ATTR_BORD_MITER_LIMIT, NULL)

/** JoinStyle *g_style_get_bord_miter_limit(GStyle *gs) */
#define g_style_get_bord_miter_limit(gs, default_style) \
  ((Real *) g_style_attr_get((gs), default_style, \
                             G_STYLE_ATTR_BORD_MITER_LIMIT))

/** Draw following the style 'gs' and only if 'now' is allowed
 * by the DrawWhen value set in 'gs'.
 */
int g_rdraw(GStyle *gs, GStyle *deafult_style, DrawWhen now);

/** Copy the attributes specified in the array sel from 'src' to 'dest'. */
void g_style_copy_selected(GStyle *dest, GStyle *src,
                           int sel[G_STYLE_ATTR_NUM]);

#endif
