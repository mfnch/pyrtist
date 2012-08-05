/****************************************************************************
 * Copyright (C) 2008-2011 by Matteo Franchin                               *
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
#include <assert.h>

#include <box/types.h>
#include <box/mem.h>
#include <box/vm_private.h>
#include <box/str.h>

#include "graphic.h"
#include "g.h"
#include "bb.h"
#include "fig.h"
#include "autoput.h"
#include "pointlist.h"
#include "obj.h"
#include "winmap.h"
#include "i_pointlist.h"
#include "i_window.h"
#include "i_line.h"
#include "i_put.h"
#include "i_style.h"
#include "i_gradient.h"

/*#define DEBUG*/
#define DEBUG_WIN_REFS 0

static void My_Init_Style(GStyle *gs, FillStyle ds, DrawWhen dw) {
  g_style_new(gs, G_STYLE_NONE);
  g_style_set_fill_style(gs, ds);
  g_style_set_draw_when(gs, dw);
}

static void My_Init_Win_Styles(Window *w) {
  /* For Circle, we set a style which allows easily to draw dognuts */
  My_Init_Style(& w->circle.default_style, FILLSTYLE_EO, DRAW_WHEN_END);
  /* For Poly, we use the same style as Circle to easily allow holes */
  My_Init_Style(& w->poly.default_style, FILLSTYLE_EO, DRAW_WHEN_END);
  /* For Text and Line, we usually don't want holes to appear! */
  My_Init_Style(& w->text.default_style, FILLSTYLE_PLAIN,DRAW_WHEN_PAUSE);
  My_Init_Style(& w->line.default_style, FILLSTYLE_PLAIN,DRAW_WHEN_PAUSE);

  /* The main Window style is left completely unset (i.e. transparent
   * to the default styles.
   */
  g_style_new(& w->style, (GStyle *) NULL);
}

static void My_Finish_Win_Styles(Window *w) {
  g_style_clear(& w->circle.default_style);
  g_style_clear(& w->poly.default_style);
  g_style_clear(& w->text.default_style);
  g_style_clear(& w->line.default_style);
  g_style_clear(& w->style);
}

BoxTask Box_Lib_G_Init_At_Window(BoxVMX *vm) {
  WindowPtr *wp = (WindowPtr *) BoxVMX_Get_Parent_Target(vm);
  Window *w;

  w = *wp = (WindowPtr) BoxMem_Alloc(sizeof(Window));
  if (w == NULL)
    return BOXTASK_FAILURE;

#ifdef DEBUG
  printf("Window object allocated (%p)\n", w);
  printf("WindowPtr object: %p\n", wp);
#endif

  /* NOTE: using reference counting for the Window object is somewhat silly and
   * we should rather use Box memory handling system. However, most of the C
   * code in the G library (including this) is to be soon substituted with Box
   * code replacements. The solution of adding a reference counting mechanism
   * for Window object is simply the quickest way to achieve what I need now
   * (making Window behaving as a pointer) and will later be replaced by more
   * appropriate code (as soon as pointers are introduced in Box).
   */
  w->num_references = 1;

#if DEBUG_WIN_REFS == 1
  printf("Creating Window object at %p\n", w);
#endif

  w->initialised = 0;
  w->plan.have.type = 0;
  w->plan.type = BoxGWin_Type_From_String("fig");
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

  w->window = BoxGWin_Create_Faulty(stderr, "Cannot use a window before "
                                    "completing the initialization stage.");

  My_Init_Win_Styles(w);

  BOXTASK( pointlist_init(& w->pointlist) );

  BOXTASK( line_window_init(w) );
  BOXTASK( put_window_init(w) );
  return BOXTASK_OK;
}

static void My_Window_Reference(Window *w) {
  ++(w->num_references);
#if DEBUG_WIN_REFS == 1
  printf("Referencing Window object at %p (refs --> %d)\n",
         w, (int) w->num_references);
#endif
}

static void My_Window_Unreference(Window **w_ptr) {
  Window *w = *w_ptr;

  --(w->num_references);
  assert(w->num_references >= 0);

#if DEBUG_WIN_REFS == 1
  printf("Unreferencing Window object at %p (refs --> %d)\n",
         w, (int) w->num_references);
#endif

  if (w->num_references == 0) {
    BoxMem_Free(w->plan.file_name);

    BoxGWin_Destroy(w->window);

    My_Finish_Win_Styles(w);
    pointlist_destroy(& w->pointlist);
    put_window_destroy(w);
    line_window_destroy(w);
    BoxMem_Free(w);
    *w_ptr = (Window *) NULL;
  }
}

BoxTask Box_Lib_G_Finish_At_Window(BoxVMX *vm) {
  WindowPtr *wp = BoxVMX_Get_Parent_Target(vm);
  My_Window_Unreference(wp);
  return BOXTASK_OK;
}

BoxTask Box_Lib_G_Window_Copy_Window(BoxVMX *vm) {
  WindowPtr *wp_dst = BoxVMX_Get_Parent_Target(vm),
            w_src = *((WindowPtr *) BoxVMX_Get_Child_Target(vm));
  My_Window_Reference(w_src);
  My_Window_Unreference(wp_dst);
  *wp_dst = w_src;
  return BOXTASK_OK;
}

BoxTask Box_Lib_G_Window_At_Valid(BoxVMX *vm) {
  BoxInt *valid = BoxVMX_Get_Parent_Target(vm);
  Window *w = *((WindowPtr *) BoxVMX_Get_Child_Target(vm));
  *valid = (*valid && w->initialised);
  return BOXTASK_OK;
}

BoxTask Box_Lib_G_Color_At_Window(BoxVMX *vmp) {
  Window *w = BOX_VM_THIS(vmp, WindowPtr);
  Color *c = BOX_VM_ARG1_PTR(vmp, Color);
  if (w->window != NULL)
    BoxGWin_Set_Fg_Color(w->window, c);
  return BOXTASK_OK;
}

BoxTask Box_Lib_G_Gradient_At_Window(BoxVMX *vmp) {
  Window *w = BOX_VM_THIS(vmp, WindowPtr);
  Gradient *g = BOX_VM_ARG1(vmp, GradientPtr);
  if (w->window != NULL)
    BoxGWin_Set_Gradient(w->window, & g->gradient);
  return BOXTASK_OK;
}

BoxTask Box_Lib_G_Str_At_Window(BoxVMX *vm) {
  WindowPtr wp = BOX_VM_THIS(vm, WindowPtr);
  Window *w = wp;
  BoxStr *s = BOX_VM_ARG_PTR(vm, BoxStr);
  const char *type_str = (char *) s->ptr;

  if (w->plan.have.type) {
    g_warning("You have already specified the window type!");
  }


  w->plan.type = BoxGWin_Type_From_String(type_str);
  if (w->plan.type < 0) {
    g_error("Unrecognized window type!");
    return BOXTASK_FAILURE;
  }
  w->plan.have.type = 1;
  return BOXTASK_OK;
}

BoxTask window_size(BoxVMX *vmp) {
  WindowPtr wp = BOX_VM_THIS(vmp, WindowPtr);
  Window *w = (Window *) wp;
  Point *win_size = BOX_VM_ARG1_PTR(vmp, Point);

  if (w->plan.have.size) {
    g_error("You have already specified the window size!");
    return BOXTASK_FAILURE;
  }

  w->plan.have.size = 1;
  w->plan.size = *win_size;
  return BOXTASK_OK;
}

BoxTask Box_Lib_G_OldStyle_At_Window(BoxVMX *vmp) {
  Window *w = BOX_VM_THIS(vmp, WindowPtr);
  IStyle *s = BOX_VM_ARG(vmp, IStylePtr);
  g_style_copy_selected(& w->style, & s->style, s->have);
  return BOXTASK_OK;
}

BoxTask Box_Lib_G_Close_At_Window(BoxVMX *vm) {
  WindowPtr w = *((WindowPtr *) BoxVMX_Get_Parent_Target(vm));

  if (!w->initialised) {
    w->plan.have.resolution = 1;
    w->plan.have.origin = 1;
    w->window = BoxGWin_Create(& w->plan);
    if (w->window == NULL) {
      g_error("cannot create the window!");
      return BOXTASK_FAILURE;
    }

    w->initialised = 1;
  }

  return BOXTASK_OK;
}

BoxTask Box_Lib_G_Open_At_Figure(BoxVMX *vm) {
  return Box_Lib_G_Close_At_Window(vm);
}

BoxTask window_window(BoxVMX *vmp) {
  Window *w = BOX_VM_THIS(vmp, WindowPtr);
  Window *src = BOX_VM_ARG1(vmp, WindowPtr);
  BoxGWin_Fig_Draw_Fig(w->window, src->window);
  return BOXTASK_OK;
}

BoxTask GLib_Obj_At_Window(BoxVMX *vm) {
  Window *w = BOX_VM_THIS(vm, WindowPtr);
  BoxGObjPtr gobj = BOX_VM_ARG(vm, BoxGObjPtr);
  BoxGWinMap wm;
  BoxGWinMap_Init_Identity(& wm);
  BoxGWin_Interpret_Obj(w->window, gobj, & wm);
  return BOXTASK_OK;
}

BoxTask window_origin_point(BoxVMX *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  Point *origin = BOX_VM_ARG1_PTR(vmp, Point);

  if (w->plan.have.origin) {
    g_error("You have already specified the origin of the window!");
    return BOXTASK_FAILURE;
  }

  w->plan.have.origin = 1;
  w->plan.origin = *origin;
  return BOXTASK_OK;
}

BoxTask window_save_begin(BoxVMX *vmp) {
  Window *w  = BOX_VM_SUB_PARENT(vmp, WindowPtr);
  w->save_file_name = NULL;
  w->saved = 0;
  return BOXTASK_OK;
}

BoxTask window_save_str(BoxVMX *vm) {
  Window *w  = BOX_VM_SUB_PARENT(vm, WindowPtr);
  BoxStr *s = BOX_VM_ARG_PTR(vm, BoxStr);

  if (w->save_file_name != NULL) {
    printf("Window.Save: changing save target from '%s' to '%s'\n",
           w->save_file_name, s->ptr);
    BoxMem_Free(w->save_file_name);
  }

  w->save_file_name = BoxMem_Strdup(s->ptr);
  return BOXTASK_OK;
}

BoxTask window_save_window(BoxVMX *vmp) {
  Window *src  = BOX_VM_SUB_PARENT(vmp, WindowPtr);
  Window *dest = BOX_VM_ARG1(vmp, WindowPtr);
  Point translation = {0.0, 0.0}, center = {0.0, 0.0};
  Real sx = 1.0, sy = 1.0, rot_angle = 0.0;
  Matrix m;

  int type_fig = BoxGWin_Type_From_String("fig");
  if (src->plan.type != type_fig) {
    g_error("Window.Save: Saving to arbitrary targets is only available "
            "for \"fig\" windows. Windows of different type accept only "
            "the syntax window.Save[\"filename\"]");
    return BOXTASK_FAILURE;
  }
  if (src == dest) {
    g_error("Window.Save: saving a window into itself is not allowed.");
    return BOXTASK_FAILURE;
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
  if (BoxGWin_Is_Faulty(dest->window)) {
    BoxGBBox bbox;
    if (!BoxGBBox_Compute(& bbox, src->window)) {
      g_warning("Computed bounding box is degenerate: "
                "cannot save the figure!");
      return BOXTASK_FAILURE;
    }

    if (src->save_file_name != NULL) {
      dest->plan.have.file_name = 1;
      dest->plan.file_name = BoxMem_Strdup(src->save_file_name);
    }

    /*printf("Bounding box (%f, %f) - (%f, %f)\n",
             bb_min.x, bb_min.y, bb_max.x, bb_max.y);*/

    if (dest->plan.have.origin) {
      translation.x = -bbox.min.x;
      translation.y = -bbox.min.y;

    } else{
      dest->plan.origin.x = bbox.min.x;
      dest->plan.origin.y = bbox.min.y;
      dest->plan.have.origin = 1;
    }

    if (dest->plan.have.size) {
      sx = dest->plan.size.x/(bbox.max.x - bbox.min.x);
      sy = dest->plan.size.y/(bbox.max.y - bbox.min.y);

    } else {
      dest->plan.size.x = bbox.max.x - bbox.min.x;
      dest->plan.size.y = bbox.max.y - bbox.min.y;
      dest->plan.have.size = 1;
    }

    BoxGWin_Destroy(dest->window);
    dest->window = BoxGWin_Create(& dest->plan);
    if (dest->window == NULL) {
      g_error("Window.Save: cannot create the window!");
      return BOXTASK_FAILURE;
    }

    if (BoxGWin_Is_Faulty(dest->window)) {
      g_error("Window.Save: cannot complete the given window!");
      return BOXTASK_FAILURE;
    }

  } else {
    BoxGBBox bbox;
    if (!BoxGBBox_Compute(& bbox, src->window)) {
      g_warning("Computed bounding box is degenerate: "
                "cannot save the figure!");
      return BOXTASK_FAILURE;
    }
    translation.x = -bbox.min.x;
    translation.y = -bbox.min.y;
    sx = dest->plan.size.x/(bbox.max.x - bbox.min.x);
    sy = dest->plan.size.y/(bbox.max.y - bbox.min.y);
  }

  BoxGMatrix_Set(& m, & translation, & center, rot_angle, sx, sy);
  BoxGWin_Fig_Draw_Fig_With_Matrix(dest->window, src->window, & m);
  if (dest->plan.have.file_name)
    BoxGWin_Save_To_File(dest->window, dest->plan.file_name);
    /* ^^^ Some terminals require an explicit save! */

  if (src->save_file_name != NULL) {
    BoxMem_Free(src->save_file_name);
    src->save_file_name = NULL;
    dest->plan.file_name = NULL;
  }
  src->saved = 1;
  return BOXTASK_OK;
}

BoxTask window_save_end(BoxVMX *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);

  if (w->saved) {
    if (w->save_file_name != NULL) {
      BoxMem_Free(w->save_file_name);
      w->save_file_name = NULL;
      g_warning("Window.Save: given file name was not used.\n");
    }
    return BOXTASK_OK;

  } else {
    int all_ok;

    if (w->save_file_name == NULL) {
      g_error("window not saved: need a file name!\n");
      return BOXTASK_FAILURE;
    }

    all_ok = BoxGWin_Save_To_File(w->window, w->save_file_name);
    BoxMem_Free(w->save_file_name);
    w->save_file_name = NULL;
    w->saved = 1;
    return (all_ok) ? BOXTASK_OK : BOXTASK_FAILURE;
  }
}

BoxTask window_hot_begin(BoxVMX *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  w->hot.got.name = 0;
  w->hot.got.point = 0;
  w->hot.name = (char *) NULL;
  return BOXTASK_OK;
}

BoxTask window_hot_point(BoxVMX *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  Point *point = BOX_VM_ARG1_PTR(vmp, Point);
  char *name = (w->hot.got.name) ? w->hot.name : (char *) NULL;
  BoxTask t = pointlist_add(& w->pointlist, point, name);
  if (w->hot.got.name) {
    w->hot.got.name = 0;
    free(w->hot.name);
    w->hot.name = (char *) NULL; /* To avoid double free() */
  }
  w->hot.got.point = 1;
  return t;
}

BoxTask window_hot_string(BoxVMX *vm) {
  SUBTYPE_OF_WINDOW(vm, w);
  BoxStr *s = BOX_VM_ARG_PTR(vm, BoxStr);
  const char *name = (char *) s->ptr;
  w->hot.name = strdup(name);
  w->hot.got.name = 1;
  return BOXTASK_OK;
}

static BoxTask _add_from_pointlist(Int index, char *name,
                                   void *object, void *data) {
  PointList *dest_pl = (PointList *) data;
  Point *p = (Point *) object;
  return pointlist_add(dest_pl, p, name);
}

BoxTask window_hot_pointlist(BoxVMX *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  IPointList *ipl_to_add = BOX_VM_ARG1(vmp, IPointList *);
  return pointlist_iter(& ipl_to_add->pl, _add_from_pointlist, & w->pointlist);
}

BoxTask window_hot_end(BoxVMX *vmp) {
  Window *w  = BOX_VM_SUB_PARENT(vmp, WindowPtr);
  Point *p = BOX_VM_SUB_CHILD_PTR(vmp, Point);

  if (! w->hot.got.point)
    g_warning("Hot[] did not get a point!");
  if (w->hot.got.name)
    g_warning("Hot[] got a name, but not the corresponding point!");

  *p = *pointlist_get(& w->pointlist, 0); /* return the last point */
  return BOXTASK_OK;
}

BoxTask window_file_string(BoxVMX *vm) {
  SUBTYPE_OF_WINDOW(vm, w);
  BoxStr *s = BOX_VM_ARG_PTR(vm, BoxStr);

  if (w->plan.have.file_name) {
    g_warning("You have already provided a file name for the window.");
    BoxMem_Free(w->plan.file_name);
  }
  w->plan.have.file_name = 1;
  w->plan.file_name = BoxMem_Strdup(s->ptr);
  return BOXTASK_OK;
}

BoxTask window_res_point(BoxVMX *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  Point *res = BOX_VM_ARG1_PTR(vmp, Point);
  if (w->plan.have.resolution) {
    g_warning("You have already provided the window resolution.");
  }
  w->plan.resolution.x = res->x;
  w->plan.resolution.y = res->y;
  w->plan.have.resolution = 1;
  return BOXTASK_OK;
}

BoxTask window_res_real(BoxVMX *vmp) {
  Window *w = BOX_VM_SUB_PARENT(vmp, WindowPtr);
  Real *res = BOX_VM_ARG1_PTR(vmp, Real);
  if (w->plan.have.resolution) {
    g_warning("You have already provided the window resolution.");
  }
  w->plan.resolution.y = w->plan.resolution.x = *res;
  w->plan.have.resolution = 1;
  return BOXTASK_OK;
}

BoxTask window_show_point(BoxVMX *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  Point *p = BOX_VM_ARG1_PTR(vmp, Point);
  BoxGWin_Add_Fake_Point(w->window, p);
  return BOXTASK_OK;
}

BoxTask Box_Lib_G_Window_At_BBox(BoxVMX *vm) {
  BoxGBBox *b = BOX_VM_THIS_PTR(vm, BoxGBBox);
  Window *w = BOX_VM_ARG1(vm, WindowPtr);
  (void) BoxGBBox_Compute(b, w->window);
  return BOXTASK_OK;
}

BoxTask Box_Lib_G_Window_At_Num(BoxVMX *vm) {
  BoxInt *num = BoxVMX_Get_Parent_Target(vm);
  Window *w = *((WindowPtr *) BoxVMX_Get_Child_Target(vm));
  *num += pointlist_num(& w->pointlist);
  return BOXTASK_OK;
}

BoxTask Box_Lib_G_Str_At_Window_Get(BoxVMX *vm) {
  BoxSubtype *window_get = BoxVMX_Get_Parent_Target(vm);
  BoxPoint *point = BoxSubtype_Get_Child_Target(window_get);
  Window *w = *((WindowPtr *) BoxSubtype_Get_Parent_Target(window_get));
  BoxStr *name = BoxVMX_Get_Child_Target(vm);
  char *c_name = BoxStr_To_C_String(name);
  BoxPoint *found = pointlist_find(& w->pointlist, c_name);
  if (found) {
    *point = *found;
    return BOXTASK_OK;

  } else {
    BoxVMX_Set_Fail_Msg(vm, "Cannot find hot point with the given name "
                           "in the Window");
    return BOXTASK_FAILURE;
  }
}

BoxTask Box_Lib_G_Int_At_Window_Get(BoxVMX *vm) {
  BoxSubtype *window_get = BoxVMX_Get_Parent_Target(vm);
  BoxPoint *point = BoxSubtype_Get_Child_Target(window_get);
  Window *w = *((WindowPtr *) BoxSubtype_Get_Parent_Target(window_get));
  BoxInt idx = *((BoxInt *) BoxVMX_Get_Child_Target(vm)) + 1;
  BoxPoint *found = pointlist_get(& w->pointlist, idx);
  if (found != NULL) {
    *point = *found;
    return BOXTASK_OK;

  } else {
    BoxVMX_Set_Fail_Msg(vm, "The Window does not have any hot points");
    return BOXTASK_FAILURE;
  }
}

BoxTask Box_Lib_G_Int_At_Window_HotPointHasName(BoxVMX *vm) {
  BoxSubtype *window_get = BoxVMX_Get_Parent_Target(vm);
  BoxInt *hotpoint_has_name = BoxSubtype_Get_Child_Target(window_get);
  Window *w = *((WindowPtr *) BoxSubtype_Get_Parent_Target(window_get));
  BoxInt idx = *((BoxInt *) BoxVMX_Get_Child_Target(vm)) + 1;
  *hotpoint_has_name = (pointlist_get_name(& w->pointlist, idx) != NULL);
  return BOXTASK_OK;
}

BoxTask Box_Lib_G_Int_At_Window_GetHotPointName(BoxVMX *vm) {
  BoxSubtype *window_get = BoxVMX_Get_Parent_Target(vm);
  BoxStr *hotpoint_name = BoxSubtype_Get_Child_Target(window_get);
  Window *w = *((WindowPtr *) BoxSubtype_Get_Parent_Target(window_get));
  BoxInt idx = *((BoxInt *) BoxVMX_Get_Child_Target(vm)) + 1;
  const char *name = pointlist_get_name(& w->pointlist, idx);
  if (name != NULL) {
    BoxStr_Set_From_C_String(hotpoint_name, name);
    return BOXTASK_OK;

  } else {
    BoxVMX_Set_Fail_Msg(vm, "The Window Hot point does not have a name");
    return BOXTASK_FAILURE;
  }
}
