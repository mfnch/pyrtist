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

/* Interface for Window */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "types.h"
#include "virtmach.h"
#include "graphic.h"
#include "g.h"
#include "pointlist.h"
#include "i_pointlist.h"
#include "i_window.h"
#include "i_line.h"
#include "i_put.h"
#include "i_style.h"

/*#define DEBUG*/

static void set_default_style(GStyle *gs, FillStyle ds, DrawWhen dw) {
  g_style_new(gs, G_STYLE_NONE);
  g_style_set_fill_style(gs, ds);
  g_style_set_draw_when(gs, dw);
}

static void init_default_styles(Window *w) {
  /* For Circle, we set a style which allows easily to draw dognuts */
  set_default_style(& w->circle.default_style, DRAW_EOFILL, DRAW_WHEN_END);
  /* For Poly, we use the same style as Circle to easily allow holes */
  set_default_style(& w->poly.default_style, DRAW_EOFILL, DRAW_WHEN_END);
  /* For Text and Line, we usually don't want holes to appear! */
  set_default_style(& w->text.default_style, DRAW_FILL, DRAW_WHEN_PAUSE);
  set_default_style(& w->line.default_style, DRAW_FILL, DRAW_WHEN_PAUSE);

  /* The main Window style is left completely unset (i.e. transparent
   * to the default styles.
   */
  g_style_new(& w->style, (GStyle *) NULL);
}

static void destroy_styles(Window *w) {
  g_style_clear(& w->circle.default_style);
  g_style_clear(& w->poly.default_style);
  g_style_clear(& w->text.default_style);
  g_style_clear(& w->line.default_style);
  g_style_clear(& w->style);
}

Task window_begin(VMProgram *vmp) {
  WindowPtr *wp = BOX_VM_CURRENTPTR(vmp, WindowPtr);
  Window *w;
  w = *wp = (WindowPtr) malloc(sizeof(Window)); /* Should use Mem_Alloc, but this requires to link against the internal box library (which still does not exist) */
  if (w == (WindowPtr) NULL) return Failed;
#ifdef DEBUG
  printf("Window object allocated (%p)\n", w);
  printf("WindowPtr object: %p\n", wp);
#endif

  w->plan.have.type = 0;
  w->plan.type = grp_window_type_from_string("fig");
  w->plan.have.origin = 1;
  w->plan.origin.x = 0.0;
  w->plan.origin.y = 0.0;
  w->plan.have.size = 0;
  w->plan.size.x = 100.0;
  w->plan.size.y = 100.0;
  w->plan.have.resolution = 0;
  w->plan.resolution.x = 2.0;
  w->plan.resolution.y = 2.0;
  w->plan.have.file_name = 0;
  w->plan.file_name = (char *) NULL;

  w->save_file_name = (char *) NULL;

  w->window = (grp_window *) NULL;

  init_default_styles(w);

  TASK( pointlist_init(& w->pointlist) );

  TASK( line_window_init(w) );
  TASK( put_window_init(w) );
  return Success;
}

Task window_color(VMProgram *vmp) {
  WindowPtr wp = BOX_VM_CURRENT(vmp, WindowPtr);
  Window *w = (Window *) wp;
  Color *c = BOX_VM_ARGPTR1(vmp, Color);
  if (w->window != (grp_window *) NULL) {
    grp_window *cur_win = grp_win;
    grp_win = w->window;
    grp_rfgcolor(c);
    grp_win = cur_win;
  }
  return Success;
}

Task window_destroy(VMProgram *vmp) {
  WindowPtr wp = BOX_VM_CURRENT(vmp, WindowPtr);
  Window *w = (Window *) wp;
  grp_window *cur_win = grp_win;
#ifdef DEBUG
  printf("Window object deallocated\n");
#endif

  grp_win = w->window;
  grp_close_win();
  grp_win = cur_win;

  destroy_styles(w);
  pointlist_destroy(& w->pointlist);
  put_window_destroy(w);
  line_window_destroy(w);
  free(wp);
  return Success;
}

Task window_str(VMProgram *vmp) {
  WindowPtr wp = BOX_VM_CURRENT(vmp, WindowPtr);
  Window *w = (Window *) wp;
  char *type_str = BOX_VM_ARGPTR1(vmp, char);

  if (w->plan.have.type) {
    g_warning("You have already specified the window type!");
  }


  w->plan.type = grp_window_type_from_string(type_str);
  if (w->plan.type < 0) {
    g_error("Unrecognized window type!");
    return Failed;
  }
  w->plan.have.type = 1;
  return Success;
}

Task window_size(VMProgram *vmp) {
  WindowPtr wp = BOX_VM_CURRENT(vmp, WindowPtr);
  Window *w = (Window *) wp;
  Point *win_size = BOX_VM_ARGPTR1(vmp, Point);

  if (w->plan.have.size) {
    g_error("You have already specified the window size!");
    return Failed;
  }

  w->plan.have.size = 1;
  w->plan.size = *win_size;
  return Success;
}

Task window_style(VMProgram *vmp) {
  Window *w = BOX_VM_THIS(vmp, WindowPtr);
  IStyle *s = BOX_VM_ARG(vmp, IStylePtr);
  g_style_copy_selected(& w->style, & s->style, s->have);
  return Success;
}

Task window_end(VMProgram *vmp) {
  WindowPtr wp = BOX_VM_CURRENT(vmp, WindowPtr);
  Window *w = (Window *) wp;

  w->plan.have.resolution = 1;
  w->plan.have.size = 1;
  w->window = grp_window_open(& w->plan);
  if (w->window == (grp_window *) NULL) {
    g_error("cannot create the window!");
    return Failed;
  }

  return Success;
}

Task window_origin_point(VMProgram *vmp) {
  WindowPtr wp = BOX_VM_CURRENT(vmp, WindowPtr);
  Window *w = (Window *) wp;
  Point *origin = BOX_VM_ARGPTR1(vmp, Point);

  if (w->plan.have.origin) {
    g_error("You have already specified the origin of the window!");
    return Failed;
  }

  w->plan.have.origin = 1;
  w->plan.origin = *origin;
  return Success;
}

Task window_save_str(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  char *file_name = BOX_VM_ARGPTR1(vmp, char);

  if (w->save_file_name != (char *) NULL) {
    printf("Window.Save: changing save target from '%s' to '%s'\n",
           w->save_file_name, file_name);
  }

  w->save_file_name = file_name;
  return Success;
}

Task window_save_end(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);

  if (w->save_file_name == (char *) NULL) {
    g_error("window not saved: need a file name!\n");
    return Failed;

  } else if (w->window == (grp_window *) NULL) {
    g_error("cannot save window: it is not initialized!");
    return Failed;

  } else {
    grp_window *cur_win;
    int all_ok;

    cur_win = grp_win;
    grp_win = w->window;
    all_ok = grp_save(w->save_file_name);
    grp_win = cur_win;
    w->save_file_name = (char *) NULL;
    return (all_ok) ? Success : Failed;
  }

#ifdef DEBUG
  printf("Saved window to '%s'\n", w->save_file_name);
#endif
  return Success;
}

Task window_hot_begin(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  w->hot.got.name = 0;
  w->hot.got.point = 0;
  w->hot.name = (char *) NULL;
  return Success;
}

Task window_hot_point(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  Point *point = BOX_VM_ARGPTR1(vmp, Point);
  char *name = (w->hot.got.name) ? w->hot.name : (char *) NULL;
  Task t = pointlist_add(& w->pointlist, point, name);
  if (w->hot.got.name) {
    w->hot.got.name = 0;
    free(w->hot.name);
    w->hot.name = (char *) NULL; /* To avoid double free() */
  }
  w->hot.got.point = 1;
  return t;
}

Task window_hot_string(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  char *name = strdup(BOX_VM_ARGPTR1(vmp, char));
  w->hot.name = name;
  w->hot.got.name = 1;
  return Success;
}

static Task _add_from_pointlist(Int index, char *name,
                                void *object, void *data)
{
  PointList *dest_pl = (PointList *) data;
  Point *p = (Point *) object;
  return pointlist_add(dest_pl, p, name);
}

Task window_hot_pointlist(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  IPointList *ipl_to_add = BOX_VM_ARG1(vmp, IPointList *);
  return pointlist_iter(& ipl_to_add->pl, _add_from_pointlist, & w->pointlist);
}

Task window_hot_end(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  if (! w->hot.got.point)
    g_warning("Hot[] did not get a point!");
  if (w->hot.got.name)
    g_warning("Hot[] got a name, but not the corresponding point!");

  return Success;
}

/* FIXME: w->save_file_name needs to have a copy of the string. Box does
 *  not give the guarantee that the string will not be deallocated after
 *  this function has returned.
 */
Task window_file_string(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  if (w->plan.have.file_name) {
    g_warning("You have already provided a file name for the window.");
  }
  w->plan.have.file_name = 1;
  w->plan.file_name = BOX_VM_ARGPTR1(vmp, char);
  return Success;
}

Task window_res_point(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  Point *res = BOX_VM_ARGPTR1(vmp, Point);
  if (w->plan.have.resolution) {
    g_warning("You have already provided the window resolution.");
  }
  w->plan.resolution.x = res->x;
  w->plan.resolution.y = res->y;
  w->plan.have.resolution = 1;
  return Success;
}

Task window_show_point(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  Point *p = BOX_VM_ARGPTR1(vmp, Point);
  grp_window *cur_win = grp_win;
  grp_win = w->window;
  grp_fake_point(p);
  grp_win = cur_win;
  return Success;
}
