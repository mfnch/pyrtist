
#ifdef _DEF_WINDOW_SUBOBJECTS

/* This is part of the Window object */
typedef struct {
  enum {POLY_GOT_NOTHING,
        POLY_GOT_POINT,
        POLY_GOT_1ST_FLOAT,
        POLY_GOT_2ND_FLOAT} state;

  int num_points, num_margins, last_idx;
  Point first_points[2], last_point, lastb;
  Real margin[2];

  OptColor color;
} WindowPoly;

#else

#  ifndef _I_POLY_H
#    define _I_POLY_H

Task poly_window_init(Window *w);

#  endif

#endif
