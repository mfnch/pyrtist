
#ifdef _DEF_WINDOW_SUBOBJECTS

/* This is part of the Window object */
typedef struct {
  int auto_transforms;
  buff fig_points, back_points, weights;
  Real rot_angle;
  Point rot_center, translation, scale;
  void *figure;
  struct {
    int constraints : 1;
    int figure : 1;
    int compute : 1;
    int translation : 1;
    int rot_angle : 1;
    int scale : 1;
  } got;
} WindowPut;

#else

#  ifndef _I_PUT_H
#    define _I_PUT_H

Task put_window_init(Window *w);
void put_window_destroy(Window *w);

#  endif

#endif
