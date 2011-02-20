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
#include <math.h>

#include "types.h"
#include "virtmach.h"
#include "str.h"
#include "graphic.h"
#include "g.h"
#include "bb.h"
#include "fig.h"
#include "autoput.h"
#include "pointlist.h"
#include "i_pointlist.h"
#include "i_window.h"
#include "i_line.h"
#include "i_put.h"
#include "i_style.h"
#include "i_gradient.h"
#include "i_bbox.h"

/*#define DEBUG*/

static void set_default_style(GStyle *gs, FillStyle ds, DrawWhen dw) {
  g_style_new(gs, G_STYLE_NONE);
  g_style_set_fill_style(gs, ds);
  g_style_set_draw_when(gs, dw);
}

static void init_default_styles(Window *w) {
  /* For Circle, we set a style which allows easily to draw dognuts */
  set_default_style(& w->circle.default_style, FILLSTYLE_EO, DRAW_WHEN_END);
  /* For Poly, we use the same style as Circle to easily allow holes */
  set_default_style(& w->poly.default_style, FILLSTYLE_EO, DRAW_WHEN_END);
  /* For Text and Line, we usually don't want holes to appear! */
  set_default_style(& w->text.default_style, FILLSTYLE_PLAIN,DRAW_WHEN_PAUSE);
  set_default_style(& w->line.default_style, FILLSTYLE_PLAIN,DRAW_WHEN_PAUSE);

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

void err_not_init(const char *location) {
  g_error("Cannot use the window: it has not been initialized, yet!");
}

Task window_begin(BoxVM *vmp) {
  WindowPtr *wp = BOX_VM_CURRENTPTR(vmp, WindowPtr);
  Window *w;
  w = *wp = (WindowPtr) malloc(sizeof(Window)); /* Should use Mem_Alloc, but this requires to link against the internal box library (which still does not exist) */
  if (w == (WindowPtr) NULL) return Failed;
#ifdef DEBUG
  printf("Window object allocated (%p)\n", w);
  printf("WindowPtr object: %p\n", wp);
#endif

  w->initialised = 0;
  w->plan.have.type = 0;
  w->plan.type = Grp_Window_Type_From_String("fig");
  w->plan.have.origin = 0;
  w->plan.origin.x = 0.0;
  w->plan.origin.y = 0.0;
  w->plan.have.size = 0;
  w->plan.size.x = 100.0;
  w->plan.size.y = 100.0;
  w->plan.have.resolution = 0;
  w->plan.resolution.x = 2.0;
  w->plan.resolution.y = 2.0;
  w->plan.have.file_name = 0;
  w->plan.file_name = NULL;

  w->save_file_name = NULL;

  w->window = Grp_Window_Error(stderr, "Cannot use a window before "
                               "completing the initialization stage.");

  init_default_styles(w);

  TASK( pointlist_init(& w->pointlist) );

  TASK( line_window_init(w) );
  TASK( put_window_init(w) );
  return Success;
}

Task window_color(BoxVM *vmp) {
  Window *w = BOX_VM_THIS(vmp, WindowPtr);
  Color *c = BOX_VM_ARG1_PTR(vmp, Color);
  if (w->window != (GrpWindow *) NULL) {
    GrpWindow *cur_win = grp_win;
    grp_win = w->window;
    grp_rfgcolor(c);
    grp_win = cur_win;
  }
  return Success;
}

Task window_gradient(BoxVM *vmp) {
  Window *w = BOX_VM_THIS(vmp, WindowPtr);
  Gradient *g = BOX_VM_ARG1(vmp, GradientPtr);
  if (w->window != (GrpWindow *) NULL) {
    GrpWindow *cur_win = grp_win;
    grp_win = w->window;
    grp_rgradient(& g->gradient);
    grp_win = cur_win;
  }
  return Success;
}

Task window_destroy(BoxVM *vmp) {
  WindowPtr wp = BOX_VM_CURRENT(vmp, WindowPtr);
  Window *w = (Window *) wp;
  GrpWindow *cur_win = grp_win;
#ifdef DEBUG
  printf("Window object deallocated\n");
#endif

  free(w->plan.file_name);

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

Task window_str(BoxVM *vm) {
  WindowPtr wp = BOX_VM_CURRENT(vm, WindowPtr);
  Window *w = (Window *) wp;
  BoxStr *s = BOX_VM_ARG_PTR(vm, BoxStr);
  const char *type_str = (char *) s->ptr;

  if (w->plan.have.type) {
    g_warning("You have already specified the window type!");
  }


  w->plan.type = Grp_Window_Type_From_String(type_str);
  if (w->plan.type < 0) {
    g_error("Unrecognized window type!");
    return Failed;
  }
  w->plan.have.type = 1;
  return Success;
}

Task window_size(BoxVM *vmp) {
  WindowPtr wp = BOX_VM_CURRENT(vmp, WindowPtr);
  Window *w = (Window *) wp;
  Point *win_size = BOX_VM_ARG1_PTR(vmp, Point);

  if (w->plan.have.size) {
    g_error("You have already specified the window size!");
    return Failed;
  }

  w->plan.have.size = 1;
  w->plan.size = *win_size;
  return Success;
}

Task window_style(BoxVM *vmp) {
  Window *w = BOX_VM_THIS(vmp, WindowPtr);
  IStyle *s = BOX_VM_ARG(vmp, IStylePtr);
  g_style_copy_selected(& w->style, & s->style, s->have);
  return Success;
}

Task window_end(BoxVM *vmp) {
  WindowPtr wp = BOX_VM_CURRENT(vmp, WindowPtr);
  Window *w = (Window *) wp;

  if (!w->initialised) {
    w->plan.have.resolution = 1;
    w->plan.have.origin = 1;
    w->window = Grp_Window_Open(& w->plan);
    if (w->window == (GrpWindow *) NULL) {
      g_error("cannot create the window!");
      return Failed;
    }

    w->initialised = 1;
  }

  return Success;
}

Task window_window(BoxVM *vmp) {
  Window *w = BOX_VM_THIS(vmp, WindowPtr);
  Window *src = BOX_VM_ARG1(vmp, WindowPtr);
  grp_window *cur_win = grp_win;
  grp_win = w->window;
  Fig_Draw_Fig(src->window);
  grp_win = cur_win;
  return Success;
}

Task window_origin_point(BoxVM *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  Point *origin = BOX_VM_ARG1_PTR(vmp, Point);

  if (w->plan.have.origin) {
    g_error("You have already specified the origin of the window!");
    return Failed;
  }

  w->plan.have.origin = 1;
  w->plan.origin = *origin;
  return Success;
}

Task window_save_begin(BoxVM *vmp) {
  Window *w  = BOX_VM_SUB_PARENT(vmp, WindowPtr);
  w->save_file_name = NULL;
  w->saved = 0;
  return Success;
}

Task window_save_str(BoxVM *vm) {
  Window *w  = BOX_VM_SUB_PARENT(vm, WindowPtr);
  BoxStr *s = BOX_VM_ARG_PTR(vm, BoxStr);

  if (w->save_file_name != NULL) {
    printf("Window.Save: changing save target from '%s' to '%s'\n",
           w->save_file_name, s->ptr);
    free(w->save_file_name);
  }

  w->save_file_name = strdup(s->ptr);
  return Success;
}

Task window_save_window(BoxVM *vmp) {
  Window *src  = BOX_VM_SUB_PARENT(vmp, WindowPtr);
  Window *dest = BOX_VM_ARG1(vmp, WindowPtr);
  Point translation = {0.0, 0.0}, center = {0.0, 0.0};
  Real sx = 1.0, sy = 1.0, rot_angle = 0.0;
  Matrix m;
  GrpWindow *cur_win = grp_win;

  int type_fig = Grp_Window_Type_From_String("fig");
  if (src->plan.type != type_fig) {
    g_error("Window.Save: Saving to arbitrary targets is only available "
            "for \"fig\" windows. Windows of different type accept only "
            "the syntax window.Save[\"filename\"]");
    return Failed;
  }
  if (src == dest) {
    g_error("Window.Save: saving a window into itself is not allowed.");
    return Failed;
  }

  /* Here we have two possibilities:
   * 1. 'dest' is an incomplete window (it can't be used for normal drawing):
   *    we then want to complete it, using the bounding box of 'src'.
   *    After this step, the window will become a normal, initialized one,
   *    which can be used elsewhere in the program.
   * 2. 'dest' is a normal, correctly initialized  window: we then want
   *    to scale and translate 'src', such that its bounding box matches
   *    the bounding box of 'dest'.
   */
  if (Grp_Window_Is_Error(dest->window)) {
    Point bb_min, bb_max;
    if (!BB_Bounding_Box(src->window, & bb_min, & bb_max)) {
      g_warning("Computed bounding box is degenerate: "
                "cannot save the figure!");
      return Failed;
    }

    if (src->save_file_name != NULL) {
      dest->plan.have.file_name = 1;
      dest->plan.file_name = strdup(src->save_file_name);
    }

    /*printf("Bounding box (%f, %f) - (%f, %f)\n",
             bb_min.x, bb_min.y, bb_max.x, bb_max.y);*/

    if (dest->plan.have.origin) {
      translation.x = -bb_min.x;
      translation.y = -bb_min.y;

    } else{
      dest->plan.origin.x = bb_min.x;
      dest->plan.origin.y = bb_min.y;
      dest->plan.have.origin = 1;
    }

    if (dest->plan.have.size) {
      sx = dest->plan.size.x/(bb_max.x - bb_min.x);
      sy = dest->plan.size.y/(bb_max.y - bb_min.y);

    } else {
      dest->plan.size.x = bb_max.x - bb_min.x;
      dest->plan.size.y = bb_max.y - bb_min.y;
      dest->plan.have.size = 1;
    }

    grp_win = dest->window;
    grp_close_win();
    grp_win = cur_win;
    dest->window = Grp_Window_Open(& dest->plan);
    if (dest->window == NULL) {
      g_error("Window.Save: cannot create the window!");
      return Failed;
    }

    if (Grp_Window_Is_Error(dest->window)) {
      g_error("Window.Save: cannot complete the given window!");
      return Failed;
    }

  } else {
    Point bb_min, bb_max;
    if (!BB_Bounding_Box(src->window, & bb_min, & bb_max)) {
      g_warning("Computed bounding box is degenerate: "
                "cannot save the figure!");
      return Failed;
    }
    translation.x = -bb_min.x;
    translation.y = -bb_min.y;
    sx = dest->plan.size.x/(bb_max.x - bb_min.x);
    sy = dest->plan.size.y/(bb_max.y - bb_min.y);
  }

  grp_win = dest->window;
  Grp_Matrix_Set(& m, & translation, & center, rot_angle, sx, sy);
  Fig_Draw_Fig_With_Matrix(src->window, & m);
  if (dest->plan.have.file_name)
    grp_save(dest->plan.file_name);
    /* ^^^ Some terminals require an explicit save! */

  grp_win = cur_win;

  if (src->save_file_name != NULL) {
    free(src->save_file_name);
    src->save_file_name = NULL;
    dest->plan.file_name = NULL;
  }
  src->saved = 1;
  return Success;
}

Task window_save_end(BoxVM *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);

  if (w->saved) {
    if (w->save_file_name != NULL) {
      free(w->save_file_name);
      w->save_file_name = NULL;
      g_warning("Window.Save: given file name was not used.\n");
    }
    return Success;

  } else {
    GrpWindow *cur_win;
    int all_ok;

    if (w->save_file_name == NULL) {
      g_error("window not saved: need a file name!\n");
      return Failed;
    }

    cur_win = grp_win;
    grp_win = w->window;
    all_ok = grp_save(w->save_file_name);
    grp_win = cur_win;
    free(w->save_file_name);
    w->save_file_name = NULL;
    w->saved = 1;
    return (all_ok) ? Success : Failed;
  }
}

Task window_hot_begin(BoxVM *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  w->hot.got.name = 0;
  w->hot.got.point = 0;
  w->hot.name = (char *) NULL;
  return Success;
}

Task window_hot_point(BoxVM *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  Point *point = BOX_VM_ARG1_PTR(vmp, Point);
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

Task window_hot_string(BoxVM *vm) {
  SUBTYPE_OF_WINDOW(vm, w);
  BoxStr *s = BOX_VM_ARG_PTR(vm, BoxStr);
  const char *name = (char *) s->ptr;
  w->hot.name = strdup(name);
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

Task window_hot_pointlist(BoxVM *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  IPointList *ipl_to_add = BOX_VM_ARG1(vmp, IPointList *);
  return pointlist_iter(& ipl_to_add->pl, _add_from_pointlist, & w->pointlist);
}

Task window_hot_end(BoxVM *vmp) {
  Window *w  = BOX_VM_SUB_PARENT(vmp, WindowPtr);
  Point *p = BOX_VM_SUB_CHILD_PTR(vmp, Point);

  if (! w->hot.got.point)
    g_warning("Hot[] did not get a point!");
  if (w->hot.got.name)
    g_warning("Hot[] got a name, but not the corresponding point!");

  *p = *pointlist_get(& w->pointlist, 0); /* return the last point */
  return Success;
}

Task window_file_string(BoxVM *vm) {
  SUBTYPE_OF_WINDOW(vm, w);
  BoxStr *s = BOX_VM_ARG_PTR(vm, BoxStr);

  if (w->plan.have.file_name) {
    g_warning("You have already provided a file name for the window.");
    free(w->plan.file_name);
  }
  w->plan.have.file_name = 1;
  w->plan.file_name = strdup(s->ptr);
  return Success;
}

Task window_res_point(BoxVM *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  Point *res = BOX_VM_ARG1_PTR(vmp, Point);
  if (w->plan.have.resolution) {
    g_warning("You have already provided the window resolution.");
  }
  w->plan.resolution.x = res->x;
  w->plan.resolution.y = res->y;
  w->plan.have.resolution = 1;
  return Success;
}

Task window_res_real(BoxVM *vmp) {
  Window *w = BOX_VM_SUB_PARENT(vmp, WindowPtr);
  Real *res = BOX_VM_ARG1_PTR(vmp, Real);
  if (w->plan.have.resolution) {
    g_warning("You have already provided the window resolution.");
  }
  w->plan.resolution.y = w->plan.resolution.x = *res;
  w->plan.have.resolution = 1;
  return Success;
}

Task window_show_point(BoxVM *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  Point *p = BOX_VM_ARG1_PTR(vmp, Point);
  GrpWindow *cur_win = grp_win;
  grp_win = w->window;
  grp_fake_point(p);
  grp_win = cur_win;
  return Success;
}

Task window_bbox(BoxVM *vmp) {
  BBox *b = BOX_VM_THIS_PTR(vmp, BBox);
  Window *w = BOX_VM_ARG1(vmp, WindowPtr);
  int not_degenerate = bb_bounding_box(w->window, & b->min, & b->max);
  b->n = not_degenerate ? 3 : 0;
  return Success;
}
