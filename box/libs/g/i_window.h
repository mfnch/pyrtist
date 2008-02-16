
#ifndef _I_WINDOW_H
#  define _I_WINDOW_H
#  include "types.h"
#  include "g.h"
#  include "buffer.h"
#  include "pointlist.h"

#  define _DEF_WINDOW_SUBOBJECTS
#  include "i_line.h"
#  include "i_circle.h"
#  include "i_poly.h"
#  include "i_put.h"
#  undef _DEF_WINDOW_SUBOBJECTS

typedef struct {
  struct {
    int type : 1;
    int size : 1;
  } have;

  enum {BM1, BM4, BM8, FIG, PS} type;
  Point size, origin, res;

  grp_window *window;

  PointList pointlist;
  OptColor fg_color;

  WindowLine line;
  WindowCircle circle;
  WindowPoly poly;
  WindowPut put;

  struct {
    struct {
      int point : 1;
      int name : 1;
    } got;
    char *name;
  } hot;

  char *save_file_name;
} Window;

typedef void *WindowPtr;


  #define SUBTYPE_OF_WINDOW(vmp, w) \
    Window *w = *( (Window **) \
      SUBTYPE_PARENT_PTR(BOX_VM_CURRENTPTR(vmp, Subtype), WindowPtr) );

#endif
