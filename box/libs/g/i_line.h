
#ifdef _DEF_WINDOW_SUBOBJECTS

typedef struct {
  Real width1, width2;
  Point point;
  LineStyle style;
  void *fig;
} WindowLinePiece;

/* This is part of the Window object */
typedef struct {
  enum {GOT_NOTHING,
        GOT_POINT,
        GOT_1ST_FLOAT,
        GOT_2ND_FLOAT} state;
  Int num_points;

  OptColor color;
  int close;
  WindowLinePiece this_piece;
  buff pieces;
} WindowLine;

#else

#  ifndef _I_LINE_H
#    define _I_LINE_H
Task line_window_init(Window *w);
void line_window_destroy(Window *w);
#  endif

#endif
