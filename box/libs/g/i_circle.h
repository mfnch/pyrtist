

#ifdef _DEF_WINDOW_SUBOBJECTS

/* This is part of the Window object */
typedef struct {
  struct {
    enum {GOT_NOT, GOT_NOW, GOT_BEFORE} center, radius_a, radius_b;
    int color : 1;
  } got;
  Color color;
  Point center;
  Real radius_a, radius_b;
} WindowCircle;

#else

#  ifndef _I_CIRCLE_H
#    define _I_CIRCLE_H

#  endif

#endif
