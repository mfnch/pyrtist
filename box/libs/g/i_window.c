/* Interface for Window */

#include <stdlib.h>
#include <stdio.h>

#include "types.h"
#include "virtmach.h"
#include "graphic.h"
#include "g.h"
#include "pointlist.h"
#include "i_window.h"
#include "i_line.h"
#include "i_circle.h"
#include "i_poly.h"
#include "i_put.h"

/*#define DEBUG*/

/*enum {FLAG_TYPE=0x1, FLAG_SIZE=0x2, FLAG_ORIGIN=0x4,
 FLAG_RESX=0x8, FLAG_RESY=0x10, FLAG_NUMLAYERS=0x20,
 FLAG_FILENAME=0X40};
*/

Task window_begin(VMProgram *vmp) {
  WindowPtr *wp = BOX_VM_CURRENTPTR(vmp, WindowPtr);
  Window *w;
  w = *wp = (WindowPtr) malloc(sizeof(Window)); /* Should use Mem_Alloc, but this requires to link against the internal box library (which still does not exist) */
  if (w == (WindowPtr) NULL) return Failed;
#ifdef DEBUG
  printf("Window object allocated (%p)\n", w);
  printf("WindowPtr object: %p\n", wp);
#endif
  w->type = FIG;
  w->have.type = 0;
  w->origin.x = 0.0;
  w->origin.y = 0.0;
  w->size.x = 100.0;
  w->size.y = 100.0;
  w->res.x = 2.0;
  w->res.y = 2.0;
  w->have.size = 0;
  w->save_file_name = (char *) NULL;

  TASK( pointlist_init(& w->pointlist) );

  /* This is the parent colors: no alternatives! */
  g_optcolor_alternative_set(& w->fg_color, (OptColor *) NULL);
  (void) g_optcolor_set_rgb(& w->fg_color, 0.0, 0.0, 0.0);

  TASK( line_window_init(w) );
  TASK( circle_window_init(w) );
  TASK( poly_window_init(w) );
  TASK( put_window_init(w) );
  return Success;
}

Task window_color(VMProgram *vmp) {
  WindowPtr wp = BOX_VM_CURRENT(vmp, WindowPtr);
  Window *w = (Window *) wp;
  Color *c = BOX_VM_ARGPTR1(vmp, Color);
  return g_optcolor_set(& w->fg_color, c);
}

Task window_destroy(VMProgram *vmp) {
  WindowPtr wp = BOX_VM_CURRENT(vmp, WindowPtr);
  Window *w = (Window *) wp;
#ifdef DEBUG
  printf("Window object deallocated\n");
#endif
  pointlist_destroy(& w->pointlist);
  put_window_destroy(w);
  line_window_destroy(w);
  (void) free(wp);
  return Success;
}

Task window_str(VMProgram *vmp) {
  WindowPtr wp = BOX_VM_CURRENT(vmp, WindowPtr);
  Window *w = (Window *) wp;
  char *type_str = BOX_VM_ARGPTR1(vmp, char);

  int i;
# define NUM_WIN_TYPES 5
  struct {char *type_str; int type_id;} win_type[NUM_WIN_TYPES] = {
    {"bm1", BM1}, {"bm4", BM4}, {"bm8", BM8}, {"fig", FIG}, {"ps", PS}
  };

  if (w->have.type) {
    g_error("You have already specified the window type!");
    return Failed;
  }

  for(i=0; i<NUM_WIN_TYPES; i++) {
    if (strcasecmp(win_type[i].type_str, type_str) == 0) {
      w->type = win_type[i].type_id;
      w->have.type = 1;
      return Success;
    }
  }

  g_error("Unrecognized window type!");
  return Failed;
}

Task window_size(VMProgram *vmp) {
  WindowPtr wp = BOX_VM_CURRENT(vmp, WindowPtr);
  Window *w = (Window *) wp;
  Point *win_size = BOX_VM_ARGPTR1(vmp, Point);

  if (w->have.size) {
    g_error("You have already specified the window size!");
    return Failed;
  }

  w->have.size = 1;
  w->size = *win_size;
  return Success;
}

Task window_end(VMProgram *vmp) {
  WindowPtr wp = BOX_VM_CURRENT(vmp, WindowPtr);
  Window *w = (Window *) wp;

  switch(w->type) {
  case PS:
    if (w->save_file_name == (char *) NULL) {
      g_error("Window: window type 'ps' requires a file name. "
              "Please provide one.");
      return Failed;
    }
    w->window = ps_open_win(w->save_file_name);
    break;

  case BM1:
    w->window = gr1b_open_win(w->origin.x, w->origin.y,
                              w->origin.x+w->size.x, w->origin.y+w->size.y,
                              w->res.x, w->res.y);
    break;

  case BM4:
    w->window = gr4b_open_win(w->origin.x, w->origin.y,
                              w->origin.x+w->size.x, w->origin.y+w->size.y,
                              w->res.x, w->res.y);
    break;

  case BM8:
    w->window = gr8b_open_win(w->origin.x, w->origin.y,
                              w->origin.x+w->size.x, w->origin.y+w->size.y,
                              w->res.x, w->res.y);
    break;

  case FIG:
    w->window = fig_open_win(1);
    break;

  default:
    g_error("window_end: shouldn't happen!");
    return Failed;
  }

  if (w->window == (grp_window *) NULL) {
    g_error("cannot create the window!");
    return Failed;
  }

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
  w->save_file_name = BOX_VM_ARGPTR1(vmp, char);
  return Success;
}

Task window_res_point(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  Point *res = BOX_VM_ARGPTR1(vmp, Point);
  w->res.x = res->x;
  w->res.y = res->y;
  return Success;
}
